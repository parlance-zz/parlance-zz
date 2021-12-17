import argparse
import os
import numpy as np
import numba
from scipy.special import erfinv
from scipy.special import kv # modified bessel function of 2nd kind with complex domain

DATA_DIR = './data'

FFT_LEN = 2048
CHUNK_STEP = 256
FILTER_STD = FFT_LEN * 1.3
NUM_FILTERS = 768
NUM_OCTAVES = 8
BANDWIDTH = 0.7
FILTER_CUTOFF = 1e-10
QUANT_POWOFFSET = -4
QUANT_POWSCALE = 1.6 / 2
QUANT_MAX = 16 // 2

class CircularBuffer(np.ndarray):
    
    def __new__(cls, *args, **kwargs):
        return super(CircularBuffer, cls).__new__(cls, *args, **kwargs)

    def __init__(self, *args, **kwargs):
        self.cPos = 0
        return super(CircularBuffer, self).__init__()
        
    def push(self, val):
        rVal = self[self.cPos]
        self[self.cPos] = val
        self.cPos = self.cPos + 1
        if self.cPos == len(self): self.cPos = 0
        return rVal

# both of these have bugged variance

def get_analytic_filter_approx(sample_len, variance, scale): # approximate (fast) analytic filter for exp(-v*(x - 1/x))
                                                             # based on approximating besselK[1, x] as exp(-x)/x
    x = (np.arange(sample_len) - sample_len/2) / sample_len * 2 * scale
    #return variance * np.exp(-(2 * np.sqrt(variance) * np.sqrt(1j * x + variance)) + 2 * variance) / np.sqrt(variance + 1j * x) / (2 * np.sqrt(variance) * np.sqrt(1j * x + variance))
    #return np.exp(-2 * (1 + 1j * x) ** (1/2)) / (1 + 1j * x) ** (3/4)
    #return np.exp(variance * (1 - (x**2 + 1)**(1/4) * np.exp(1j * np.arctan(x)/2)))
    return np.exp(variance * (1 - (x**2 + 1)**(1/5) * np.exp(1j * np.arctan(x)/2.5)))
    
def get_analytic_filter(sample_len, variance, scale): # analytic filter for exp(-v*(x - 1/x))
                                                      # seems accurate with any bessel order, though it SHOULD be 1
    x = (np.arange(sample_len) - sample_len/2) / sample_len * 2 * scale
    return variance * np.exp(2*variance) * kv(1, 2 * np.sqrt(variance) * np.sqrt(1j * x + variance)) / np.sqrt(variance + 1j * x)
    
def get_ln_map(numUnits, temperature=1):
    rng = np.arange(numUnits) / numUnits + 1e-100
    weights = np.exp(temperature * erfinv(2 * rng - 1))
    weights /= np.max(weights)
    return weights
    
@numba.jit
def get_analytic_signal(audio, kWidth=4096):
    even = audio[::2]
    odd = audio[1::2]
    
    if len(even) < len(odd): even = np.concatenate((even, np.zeros(1)))
    if len(odd) < len(even): odd = np.concatenate((odd, np.zeros(1)))
    
    splitLen = np.minimum(len(even), len(odd))
    
    hKernelR = 1 / (2.98 * (np.arange(kWidth) + 1))
    hKernelL = -hKernelR[::-1]
    anal = np.zeros(len(audio), np.complex128)
    
    for s in range(0, len(audio), 2):
        
        mid1 = s // 2
        start1 = np.maximum(mid1 - kWidth, 0)
        end1 = np.minimum(mid1 + kWidth, splitLen)

        mid2 = s // 2 + 1
        start2 = np.maximum(mid2 - kWidth, 0)
        end2 = np.minimum(mid2 + kWidth, splitLen)
        
        anal[s+0] = audio[s+0]
        anal[s+1] = audio[s+1]
        
        if mid1 - start1 > 0:
            anal[s+0] += np.sum(odd[start1:mid1] * hKernelL[0:mid1-start1]) * 1j
            anal[s+1] += np.sum(even[start2:mid2] * hKernelL[0:mid2-start2]) * 1j
        if end1 - mid1 > 0:
            anal[s+0] += np.sum(odd[mid1:end1] * hKernelR[0:end1-mid1]) * 1j
            anal[s+1] += np.sum(even[mid2:end2] * hKernelR[0:end2-mid2]) * 1j
            
    return anal
    
@numba.jit
def downsample_audio_2x(audio, kWidth=1024):
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
def upsample_audio_2x(audio, kWidth = 1024):
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
    
def quantize_audio_i8(audio):
    u = 255
    clipped = np.minimum(np.maximum(audio, -1), 1)
    quantized = (u / 2 * np.sign(clipped) * np.log(1 + u * np.absolute(clipped)) / np.log(1 + u)).astype(np.int8)
    return quantized

def dequantize_audio_i8(quantized):
    u = 255
    audio = (quantized.astype(np.float64)) / u * 2
    audio = np.sign(audio) * (1 / u) * (((1 + u) ** np.absolute(audio)) - 1)
    return audio

def dequantize_sample_i8(quantized):
    u = 255
    audio = quantized / u * 2
    audio = np.sign(audio) * (1 / u) * (((1 + u) ** np.absolute(audio)) - 1)
    return audio
    
def create_sliding_dft(fftLen):
    fft = np.zeros(fftLen, np.complex128)
    fftq = np.zeros(fftLen, np.complex128)
    for q in range(fftLen): fftq[q] = np.exp(complex(0,  (q + 0.5) / fftLen * np.pi))
    
    comb = np.ones(fftLen)
    for q in range(1, fftLen, 2): comb[q] = -1
    
    return fft, fftq, comb

@numba.jit
def slide_dft(fft, fftq, sample, comb):
    fft = (fft + sample) * fftq
    fft -= np.sum(fft).real / len(fft)
    return fft, np.sum(fft * comb) / len(fft)

@numba.jit
def get_filter_bounds(filter, cutoff):
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
    
def synth_log_normal_filters(filterLen, numFilters, numOctaves, bandwidth, sampleStd, filterCutoff=0, logGabor=False, spatialFilter=False): 
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
        
        coverage += f * np.sqrt(gain)
        if spatialFilter: f = np.fft.fftshift(np.fft.ifft(f))
        filterBounds.append(get_filter_bounds(f, filterCutoff))
        f *= np.sqrt(gain)
        filters.append(f)
        
    return filters, filterBounds, coverage / np.max(coverage), sampleX

@numba.jit    
def sample_filter(sample, filter, filterBounds): 
    layerResponse = np.zeros(len(sample) - FFT_LEN, np.complex128)
    for i in range(len(layerResponse)):
        layerResponse[i] = np.sum(sample[i + filterBounds[0]:i + filterBounds[1]] * filter[filterBounds[0]:filterBounds[1]])
    
    return layerResponse

def sample_filter_set(sample, filters, filterBounds=None):
    response = np.zeros(len(filters), np.complex128)
    if filterBounds == None:
        for i in range(len(filters)): response[i] = np.dot(sample, filters[i])
    else:
        for i in range(len(filters)): response[i] = np.dot(sample[filterBounds[i][0]:filterBounds[i][1]], filters[i][filterBounds[i][0]:filterBounds[i][1]])
    
    return response
   
def inverse_filter_set(sampleLen, filters, coeff, filterBounds=None):
    buffer = np.zeros(sampleLen, coeff[0].dtype)
    if filterBounds == None:
        for i in range(len(filters)): buffer += filters[i] * coeff[i]
    else:
        for i in range(len(filters)): buffer[filterBounds[i][0]:filterBounds[i][1]] += filters[i][filterBounds[i][0]:filterBounds[i][1]] * coeff[i]
    
    return buffer
        
def dump_filters_to_file(filters, path):
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
def complex_quantize(filter, quantMax, powOffset, powScale):  
    quantized = np.zeros(len(filter), np.complex128)
    for i in range(len(filter)):
        c = filter[i]
        phase = np.floor((np.angle(c) + np.pi) / 2 / np.pi * quantMax + 0.5) % quantMax
        pow = np.minimum(np.floor(np.maximum(np.log(np.absolute(c)) - powOffset, 0) * powScale + 0.5), quantMax - 1)
        if np.isfinite(pow) == False: pow = 0
        quantized[i] = complex(pow, phase)
    return quantized
        
@numba.jit
def complex_dequantize(quantized, quantMax, powOffset, powScale):
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

def load_quant_file(path):
    quants = np.fromfile(path, np.ubyte)
    quants = np.reshape(quants, (len(quants) // NUM_FILTERS, NUM_FILTERS))
    cQuants = np.zeros((quants.shape[0], NUM_FILTERS), np.complex128)
    cQuants = (quants // QUANT_MAX) + (quants % QUANT_MAX) * 1j
    return cQuants

def save_quant_file(cQuants, path):
    quants = np.zeros((cQuants.shape[0], NUM_FILTERS), np.ubyte)
    quants = (np.real(cQuants) * QUANT_MAX + np.imag(cQuants)).astype(np.ubyte)
    quants.tofile(path)
    return

def normalize(a):
    return np.nan_to_num(a / np.sqrt(np.sum(np.absolute(a) ** 2)), 0)

    
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Filter and quantize a float64 raw PCM 44.1khz mono audio file')
    parser.add_argument('--inputraw', default='',
                        help='path of the audio file to quantize (optional)')
    parser.add_argument('--inputrawlen', type=int, default=-1,
                        help='number of samples to read from audio file (optional)')                        
    parser.add_argument('--inputquants', default='',
                        help='path of the quant file to synthesize (optional)')
    parser.add_argument('--outputraw', default='',
                        help='path of the output audio file (optional)')
    parser.add_argument('--outputquants', default='',
                        help='path of the output quant file (optional)')
    parser.add_argument('--debugfilters', action='store_true',
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
        
    lgFilters, lgFilterBounds, lgTotalResponse, lgQ = synth_log_normal_filters(FFT_LEN, NUM_FILTERS, NUM_OCTAVES, BANDWIDTH, FILTER_STD, FILTER_CUTOFF)
    
    if args.debugfilters:
        dump_filters_to_file(lgFilters, "lgFilterSet.raw")
        print("LG Filter frequency range: " + str(lgQ[0] * 22050) + " - " + str(lgQ[-1] * 22050))
        (lgTotalResponse / np.max(lgTotalResponse)).tofile("lgFilterSetCoverage.raw")

    if args.inputraw != '':
        inputRawPath = os.path.join(DATA_DIR, args.inputraw)
        print("Loading audio from " + inputRawPath + "...")
        inData = np.fromfile(inputRawPath, count=args.inputrawlen)
        numOutputSamples = len(inData)
    if args.inputquants != '':
        inputQuantPath = os.path.join(DATA_DIR, args.inputquants)
        print("Loading quants from " + inputQuantPath + "...")
        inputQuants = load_quant_file(inputQuantPath)
        numOutputSamples = inputQuants.shape[0] * CHUNK_STEP + FFT_LEN
    if args.outputraw != '':
        outData = np.zeros(numOutputSamples)
    if args.outputquants != '':
        outputQuants = np.zeros(((numOutputSamples - FFT_LEN) // CHUNK_STEP + 1, NUM_FILTERS), np.complex128)
        
    step = 0

    for i in range(0, numOutputSamples - FFT_LEN, CHUNK_STEP):
        if args.inputraw != '':
            fft = np.fft.fft(np.fft.fftshift(inData[i:i + FFT_LEN]))
            lgResponse = sample_filter_set(fft, lgFilters, lgFilterBounds)
            #lgResponse = complex_quantize(lgResponse, QUANT_MAX, QUANT_POWOFFSET, QUANT_POWSCALE)
            
            if args.outputquants != '':
                outputQuants[step] = lgResponse
        
        if args.inputquants != '':
            lgResponse = inputQuants[step]
            
        if args.outputraw != '':
            #lgResponse = complex_dequantize(lgResponse, QUANT_MAX, QUANT_POWOFFSET, QUANT_POWSCALE)
            reconstructed = inverse_filter_set(FFT_LEN, lgFilters, lgResponse, lgFilterBounds)
            chunk = np.fft.fftshift(np.fft.ifft(reconstructed))
            outData[i:i + FFT_LEN] += chunk.real
        
        step += 1
        if step & 63 == 0:
            progress = i / (numOutputSamples - FFT_LEN) * 100
            print("Filtering... {0:.0f}%".format(progress), end='\r')

    print("Filtering... 100%")

    if args.outputquants:
        qPath = os.path.join(DATA_DIR, args.outputquants)
        print('Saving quants to ' + qPath + '...')
        #save_quant_file(outputQuants, qPath)
        logResponse = np.zeros((step, NUM_FILTERS))
        logResponse[:] = np.nan_to_num(np.real(np.log(outputQuants)), 0)
        logResponse = np.maximum(logResponse + 4, 0)
        #logResponse[:] = np.real(outputQuants)
        logResponse.tofile(qPath)
    if args.outputraw:
        oPath = os.path.join(DATA_DIR, args.outputraw)
        print('Saving output to ' + oPath + '...')
        (outData / np.max(outData)).tofile(oPath)
 
