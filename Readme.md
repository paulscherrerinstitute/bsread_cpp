# Overview
__bsread__ provides a fast IOC based continuous readout functionality. It reads configured channels a configured 
time interval and streams out the data via ZMQ. All channels to be read out need to reside on the same IOC than 
the __bsread__ code is running.

The ZMQ message send out for each readout is split in a header and body part. The header part contains JSON 
encoded data containing the __htype__ and the number of (double) __elements__
contained in the body part. The body part contains, for each configured channel its double value (8 bytes) 
in BIG_ENDIAN format.

The data stream is served in a ZMQ PUSH/PULL delivery scheme.

```
+-----------------------------------------------------------------------------------------+           
|                                                                                         |           
|                                                                                         |           
|  +--------------+                        +---------------+                  +--------+  |           
|  | Watchdoc ISR | +----> Semaphore <---+ | Readout Logic +------> Pipe <--+ | Sender | +-------> ZMQ
|  +--------------+                        +---------------+                  +--------+  |           
|                                                                                         |           
|                                                                                   IOC   |           
+-----------------------------------------------------------------------------------------+           
```

# Installation
To configure __bsread__ on your IOC continue as follows:

  * Copy latest _bsread.template_ file from the __bsread__ project to your IOC configuration folder
  * Create substitution file _bsread.subs_ for bsread.template ([Example](doc/EXAMPLE_bsread.subs))
 
	```
	file bsread.template {
		{
			P = "YOUR-IOC-CRLOGIC-PREFIX"
		} 
	} 
	```

  * Configure SNL startup script. The IOC SNL Startup Script which is located in the *snl* directory of the IOC project.It has to include following line:

	```
	# pvPrefix  -  Prefix of the epics records
	bsreadInitializeCore "<pvPrefix>"
	```
  
    * A detailed [example](doc/EXAMPLE_snl_startup.script) can be found [here](doc/EXAMPLE_snl_startup.script) )

  * Configure startup script as follows:
    * Increase the system clock rate (*Note*: increasing the system clock rate might have effects on the IOC and its hardware. Please check if things working properly after increasing the clock rate)
    
	```
	sysClkRateSet 1000
	```
    
    * Load ZMQ and BSREAD drivers
    
	```
	require "ZMQ", "<version>"
	require "BSREAD", "<version>"
	```

A detailed configuration example can be found [here](doc/EXAMPLE_startup.script)

# Usage
There are following channels to control and configure __bsread__:

  * __$(P):STATUS__ - Status of the readout logic (SETUP, INACTIVE, INITIALIZE, ACTIVE, STOP, FAULT, ERROR)
  * __$(P):MSG__ - Fault message given by the logic (if there is one)	 
  * __$(P):TBINT__ - Number of ticks between two readout interrupts
  * __$(P):RRES__ - Ids of the configured resources to be read out/triggered

# Development


* Compilation

```
make
```

* Installation

```
make install
```

__Note__: For spotting problems easier and quicker it is recommended to prepend the __dye__ command before the actual make statement.

# Debugging
Start medm panel:
```
medm -x -macro "P=MTEST-HW3-CRL" /work/sls/config/medm/bsread.adl
```

Commands to set readout resources and intervall:

```
caput MTEST-HW3-R:RRES 'MTEST-HW3:MOT1.RBV MTEST-HW3:MOT2.RBV ""'
caput MTEST-HW3-R:TBINT 100
```

# Todo

  * Replace VXWorks specific calls with highlevel functions: http://www.aps.anl.gov/epics/base/R3-14/12-docs/AppDevGuide/node21.html
  * Support other datatypes than doubles
  * Support different readout/send frequencies

