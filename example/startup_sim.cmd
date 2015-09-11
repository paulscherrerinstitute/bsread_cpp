require "bsread"

runScript $(bsread_DIR)/bsread_sim.cmd, "SYS=SLEJKO-TEST"

dbLoadRecords("bsread_test.template","P=SLEJKO-TEST-FAKEDATA")

