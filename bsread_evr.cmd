##BSREAD USING EVR
##
## Optional parameters/macros [default]
##  - SYS [$(IOC)]
##  - INSTANCE [default]
##  - BSREAD_PORT [9999]
##  - BSREAD_MODE [PUSH]
##	- EVR [EVR0]
##  - BSREAD_EVENT [28]
##	- BSREAD_PULSEID [$(SYS=$(IOC))-$(EVR):RX-PULSEID]
##	- BSREAD_TS_SEC [$(SYS=$(IOC))-$(EVR):RX-MTS-SEC]
##	- BSREAD_TS_NSEC [$(SYS=$(IOC))-$(EVR):RX-MTS-NSEC]
##  - READ_FLNK []
##  - PULSEID_OFFSET [0]
##
## To disable the loading of the EVR template use following parameter/macro: NO_EVR=#

require bsread
$(NO_EVR=)dbLoadRecords "bsread_evr.template", "P=$(SYS=$(IOC))-BSREAD,EVR=$(EVR=EVR0),BSREAD_EVENT=$(BSREAD_EVENT=28)"
dbLoadRecords "bsread.template", "P=$(SYS=$(IOC))-BSREAD,PULSEID_OFFSET=$(PULSEID_OFFSET=0),BSREAD_PULSEID=$(BSREAD_PULSEID=$(SYS=$(IOC))-$(EVR=EVR0):RX-PULSEID),BSREAD_TS_SEC=$(BSREAD_TS_SEC=$(SYS=$(IOC))-$(EVR=EVR0):RX-MTS-SEC),BSREAD_TS_NSEC=$(BSREAD_TS_NSEC=$(SYS=$(IOC))-$(EVR=EVR0):RX-MTS-NSEC),READ_FLNK=$(READ_FLNK=)"

bsreadConfigure($(INSTANCE=default),$(BSREAD_PORT=9999),$(BSREAD_MODE=PUSH),$(HWM=10))
