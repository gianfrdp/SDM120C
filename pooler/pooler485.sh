#!/bin/bash

ADDRESSES="$1"
BAUD_RATE="$2"
DEVICE="$3"

ADDR_ARR=$(echo $ADDRESSES | tr "," "\n")

while [ true ]; do

    ID=0
    POWER=""
    ENERGY=""

    for ADDRESS in $ADDR_ARR
    do
	#((ID++))
	ID=$ADDRESS
	CMD="sdm120c -a ${ADDRESS} -b ${BAUD_RATE} -i -p -m ${DEVICE}"

	#echo $CMD
	
	VALUE=`$CMD`
	#POWER=$(echo ${VALUE} | grep "Power:\ " | cut -d: -f2 | cut -d\  -f2)
	#ENERGY=$(echo ${VALUE} | grep "Import Active Energy:\ " | cut -d: -f3 | cut -d\  -f2)
	#echo -e "$ID($POWER*W)\n$ID($ENERGY*Wh)"  > /run/shm/metern${ADDRESS}.txt
	echo -e "$VALUE"  > /run/shm/metern${ADDRESS}.txt
	sleep 2s

    done

done
