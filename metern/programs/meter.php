<?php
// Credit Louviaux Jean-Marc 2015
define('checkaccess', TRUE);
include('../config/config_main.php');
include('../scripts/memory.php');
date_default_timezone_set($DTZ);

$dir    = '../data/csv/';
$output = glob($dir . '/*.csv');
rsort($output);

@$shmid = shmop_open($LIVEMEMORY, 'a', 0, 0);
if (!empty($shmid) && isset($output[0])) {
    $size = shmop_size($shmid);
    shmop_close($shmid);
    $shmid = shmop_open($LIVEMEMORY, 'c', 0644, $size);
    $memdata  = shmop_read($shmid, 0, $size);
    $live = json_decode($memdata, true);
    shmop_close($shmid);

    $array = array();
    for ($i = 1; $i <= $NUMMETER; $i++) {
        include("../config/config_met$i.php");

        if (${'TYPE' . $i} != 'Sensor') {
            $file       = file($output[0]);
            $month      = substr($output[0], -8, 2);
            $day        = substr($output[0], -6, 2);
            $contalines = count($file);
            $prevarray  = preg_split('/,/', $file[1]);
            $linearray  = preg_split('/,/', $file[$contalines - 1]);

            $val_first = $prevarray[$i];
            $val_last  = $linearray[$i];
            settype($val_first, 'float');
            settype($val_last, 'float');

            if (${'TYPE' . $i} == 'Elect') {
                $val_tot = $val_last / 1000;
                $val_tot = $val_last;
            } else {
                settype(${'PRECI' . $i}, 'integer');
                $val_tot = $val_last;
                $prefix  = '';
            }
        } else {
            $val_tot  = 0;
            $val_last = 0;
        }

        $data["Totalcounter$i"] = $val_tot;

        if ($val_first <= $val_last) {
            $val_last -= $val_first;
        } else { // counter pass over
            $val_last += ${'PASSO' . $metnum} - $val_first;
        }

        if (${'TYPE' . $i} == 'Elect') {
            $val_last /= 1000;
        } else {
            settype(${'PRECI' . $i}, 'integer');
        }
        $data["Dailycounter$i"] = $val_last;

        $array["${'METNAME'.$i}$i"] = array("KW" => $live["${'METNAME'.$i}$i"], "KWH" => round($data["Dailycounter$i"],${'PRECI' . $i}));
    }
} else {
    for ($i = 1; $i <= $NUMMETER; $i++) {
        include("../config/config_met$i.php");

        $array["${'METNAME'.$i}$i"] = array("KW" => 0, "KWH" => 0);
    }
}

header("Content-type: text/json");
echo json_encode($array);
?>