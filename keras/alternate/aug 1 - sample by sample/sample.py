import argparse
import json
import os

import numpy as np
import numba

from keras.models import Sequential, load_model
from keras.layers import LSTM, Dropout, TimeDistributed, Dense, Activation, Embedding, Input

import lgprocess
from model import build_model, load_weights
#import train
DATA_DIR = './data'
MODEL_DIR = './model'

def build_sample_model():
    return build_model(1, 1, True)

def collapse(result, temperature):
    sample = np.nan_to_num(result, 0)
    qList = list(range(lgprocess.SAMPLE_QUANT_MAX))
    dist = np.nan_to_num(np.exp(np.log(sample[0, 0]) / temperature), 0)
    m = np.sum(dist)
    if m > 0:
        dist /= m
    else:
        dist += 1
        dist /= np.sum(dist)

    quant = np.random.choice(qList, p=dist)
    return quant
    
def sample(epoch, seedraw, seedquants, seedLen, num_samples, temperature):

    model = build_sample_model()
    load_weights(epoch, model)
    #model.save(os.path.join(MODEL_DIR, 'model.{}.h5'.format(epoch)))
    
    lgFilters, lgFilterBounds, lgTotalResponse, lgQ = lgprocess.SynthLogNormalFilters(lgprocess.FFT_LEN, lgprocess.NUM_FILTERS, lgprocess.NUM_OCTAVES, lgprocess.BANDWIDTH, lgprocess.FILTER_STD, lgprocess.FILTER_CUTOFF)
    lgResponse = np.zeros(lgprocess.NUM_FILTERS, np.complex128)
    fft, fftq, comb = lgprocess.CreateSlidingDFT(lgprocess.FFT_LEN)
    for f in lgFilters: f *= comb
    
    sampled = np.zeros(num_samples)
    
    if seedraw != '' and seedquants != '':
        seedaudio = np.fromfile(os.path.join(DATA_DIR, seedraw))
        seedquants = np.fromfile(os.path.join(DATA_DIR, seedquants), np.int8)
        seedquants = np.reshape(seedquants, (len(seedquants) // lgprocess.NUM_FILTERS // 2, lgprocess.NUM_FILTERS, 2))
        assert seedquants.shape[0] == len(seedaudio)
        if seedquants.shape[0] < seedLen: seedLen = seedquants.shape[0]
        for i in range(seedLen):
            batch = np.zeros((1, 1, lgprocess.NUM_FILTERS * 2, lgprocess.QUANT_MAX), np.float32)
            for q in range(lgprocess.NUM_FILTERS * 2):
                batch[0, 0, q, seedquants[i, q // 2, q % 2]] = 1
            result = model.predict_on_batch(batch)
            fft, outSample = lgprocess.SlideDFT(fft, fftq, seedaudio[i], comb)
            sampled[i] = outSample.real
            
    else:
        print("Error: Seed data is required")
        exit(1)
    
    for i in range(seedLen, num_samples):
        batch = np.zeros((1, 1, lgprocess.NUM_FILTERS * 2, lgprocess.QUANT_MAX), np.float32)
        
        lgResponse = lgprocess.SampleFilterSet(fft, lgFilters, lgFilterBounds)
        lgResponse = lgprocess.ComplexQuantize(lgResponse, lgprocess.QUANT_MAX, lgprocess.QUANT_POWOFFSET, lgprocess.QUANT_POWSCALE)
        for q in range(0, lgprocess.NUM_FILTERS * 2, 2):
            batch[0, 0, q + 0, int(lgResponse[q // 2].real)] = 1
            batch[0, 0, q + 1, int(lgResponse[q // 2].imag)] = 1
            
        result = model.predict_on_batch(batch)
        sample = lgprocess.DequantizeSample(np.int8(collapse(result, temperature)))
        
        fft, outSample = lgprocess.SlideDFT(fft, fftq, sample, comb)
        sampled[i] = outSample.real
        
        """
        lgResponse = np.zeros(lgprocess.NUM_FILTERS, np.complex128)
        for q in range(lgprocess.NUM_FILTERS):
            lgResponse[q] = complex(np.argmax(result[0, 0, q * 2 + 0]), np.argmax(result[0, 0, q * 2 + 1]))
        lgResponse = lgprocess.ComplexDequantize(lgResponse, lgprocess.QUANT_MAX, lgprocess.QUANT_POWOFFSET, lgprocess.QUANT_POWSCALE)            
        totalPow = np.sum(np.absolute(lgResponse) ** 2)
        reconstructed = lgprocess.InverseFilterSet(lgprocess.FFT_LEN, lgFilters, lgResponse, lgFilterBounds)
        lgResponse = lgprocess.SampleFilterSet(reconstructed, lgFilters, lgFilterBounds)
        resampledPow = np.sum(np.absolute(lgResponse) ** 2)
        lgResponse *= np.sqrt(totalPow) / np.sqrt(resampledPow)
        lgResponse = lgprocess.ComplexQuantize(lgResponse, lgprocess.QUANT_MAX, lgprocess.QUANT_POWOFFSET, lgprocess.QUANT_POWSCALE)
        for q in range(lgprocess.NUM_FILTERS):
            sample[0, 0, q * 2 + 0] = 0
            sample[0, 0, q * 2 + 0, int(lgResponse[q].real)] = 1
            sample[0, 0, q * 2 + 1] = 0
            sample[0, 0, q * 2 + 1, int(lgResponse[q].imag)] = 1
        """
        
        if i & 0xFFF == 0:
            progress = i / num_samples * 100
            if print("Sampling... {0:.2f}%".format(progress), end='\r')

    print("Sampling... 100.00%")
    return sampled

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Sample some audio from the trained model.')
    parser.add_argument('epoch', type=int, help='epoch checkpoint to sample from')
    parser.add_argument('--seedraw', default='', help='initial seed audio for the generated audio')
    parser.add_argument('--seedquants', default='', help='initial seed quants for the generated audio')
    parser.add_argument('--seedlen', type=int, default=(lgprocess.FFT_LEN * 8), help='number of samples to read from seed audio')
    parser.add_argument('--len', type=int, default=(44100 * 12), help='number of samples to sample')
    parser.add_argument('--temperature', type=float, default=1, help='sampling temperature (optional, 1 is default)')
    parser.add_argument('--outfile', default='sampled.raw', help='output file path')
    args = parser.parse_args()

    output = sample(args.epoch, args.seedraw, args.seedquants, args.seedlen, args.len, args.temperature)
    output.tofile(os.path.join(DATA_DIR, args.outfile))
	