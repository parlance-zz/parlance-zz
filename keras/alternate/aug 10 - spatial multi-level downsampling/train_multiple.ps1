
$layers = @(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13)
#$layers = @(3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13)
$layerEpochs = @{0 = 5000; 1 = 4000; 2 = 1000; 3 = 800; 4 = 500; 5 = 400; 6 = 200; 7 = 160; 8 = 120; 9 = 100; 10 = 80; 11 = 60; 12 = 40; 13 = 30}
$layerFreqs = @{0 = 1000; 1 = 1000; 2 = 100; 3 = 100; 4 = 100; 5 = 50; 6 = 20; 7 = 20; 8 = 20; 9 = 5; 10 = 5; 11 = 5; 12 = 4; 13 = 2}

"Layers : " + $layers
"Layer Epochs: " + $layerEpochs.Values
"Layer Freqs: " + $layerFreqs.Values

foreach ($layer in $layers)
{
	py ./train.py --layer $layer --epochs $layerEpochs[$layer] --freq $layerFreqs[$layer]
}