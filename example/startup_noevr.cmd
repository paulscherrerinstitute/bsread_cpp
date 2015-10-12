require "bsread"

runScript $(bsread_DIR)/bsread_evr.cmd, "SYS=SLEJKO-TEST,EVR=EVR0,NO_EVR=#"

dbLoadRecords("bsread_test.template","P=SLEJKO-TEST-FAKEDATA")

