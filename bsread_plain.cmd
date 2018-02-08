##BSREAD USING EVR
##
## Optional parameters/macros [default]
##  - SYS [$(IOC)]
##  - BSREAD_PORT [9999]
##  - BSREAD_MODE [PUSH]
##	- EVR [EVR0]
##  - BSREAD_EVENT [28]
##	- BSREAD_PULSEID [$(SYS=$(IOC))-$(EVR):RX-PULSEID]
##	- BSREAD_TS_SEC [$(SYS=$(IOC))-$(EVR):RX-MTS-SEC]
##	- BSREAD_TS_NSEC [$(SYS=$(IOC))-$(EVR):RX-MTS-NSEC]
##  - PULSEID_OFFSET [0]
##
## To disable the loading of the EVR template use following parameter/macro: NO_EVR=#

require bsread
dbLoadRecords "bsread.template", "P=$(SYS=$(IOC))-BSREAD$(BSREAD_PORT=9999),BSREAD_PORT=$(BSREAD_PORT=9999),EVENT_NAME=$(EVENT_NAME=bsread$(BSREAD_PORT=9999)),PULSEID_OFFSET=$(PULSEID_OFFSET=0),BSREAD_PULSEID=$(BSREAD_PULSEID),BSREAD_TS_SEC=$(BSREAD_TS_SEC),BSREAD_TS_NSEC=$(BSREAD_TS_NSEC),READ_FLNK=$(READ_FLNK=)"

bsreadConfigure($(BSREAD_PORT=9999),$(BSREAD_PORT=9999),$(BSREAD_MODE=PUSH),$(HWM=10))
