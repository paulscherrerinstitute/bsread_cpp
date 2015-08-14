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

