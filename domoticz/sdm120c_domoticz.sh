#!/bin/sh

DOMOTICZ="http://192.168.2.31:8080/json.htm?type=command"

values=`sudo sdm120c -a 1 -b 9600 -z 10 -w 5 -j 1 -i -p -v -c -P N -q /dev/ttyUSB0`

echo ${values}

V=`echo ${values} | awk '{print $1}'`
A=`echo ${values} | awk '{print $2}'`
W=`echo ${values} | awk '{print $3}'`
WH=`echo ${values} | awk '{print $4}'`

curl -s "${DOMOTICZ}&param=udevice&idx=3&nvalue=0&svalue=$W"
curl -s "${DOMOTICZ}&param=udevice&idx=4&nvalue=0&svalue=$V"
curl -s "${DOMOTICZ}&param=udevice&idx=5&nvalue=0&svalue=$A"
curl -s "${DOMOTICZ}&param=udevice&idx=2&nvalue=0&svalue=$W;$WH"
curl -s "${DOMOTICZ}&param=addlogmessage&message=SDM120C"
