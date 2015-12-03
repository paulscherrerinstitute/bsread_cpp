require "bsread"

runScript $(bsread_DIR)/bsread_evr.cmd, "SYS=SOME-TEST,EVR=EVR0"

dbLoadRecords("bsread_test.template","P=SOME-TEST-FAKEDATA")
