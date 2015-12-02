require "bsread"

runScript $(bsread_DIR)/bsread_sim.cmd, "SYS=SOME-TEST,BSREAD_PORT=9999"

dbLoadRecords("bsread_test.template","P=SOME-TEST-FAKEDATA")
