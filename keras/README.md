This project was an experiment to test different neural network architectures' performance on raw audio generation with various pre-processed input formats.

rebuild_dataset.ps1 - Takes a compressed input dataset and converts to raw uncompressed mono audio at a fixed sample rate
lgprocess.ps1 - Preprocess audio for input to neural network. Can generate and decode power-spectral-density log-spectrograms.
model.py - Keras model definition
train.py - Model training commands
sample.py - Model sampling commands
