#!/bin/bash

ADDRESS="$2"
BAUD_RATE="$3"
DEVICE="$4"

TYPE="$1"

if [ "$TYPE" == 'relect' ]; then

    VALUE_ENERGY=`sdm120c -a ${ADDRESS} -b ${BAUD_RATE} -i -m ${DEVICE}`
    echo ${VALUE_ENERGY}

elif [ "$TYPE" == 'live' ]; then

    while [ true ]; do
	VALUE_LIVE=`sdm120c -a ${ADDRESS} -b ${BAUD_RATE} -p -m ${DEVICE}`
	#echo ${VALUE_LIVE}
	echo ${VALUE_LIVE} > /run/shm/metern${ADDRESS}.txt
	sleep 2s
    done

fi
