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
	CMD="sdm120c -a ${ADDRESS} -b ${BAUD_RATE} -z 10 -i -p -v -c -f -g -P E -q ${DEVICE}"

	#echo $CMD
	
	VALUE=`$CMD`
	
	VOLTAGE=$(echo ${VALUE}   | awk '{print $1}')
	CURRENT=$(echo ${VALUE}   | awk '{print $2}')
	POWER=$(echo ${VALUE}     | awk '{print $3}')
	FACTOR=$(echo ${VALUE}    | awk '{print $4}')
	FREQUENCY=$(echo ${VALUE} | awk '{print $5}')
	ENERGY=$(echo ${VALUE}    | awk '{print $6}')

	if [ "$ENERGY" != "0" -a x"$ENERGY" != x -a "$POWER" != "0" -a x"$POWER" != x ]; then
	    echo -e "$ID($POWER*W)\n$ID($ENERGY*Wh)\n${ID}_1($VOLTAGE*V)\n${ID}_2($CURRENT*A)\n${ID}_3($FREQUENCY*Hz)\n${ID}_4($FACTOR*F)" > /run/shm/metern${ADDRESS}.txt
	    #echo -e "$VALUE"  > /run/shm/metern${ADDRESS}.txt
	fi
	sleep 5s

    done

done
