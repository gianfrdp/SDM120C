<?php
if(!defined('checkaccess')){die('Direct access not permitted');}
// sdm120c is a command line program for reading the parameters out of EASTRON SDM120C ModBus Smart meter.
// http://github.com/gianfrdp/SDM120C

$SDTE = date("Ymd H:i:s");

$I1V = null;
$I1A = null;
$I1P = null;

$CMD_POOLING = "sdm120c -a ${'ADR'.$invt_num} ${'COMOPTION'.$invt_num} -q ${'PORT'.$invt_num}";
//$CMD_POOLING = "sdm120c -a 2 -b 9600 -q /dev/ttyUSB0";

//echo "$CMD_POOLING\n";

$CMD_RETURN = exec($CMD_POOLING);
//echo "$CMD_RETURN\n";

$dataarray  = preg_split('/[[:space:]]+/', $CMD_RETURN);
//var_dump($dataarray);

$G1V = $dataarray[0];
settype($G1V, 'float');

$G1A = $dataarray[1];
settype($G1A, 'float');

$G1P = $dataarray[2];
settype($G1P, 'float');

$FRQ = $dataarray[4];
settype($FRQ, 'float');

$EFF = (float) 0.0;

$INVT = null;

$BOOT = null;

$KWHT = $dataarray[5];
settype($KWHT, 'float');
$KWHT = $KWHT/1000;

$RET = 'OK';

//echo " $G1V \n $G1A \n $G1P \n $FRQ \n $EFF \n $KWHT\n";

?>
