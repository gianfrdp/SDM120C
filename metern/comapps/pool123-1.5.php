<?php
if ($_SERVER['SERVER_ADDR'] != '127.0.0.1' && $_SERVER['SERVER_ADDR'] != 'localhost') {
    die('Direct access not permitted');
}

// This script will output a 123solar counter into a meterN compatible format
// You'll need to setup the path to 123, your inverter number and your meter id
// Request live command with "curl http://localhost/metern/comapps/pool123solar.php?cmd=1"
// Main command with "curl http://localhost/metern/comapps/pool123solar.php?cmd=2"

$pathto123s = '/srv/http/123solar';
//$pathto123s = '/var/www/123solar';
// for multiple inverters
$invtnums   = array (0 => 1, 1 => 2);
$meterid    = 4;
//$server = 'http://192.168.2.12/123solar/programs/programmultilive.php';
$server = 'http://localhost/123solar/programs/programmultilive.php';

//parse_str(implode('&', array_slice($argv, 1)), $_GET);

// No edit is needed below
if (!empty($_GET['cmd']) && is_numeric($_GET['cmd'])) {
    $cmd = $_GET['cmd'];
} else {
    die('Wrong command');
}

define('checkaccess', TRUE);
include("$pathto123s/config/config_main.php");
include("$pathto123s/scripts/memory.php");

date_default_timezone_set($DTZ);

$KWHT = 0;

if ($cmd == 2) {

    for ($i = 0; $i < count($invtnums); $i++) {
        $invtnum = $invtnums[$i];
        include("$pathto123s/config/config_invt$invtnum.php");

        $dir    = $pathto123s . '/data/invt' . $invtnum . '/csv';
        $output = glob($dir . "/*.csv");
        sort($output);
        $xdays = count($output);
        if ($xdays > 1) {
            $lastlog    = $output[$xdays - 1];
            $lines      = file($lastlog);
            $contalines = count($lines);
            $array_last = preg_split('/,/', $lines[$contalines - 1]);

            $KWHT       += round(($array_last[14] * ${'CORRECTFACTOR'} * 1000), 0); //in Wh
        } else {
            $KWHT += 0;
        }
    }

    echo "$meterid($KWHT*Wh)\n";
} elseif ($cmd == 1) {

    $GP = 0;

    $data = file_get_contents($server);
    $memarray = json_decode($data, true);
    $GP = $memarray[0]['GPTOT'];
    if ($GP > 1000) {
        $GP = round($GP, 0);
    } else {
        $GP = round($GP, 1);
    }

    echo "$meterid($GP*W)\n";
}

?>
