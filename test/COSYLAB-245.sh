# Test for COSYLAB-245
#
# 1. run IOC
# 2. run bsread_receiver.py
# 3. set SYS acording to your IOC
# 3. run this script
#
# 

SYS=TEST

CONFIG="{\"channels\": []}"
/usr/local/epics/base/bin/SL6-x86_64/caput -S $SYS-BSREAD:CONFIGURATION $CONFIG
echo "Observe that IOC does not show an error while applying this configuration. No output on the receiver as well."
sleep 4
echo ""
echo ""

CONFIG="{\"kanals\": []}"
/usr/local/epics/base/bin/SL6-x86_64/caput -S $SYS-BSREAD:CONFIGURATION $CONFIG
echo "Error in IOC is shown since configuration is not valid (missing channel attribute in configuration)."
sleep 4
echo ""
echo ""

CONFIG="{\"channels\": [{\"name\":\"$SYS-BSREAD-TEST:TEST_2\"}]}"
/usr/local/epics/base/bin/SL6-x86_64/caput -S $SYS-BSREAD:CONFIGURATION $CONFIG
echo "Setting a readout on a channel now."
sleep 4
echo ""
echo ""

CONFIG="{\"channels\": []}"
/usr/local/epics/base/bin/SL6-x86_64/caput -S $SYS-BSREAD:CONFIGURATION $CONFIG
echo "The configuration is now cleared."
echo ""
