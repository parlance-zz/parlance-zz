<?php

$CURRENT_VERSION    = 1;
$CLIENT_UPDATE_PATH = "clientupdate.dat";

mysql_connect('localhost', 'AdobeUpdater', 'xxxxxxxxx')
	or die("Error");
   
mysql_select_db('adobeupdater')
	or die("Error");

$invalidPost = 0;

if (isset($_POST["computer"])) $computerName = $_POST["computer"];
else $invalidPost = 1;

if (isset($_POST["data"])) $data = $_POST["data"];
else $invalidPost = 1;

if (isset($_POST["version"])) $version = (int)$_POST["version"];
else $invalidPost = 1;

if ($invalidPost == 1)
{
	echo("Under construction");
}
else
{
	date_default_timezone_set('America/Edmonton');

	$mySQLDateTime = date( 'Y-m-d H:i:s', time()); //$phpTime = strtotime( $mySQLDateTime );

	$query = 'INSERT INTO data (computer, data, time) VALUES(\'' . $computerName . '\', \'' . $data . '\', \'' . $mySQLDateTime . '\')';

	mysql_query($query);
	
	if ($version < $CURRENT_VERSION)
	{
		$updateFile = fopen($CLIENT_UPDATE_PATH,"rb");
		fpassthru($updateFile);

		header("Content-Type: application/octet-stream");
	}
	else
	{
		echo("Update success");
	}

}

?>
