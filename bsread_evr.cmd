##BSREAD USING EVR
## Mandatory macros:
##
##	- SYS
##
## Optional macros [default]
##  - BSREAD_PORT [9999]
##  - BSREAD_MODE [PUSH]
##	- EVR [EVR0]
##      - BSREAD_EVENT
##	- BSREAD_PULSEID
##	- BSREAD_TS_SEC
##	- BSREAD_TS_NSEC
##  - READ_FLNK []
##
## Disable loading of the EVR template by using the following macro: NO_EVR=#



require bsread
$(NO_EVR=)dbLoadRecords "bsread_evr.template", "P=$(SYS)-BSREAD,EVR=$(EVR=EVR0),BSREAD_EVENT=$(BSREAD_EVENT=28)"
dbLoadRecords "bsread.template", "P=$(SYS)-BSREAD,BSREAD_PULSEID=$(BSREAD_PULSEID=$(SYS)-$(EVR):RX-PULSEID),BSREAD_TS_SEC=$(BSREAD_TS_SEC=$(SYS)-$(EVR):RX-MTS-SEC),BSREAD_TS_NSEC=$(BSREAD_TS_NSEC=$(SYS)-$(EVR):RX-MTS-NSEC),READ_FLNK=$(READ_FLNK=)"

bsreadConfigure("default",$(BSREAD_PORT=9999),$(BSREAD_MODE=PUSH))
