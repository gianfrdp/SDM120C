<?php
if(!defined('checkaccess')){die('Direct access not permitted');}
// sdm120c is a command line program for reading the parameters out of EASTRON SDM120C ModBus Smart meter.
// http://github.com/gianfrdp/SDM120C

$SDTE = date("Ymd H:i:s");

$I1V = null;
$I1A = null;
$I1P = null;

$errornum = null;

// Ask sdm120c: 
//      - Voltage (-v)
//      - Power (-p)
//      - Current (-c)
//      - Frequency (-f)
//      - Imported energy (-i)
 
$CMD_POOLING = "sdm120c -a ${'ADR'.$invt_num} ${'COMOPTION'.$invt_num} -vpcfi -q ${'PORT'.$invt_num}";

if ($DEBUG != 0) {
   error_log("$CMD_POOLING",0);
}

$CMD_RETURN = exec($CMD_POOLING, $errornum);

if ($DEBUG != 0) {
  error_log("$CMD_RETURN",0);
}

$dataarray  = preg_split('/[[:space:]]+/', $CMD_RETURN);
//var_dump($dataarray);

$G1V = $dataarray[0];
settype($G1V, 'float');

$G1A = $dataarray[1];
settype($G1A, 'float');

$G1P = $dataarray[2];
settype($G1P, 'float');

$FRQ = $dataarray[3];
settype($FRQ, 'float');

$EFF = (float) 0.0;

$INVT = null;

$BOOT = null;

$KWHT = $dataarray[4];
settype($KWHT, 'float');
$KWHT = $KWHT/1000;

if ($errornum == 0) {
  $RET = 'OK';
} else {
  $RET = 'NOK';
}


if ($DEBUG != 0) {
   error_log("G1V = $G1V \n G1A = $G1A \n G1P = $G1P \n FRQ = $FRQ \n $EFF \n $KWHT",0);
}

?>
