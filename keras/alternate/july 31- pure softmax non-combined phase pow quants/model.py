import os

from keras.models import Sequential, load_model, Model
from keras.layers import LSTM, Dropout, TimeDistributed, Dense, Activation, Embedding, SimpleRNN, Input, Reshape, Concatenate,Conv1D
from keras.layers.normalization import BatchNormalization
from keras.activations import softmax
from keras import backend as KeyError

import numpy as np
import keras.backend as K

import lgprocess
import train

def softMaxAxis3(x):
    return softmax(x,axis=3)
  
def save_weights(epoch, model):
    if not os.path.exists(train.MODEL_DIR):
        os.makedirs(train.MODEL_DIR)
    model.save_weights(os.path.join(train.MODEL_DIR, 'weights.{}.h5'.format(epoch)))

def load_weights(epoch, model):
    model.load_weights(os.path.join(train.MODEL_DIR, 'weights.{}.h5'.format(epoch)))
    
def build_model(batch_size, seq_len, sampling=False):

    input = Input(batch_input_shape=(batch_size, seq_len, lgprocess.NUM_FILTERS * 2, lgprocess.QUANT_MAX))

    hidden = Reshape((seq_len, lgprocess.NUM_FILTERS * 2 * lgprocess.QUANT_MAX))(input)
    
    #hidden = SimpleRNN(2048, return_sequences=True, stateful=True, activation='relu', recurrent_initializer='zeros')(hidden)
    #hidden = TimeDistributed(Conv1D(64, 16, activation='relu'))(hidden)
    #recurrent = LSTM(512, return_sequences=True, stateful=True)
    #hidden = Concatenate()([hidden, recurrent])
    #hidden = SimpleRNN(128, return_sequences=True, stateful=True, activation='relu')(hidden)
    
    hidden = TimeDistributed(Dense(2048, activation='relu'))(hidden)
    #hidden = TimeDistributed(Dense(2048))(hidden)
    #hidden = Reshape((seq_len, 1, 2048))(hidden)
    #hidden = Activation(softMaxAxis3)(hidden)
    #hidden = Reshape((seq_len, 2048))(hidden)
    
    out = TimeDistributed((Dense(lgprocess.NUM_FILTERS * 2 * lgprocess.QUANT_MAX)))(hidden)
    out = Reshape((seq_len, lgprocess.NUM_FILTERS * 2, lgprocess.QUANT_MAX))(out)
    out = Activation(softMaxAxis3)(out)

    model = Model(inputs=input, outputs=out)

    return model

if __name__ == '__main__':
    model = build_model(train.BATCH_SIZE, train.SEQ_LEN)
    model.summary()
