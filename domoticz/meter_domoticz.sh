#!/bin/sh

ID=1
DOMOTICZ="http://192.168.2.31:8080/json.htm?type=command"

W=`cat /run/shm/metern${ID}.txt  | egrep "^1\(" | grep "*W)" | egrep -o '[0-9]*(\.)?[0-9]*\*' | egrep -o '[0-9]*(\.)?[0-9]*'`
WH=`cat /run/shm/metern${ID}.txt  | egrep "^1\(" | grep "*Wh)" | egrep -o '[0-9]*\*' | egrep -o '[0-9]*'`
V=`cat /run/shm/metern${ID}.txt  | egrep "^1_1\(" | grep "*V)" | egrep -o '[0-9]*(\.)?[0-9]*\*' | egrep -o '[0-9]*(\.)?[0-9]*'`
A=`cat /run/shm/metern${ID}.txt  | egrep "^1_2\(" | grep "*A)" | egrep -o '[0-9]*(\.)?[0-9]*\*' | egrep -o '[0-9]*(\.)?[0-9]*'`

curl -s "${DOMOTICZ}&param=udevice&idx=3&nvalue=0&svalue=$W"
curl -s "${DOMOTICZ}&param=udevice&idx=4&nvalue=0&svalue=$V"
curl -s "${DOMOTICZ}&param=udevice&idx=5&nvalue=0&svalue=$A"
curl -s "${DOMOTICZ}&param=udevice&idx=2&nvalue=0&svalue=$W;$WH"
curl -s "${DOMOTICZ}&param=addlogmessage&message=SDM120C:$W;$WH"
