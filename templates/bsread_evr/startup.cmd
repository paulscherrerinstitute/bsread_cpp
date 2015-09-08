##BSREAD USING EVR
## Mandatory macros: 
##	
##	- SYS
##
## Optional macros [default]
##	- EVR [EVR0]
##	- BSREAD_PULSEID 
##	- BSREAD_TS_SEC
##	- BSREAD_TS_NSEC

require bsread
dbLoadRecords "bsread_evr.template", "P=$(SYS)-BSREAD,EVR=$(EVR=EVR0)"
dbLoadRecords "bsread.template", "P=$(SYS)-BSREAD,BSREAD_PULSEID=$(BSREAD_PULSEID=$(SYS)-DBUF-$(EVR=EVR0):BunchIdRx-I),BSREAD_TS_SEC=$(BSREAD_TS_SEC=$(SYS)-DBUF-$(EVR):BunchIdRx-MASTER-TS-SEC),BSREAD_TS_NSEC=$(BSREAD_TS_NSEC= $(SYS)-DBUF-$(EVR):BunchIdRx-MASTER-TS-NSEC)"


