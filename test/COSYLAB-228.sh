# Test for COSYLAB-228
#
# 1. run IOC
# 2. run bsread_receiver.py
# 3. set SYS acording to your IOC
# 3. run this script
#
# 

SYS=TEST

CONFIG="{\"channels\": [{\"name\":\"$SYS-BSREAD-TEST:TEST_2\"}]}"
/usr/local/epics/base/bin/SL6-x86_64/caput -S $SYS-BSREAD:CONFIGURATION $CONFIG
echo "Setting a readout on a channel."
sleep 4
echo ""
echo ""

/usr/local/epics/base/bin/SL6-x86_64/caput -S $SYS-BSREAD:INHIBIT 1
echo "Inhibits the readout."
sleep 4
echo ""
echo ""

/usr/local/epics/base/bin/SL6-x86_64/caput -S $SYS-BSREAD:INHIBIT 0
echo "Resume normal operation."
sleep 4
echo ""
echo ""

/usr/local/epics/base/bin/SL6-x86_64/caput -S $SYS-BSREAD:INHIBIT 1
echo "Inhibits the readout."
sleep 4
echo ""
echo ""

/usr/local/epics/base/bin/SL6-x86_64/caput -S $SYS-BSREAD:INHIBIT 0
echo "Resume normal operation."
echo ""
