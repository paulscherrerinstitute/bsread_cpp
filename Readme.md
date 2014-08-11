# Overview
CRLOGIC provides a fast IOC based continuous scan functionality. It reads configured hardware resources within a certain
time interval and streams out the data via ZMQ. All hardware resources to be read out need to reside on the same IOC than the CRLOGIC code is running.


Currently following hardware resources are supported:

  * VME58 - OMS motor card
  * VME58E - OMS motor card encoder field
  * MaxV - MaxV motor card
  * ECM5xx - ECM encoder card
  * VSC16 - VSC16 Scaler
  * DCR508 - DCR Scaler
  * Hy8001Trigger - Hytec 8001 digital out trigger (generates a trigger and reads back the trigger number)
  * Hy8401 - Hytec ADC
  * Timestamp - IOC timestamp of readout
  * Channel - Epics channel on IOC 

The ZMQ message send out for each readout is split in a header and body part. The header part contains JSON encoded data containing the __htype__ and the number of (double) __elements__
contained in the body part. The body part contains, for each configured readout resource its double value (8 bytes) in BIG_ENDIAN format.

The data stream is served in a ZMQ PUB/SUB delivery scheme.


# Installation
To configure CRLOGIC on your IOC continue as follows:

  * Copy latest _crlogic.template_ file from the CRLOGIC project to your IOC configuration folder
  * Create substitution file _crlogic.subs_ for crlogic.template ([Example](doc/EXAMPLE_crlogic.subs))
 
	```
	file crlogic.template {
		{
			P = "YOUR-IOC-CRLOGIC-PREFIX"
		} 
	} 
	```

  * Configure SNL startup script. The IOC SNL Startup Script which is located in the *snl* directory of the IOC project.It has to include following line:

	```
	# pvPrefix  -  Prefix of the CRLOGIC Epics records
	crlogicInitializeCore "<pvPrefix>"
	```
  
    * A detailed [example](doc/EXAMPLE_snl_startup.script) can be found [here](doc/EXAMPLE_snl_startup.script) )

  * Configure startup script as follows:
    * Increase the system clock rate (*Note*: increasing the system clock rate might have effects on the IOC and its hardware. Please check if things working properly after increasing the clock rate)
    
	```
	sysClkRateSet 1000
	```
    
    * Load ZMQ and CRLOGIC drivers
    
	```
	require "ZMQ", "<version>"
	require "CRLOGIC", "<version>"
	```
    
    * Configure potential resources to be read out by CRLOGIC
      * VME58
      
	```
	# resourceID       -  ID of the "resource" (used to configure and identify the readout)
	# slot             -  Slot of the motor card (counting starts at 0)
	# cardBaseAddress  -  Base address of the motor card
	crlogicAddVME58MotorResource "<resourceID>", <cardBaseAddress>, <slot>
	```
      
      * VME58E
      
	```
	# resourceID       -  ID of the "resource" (used to configure and identify the readout)
	# slot             -  Slot of the motor card (counting starts at 0)
	# cardBaseAddress  -  Base address of the motor card
	crlogicAddVME58EMotorResource "<resourceID>", <cardBaseAddress>, <slot>
	```
      
      * MaxV
      
	```
	# resourceID       -  ID of the "resource" (used to configure and identify the readout)
	# slot             -  Slot of the motor card (counting starts at 0)
	# cardBaseAddress  -  Base address of the motor card
	crlogicAddVME58EMotorResource "<resourceID>", <cardBaseAddress>, <slot>
	```
      * MaxV Internal Encoder
      
	```
	# resourceID       -  ID of the "resource" (used to configure and identify the readout)
	# slot             -  Slot of the card (counting starts at 0)
	# cardBaseAddress  -  Base address of the encoder card
	crlogicAddECM5xxEncoderResource "<resourceID>", <cardBaseAddress>, <slot>
	```

      * VSC16
      
	```
	# resourceID       -  ID of the "resource" (used to configure and identify the readout)
	# channel          -  Slot of the scaler card (counting starts at 0)
	# cardBaseAddress  -  Base address of the scaler card
	crlogicAddVSC16Resource "<resourceID>", <cardBaseAddress>, <channel>
	```

      * Hy8001 - Trigger
      
	```
	# resourceID  -  ID of the "resource" (used to configure and identify the "readout"/trigger)
	# cardNumber  -  Number of the card as configured in the Hy8001 driver setup
	# signal      -  Signal of the card to send the trigger out [signal starts at 0]
	crlogicAddHy8001TriggerResource "<resourceID>", <cardNumer>, <signal>
	```

      * Hy8401 - Analog-In
      
	```
	# resourceID  -  ID of the "resource" (used to configure and identify the "readout"/trigger)
	# cardNumber  -  Number of the card as configured in the Hy8001 driver setup
	# signal      -  Signal of the card to send the trigger out [signal starts at 0]
	crlogicAddHy8401Resource "<resourceID>", <cardNumer>, <signal>
	```
      
      * Channel
      
	```
	# key  -  Key/Channel name of the resource
	crlogicAddChannelResource "key"
	```

      * A detailed configuration example can be found [here](doc/EXAMPLE_startup.script)

# Usage
There are following channels to control and configure CRLOGIC:

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
medm -x -macro "P=MTEST-HW3-CRL" /work/sls/config/medm/G_CRLOGIC_expert.adl
```

Commands to set readout resources and intervall:

```
caput MTEST-HW3-R:RRES 'MTEST-HW3:MOT1.RBV MTEST-HW3:MOT2.RBV ""'
caput MTEST-HW3-R:TBINT 100
```

