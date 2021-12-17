import argparse
import json
import os
from pathlib import Path
import sys
import random

import numpy as np
import numba

from model import Model
import lgprocess

DATA_DIR = './data'
LOG_DIR = './logs'
MODEL_DIR = './model'

SEQ_OVERLAP = 3

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
    
def read_batches(qFiles):
    random.shuffle(qFiles)
    for file in qFiles:
        for i in range(0, file.shape[0] - model.SEQ_LEN, SEQ_LEN // SEQ_OVERLAP):
            X = file[i:i + SEQ_LEN]
            yield X

def train(epochs, save_freq):

    resume_epoch = 0
    print("Loading last checkpoint...")
    model_dir = Path(MODEL_DIR)
    checkpoints = list(model_dir.glob('model.*.rmx'))
    if not checkpoints:
        print('No checkpoints found to resume from. Create a model with model.py first.')
        exit(1)
    else:
        resume_epoch = max(int(p.name.split('.')[1]) for p in checkpoints)
        print("Resuming from epoch", resume_epoch, "...")

    model = Model(resume_epoch)
    log = TrainLogger('training_log.csv', resume_epoch)

    print('Loading data set from {}...'.format(DATA_DIR))
    files = list(Path(DATA_DIR).glob('*.q'))    
    qFiles = []
    for file in files:
        quants = np.fromfile(file, np.int8)
        quants = np.reshape(quants, (len(quants) // lgprocess.NUM_FILTERS, lgprocess.NUM_FILTERS)).astype(np.int64)
        qFiles.append(quants)
        
    for epoch in range(epochs):
        print('Epoch {0}'.format(resume_epoch + epoch + 1))
        losses, accs = [], []
        for i, (X) in enumerate(read_batches(qFiles)):
            loss, acc = model.train_on_batch(X)
            print('Batch {}: loss = {:.4f}, acc = {:.5f}'.format(i + 1, loss, acc))
            losses.append(loss)
            accs.append(acc)

        log.add_entry(np.average(losses), np.average(accs))

        if (epoch + 1) % save_freq == 0:
            print('Saving checkpoint to', 'model.{}.rmx...'.format(resume_epoch + epoch + 1))
            model.save(resume_epoch + epoch + 1)
            
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Train the model on the quantized dataset')
    parser.add_argument('--epochs', type=int, default=10, help='number of epochs to train for')
    parser.add_argument('--freq', type=int, default=1, help='checkpoint save frequency')
    args = parser.parse_args()

    if not os.path.exists(LOG_DIR):
        os.makedirs(LOG_DIR)
    if not os.path.exists(MODEL_DIR):
        os.makedirs(MODEL_DIR)
    if not os.path.exists(DATA_DIR):
        os.makedirs(DATA_DIR)
    
    train(args.epochs, args.freq)
