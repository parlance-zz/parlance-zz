$DATA_DIR = './data'

$raws = Get-ChildItem $DATA_DIR -Filter '*.raw'
$raws
"Processing " + ($raws.Count) + " files..."

ForEach ($raw in $raws)
{
	if ($raw.name -like "*_processed.raw") { continue }
	if ($raw.name -like "out*") { continue }
	
	$outquants = ($raw.Name).Replace('.raw', '.q')
	$outraw = ($raw.Name).Replace('.raw', '_processed.raw')
	
	py ./lgprocess.py --inputraw ($raw.Name) --outputquants $outquants --outputraw $outraw
}