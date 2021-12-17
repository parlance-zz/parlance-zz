import os

from keras.models import Sequential, load_model, Model
from keras.layers import LSTM, Dropout, TimeDistributed, Dense, Activation, Embedding, SimpleRNN, Input, Reshape, Concatenate
from keras.layers.normalization import BatchNormalization
from keras.activations import softmax
from keras import backend as KeyError

import numpy as np
import keras.backend as K

import lgprocess
import train

def softMaxAxis3(x):
    return softmax(x,axis=3)

def phasemod_activation(x):
    return (x - K.round(x / (200 * 2 * np.pi)) * 200 * 2 * np.pi)

def phasemod_activation_sampling(x):
    return (x - K.round(x / (2 * np.pi)) * 2 * np.pi)
    
def save_weights(epoch, model):
    if not os.path.exists(train.MODEL_DIR):
        os.makedirs(train.MODEL_DIR)
    model.save_weights(os.path.join(train.MODEL_DIR, 'weights.{}.h5'.format(epoch)))

def load_weights(epoch, model):
    model.load_weights(os.path.join(train.MODEL_DIR, 'weights.{}.h5'.format(epoch)))
    
def build_model(batch_size, seq_len, sampling=False):

    input = Input(batch_input_shape=(batch_size, seq_len, lgprocess.NUM_FILTERS, lgprocess.QUANT_MAX))
    phaseInput = Input(batch_input_shape=(batch_size, seq_len, lgprocess.NUM_FILTERS))
    
    reshapedPhaseInput = Reshape((seq_len, lgprocess.NUM_FILTERS, 1))(phaseInput)
    combinedInput = Concatenate()([input, reshapedPhaseInput])
    
    hidden = Reshape((seq_len, lgprocess.NUM_FILTERS * (lgprocess.QUANT_MAX + 1)))(combinedInput)
    #hidden = SimpleRNN(4096, return_sequences=True, stateful=True, activation='relu', recurrent_initializer='zeros')(hidden)
    hidden = TimeDistributed(Dense(8192, activation='relu'))(hidden)
    
    out = TimeDistributed((Dense(lgprocess.NUM_FILTERS * lgprocess.QUANT_MAX)))(hidden)
    out = Reshape((seq_len, lgprocess.NUM_FILTERS, lgprocess.QUANT_MAX))(out)
    out = Activation(softMaxAxis3)(out)

    phaseOut = TimeDistributed((Dense(lgprocess.NUM_FILTERS)))(hidden)
    if sampling:
        phaseOut = Activation(phasemod_activation_sampling)(phaseOut)
    else:
        phaseOut = Activation(phasemod_activation)(phaseOut)
    
    model = Model(inputs=[input, phaseInput], outputs=[out, phaseOut])

    return model

if __name__ == '__main__':
    model = build_model(train.BATCH_SIZE, train.SEQ_LEN)
    model.summary()
