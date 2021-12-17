import argparse
import os
import numpy as np
import numba

DATA_DIR = './data'

FFT_LEN = 128
FILTER_STD = 8 * np.pi
NUM_FILTERS = 1
NUM_OCTAVES = 1
BANDWIDTH = 0.5
FILTER_CUTOFF = 1e-12

NUM_LAYERS = 14

QUANT_POWOFFSET = -11
QUANT_POWSCALE = 3.5 / 2
QUANT_MAX = 32

@numba.jit
def DownsampleAudio2x(audio, kWidth = 1024):
    downSample2xK = np.zeros(kWidth)
    phase = 1
    for q in range(kWidth):
        downSample2xK[q] = 2 / ((q * 2 + 1) * np.pi) * phase
        phase *= -1
    downSampleScale = 1 / np.sqrt(np.sum(np.absolute(downSample2xK) ** 2)) 
    downSample2xK *= downSampleScale
    
    downSampled = np.zeros((len(audio) + 1) // 2, dtype=audio.dtype)
    for i in range(len(audio)):
        dsi = i // 2
        downSampled[dsi] += audio[i] * downSampleScale
        
        for q in range(kWidth):
            iq = i + 1 + q * 2
            if iq >= len(audio): break
            downSampled[dsi] += audio[iq] * downSample2xK[q]
        
        for q in range(kWidth):
            iq = i - 1 - q * 2
            if iq < 0: break
            downSampled[dsi] += audio[iq] * downSample2xK[q]
    
    return downSampled * 0.18 # approx renormalizing constant (???)

@numba.jit
def UpsampleAudio2x(audio, kWidth = 1024):
    upSample2xK = np.zeros(kWidth)
    phase = 1
    for q in range(kWidth):
        upSample2xK[q] = 2 / ((q * 2 + 1) * np.pi) * phase
        phase *= -1

    upSampled = np.zeros(len(audio) * 2, dtype=audio.dtype)
    for i in range(len(audio)): upSampled[i * 2] = audio[i]
    
    for i in range(1, len(upSampled), 2):
      
        for q in range(kWidth):
            iq = i + 1 + q * 2
            if iq >= len(upSampled): break
            upSampled[i] += upSampled[iq] * upSample2xK[q]

        for q in range(kWidth):
            iq = i - 1 - q * 2
            if iq < 0: break
            upSampled[i] += upSampled[iq] * upSample2xK[q]
    
    return upSampled
    
def QuantizeAudio(audio):
    u = 255
    clipped = np.minimum(np.maximum(audio, -1), 1)
    quantized = (u / 2 * np.sign(clipped) * np.log(1 + u * np.absolute(clipped)) / np.log(1 + u)).astype(np.int8)
    return quantized

def DequantizeAudio(quantized):
    u = 255
    audio = (quantized.astype(np.float64)) / u * 2
    audio = np.sign(audio) * (1 / u) * (((1 + u) ** np.absolute(audio)) - 1)
    return audio

def DequantizeSample(quantized):
    u = 255
    audio = quantized / u * 2
    audio = np.sign(audio) * (1 / u) * (((1 + u) ** np.absolute(audio)) - 1)
    return audio
    
def CreateSlidingDFT(fftLen):
    fft = np.zeros(fftLen, np.complex128)
    
    fftq = np.zeros(fftLen, np.complex128)
    for q in range(fftLen): fftq[q] = np.exp(complex(0,  (q + 0.5) / fftLen * np.pi))
    
    comb = np.ones(fftLen)
    for q in range(1, fftLen, 2): comb[q] = -1
    
    return fft, fftq, comb

@numba.jit
def SlideDFT(fft, fftq, sample, comb):
    fft = (fft + sample) * fftq
    fft -= np.sum(fft).real / len(fft)
    return fft, np.sum(fft * comb) / len(fft)
    
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
    
def SynthLogNormalFilters(filterLen, numFilters, numOctaves, bandwidth, sampleStd, filterCutoff=0, logGabor=True, spatialFilter=True): 
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
        if logGabor:
            f = np.exp(-(np.log(sample0 / sampleX[i]) ** 2) * sampleStd * sampleX[i] * sampleX[i])
            gain = sampleX[i] ** 2
        else:
            f = np.exp(-((sample0 - sampleX[i]) ** 2) * sampleStd / np.sqrt(sampleX[i] * sampleX[0]))
            gain = sampleX[i] ** (3/4)
        
        f[64:128] = 0
        
        coverage += f * np.sqrt(gain)
        if spatialFilter: f = np.fft.fftshift(np.fft.ifft(f))
        filterBounds.append(GetFilterBounds(f, filterCutoff))
        f *= np.sqrt(gain)
        filters.append(f)
        
    return filters, filterBounds, coverage / np.max(coverage), sampleX

@numba.jit    
def SampleFilter(sample, filter, filterBounds): 
    layerResponse = np.zeros(len(sample) - FFT_LEN, np.complex128)
    for i in range(len(layerResponse)):
        layerResponse[i] = np.sum(sample[i + filterBounds[0]:i + filterBounds[1]] * filter[filterBounds[0]:filterBounds[1]])
    
    return layerResponse
    
def SampleFilterSet(sample, filters, filterBounds=None):
    response = np.zeros(len(filters), filters[0].dtype)
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

def SaveQuantFile(quants, path):                  
    quants.tofile(path)
    return

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Filter and quantize a float64 raw PCM 44.1khz audio file')
    parser.add_argument('--inputraw', default='',
                        help='path of the audio file to quantize (optional)')
    parser.add_argument('--inputrawlen', type=int, default=-1,
                        help='number of samples to read from audio file (or -1 for all)')                            
    parser.add_argument('--outputraw', default='',
                        help='path of the audio file to synthesize from quants(optional)')
    parser.add_argument('--inputquants', default='',
                        help='path of the quant file to synthesize(optional)')                         
    parser.add_argument('--outputquants', default='',
                        help='path of the quant file to save(optional)')                          
    parser.add_argument('--debugfilters', default=False,
                        help='enable to dump filter set and coverage')
    parser.add_argument('--progress', default=False,
                        help='enable to show progress while rescaling and filtering')                        
                        
    args = parser.parse_args()
        
    lgFilters, lgFilterBounds, lgTotalResponse, lgQ = SynthLogNormalFilters(FFT_LEN, NUM_FILTERS, NUM_OCTAVES, BANDWIDTH, FILTER_STD, FILTER_CUTOFF)
    
    if args.debugfilters:
        DumpFiltersToFile(lgFilters, "lgFilterSet.raw")
        print("LG Filter frequency range: " + str(lgQ[0] * 22050) + " - " + str(lgQ[-1] * 22050))
        lgTotalResponse.tofile("lgFilterSetCoverage.raw")

    if args.inputraw != '':
        inputRawPath = os.path.join(DATA_DIR, args.inputraw)
        print("Loading audio from " + inputRawPath + "...")
        inData = np.fromfile(inputRawPath, count=args.inputrawlen)

        lgResponse = np.zeros(NUM_FILTERS, np.complex128)
        
        layerResponses = []
        quantizedResponses = []
        
        for L in range(NUM_LAYERS):
            if args.progress: print("Filtering layer {0}({1})...".format(L, len(inData))) 
            if L > 0: inData = DownsampleAudio2x(inData)
            
            padded = np.zeros(len(inData) + FFT_LEN)
            padded[FFT_LEN // 2: FFT_LEN // 2 + len(inData)] = inData
            
            layerResponse = SampleFilter(padded, lgFilters[0], lgFilterBounds[0])
            
            layerResponse = ComplexQuantize(layerResponse, QUANT_MAX, QUANT_POWOFFSET, QUANT_POWSCALE)
            quantizedResponses.append(layerResponse)
            layerResponse = ComplexDequantize(layerResponse, QUANT_MAX, QUANT_POWOFFSET, QUANT_POWSCALE)
            layerResponses.append(layerResponse)
        
        layerResponses.reverse()
        quantizedResponses.reverse()
        
        if args.outputquants != '':
            outputQuantsPath = os.path.join(DATA_DIR, args.outputquants)
            print("Saving quants to " + outputQuantsPath + "...")
            
            concat = np.concatenate(quantizedResponses)
            quantized = np.zeros(len(concat), np.int16)
            for i in range(len(quantized)): quantized[i] = np.int16(concat[i].real + concat[i].imag * 256)
            quantized.tofile(outputQuantsPath)
            
            layerLengths = np.zeros(len(quantizedResponses), np.int64)
            for i in range(len(quantizedResponses)): layerLengths[i] = len(quantizedResponses[i])
            outputLayerInfoPath = os.path.join(DATA_DIR, args.outputquants + '.layers')
            print("Saving layer info to " + outputLayerInfoPath + "...")
            layerLengths.tofile(outputLayerInfoPath)
            
    if args.inputquants != '':
        inputQuantsPath = args.inputquants
        inputQuantsLayersPath = args.inputquants + '.layers'
        print("Loading layers from " + inputQuantsLayersPath + "...")
        layerLens = np.fromfile(inputQuantsLayersPath, dtype=np.int64)

        print("Loading quants from " + inputQuantsPath + "...")
        quants = np.fromfile(inputQuantsPath, np.int8)
        quants = np.reshape(quants, (len(quants) // 2, 2))
        layerResponses = []
        offset = 0
        for L in range(len(layerLens)):
            layerResponse = np.zeros(layerLens[L], np.complex128)
            for i in range(layerLens[L]):
                layerResponse[i] = complex(quants[offset + i, 0], quants[offset + i, 1])
            layerResponse = ComplexDequantize(layerResponse, QUANT_MAX, QUANT_POWOFFSET, QUANT_POWSCALE)
            layerResponses.append(layerResponse)
            offset += layerLens[L]
        
    if args.outputraw != '':
        outputRawPath = args.outputraw
        print("Saving audio to " + outputRawPath + "...")
        
        layerResponse = np.zeros(len(layerResponses[0]), np.complex128)
        for L in range(len(layerResponses)):
            layerResponse[0:len(layerResponses[L])] += layerResponses[L]
            if L < (NUM_LAYERS - 1):
                if args.progress: print('Upsampling layer {0}...'.format(NUM_LAYERS - 1 - L))
                layerResponse = UpsampleAudio2x(layerResponse)
        
        layerResponse.tofile(outputRawPath)
