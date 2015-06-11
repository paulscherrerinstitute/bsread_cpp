#!/bin/bash

if [ -z "$INSTBASE" ]; then 
	echo "INSTBASE is not set, aborting!";
	exit 1;
fi

INSTDIR=$INSTBASE/iocBoot/templates/BSREAD

echo "INSTBASE: $INSTBASE"
echo "Installing to $INSTDIR"

cp -v template/* $INSTDIR

echo ""
echo "All done!"