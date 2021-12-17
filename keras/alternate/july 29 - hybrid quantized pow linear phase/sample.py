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
    qList = list(range(lgprocess.QUANT_MAX))
    for q in range(lgprocess.NUM_FILTERS):
        dist = sample[0, 0, q] ** (1 / temperature)
        dist /= np.sum(dist)
        """
        if m > 0:
            dist /= m
        else:
            dist += 1
            dist /= np.sum(dist)
        """
        quant = np.random.choice(qList, p=dist)
        sample[0, 0, q] = 0
        sample[0, 0, q, quant] = 1
    
    return sample
    
def sample(epoch, header, headerLen, num_chars, temperature):

    model = build_sample_model()
    load_weights(epoch, model)
    #model.save(os.path.join(MODEL_DIR, 'model.{}.h5'.format(epoch)))
    lgFilters, lgFilterBounds, lgTotalResponse, lgQ = lgprocess.SynthLogNormalFilters(lgprocess.FFT_LEN, lgprocess.NUM_FILTERS, lgprocess.NUM_OCTAVES, lgprocess.BANDWIDTH, lgprocess.FILTER_STD, lgprocess.FILTER_CUTOFF)
    
    sampled = []
    if header != "":
        seeddata = np.fromfile(os.path.join(DATA_DIR, header))
        seeddata = np.reshape(seeddata, (len(seeddata) // lgprocess.NUM_FILTERS // 2, lgprocess.NUM_FILTERS, 2))
        if seeddata.shape[0] < headerLen: headerLen = seeddata.shape[0]
        for i in range(headerLen):
            s = np.zeros((1, 1, lgprocess.NUM_FILTERS, lgprocess.QUANT_MAX), np.float32)
            p = np.zeros((1, 1, lgprocess.NUM_FILTERS), np.float32)
            for q in range(lgprocess.NUM_FILTERS):
                s[0, 0, q, int(seeddata[i, q, 0])] = 1
                p[0, 0, q] = seeddata[i, q, 1]
            sampled.append([s,p])
            
    else:
        print("Error: Seed data is required")
        exit(1)
    
    for i in range(num_chars):
        s = np.zeros((1, 1, lgprocess.NUM_FILTERS, lgprocess.QUANT_MAX), np.float32)
        p = np.zeros((1, 1, lgprocess.NUM_FILTERS), np.float32)
        batch = [s,p]
        
        if i < len(sampled):
            batch = sampled[i]
        else:
            batch = sampled[-1]
            
        result, phaseResult = model.predict_on_batch(batch)
        sample = [collapse(result, temperature), phaseResult.copy()]
        
        """
        lgResponse = np.zeros(lgprocess.NUM_FILTERS, np.complex128)
        for q in range(lgprocess.NUM_FILTERS):
            lgResponse[q] = complex(np.argmax(result[0, 0, q]), phaseResult[0, 0, q])
        lgResponse = lgprocess.ComplexDequantize(lgResponse, lgprocess.QUANT_MAX, lgprocess.QUANT_POWOFFSET, lgprocess.QUANT_POWSCALE)            
        totalPow = np.sum(np.absolute(lgResponse) ** 2)
        reconstructed = lgprocess.InverseFilterSet(lgprocess.FFT_LEN, lgFilters, lgResponse, lgFilterBounds)
        lgResponse = lgprocess.SampleFilterSet(reconstructed, lgFilters, lgFilterBounds)
        resampledPow = np.sum(np.absolute(lgResponse) ** 2)
        lgResponse *= np.sqrt(totalPow) / np.sqrt(resampledPow)
        lgResponse = lgprocess.ComplexQuantize(lgResponse, lgprocess.QUANT_MAX, lgprocess.QUANT_POWOFFSET, lgprocess.QUANT_POWSCALE)
        for q in range(lgprocess.NUM_FILTERS):
            sample[0][0, 0, q] = 0
            sample[0][0, 0, q, int(lgResponse[q].real)] = 1
            sample[1][0, 0, q] = lgResponse[q].imag
        #""" 
        
        if i < len(sampled):
            #sampled[i] = sample
            b = 2
        else:
            sampled.append(sample)
        
        progress = i / num_chars * 100
        print("Sampling... {0:.0f}%".format(progress), end='\r')

    print("Sampling... 100%")
    return sampled

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Sample some text from the trained model.')
    parser.add_argument('epoch', type=int, help='epoch checkpoint to sample from')
    parser.add_argument('--seed', default='', help='initial seed for the generated text')
    parser.add_argument('--seedlen', type=int, default=1, help='number of characters to read from seed text')
    parser.add_argument('--len', type=int, default=1200, help='number of characters to sample ')
    parser.add_argument('--temperature', type=float, default=1, help='sampling temperature (optional, 1 is default)')
    parser.add_argument('--outfile', default='sampled.q', help='output file path')
    args = parser.parse_args()

    output = sample(args.epoch, args.seed, args.seedlen, args.len, args.temperature)
    
    packedOut = np.zeros((len(output), lgprocess.NUM_FILTERS, 2))
    for i in range(len(output)):
        s = output[i][0]
        p = output[i][1]
        for q in range(lgprocess.NUM_FILTERS):
            packedOut[i, q, 0] = np.argmax(s[0, 0, q])
            packedOut[i, q, 1] = p[0, 0, q]
    packedOut.tofile(os.path.join(DATA_DIR, args.outfile))
	
