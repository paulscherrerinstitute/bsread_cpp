# Overview
__bsread__ provides a fast IOC based readout functionality. It reads configured channels and streams out the data
via ZMQ. All channels to be read out need to reside on the same IOC than the __bsread__ code is running.

The ZMQ data stream is served in a ZMQ PUSH/PULL delivery scheme. The default port is 9999.
The stream consists of messages consisting of several sub-messages.

```
+-------------------------+
|  Main Header            |
|                         |
+-------------------------+
|  Data Header            |
|                         |
+-------------------------+
|  Value                  |
+-------------------------+
|  Timestamp              |
+-------------------------+
           ...
+-------------------------+
|  Value                  |
+-------------------------+
|  Timestamp              |
+-------------------------+

```

The specification can be be found at https://docs.google.com/document/d/1BynCjz5Ax-onDW0y8PVQnYmSssb6fAyHkdDl1zh21yY/edit#

#Usage

__Currently only installed to fin/devel since usage relies on updated _require_ and *drive.makefile*__


3 modules are provided: 
 - __bsread__
    Core bsread libraries and templates.
 - __bsread_evr__
    bsread startup command predifined for standard SwissFEL EVR setup
 - __bsread_sim__
    bsread startup command predifined for standard SwissFEL EVR setup 

Also see _examples_ directory.

##bsread_sim

In order to run bsread without a EVR receiver (e.g. for testing or for use with softIOC and systems without timing) a bsread_sim module can be used. This module creates a 100Hz scan record and connects it to bsread and free running pulseid counter. Note that any data obtained in this way is not (and can not be) synchronised with data obtained from bsread systems that are connected to timing system.

Simply append the following line to startup script: 

    require "bsread"
    runScript $(bsread_DIR)/bsread_sim.cmd, "SYS=SLEJKO-TEST"

Paramaters that can be passed to the module are: 
    
    - mandatory
      - SYS: A system prefix (e.g. my IOC0), is expanded to $(SYS)-BSREAD:xx

    - optional:
      - BSREAD_PULSEID Record used to obtaian pulse id 
      - BSREAD_TS_SEC Record used to obtaian global timestamp sec
      - BSREAD_TS_NSEC Record used to obtaian global timestamp sec


##bsread_evr 

Majority of systems will use bsread in connection with hardware timing receiver (EVR). To simplify setup a bsread_evr module is provided. This module loads bsread and connects it to correct EVR records (so that it is triggered by hardware timing event and that pulse_id and global timestamp are obtained from databuffer)


Simply append the following line to startup script: 

    require "bsread"
    runScript $(bsread_DIR)/bsread_evr.cmd, "SYS=SLEJKO-TEST,EVR=EVR0"

Paramaters that can be passed to the module are: 
    
    - mandatory
      - SYS: A system prefix (e.g. my IOC0), is expanded to $(SYS)-BSREAD:xx

    - optional [default]:
      - EVR Id of EVR to be used [EVR0]
      - BSREAD_EVENT timing event that should be used to trigger bsread acquisition
      - BSREAD_PULSEID Record used to obtaian pulse id 
      - BSREAD_TS_SEC Record used to obtaian global timestamp sec
      - BSREAD_TS_NSEC Record used to obtaian global timestamp sec





# Installation
To configure __bsread__ on your IOC continue as follows:

  * Copy latest _bsread.template_ file from the __bsread__ project to your IOC configuration folder
  * Create substitution file _bsread.subs_ for bsread.template ([Example](ioc/bsread.subs))
 
	```
	file bsread.template {
		{
			P = "YOUR-PREFIX"
		} 
	} 
	```

  * Configure startup script as follows:
    
    * Load ZMQ and BSREAD drivers
    
	```
	require "ZMQ", "<version>"
	require "BSREAD", "<version>"
	```

# Usage
There are following channels to control and configure __bsread__:

  * __$(P):CONFIGURATION__ - Configuration - i.e. channels to be read out
  * __$(P):CONFIGURE__ - For internal use only
  * __$(P):READ__ - Readout record - whenever processed a readout will be triggered	 

## Debug Channels
  * __$(P):READ.FTVA__ - Time in seconds required to take the last snapshot (double)
  * __$(P):READ.FTVB__ - Number of snapshot time overruns (FTVA > 1ms) (ulong)


# Development

* Compilation

```
make
```

* Installation

```
make install
```

__Note__: For spotting problems easier and quicker it is recommended to prepend the __dye__ command before the
actual make statement.

## JSON
The json.cc and json.h files where generated from the json-cpp project. Following steps are required to do so

Enter the json-cpp folder and run:

    python amalgamate.py

This will generate json.cc and json.h files in ./dist. See README in json-cpp sources for more details.
 

## Coding Style
The code follows the following coding style: http://google-styleguide.googlecode.com/svn/trunk/cppguide.html

# Testing

Configure readout channels and read out the channels:

```
export EPICS_CA_ADDR_LIST=gfalc6064
/usr/local/epics/base/bin/SL6-x86_64/caput -S  BSREAD:CONFIGURATION '{"channels": [{"name":   "BSREAD-TEST:TEST_1","offset":1, "frequency":100 }, {"name":"BSREAD-TEST:TEST_2", "offset":1, "frequency":10} ]}'
```

# Test IOC
There is a test ioc inside the git repository inside the `ioc` folder. To use the IOC for testing, compile the
__BSREAD__ sources inside the `src` folder (use the Makefile inside the folder!), switch to the ioc directory,
execut the `makeioc.sh` script and start the ioc via `iocsh startup.script`.

The testioc comes with 4 counters incrementing at different speeds.

The readout can be trigger manually by triggering the processing of the read record:

```
caput BSREAD:READ.PROC 1
```

The testioc also comes with a 100Hz counter that can be used for triggering readouts. You can use this counter to trigger the readout by settings
its __FLNK__ field to the read record.

```
caput BSREAD:SIM-PULSE.FLNK BSREAD:READ
``` 

To stop the readout unset the __FLNK__ field of the test trigger.

## Python Client

For receiving data you an use a simple Python client. The only requirement is to have the `pyzmq` package installed.

```python
import zmq
import array

context = zmq.Context.instance()

sock = context.socket(zmq.PULL)
sock.connect('tcp://gfalc6064:9999')

while True:
    message = sock.recv()
    print message
    ## value = array.array('d',message)
    # value.byteswap() # if different endianness
    ## print value
```