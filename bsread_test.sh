#!/bin/bash

export SYSTEM=$1

echo "Checking bsread system $SYSTEM"
echo
echo

export SYSTEM=${SYSTEM}-BSREAD9999


echo "--------------------"
echo
echo 'Time in milliseconds required for last readout:'
caget ${SYSTEM}:READ.VALA
echo
echo 'Number of times time for read was > 1ms:'
caget ${SYSTEM}:READ.VALB
echo
echo 'Number of ZMQ buffer overflows:'
caget ${SYSTEM}:READ.VALC
echo
echo 'Check inhibit status:'
caget ${SYSTEM}:INHIBIT
echo

echo "--------------------"
echo "Check existence linked records"
echo
echo "Check what pulse-id record is configured:"
RECORD=$(caget ${SYSTEM}:READ.INPA | sed -e 's/[^"]*"//' | sed -e 's/ \w.*//')
echo $RECORD
caget $RECORD
echo
echo "Check what record is configured for global timestamp seconds:"
RECORD=$(caget ${SYSTEM}:READ.INPB | sed -e 's/[^"]*"//' | sed -e 's/ \w.*//')
echo $RECORD
caget $RECORD
echo
echo "Check what record is configured for global timestamp nanoseconds:"
RECORD=$(caget ${SYSTEM}:READ.INPC | sed -e 's/[^"]*"//' | sed -e 's/ \w.*//')
echo $RECORD
caget $RECORD
echo
echo "--------------------"
