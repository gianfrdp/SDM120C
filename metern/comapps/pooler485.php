#!/usr/bin/php
<?php
if (isset($_SERVER['REMOTE_ADDR'])) {
    die('Direct access not permitted');
}
// This script will output a meterN compatible format for the main command
// You'll need to setup the path to meterN ($pathtomn). Put the meters numbers ($metnum) and the corresponding command ($cmd) :

$pathtomn = '/var/www/metern';
$output = shell_exec('pkill -f poolmeters485.sh > /dev/null 2>&1 &'); // Kill temporary the "live values fetching"

if (isset($argv[1])) {

    $metnum = $argv[2];
    $address = $argv[3];
    $baud = $argv[4];
    $device = $argv[5];

    if ($argv[1] == 'elect') {
        $cmd    = "$pathtomn/comapps/poolmeters485.sh relect $address $baud $device"; // Request counters values during a 5 min period
	#echo "$cmd\n";
    } else if ($argv[1] == 'live') {
	$cmd = "$pathtomn/comapps/poolmeters485.sh live $address $baud $device > /dev/null 2>&1 &"; // Restart live fetching at the last counter request
	echo "$cmd\n";
	$output = shell_exec($cmd);
	exit(0);
    } else {
        die('Aborting: no valid argument given\n');
    }
} else {
    die("Usage: pooler485 {elect|live} metnum address baud device\n");
}
// End of setup

define('checkaccess', TRUE);
include("$pathtomn/config/config_main.php");
include("$pathtomn/config/config_met$metnum.php");

// Retrieve previous value in the last daily csv
$dir    = "$pathtomn/data/csv";
$output = glob($dir . '/*.csv');
sort($output);
$csvcnt  = count($output);
$file = file($output[$csvcnt - 1]);
$cnt  = count($file);

if ($cnt==1 && $csvcnt>1) { // Midnight takes yesterday file
$file = file($output[$csvcnt - 2]);
}
$cnt  = count($file);

$i = 0;
while (!isset($prevval)) {
    $i++;
    $array = preg_split('/,/', $file[$cnt - $i]);
    if (!empty($array[$metnum])) {
        $prevval = $array[$metnum];
    }
    if ($i == $cnt) {
        $prevval = 0; // didn't find any prev. value
    }
}
sleep(1); // oh why ?
// Now retrieve the current value
$datareturn = shell_exec($cmd);
#echo "$datareturn\n";
$datareturn = trim($datareturn);
#echo "$datareturn\n";
$datareturn = preg_replace("/^${'ID'.$metnum}\(/i", '', $datareturn); // VALUE*UNIT)
#echo "$datareturn\n";
$lastval    = preg_replace("/\*[a-z0-9]+\)$/i", '', $datareturn); // VALUE
#echo "$lastval\n";

settype($lastval, 'float');
settype($prevval, 'float');
if ($lastval == 0)
    $lastval = $prevval;

if (${'PASSO' . $metnum} > 0 && $lastval > ${'PASSO' . $metnum}) { // counter pass over
    $lastval -= ${'PASSO' . $metnum};
}
$lastval = round($lastval, ${'PRECI' . $metnum});
$str     = utf8_decode("${'ID'.$metnum}($lastval*${'UNIT'.$metnum})\n");
echo "$str";

?>
