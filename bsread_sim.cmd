##BSREAD SIMULATION MODE
##
## Optional parameters/macros [default]
##  - SYS [$(IOC)]
##  - BSREAD_PORT [9999]
##  - BSREAD_MODE [PUSH]
##	- BSREAD_PULSEID [$(SYS)-$(EVR):RX-PULSEID]
##	- BSREAD_TS_SEC [$(SYS)-$(EVR):RX-MTS-SEC]
##	- BSREAD_TS_NSEC [$(SYS)-$(EVR):RX-MTS-NSEC]
##  - PULSEID_OFFSET=$(PULSEID_OFFSET=0)

# If having multiple instances in simulation mode, there will be only one set of
# simulation record for the pulse-id and global timestamp!

require misc
require bsread
addScan .01
dbLoadRecords "bsread_sim.template","P=$(SYS=$(IOC))-BSREAD"
dbLoadRecords "bsread.template", "P=$(SYS=$(IOC))-BSREAD$(BSREAD_PORT=9999),EVENT_NAME=$(EVENT_NAME=bsread),BSREAD_PORT=$(BSREAD_PORT=9999),PULSEID_OFFSET=$(PULSEID_OFFSET=0),BSREAD_PULSEID=$(BSREAD_PULSEID=$(SYS=$(IOC))-BSREAD:SIM-PULSE),BSREAD_TS_SEC=$(BSREAD_TS_SEC=$(SYS=$(IOC))-BSREAD:SIM-TS-SEC),BSREAD_TS_NSEC=$(BSREAD_TS_NSEC=$(SYS=$(IOC))-BSREAD:SIM-TS-SEC)"
bsreadConfigure($(BSREAD_PORT=9999),$(BSREAD_PORT=9999),$(BSREAD_MODE=PUSH),$(HWM=10))
