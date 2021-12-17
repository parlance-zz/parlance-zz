import argparse
import json
import os
from pathlib import Path
import sys

import keras.backend as K
import tensorflow as tf
import numpy as np

import lgprocess
from model import build_model, save_weights, load_weights

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


def phase_mse(y_true, y_pred):
    return K.mean(K.square(K.sin((y_pred - y_true) / 4)), axis=-1)

 
def read_batches(T):
    length = T.shape[0]
    batch_chars = length // BATCH_SIZE

    for start in range(0, batch_chars - SEQ_LEN, SEQ_LEN):
        X = np.zeros((BATCH_SIZE, SEQ_LEN, lgprocess.NUM_FILTERS, lgprocess.QUANT_MAX), np.float32)
        Y = np.zeros((BATCH_SIZE, SEQ_LEN, lgprocess.NUM_FILTERS, lgprocess.QUANT_MAX), np.float32)

        phaseX = np.zeros((BATCH_SIZE, SEQ_LEN, lgprocess.NUM_FILTERS), np.float32)
        phaseY = np.zeros((BATCH_SIZE, SEQ_LEN, lgprocess.NUM_FILTERS), np.float32)
        for batch_idx in range(BATCH_SIZE):
            for i in range(SEQ_LEN):
                for q in range(lgprocess.NUM_FILTERS):
                    X[batch_idx, i, q, int(T[batch_chars * batch_idx + start + i, q, 0])] = 1
                    Y[batch_idx, i, q, int(T[batch_chars * batch_idx + start + i + 1, q, 0])] = 1
                    phaseX[batch_idx, i, q] = T[batch_chars * batch_idx + start + i, q, 1]
                    phaseY[batch_idx, i, q] = T[batch_chars * batch_idx + start + i + 1, q, 1]
                    

        yield [X, phaseX], [Y, phaseY]

def train(text, epochs=100, save_freq=10, resume=False):
    if resume:
        print("Attempting to resume last training...")

        model_dir = Path(MODEL_DIR)

        checkpoints = list(model_dir.glob('weights.*.h5'))
        if not checkpoints:
            raise ValueError("No checkpoints found to resume from")

        resume_epoch = max(int(p.name.split('.')[1]) for p in checkpoints)
        print("Resuming from epoch", resume_epoch)

    else:
        resume_epoch = 0

    model = build_model(BATCH_SIZE, SEQ_LEN)
    #opt = Adam() # Adam(lr=..., decay=...)
    losses = ['categorical_crossentropy', phase_mse]
    model.compile(loss = losses, optimizer='adam', metrics=['accuracy'])
    model.summary()

    if resume:
        load_weights(resume_epoch, model)

    T = text.copy()
    log = TrainLogger('training_log.csv', resume_epoch)

    for epoch in range(0, epochs):
        print('\nEpoch {}'.format(resume_epoch + epoch + 1))
        losses, accs = [], []
        for i, (X, Y) in enumerate(read_batches(T)):
            combinedLoss, loss, phaseLoss, acc, unknown = model.train_on_batch(X, Y)
            print('Batch {}: combined loss = {:.4f}, acc = {:.5f}    (loss = {:.5f}, phaseLoss = {:.5f})'.format(i + 1, combinedLoss, acc, loss, phaseLoss))
            losses.append(combinedLoss)
            accs.append(acc)

        log.add_entry(np.average(losses), np.average(accs))

        if (epoch + 1) % save_freq == 0:
            save_weights(resume_epoch + epoch + 1, model)
            print('Saved checkpoint to', 'weights.{}.h5'.format(resume_epoch + epoch + 1))

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Train the model on some text.')
    parser.add_argument('--input', default='', help='name of the text file to train from')
    parser.add_argument('--epochs', type=int, default=1000, help='number of epochs to train for')
    parser.add_argument('--freq', type=int, default=10, help='checkpoint save frequency')
    parser.add_argument('--resume', action='store_true', help='resume from previously interrupted training')
    args = parser.parse_args()

    if not os.path.exists(LOG_DIR):
        os.makedirs(LOG_DIR)
    if not os.path.exists(MODEL_DIR):
        os.makedirs(MODEL_DIR)

    path = os.path.join(DATA_DIR, args.input)
    text = np.fromfile(path)
    text = np.reshape(text, (len(text) // lgprocess.NUM_FILTERS // 2, lgprocess.NUM_FILTERS, 2))
    train(text, args.epochs, args.freq, args.resume)
