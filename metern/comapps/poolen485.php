#!/usr/bin/php -f
<?php
// Credit Louviaux Jean-Marc 2015
/*
if (isset($_SERVER['REMOTE_ADDR'])) {
    die('Direct access not permitted');
}
*/
define('checkaccess', TRUE);
include('../scripts/memory.php');
include('../config/config_main.php');

date_default_timezone_set($DTZ);

$val_tot = 0;
$metnum = 0;

if (isset($argv[1])) {

    $metnum = $argv[1];

} else {
    die("Usage: $argv[0] metnum \n");
}

include("../config/config_met$metnum.php");
$dir = '/run/shm';

//$memdata = file_get_contents('http://localhost/metern/programs/programtotal.php');
//$memory = json_decode($memdata, true);

$memory = array();
//echo "MEMORY = $MEMORY\n";
@$shmid = shmop_open($MEMORY, 'a', 0, 0);
if (!empty($shmid)) {
        $size = shmop_size($shmid);
        shmop_close($shmid);
        $shmid = shmop_open($MEMORY, 'c', 0644, $size);
        $memdata  = shmop_read($shmid, 0, $size);
        $memory = json_decode($memdata, true);
        shmop_close($shmid);
}

//var_dump($memory);
$val = $memory["Totalcounter$metnum"];
$arr_val = preg_split("/ /",$val);
$last_val = (int)((float) str_replace(",", ".", $arr_val[0]) * 1000);

$cmd = 'cat '.$dir.'/metern'.$metnum.'.txt | egrep "^'.$metnum.'\(" | grep "*Wh)" | cut -d\( -f2 | cut -d* -f1';
$meter_val = (int) exec($cmd);

# workaround in case of blackout and meter value lesser than previous value
if ($meter_val < $last_val && ${'PASSO'.$metnum} - $last_val > 1000) {
    $val_tot = $last_val;
} else {
    $val_tot = $meter_val;
}

echo "$metnum($val_tot*Wh)\n";
?>