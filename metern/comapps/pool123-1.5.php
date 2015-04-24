<?php
if ($_SERVER['SERVER_ADDR'] != '127.0.0.1') {
    die('Direct access not permitted');
}
// This script will output a 123solar counter into a meterN compatible format
// You'll need to setup the path to 123, your inverter number and your meter id
// Request live command with "curl http://localhost/metern/comapps/pool123solar.php?cmd=1"
// Main command with "curl http://localhost/metern/comapps/pool123solar.php?cmd=2"

$pathto123s = '/srv/http/123solar';
$invtnums   = array (0 => 1, 1 => 2);
$meterid    = 4;

//parse_str(implode('&', array_slice($argv, 1)), $_GET);

// No edit is needed below
if (!empty($_GET['cmd']) && is_numeric($_GET['cmd'])) {
    $cmd = $_GET['cmd'];
} else {
    die('Wrong command');
}

define('checkaccess', TRUE);
include("$pathto123s/config/config_main.php");

date_default_timezone_set($DTZ);

$KWHT = 0;

if ($cmd == 2 && (empty($shmid) || $KWHT == 0)) { // 123s ain't running at night retrieve the value in csv

    for ($i = 0; $i < count($invtnums); $i++) {
	$invtnum = $invtnums[$i];
	include("$pathto123s/config/config_invt$invtnum.php");

	$dir    = $pathto123s . '/data/invt' . $invtnum . '/csv';
	$output = glob($dir . "/*.csv");
	sort($output);
	$xdays = count($output);
	//echo "xdays = $xdays\n";
	if ($xdays > 1) {
    	    $lastlog    = $output[$xdays - 1];
	    //echo "lastlog = $lastlog\n";
    	    $lines      = file($lastlog);
	    //echo "lines = $lines\n";
    	    $contalines = count($lines);
	    //echo "contalines = $contalines\n";
    	    $array_last = preg_split('/,/', $lines[$contalines - 1]);
	
	    //echo "array_last[14] = $array_last[14]\n";
	    //echo "${'CORRECTFACTOR'}\n";
    	    $KWHT       += round(($array_last[14] * ${'CORRECTFACTOR'} * 1000), 0); //in Wh
	    //echo "KWHT = $KWHT\n";
	} else {
    	    $KWHT += 0;
	}
    }
}

if ($cmd == 1 && empty($shmid)) { // 123s ain't running
    $GP = 0;
}

if ($cmd == 1) {
    echo "$meterid($GP*W)\n";
} elseif ($cmd == 2) {
    echo "$meterid($KWHT*Wh)\n";
}
?>