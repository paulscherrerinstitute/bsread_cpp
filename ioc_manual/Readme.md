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


