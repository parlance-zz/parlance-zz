import argparse
import os
import numpy as np
import numba

import train

FFT_LEN = 1024
CHUNK_STEP = 100
FILTER_STD = FFT_LEN / np.pi
NUM_FILTERS = 256
NUM_OCTAVES = 8.5
BANDWIDTH = 0.8
FILTER_CUTOFF = 1e-10
DEBUG_LG_FILTERS = False
QUANT_POWOFFSET = -5
QUANT_POWSCALE = 3#1.5

QUANT_MAX = 32

def CreateSlidingDFT(fftLen, dtype=np.complex64):
    fft = np.zeros(FFT_LEN, dtype)
    
    fftq = np.zeros(FFT_LEN, dtype)
    for q in range(FFT_LEN): fftq[q] = np.exp(complex(0,  (q + 0.5) / FFT_LEN * np.pi))
    
    comb = np.ones(FFT_LEN, np.float32)
    for q in range(1, FFT_LEN, 2): comb[q] = -1
    
    return fft, fftq, comb

def SlideDFT(fft, fftq, sample, comb):
    fft = (fft + sample * comb) * fftq
    out = np.sum(fft)
    fft -= out.real / FFT_LEN

    return out
    
@numba.jit
def GetFilterBounds(filter, cutoff):
    m = 0
    for start in range(len(filter)):
        m += filter[start] ** 2
        if m >= cutoff: break
    
    m = 0
    for end in range(len(filter) - 1, 0, -1):
        m += filter[end] ** 2
        if m >= cutoff: break        
    
    bounds = np.zeros(2, np.int64)
    bounds[0] = start
    bounds[1] = end
    
    return bounds
    
@numba.jit
def SynthLogNormalFilters(filterLen, numFilters, numOctaves, bandwidth, sampleStd, filterCutoff=0): 
    sample0 = np.arange(0, 1, 1 / filterLen)
    sample0 = sample0 * 2 + 1e-100
    
    sampleX = np.arange(0, 1, 1 / numFilters)
    sampleX = np.exp2(sampleX * numOctaves)
    sampleX = sampleX / np.max(sampleX) * bandwidth 
    sampleX += 1e-100

    coverage = np.zeros(filterLen)
    
    filters = []
    filterBounds = []
    for i in range(numFilters):
        #f = np.exp(-(np.log(sample0 / sampleX[i]) ** 2) * sampleStd * sampleX[i] * sampleX[i]) # swap these 2 lines for log or regular gabor filters
        f = np.exp(-((sample0 - sampleX[i]) ** 2) * sampleStd / np.sqrt(sampleX[i] * sampleX[0]))
        #f += np.exp(-((sample0 - 2 - sampleX[i]) ** 2) * sampleStd / np.sqrt(sampleX[i] * sampleX[0]))
        filterBounds.append(GetFilterBounds(f, filterCutoff))
        gain = sampleX[i] ** (3/4)
        f *= np.sqrt(gain)
        filters.append(f)
        coverage += f * np.sqrt(gain)

        
    return filters, filterBounds, coverage / np.max(coverage), sampleX

def SampleFilterSet(sample, filters, filterBounds=None):
    response = np.zeros(len(filters), sample.dtype)
    if filterBounds == None:
        for i in range(len(filters)): response[i] = np.dot(sample, filters[i])
    else:
        for i in range(len(filters)): response[i] = np.dot(sample[filterBounds[i][0]:filterBounds[i][1]], filters[i][filterBounds[i][0]:filterBounds[i][1]])
       
    return response

def InverseFilterSet(sampleLen, filters, coeff, filterBounds=None):
    buffer = np.zeros(sampleLen, coeff[0].dtype)
    if filterBounds == None:
        for i in range(len(filters)): buffer += filters[i] * coeff[i]
    else:
        for i in range(len(filters)): buffer[filterBounds[i][0]:filterBounds[i][1]] += filters[i][filterBounds[i][0]:filterBounds[i][1]] * coeff[i]
    
    return buffer
        
def DumpFiltersToFile(filters, path):
    bufferLen = 0
    for f in filters: bufferLen += len(f)
    
    buffer = np.zeros(bufferLen, filters[0].dtype)
    offset = 0
    for f in filters:
        buffer[offset:offset + len(f)] = f / np.max(f)
        offset += len(f)
        
    buffer.tofile(path)
    
    return

@numba.jit
def ComplexQuantize(filter, quantMax, powOffset, powScale):  
    quantized = np.zeros(len(filter), np.complex128)
    for i in range(len(filter)):
        c = filter[i]
        phase = np.floor((np.angle(c) + np.pi) / 2 / np.pi * quantMax + 0.5) % quantMax
        pow = np.minimum(np.floor(np.maximum(np.log(np.absolute(c)) - powOffset, 0) * powScale + 0.5), quantMax - 1)
        if np.isfinite(pow) == False: pow = 0
        quantized[i] = complex(pow, phase)
    return quantized
        
@numba.jit
def ComplexDequantize(quantized, quantMax, powOffset, powScale):
    filter = np.zeros(len(quantized), np.complex128)
    for i in range(len(quantized)):
        q = quantized[i]
        phase = q.imag / quantMax * np.pi * 2 - np.pi
        if q.real == 0:
            pow = -100
        else:
            pow = q.real / powScale + powOffset
        filter[i] = np.exp(complex(pow, phase))
    return filter

@numba.jit
def SynthNormalWindow(windowLen, wStd):
    window = np.zeros(windowLen)
    for q in range(windowLen):
        fx1 = q / windowLen
        fx2 = (windowLen - q) / windowLen
        wStd = 8 * np.pi
        fx1 *= np.sqrt(wStd)
        fx2 *= np.sqrt(wStd)
        window[q] = np.exp(-fx1 * fx1)  
        window[q] += np.exp(-fx2 * fx2)
        window /= np.max(window)
    return window

def LoadQuantFile(path):
    quants = np.fromfile(path, np.complex128)
    quants = np.reshape(quants, (len(quants) // NUM_FILTERS, NUM_FILTERS))
    return quants

def SaveQuantFile(quants, path):                  
    quants.tofile(path)
    return

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Filter and quantize a float64 raw PCM 44.1khz audio file')
    parser.add_argument('--inputraw', default='',
                        help='path of the audio file to quantize (optional)')
    parser.add_argument('--inputrawlen', type=int, default=-1,
                        help='number of samples to read from audio file (or -1 for all)')                        
    parser.add_argument('--inputquants', default='',
                        help='path of the quant file to synthesize (optional)')
    parser.add_argument('--outputraw', default='',
                        help='path of the output audio file (optional)')
    parser.add_argument('--outputquants', default='',
                        help='path of the output quant file (optional)')
    parser.add_argument('--debugfilters', default=False,
                        help='enable to dump filter set and coverage')
                        
    args = parser.parse_args()
    
    if args.inputraw != '' and args.inputquants != '':
        print('Error: Only input audio OR quants can be specified at once')
        exit(1)
     
    if args.inputraw == '' and args.inputquants == '':
        print('Error: Input audio or quants must be specified')
        exit(1)
     
    if args.outputraw == '' and args.outputquants == '':
        print('Error: Output must be raw audio, quants, or both')
        exit(1)
        
    lgFilters, lgFilterBounds, lgTotalResponse, lgQ = SynthLogNormalFilters(FFT_LEN, NUM_FILTERS, NUM_OCTAVES, BANDWIDTH, FILTER_STD, FILTER_CUTOFF)
    
    if args.debugfilters or DEBUG_LG_FILTERS:
        DumpFiltersToFile(lgFilters, "lgFilterSet.raw")
        print("LG Filter frequency range: " + str(lgQ[0] * 22050) + " - " + str(lgQ[-1] * 22050))
        lgTotalResponse.tofile("lgFilterSetCoverage.raw")

    if args.inputraw != '':
        inputRawPath = os.path.join(train.DATA_DIR, args.inputraw)
        print("Loading audio from " + inputRawPath + "...")
        inData = np.fromfile(inputRawPath, count=args.inputrawlen)
        numOutputSamples = len(inData)
    if args.inputquants != '':
        inputQuantPath = os.path.join(train.DATA_DIR, args.inputquants)
        print("Loading quants from " + inputQuantPath + "...")
        inputQuants = LoadQuantFile(inputQuantPath)
        numOutputSamples = inputQuants.shape[0] * CHUNK_STEP + FFT_LEN
    if args.outputraw != '':
        outData = np.zeros(numOutputSamples)
    if args.outputquants != '':
        outputQuants = np.zeros(((numOutputSamples - FFT_LEN) // CHUNK_STEP + 1, NUM_FILTERS), np.complex128)
        
    fft, fftq, comb = CreateSlidingDFT(FFT_LEN)
    for i in range(numOutputSamples):
        
        if args.inputraw != '':
            #fft = np.fft.fft(np.fft.fftshift(inData[i:i + FFT_LEN]))
            out = SlideDFT(fft, fftq, inData[i], comb).real
            lgResponse = SampleFilterSet(fft, lgFilters, lgFilterBounds)
            lgResponse = ComplexQuantize(lgResponse, QUANT_MAX, QUANT_POWOFFSET, QUANT_POWSCALE)
            
            if args.outputquants != '':
                outputQuants[i] = lgResponse
        
        if args.inputquants != '':
            lgResponse = inputQuants[i]
            
        if args.outputraw != '':
            outData[i] = out
        
        if i & 63 == 0:
            progress = i / numOutputSamples * 100
            print("Filtering... {0:.0f}%".format(progress), end='\r')

    print("Filtering... 100%")

    if args.outputquants:
        qPath = os.path.join(train.DATA_DIR, args.outputquants)
        print('Saving quants to ' + qPath + '...')
        SaveQuantFile(outputQuants, qPath)
    if args.outputraw:
        oPath = os.path.join(train.DATA_DIR, args.outputraw)
        print('Saving output to ' + oPath + '...')
        (outData / np.max(outData)).tofile(oPath)