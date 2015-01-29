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
	CMD="sdm120c -a ${ADDRESS} -b ${BAUD_RATE} -i -p -q ${DEVICE}"

	#echo $CMD
	
	VALUE=`$CMD`
	POWER=$(echo ${VALUE} | cut -d\  -f1)
	ENERGY=$(echo ${VALUE} | cut -d\  -f2)
	if [ "$ENERGY" != "0" -a x"$ENERGY" != x -a "$POWER" != "0" -a x"$POWER" != x ]; then
	    echo -e "$ID($POWER*W)\n$ID($ENERGY*Wh)" > /run/shm/metern${ADDRESS}.txt
	    #echo -e "$VALUE"  > /run/shm/metern${ADDRESS}.txt
	fi
	sleep 5s

    done

done
