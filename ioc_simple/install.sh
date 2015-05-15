if [ -z "$INSTBASE" ]; then 
	echo "INSTBASE is not set, aboritng!";
	exit 1;
fi

INSTDIR=$INSTBASE/iocBoot/templates/BSREAD

echo "INSTBASE: $INSTBASE"
echo "installing to $INSTDIR"

cp -v template/* $INSTDIR

echo ""
echo "All done!"