# 1. run IOC
# 2. run bsread_receiver.py
# 3. set SYS acording to your IOC
# 3. run this script
#
# 

SYS=TEST

CONFIG="{\"channels\": [{\"name\":\"$SYS-BSREAD-TEST:TEST_2\",\"modulo\":3}]}"
/usr/local/epics/base/bin/SL6-x86_64/caput -S $SYS-BSREAD:CONFIGURATION $CONFIG
echo "The channel is received every 3 pulse_ids. No messages are sent otherwise."
sleep 7
echo ""
echo ""

CONFIG="{\"channels\": [{\"name\":\"$SYS-BSREAD-TEST:TEST_2\",\"modulo\":3}, {\"name\":\"$SYS-BSREAD-TEST:TEST_1\",\"modulo\":5}]}"
/usr/local/epics/base/bin/SL6-x86_64/caput -S $SYS-BSREAD:CONFIGURATION $CONFIG
echo "First channel is received every 3 pulse_ids, last channel every 5 pulse_ids. Both are received every 15th pulse_id."
sleep 20
echo ""
echo ""

CONFIG="{\"channels\": [{\"name\":\"$SYS-BSREAD-TEST:TEST_2\",\"modulo\":5}, {\"name\":\"$SYS-BSREAD-TEST:TEST_1\",\"modulo\":3}]}"
/usr/local/epics/base/bin/SL6-x86_64/caput -S $SYS-BSREAD:CONFIGURATION $CONFIG
echo "Simmilar to previous test, just that modulos are reversed for first and last channel."
echo ""
