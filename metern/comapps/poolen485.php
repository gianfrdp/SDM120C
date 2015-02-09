#!/usr/bin/php -f
<?php
// Credit Louviaux Jean-Marc 2015
define('checkaccess', TRUE);
include('../config/config_main.php');
include('../scripts/memory.php');
date_default_timezone_set($DTZ);

$val_tot = 0;
$metnum = 0;

if (isset($argv[1])) {

    $metnum = $argv[1];

} else {
    die("Usage: $argv[0] metnum \n");
}
// End of setup

include("../config/config_met$metnum.php");
$dir = '/run/shm';

$memdata = file_get_contents('http://localhost/metern/programs/programtotal.php');
$memory = json_decode($memdata, true);

$val = $memory["Totalcounter$metnum"];
$arr_val = preg_split("/ /",$val);
$last_val = (int)((float) str_replace(",", ".", $arr_val[0]) * 1000);

$cmd = 'cat /run/shm/metern'.$metnum.'.txt | egrep "^'.$metnum.'\(" | grep "*Wh)" | cut -d\( -f2 | cut -d* -f1';
$meter_val = (int) exec($cmd);

# workaround in case of blackout and meter value lesser than previuos value
if ($meter_val < $last_val && ${'PASSO'.$metnum} - $last_val > 1000) {
    $val_tot = $val_num;
} else {
    $val_tot = $meter_val;
}

echo "$metnum($val_tot*Wh)\n";
?>