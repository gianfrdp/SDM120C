<?php
function is_valid ($id, $datareturn) //  IEC 62056 data set structure
{
    $regexp = "/^$id\(-?[0-9\.]+\*[A-z0-9³²%°]+\)$/i"; //ID(VALUE*UNIT)
    if (preg_match($regexp, $datareturn)) {
        $datareturn = preg_replace("/^$id\(/i", '', $datareturn, 1); // VALUE*UNIT)
        $datareturn = preg_replace("/\*[A-z0-9³²%°]+\)$/i", '', $datareturn, 1); // VALUE
        settype($datareturn, 'float');
    } else {
        $datareturn = null;
    }
    return $datareturn;
}
?>
