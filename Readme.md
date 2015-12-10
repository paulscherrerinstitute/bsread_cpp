# Overview
__bsread__ provides the synchronized, fast, IOC based readout functionality for the SwissFEL data acquisition system. It reads configured channels on a trigger and streams out the data via ZMQ.

The ZMQ data stream is served in a ZMQ PUSH/PULL delivery scheme. The default port is 9999. The stream consists of messages consisting of several sub-messages.

The specification can be be found at [here](https://docs.google.com/document/d/1BynCjz5Ax-onDW0y8PVQnYmSssb6fAyHkdDl1zh21yY/edit#)

#Usage

__Currently only installed to fin/devel since usage relies on updated _require_ and *drive.makefile*__


1 module is provided: 
 - __bsread__
    Core bsread libraries and templates.

2 startup scripts are provided: 
  - __bsread_sim.cmd__ used for simulation and testing
  - __bsread_evr.cmd__ used for production systems with EVR 

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
      - READ_FLNK is a forward link for the :READ record

There is additional macro used to disable loading of the EVR template. To achieve this, use macro `NO_EVR=#`.


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





# Advance Usage

## Changing default ZMQ socket options

Using bsreadConfigure iocsh function it is possible to set ZMQ socket paramters:

    bsreadConfigure <ZMQ address> <PUSH|PUB> <high watermark> 
    e.g.: 

    bsreadConfigure "tcp://*:9090" PUSH 100

Function can be used either on startup (before iocInit) or during operation, in which case the new socket will be opened and prepared in advance and switched at the end of next "read" operation. This allows for seamless handover without data loss.



There are following channels to configure, control and monitor __bsread__:

  * __$(P):CONFIGURATION__ - Configuration - i.e. channels to be read out

  * __$(P):READ.FTVA__ - Time in seconds required for last readout (double)
  * __$(P):READ.FTVB__ - Number of times time for read was > 1ms (i.e. FTVA > 1ms) (ulong)
  * __$(P):INHIBIT__ - Inhibits data readouts when set to 1. Normal operation can be resumed by setting record value back to default value 0.

## Using configuration record
The configuration record (__$(P):CONFIGURATION__) is used to set the channels to be read out together with additional properties for each channel:

 * __channel name__ - The name of the channel to read out
 * __modulo__ - Modulo applied to pulse-id to determine the readout (min: 1, max: max-integer, default: 1)
 * __offset__ - Offset in pulses (default: 0)

The actual readout of channel named __channel name__ will be at: (pulse-id - __offset__)% __modulo__.

Configuration is pushed to the configuration record as a JSON string. The following example win enable the readout of channel _BSREAD-TEST:TEST_1_ with modulo 2 and offset 0 and channel _BSREAD-TEST:TEST_2_ with modulo 10 and offset 2:

    {"channels":[
      {
        "name":"BSREAD-TEST:TEST_1",
        "offset":0,
        "modulo":2
      },
      {
        "name":"BSREAD-TEST:TEST_2",
        "offset":2,
        "modulo":10
      }
    ]}

Such configuration can be pushed to the configuration record like so (assume __$(P)__ = _TEST-BSREAD_):

    /usr/local/epics/base/bin/SL6-x86_64/caput -S TEST-BSREAD:CONFIGURATION '{"channels": [{"name":"BSREAD-TEST:TEST_1","offset":0,"modulo":2},{"name":"BSREAD-TEST:TEST_12,"offset":2,"modulo":10} ]}'


## ZMQ RPC

BSREAD also creates a second ZMQ socket (within EPICS bsread this is always zmq port + 1 [10000 by default]) that can is used to preform a RPC calls. 
Currently only 2 functions are implemented; introspection and configuration. 

- Configuration:

  request:

      {"cmd":"config","config":<configuration string>}

  response:

      {"status":["ok","error"], "error":<error description>}

- Introspection:

  request:

      {"cmd":"introspect"}

  response:

      {"status": "ok", "channels": [<enumeration of all channels>], "config":<current configuration>}


# Examples
Assume that __$(P)__ = _TEST-BSREAD_ is the following examples.

* Clear configuration

    /usr/local/epics/base/bin/SL6-x86_64/caput -S TEST-BSREAD:CONFIGURATION '{"channels": []}'

* Configure readout of channel _BSREAD-TEST:TEST_1_ with offset 5

    /usr/local/epics/base/bin/SL6-x86_64/caput -S TEST-BSREAD:CONFIGURATION '{"channels": [{"name":"BSREAD-TEST:TEST_1","offset":5}]}'

* Configure readout of channel _BSREAD-TEST:TEST_1_ and _BSREAD-TEST:TEST_2_, both with offset 0 and modulo 1

    /usr/local/epics/base/bin/SL6-x86_64/caput -S TEST-BSREAD:CONFIGURATION '{"channels": [{"name":"BSREAD-TEST:TEST_1"},{"name":"BSREAD-TEST:TEST_2"}]}'



## Python Client

The most simple client for receiving data you an use following simple Python script. The only requirement is to have the `pyzmq` package installed.


```python
import zmq

context = zmq.Context.instance()

sock = context.socket(zmq.PULL)
sock.connect('tcp://ioc_hostname:9999')

while True:
    message = sock.recv()
    print message
```


# Development

## Templates
__bsread__ comes with a set of predefined templates (i.e. to be able to easily install it on IOCs). To install/deploy these templates, set your `INSTBASE` variable to desired location (e.g. _/fin/devl_) and run

```
./install_templates.sh
```

## Running test IOC
When using predefined templates from `ioc/` folder for testing purposes, you should never run the install script (`./install_templates.sh`). One can run an IOC from `ioc/` folder in a standard PSI way. Detailed instructions are available in `ioc/readme.md`.

## Driver

To compile the _bsread_ driver within the PSI EPICS infrastructure use:

```
make
```

To install within the PSI EPICS infrastructure use:

```
make install
```

__Note__: For spotting problems easier and quicker it is recommended to prepend the __dye__ command before the actual make statement.


__Note__: To compile the driver using a vanilla EPICS build system, set the correct parameters (`EPICS_BASE`, etc) in `configure/RELEASE`, change directory to `src` and run `make`. The resulting files will be located in `$TOP/lib/$arch`

## Miscelaneous
### JSON
The `json.cc` and `json.h` files where generated from the json-cpp project. To generate these these file, download json-cpp from the [project page](https://github.com/open-source-parsers/jsoncpp), enter the json-cpp folder and run:

```
python amalgamate.py
```

This will generate `json.cc` and `json.h` files in `./dist`. See the README in json-cpp sources for more details.


## Coding Style
The code follows the following coding style: http://google-styleguide.googlecode.com/svn/trunk/cppguide.html
