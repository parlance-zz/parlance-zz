import argparse
import json
import os
from pathlib import Path
import sys
import random

import numpy as np
import numba

from model import build_model, save_weights, load_weights
import lgprocess

DATA_DIR = './data'
LOG_DIR = './logs'
MODEL_DIR = './model'

BATCH_SIZE = 64
SEQ_LEN = 64

class TrainLogger(object):
    def __init__(self, file, resume=0):
        self.file = os.path.join(LOG_DIR, file)
        self.epochs = resume
        if not resume:
            with open(self.file, 'w') as f:
                f.write('epoch,loss,acc\n')

    def add_entry(self, loss, acc):
        self.epochs += 1
        s = '{},{},{}\n'.format(self.epochs, loss, acc)
        with open(self.file, 'a') as f:
            f.write(s)
         
def BuildCorpus(qFiles, layer):
    numLayerInputs = (layer + 1) * 2
    corpus = np.zeros((0, numLayerInputs), np.int8)
    for f in range(len(qFiles)):
        file = qFiles[f]
        fileData = np.zeros((len(file[layer]), numLayerInputs), np.int8)
        for i in range(len(file[layer])):
            fileData[i, 0] = file[layer][i, 0]
            fileData[i, 1] = file[layer][i, 1]
            Li = i // 2
            inputIndex = 2
            for L in range(layer, 0, -1):
                fileData[i, inputIndex + 0] = file[layer - 1][Li, 0]
                fileData[i, inputIndex + 1] = file[layer - 1][Li, 1]
                Li = Li // 2
                inputIndex += 2
                
        corpus = np.concatenate((corpus, fileData))
    return corpus
    
def read_batches(qFiles, layer):
        
    random.shuffle(qFiles)
    corpus = BuildCorpus(qFiles, layer)
    
    print('Layer {0} has a corpus size of {1}'.format(layer, corpus.shape[0]))
    
    print(corpus.shape)
    
    length = corpus.shape[0]
    batch_chars = length // BATCH_SIZE
    numLayerInputs = (layer + 1) * 2
    
    for start in range(0, batch_chars - SEQ_LEN, SEQ_LEN):
        X = np.zeros((BATCH_SIZE, SEQ_LEN, numLayerInputs, lgprocess.QUANT_MAX), np.float32)
        Y = np.zeros((BATCH_SIZE, SEQ_LEN, 2, lgprocess.QUANT_MAX), np.float32)
        for batch_idx in range(BATCH_SIZE):
            for i in range(SEQ_LEN):
                for q in range(numLayerInputs): X[batch_idx, i, q, int(corpus[batch_chars * batch_idx + start + i, q])] = 1
                for q in range(2): Y[batch_idx, i, q, int(corpus[batch_chars * batch_idx + start + i + 1, q])] = 1
                 
        yield X, Y

def train(layer, epochs, save_freq, resume=False):

    modelPath = '{0}/layer{1}'.format(MODEL_DIR, layer)
    if not os.path.exists(modelPath):
        os.makedirs(modelPath)
    
    resume_epoch = 0
    
    if resume:
        print("Attempting to resume last training...")
        model_dir = Path(modelPath)
        checkpoints = list(model_dir.glob('weights.*.h5'))
        if not checkpoints:
            print('No checkpoints found to resume from')
        else:
            resume_epoch = max(int(p.name.split('.')[1]) for p in checkpoints)
            print("Resuming from epoch", resume_epoch)

    model = build_model(BATCH_SIZE, SEQ_LEN, layer)
    #opt = Adam() # Adam(lr=..., decay=...)
    model.compile(loss = 'categorical_crossentropy', optimizer='adam', metrics=['accuracy'])
    model.summary()

    if resume and (resume_epoch > 0):
        load_weights(resume_epoch, layer, model)

    log = TrainLogger('training_log_layer{}.csv'.format(layer), resume_epoch)

    print('Loading data set from {}...'.format(DATA_DIR))
    p = Path(DATA_DIR)
    files = list(p.glob('*.q'))    
    
    qFiles = []
    for file in files:
        quants = np.fromfile(file, np.int8)
        quants = np.reshape(quants, (len(quants) // 2, 2))
        layerLens = np.fromfile('{}.layers'.format(file), np.int64)
        assert len(layerLens) == lgprocess.NUM_LAYERS
        assert np.sum(layerLens) == len(quants)
        
        qLayers = []
        offset = 0
        for L in range(lgprocess.NUM_LAYERS):
            qLayers.append(quants[offset:offset + layerLens[L]])
            offset += layerLens[L]
        qFiles.append(qLayers)
        
    for epoch in range(0, epochs):
        print('\nLayer {0} - Epoch {1}'.format(layer, resume_epoch + epoch + 1))
        losses, accs = [], []
        for i, (X, Y) in enumerate(read_batches(qFiles, layer)):
            loss, acc = model.train_on_batch(X, Y)
            print('Batch {}: loss = {:.4f}, acc = {:.5f}'.format(i + 1, loss, acc))
            losses.append(loss)
            accs.append(acc)

        log.add_entry(np.average(losses), np.average(accs))

        if (epoch + 1) % save_freq == 0:
            save_weights(resume_epoch + epoch + 1, layer, model)
            print('Saved checkpoint to', 'weights.{}.h5'.format(resume_epoch + epoch + 1))

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Train the model on some quantized audio')
    parser.add_argument('--layer', type=int, help='layer of the model to train')
    parser.add_argument('--epochs', type=int, default=200, help='number of epochs to train for')
    parser.add_argument('--freq', type=int, default=10, help='checkpoint save frequency')
    parser.add_argument('--resume', default=True, help='resume from previously interrupted training')
    args = parser.parse_args()

    if not os.path.exists(LOG_DIR):
        os.makedirs(LOG_DIR)
    if not os.path.exists(MODEL_DIR):
        os.makedirs(MODEL_DIR)
    if not os.path.exists(DATA_DIR):
        os.makedirs(DATA_DIR)
    
    if args.layer == None:
        print('Error - Layer num is required')
        exit(1)
    assert args.layer >= 0
    assert args.layer < lgprocess.NUM_LAYERS
    
    train(args.layer, args.epochs, args.freq, args.resume)
