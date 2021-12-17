import argparse
import json
import os
from pathlib import Path

import numpy as np
import numba

import lgprocess
from model import build_model, load_weights

DATA_DIR = './data'
LOG_DIR = './logs'
MODEL_DIR = './model'

def collapse(result, temperature):
    qList = list(range(lgprocess.QUANT_MAX))
    #dist = np.nan_to_num(np.exp(np.log(result) / temperature), 0)
    dist = np.nan_to_num(result, 0)
    m = np.sum(dist)
    if m > 0:
        dist /= m
    else:
        dist += 1
        dist /= np.sum(dist)

    quant = np.random.choice(qList, p=dist)
    return quant
    
def sample(num_samples, temperature, max_layer, seedquants, seedlayers=4):

    assert max_layer > 0
    assert max_layer <= lgprocess.NUM_LAYERS
    assert seedquants != ''
    assert seedlayers > 0
    
    """
    layerLens = []
    length = num_samples
    for i in range(lgprocess.NUM_LAYERS):
        layerLens.append(length)
        length = (length + 1) // 2
    layerLens.reverse()
    """
    layerQuants = []
    
    layerLens = np.fromfile('{0}/{1}.layers'.format(DATA_DIR, seedquants), np.int64)
    seeddata = np.fromfile('{0}/{1}'.format(DATA_DIR, seedquants), np.int8)
    seeddata = np.reshape(seeddata, (len(seeddata) // 2, 2))
    
    offset = 0
    for L in range(seedlayers):
        layerQuants.append(seeddata[offset:offset + layerLens[L]])
        offset += layerLens[L]
    
    for L in range(seedlayers, max_layer):
        
        quants = np.zeros((layerLens[L], 2), np.int8)
        
        model_dir = Path('{0}/layer{1}'.format(MODEL_DIR, L))
        checkpoints = list(model_dir.glob('weights.*.h5'))
        if not checkpoints:
            raise ValueError("No weights found for layer {}".format(L))

        sample_epoch = max(int(p.name.split('.')[1]) for p in checkpoints)
        
        model = build_model(1, 1, L, True)
        load_weights(sample_epoch, L, model)
        numLayerInputs = (L + 1) * 2
    
        for i in range(layerLens[L]):
        
            batch = np.zeros((1, 1, numLayerInputs, lgprocess.QUANT_MAX), np.float32)
            if i > 0:
                batch[0, 0, 0, quants[i - 1, 0]] = 1
                batch[0, 0, 1, quants[i - 1, 1]] = 1
            else:
                batch[0, 0, 0, lgprocess.QUANT_MAX // 2] = 1
                batch[0, 0, 1, lgprocess.QUANT_MAX // 2] = 1

            Li = i // 2
            inputIndex = 2
            for layer in range(L, 0, -1):
                batch[0, 0, inputIndex + 0, layerQuants[L - 1][Li, 0]] = 1
                batch[0, 0, inputIndex + 1, layerQuants[L - 1][Li, 1]] = 1
                Li = Li // 2
                inputIndex += 2
                    
            result = model.predict_on_batch(batch)
            quants[i, 0] = np.int8(collapse(result[0, 0, 0], temperature))
            quants[i, 1] = np.int8(collapse(result[0, 0, 1], temperature))
            
            if (i & 0xFF == 0) or (i == (layerLens[L] - 1)):
                progress = i / (layerLens[L] - 1) * 100
                if i < (layerLens[L] - 1):
                    print("Sampling layer {0} at epoch {1}... {2:.2f}%".format(L, sample_epoch, progress), end='\r')
                else:
                    print("Sampling layer {0} at epoch {1}... {2:.2f}%".format(L, sample_epoch, progress))
        
        layerQuants.append(quants.copy())
        
    return layerQuants, layerLens

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Sample some audio from the trained model.')
    parser.add_argument('--seedquants', default='', help='initial seed quants for the generated audio')
    parser.add_argument('--len', type=int, default=int(2 ** 22), help='number of samples to sample')
    parser.add_argument('--temperature', type=float, default=1, help='sampling temperature (optional, 1 is default)')
    parser.add_argument('--max_layer', type=int, default=lgprocess.NUM_LAYERS, help='number of layers to include in sample (optional, max is default)')
    parser.add_argument('--outfile', default='sampled.q', help='output file path')
    args = parser.parse_args()

    output, layerLens = sample(args.len, args.temperature, args.max_layer, args.seedquants)
    
    outquants = np.zeros((0, 2), np.int8)
    for layer in output:
        outquants = np.concatenate((outquants, layer))
    print('Saving quants to {}...'.format(args.outfile))
    outquants.tofile(args.outfile)
    
    outlayers = np.array(layerLens, dtype=np.int64)
    outlayers = outlayers[0:args.max_layer]
    print('Saving layers to {}.layers...'.format(args.outfile))
    outlayers.tofile('{}.layers'.format(args.outfile))