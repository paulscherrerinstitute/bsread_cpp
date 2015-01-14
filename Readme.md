
Create Configuration (need to use the caput form EPICS base instead of PSI):

```
export EPICS_CA_ADDR_LIST=gfalc6064
/usr/local/epics/base/bin/SL6-x86_64/caput -S  BSREAD:CONFIGURATION '{"channels": [{"name":   "BSREAD-TEST:TEST_1", "offset":1, "frequency":100 }, {"name":"BSREAD-TEST:TEST_2", "offset":1, "frequency":10} ]}'
```

Test receiver

```
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


# TO BE REVIEWED

# Overview
__bsread__ provides a fast IOC based readout functionality. It reads configured channels and streams out the data via ZMQ. 
All channels to be read out need to reside on the same IOC than the __bsread__ code is running.

The ZMQ data stream is served in a ZMQ PUSH/PULL delivery scheme. The default port is 8080.
The stream solely consists of messages with consists of an array of double either encoded in LITTLE_ENDIAN if 
the IOC runs on a Linux machine or BIG_ENDIAN if the IOC runs on vxWorks.


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

  * __$(P):CHANNELS__ - Channels to be read out
  * __$(P):READ__ - Readout record - whenever processed a readout will be triggered	 
  * __$(P):CONFIGURE__ - For internal use only

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

# Testing

Configure readout channels and read out the channels:

```
caput TEST-BSREAD:CHANNELS "ACHANNEL B.CHANNEL"
caput TEST-BSREAD:READ.PROC 1
```

# Test IOC
There is a test ioc inside the git repository inside the `ioc` folder. To use the IOC for testing, compile the __BSREAD__ sources inside 
the `src` folder (use the Makefile inside the folder!), switch to the ioc directory, execut the `makeioc.sh` script and start the ioc via `iocsh startup.script`.

The testioc comes with 4 counters incrementing at different speeds. You can configure the counters to be read out as follows:

```
caput BSREAD:CHANNELS "BSREAD:TEST_1 BSREAD:TEST_2 BSREAD:TEST_3 BSREAD:TEST_4"
```

You can now manually trigger a readout by triggering a processing of the read record:

```
caput BSREAD:READ.PROC 1
```

The testioc also comes with a 100Hz counter that can be used for triggering readouts. You can use this counter to trigger the readout by settings
its __FLNK__ field to the read record.

```
caput BSREAD:TEST_TRIGGER.FLNK BSREAD:READ
``` 

To stop the readout unset the __FLNK__ field of the test trigger.

## Python Client

For receiving data you an use a simple Python client. The only requirement is to have the `pyzmq` package installed.

```python
import zmq
import array

context = zmq.Context.instance()

sock = context.socket(zmq.PULL)
sock.connect('tcp://slslc:8080')

while True:
    message = sock.recv()
    value = array.array('d',message)
    # value.byteswap() # if different endianness
    print value
```


# Todo

  * Support other datatypes than doubles
  * Support different readout/send frequencies
  * Readout time limited to 1ms at the end of a cycle
  * data transfer structure - consider sub messages to enable clients to skip certain sub-messages ...


# Coding Style
http://google-styleguide.googlecode.com/svn/trunk/cppguide.html