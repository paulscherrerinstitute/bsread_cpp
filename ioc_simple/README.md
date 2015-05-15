#BSREAD

## How to include BSREAD in your IOC application

in order to add bsread to your IOC, simply add the following line to the IOC startup script: 

		< $(TEMPLATES)/BSREAD/bsread.startup

If you want to customize options, such as BSREAD event or source of PulseID, etc.. you can do so by setting appropriate env variables before loading bsread



		epicsEnvSet EVR $(EVR=EVR0)		##Set EVR receiver id (default EVR0)
		epicsEnvSet BSREAD_EVENT $(BSREAD_EVENT=40) ##Trigger BSREAD using EVR event (default event 40)


		epicsEnvSet BSREAD_PULSEID $(BSREAD_PULSEID= $(SYS)-DBUF-$(EVR):BunchIdRx-I) ##which record to use to obtaion pulse id(default PulseID received using EVR Dbuff)
		epicsEnvSet BSREAD_TS_SEC $(BSREAD_TS_SEC= $(SYS)-DBUF-$(EVR):BunchIdRx-MASTER-TS-SEC) ##record holding timestamp seconds (default dbuff)
		epicsEnvSet BSREAD_TS_NSEC $(BSREAD_TS_NSEC= $(SYS)-DBUF-$(EVR):BunchIdRx-MASTER-TS-NSEC) ##record holding timestamp nsec (default dbuff)


		< $(TEMPLATES)/BSREAD/bsread.startup
 


## Simulation mode

BSREAD can be used without timing receiver for testing puropses, for that add the following line to the IOC startup: 


		< $(TEMPLATES)/BSREAD/bsread_sim.startup


## Template installation

Set your INSTBASE variable to desired location (e.g. /fin/devl) and run 
		./install.sh 

		