# Test for COSYLAB-246
#
# 1. run IOC
# 2. run bsread_receiver.py
# 3. set SYS acording to your IOC
# 3. run this script
#
##

SYS=TEST

CONFIG="{\"channels\": [{\"name\":\"$SYS-BSREAD-TEST:TEST_2\"}]}"
/usr/local/epics/base/bin/SL6-x86_64/caput -S $SYS-BSREAD:CONFIGURATION $CONFIG
echo "Observe modulo and offset in the data header printout of the bsread_receiver."
echo ""