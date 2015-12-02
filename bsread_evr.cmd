##BSREAD USING EVR
## Mandatory macros:
##
##	- SYS
##
## Optional macros [default]
##  - BSREAD_PORT [9999]
##  - BSREAD_MODE [PUSH]
##	- EVR [EVR0]
##	- BSREAD_PULSEID
##	- BSREAD_TS_SEC
##	- BSREAD_TS_NSEC
##  - READ_FLNK []
##
## Disable loading of the EVR template by using the following macro: NO_EVR=#

require bsread
$(NO_EVR=)dbLoadRecords "bsread_evr.template", "P=$(SYS)-BSREAD,EVR=$(EVR=EVR0),BSREAD_EVENT=$(BSREAD_EVENT=40)"
dbLoadRecords "bsread.template", "P=$(SYS)-BSREAD,BSREAD_PULSEID=$(BSREAD_PULSEID=$(SYS)-DBUF-$(EVR=EVR0):BunchIdRx-I),BSREAD_TS_SEC=$(BSREAD_TS_SEC=$(SYS)-DBUF-$(EVR):BunchIdRx-MASTER-TS-SEC),BSREAD_TS_NSEC=$(BSREAD_TS_NSEC= $(SYS)-DBUF-$(EVR):BunchIdRx-MASTER-TS-NSEC),READ_FLNK=$(READ_FLNK=)"

bsreadConfigure("default",$(BSREAD_PORT=9999),$(BSREAD_MODE=PUSH))
