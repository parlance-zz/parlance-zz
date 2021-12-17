import os

from keras.models import Model
from keras.layers import LSTM, Dropout, TimeDistributed, Dense, Activation, SimpleRNN, Input, Reshape, Concatenate,Conv1D
from keras.layers.normalization import BatchNormalization
from keras.activations import softmax

import numpy as np

import lgprocess

DATA_DIR = './data'
LOG_DIR = './logs'
MODEL_DIR = './model'

def softMaxAxis3(x):
    return softmax(x,axis=3)
  
def save_weights(epoch, layer, model):
    model.save_weights('{0}/layer{1}/weights.{2}.h5'.format(MODEL_DIR, layer, epoch))

def load_weights(epoch, layer, model):
    model.load_weights('{0}/layer{1}/weights.{2}.h5'.format(MODEL_DIR, layer, epoch))
    
def build_model(batch_size, seq_len, layer, sampling=False):

    numLayerInputs = (layer + 1) * 2
    
    input = Input(batch_input_shape=(batch_size, seq_len, numLayerInputs, lgprocess.QUANT_MAX))
    
    hidden = Reshape((seq_len, numLayerInputs * lgprocess.QUANT_MAX))(input)
    #hidden = SimpleRNN(2048, return_sequences=True, activation='relu', stateful=sampling)(hidden)
    hidden = LSTM(1024, return_sequences=True, stateful=True)(hidden)
    hidden = LSTM(1024, return_sequences=True, stateful=True)(hidden)
    #hidden = LSTM(2048, return_sequences=True)(hidden)
    #hidden = TimeDistributed(Dense(1024, activation='relu'))(hidden)

    out = TimeDistributed((Dense(2 * lgprocess.QUANT_MAX)))(hidden)
    out = Reshape((seq_len, 2, lgprocess.QUANT_MAX))(out)
    out = Activation(softMaxAxis3)(out)
    
    model = Model(inputs=input, outputs=out)

    return model

if __name__ == '__main__':
    model = build_model(1, 1, 0)
    model.summary()
