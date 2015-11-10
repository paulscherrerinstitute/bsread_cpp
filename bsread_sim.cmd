##BSREAD SIMULATION MODE
## Mandatory macros: 
##	
##	- SYS
##
## Optional macros [default]
##	- BSREAD_PULSEID 
##	- BSREAD_TS_SEC
##	- BSREAD_TS_NSEC

require misc
require bsread
addScan .01
dbLoadRecords "bsread_sim.template","P=$(SYS)-BSREAD"
dbLoadRecords "bsread.template", "P=$(SYS)-BSREAD,BSREAD_PULSEID=$(BSREAD_PULSEID=$(SYS)-BSREAD:SIM-PULSE),BSREAD_TS_SEC=$(BSREAD_TS_SEC=$(SYS)-BSREAD:SIM-TS-SEC),BSREAD_TS_NSEC=$(BSREAD_TS_NSEC=$(SYS)-BSREAD:SIM-TS-SEC)"

bsreadConfigure("default",9995,"PUSH")
