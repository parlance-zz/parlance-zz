$DATA_DIR = './data'
$sourcePath = "./data_source"
$ffmpegPath = "./FFMpeg/bin"

$sourceFiles = Get-ChildItem ($sourcePath)
ForEach ($source in $sourceFiles)
{
	$mediaFile = $false
	if ($source.extension -like ".mp3") { $mediaFile = $true }
	if ($source.extension -like ".flac") { $mediaFile = $true }
	if ($source.extension -like ".wav") { $mediaFile = $true }
	if ($source.extension -like ".m4a") { $mediaFile = $true }
	if ($mediaFile -eq $false) { Continue }
	
	$inputPath = $sourcePath + '/' + ($source.Name)
	$outputPath = $DATA_DIR + '/' + ($source.Name).Replace($source.extension, '.raw')
	
	& "$ffmpegPath/ffmpeg.exe" -y -i $inputPath -filter:a "volume=0.7" -ac 1 -ar 44100 -f f64le $outputPath
}

# ****

$raws = Get-ChildItem $DATA_DIR -Filter '*.raw'
$raws
"Processing " + ($raws.Count) + " files..."

ForEach ($raw in $raws)
{
	if ($raw.name -like "*_processed.raw") { continue }
	if ($raw.name -like "out*") { continue }
	if ($raw.name -like "sampled*") { continue }
	
	$outquants = ($raw.Name).Replace('.raw', '.q')
	
	#if (Test-Path ($DATA_DIR + '/' + $outquants))
	#{
	#	"Skipping " + $outquants + "..."
	#	Continue
	#}
	
	py ./lgprocess.py --inputraw ($raw.Name) --outputquants $outquants --outputraw out.raw
}