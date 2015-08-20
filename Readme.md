# Overview
__bsread__ provides the synchronized, fast, IOC based readout functionality for the SwissFEL data acquisition system. It reads configured channels on a trigger and streams out the data via ZMQ.

The ZMQ data stream is served in a ZMQ PUSH/PULL delivery scheme. The default port is 9999. The stream consists of messages consisting of several sub-messages.

The specification can be be found at [here](https://docs.google.com/document/d/1BynCjz5Ax-onDW0y8PVQnYmSssb6fAyHkdDl1zh21yY/edit#)


# Installation

## Standard Setup
In order to add bsread to your IOC, simply add the following line to the IOC startup script:

```
< $(TEMPLATES)/BSREAD/bsread.startup
```

If you want to customize options, such as BSREAD event or source of PulseID, etc. you can do so by setting appropriate env variables *before* loading bsread

```
epicsEnvSet EVR $(EVR=EVR0)   ##Set EVR receiver id (default EVR0)
epicsEnvSet BSREAD_EVENT $(BSREAD_EVENT=40) ##Trigger BSREAD using EVR event (default event 40)
epicsEnvSet BSREAD_PULSEID $(BSREAD_PULSEID= $(SYS)-DBUF-$(EVR):BunchIdRx-I) ##which record to use to obtaion pulse id(default PulseID received using EVR Dbuff)
epicsEnvSet BSREAD_TS_SEC $(BSREAD_TS_SEC= $(SYS)-DBUF-$(EVR):BunchIdRx-MASTER-TS-SEC) ##record holding timestamp seconds (default dbuff)
epicsEnvSet BSREAD_TS_NSEC $(BSREAD_TS_NSEC= $(SYS)-DBUF-$(EVR):BunchIdRx-MASTER-TS-NSEC) ##record holding timestamp nsec (default dbuff)
```

## Simulation

BSREAD can be used without timing receiver for testing puropses, for that add the following line to the IOC startup:

```
< $(TEMPLATES)/BSREAD/bsread_sim.startup
```



# Usage

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

### Examples
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
