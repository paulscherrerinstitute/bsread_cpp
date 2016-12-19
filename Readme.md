# Overview
__bsread__ provides synchronized, fast, Epics IOC based readout functionality for the SwissFEL data acquisition system. It reads out configured channels of the IOC at a timing system provided event and streams out the data via ZMQ.

The ZMQ data stream is served in a ZMQ PUSH/PULL delivery scheme. The default port is 9999. The detailed specification of the stream can be be found [here](https://git.psi.ch/sf_daq/bsread_specification)

# Usage

The __bsread__ module provides all you need to bring beam synchronous data acquisition on an IOC.

To use it, load the bsread module with `require "bsread"` and execute one of the module provided configuration scripts (__bsread_evr.cmd__ or __bsread_sim.cmd__) passing the correct parameters.

On an standard IOC with standard EVR configuration, use following code inside the startup script to bring beam synchronous data acquisition up and running on the IOC:

```
require "bsread"
runScript "$(bsread_DIR)/bsread_evr.cmd"
# afterInit bsreadApply default cfg/bsread_configuration.json  # Apply an initial channel configuration to bsread
```

## Client
To interact and to receive data in a point to point fasion from an bsread IOC, a Python/Command Line client is available. It is installed on all SwissFEL machines. The documentation on it can be found [here](https://git.psi.ch/sf_daq/bsread_commandline).


# Installation
bsread needs to be loaded and configured in the IOC startup script. This is a 2 step approach, first *loading the module* and then *configuring the module* with one of the provided configuration  scripts. Optionally a third step need to take care of the initial configuration as well as slight adaptations needed for specific setups.

## Module Loading
To load the bsread module on an IOC use:

```
require "bsread"
```

This will ensure that at each IOC startup the latest, production grade, version of the bsread module is loaded. If you want to freeze the bsread to a specific version use (only use this in very special cases):

```
require "bsread" <version>
```

## Module Configuration

### Standard Hardware EVR Setup - bsread_evr.cmd

For using bsread with a hardware timing event receiver (EVR) use the provided __bsread_evr.cmd__ script for configuring the module. This script connects bsread to the standard EVR records so that it is triggered by the correct (hardware) timing event and that *pulse_id* and *global timestamp* are obtained from correct channels.

```
require "bsread"
runScript "$(bsread_DIR)/bsread_evr.cmd"
```

#### Parameters

The configuration script takes several parameters that can be adjusted based on the given IOC and hardware setup.

__mandatory__


__optional__ *[default]*

- `SYS` *[$(IOC)]* - System prefix (e.g. my IOC0). This parameter gets expanded to $(SYS)-BSREAD:xx.
- `EVR` *[EVR0]* - Id of EVR to be used.
- `BSREAD_EVENT` *[28]* - Timing system event to be used to trigger the data readout.
- `PULSEID_OFFSET` *[0]* - PULSEID offset to be applied to the pulse_id. This number can be positive or negative integer.
- `BSREAD_PULSEID` *[$(SYS)-$(EVR):RX-PULSEID]* - Record used to obtain pulse id.
- `BSREAD_TS_SEC` *[$(SYS)-$(EVR):RX-MTS-SEC]* - Record used to obtain the global timestamp seconds.
- `BSREAD_TS_NSEC` *[$(SYS)-$(EVR):RX-MTS-NSEC]* - Record used to obtain global timestamp nanoseconds.


- `BSREAD_PORT` *[9999]* - bsread primary port to use for sending data, the bsread configuration port is always primary + 1.
- `BSREAD_MODE` *[PUSH]* - zmq mode used for delivering messages (PUSH or PUB).


- `READ_FLNK` *[]* - Forward link for the :READ record. This link can be used to trigger records that needs to be executed whenever data is read out.
- `NO_EVR` - Set this macro to `#` to disable the usage of the EVR.


### Simulation Setup - bsread_sim.cmd

In order to run bsread for testing purpose without an EVR receiver the __bsread_sim.cmd__ configuration script can be used. This script creates a 100Hz scan record as well as a free running pulse_id counter and connects it to bsread. Note that any data obtained in this way is not (and can not be) synchronized with data obtained from bsread systems that are connected to timing system.

```
require "bsread"
runScript "$(bsread_DIR)/bsread_sim.cmd", "SYS=TEST-IOC"
```

#### Parameters
The configuration script takes several parameters that can be adjusted based on the given IOC and hardware setup.

__optional__ *[default]*

- `SYS` *[$(IOC)]* - System prefix (e.g. my IOC0), is expanded to $(SYS)-BSREAD:xx
- `PULSEID_OFFSET` *[0]* - PULSEID offset to be applied to the pulse_id. This number can be positive or negative integer.
- `BSREAD_PULSEID` *[$(SYS)-$(EVR):RX-PULSEID]* - Record used to obtain pulse id.
- `BSREAD_TS_SEC` *[$(SYS)-$(EVR):RX-MTS-SEC]* - Record used to obtain the global timestamp seconds.
- `BSREAD_TS_NSEC` *[$(SYS)-$(EVR):RX-MTS-NSEC]* - Record used to obtain global timestamp nanoseconds.


- `BSREAD_PORT` *[9999]* - bsread primary port to use for sending data, the bsread configuration port is always primary + 1.
- `BSREAD_MODE` *[PUSH]* - zmq mode used for delivering messages (PUSH or PUB).


- `READ_FLNK` *[]* - Forward link for the :READ record. This link can be used to trigger records that needs to be executed whenever data is read out.


## Advanced Module Configuration

### Set Initial Readout Configuration - bsreadApply

At startup time of an IOC, by default, no channels are configured to be read out by bsread. To set an initial configuration the __bsreadApply__ command can be used. __bsreadApply__ accepts a Json configuration file and configures the specified bsread instance. As the loading of the configuration __must__ happen after the IOC is initialized (since only then channels are available), within an IOC startup script, the __bsreadApply__ command needs to be scheduled for execution after init with the `afterInit` command.

```
afterInit bsreadApply <instance_name> <config_file>
```

In a standard bsread setup this will result to a call similar to this:

```
afterInit bsreadApply default cfg/bsread_configuration.json
```

The bsread configuration file (ideally named __bsread_configuration.json__) should be placed inside the IOC project in the __cfg__ folder. The file can be generated as described below.



#### Creating the Configuration File

Once your IOC is up and running with bsread, you can configure it via the `bs` command described in https://git.psi.ch/sf_daq/bsread_commandline.

After the initial configuration you can use the __bsreadInfo__ command to print the current bsread configuration and status on the IOC shell.

```
bsreadInfo default # default is the name of the instance, it rarely needs be different than default
```

This command will output something like this (your output will look similar):

```
Current config:
{
   "channels" : [
      {
         "modulo" : 2,
         "name" : "sf-lc6-64:ZMQ_VER",
         "offset" : 0,
      },
      {
         "modulo" : 10,
         "name" : "sf-lc6-64:bsread_VER",
         "offset" : 2,
      }
   ]
}

Current params:
        inhibit: 0

Current status:
        ZMQ overflows: 31742
```

To create the configuration file simply copy the Json part of the output (between *Current config:* and *Current params:*) into it. For clarity it is recommended to store it in cfg/bsread_configuration.json)


Contents of cfg/bsread_configuration.json:

```json
{
   "channels" : [
      {
         "modulo" : 2,
         "name" : "sf-lc6-64:ZMQ_VER",
         "offset" : 0,
      },
      {
         "modulo" : 10,
         "name" : "sf-lc6-64:bsread_VER",
         "offset" : 2,
      }
   ]
}
```


# Debugging

## Channels
bsread provides following Epics channels for debugging purpose:

* __$(P):READ.VALA__ - Time in milliseconds required for last readout (double)
* __$(P):READ.VALB__ - Number of times time for read was > 1ms (i.e. FTVA > 1ms) (ulong)
* __$(P):READ.VALC__ - Number of ZMQ buffer overflows (ulong)
* __$(P):INHIBIT__ - Displays the status of inhibit bit.

Where __$(P)__ resolves to __$(SYS)-BSREAD__.


## Enabling Debug Output on the IOC Shell - bsread_debug

Setting a a positive integer to the variable `bsread_debug` enables debug output on the IOC shell. The debug levels are:

1. Info
2. More Info
3. Debug
4. More Debug
5. Periodic Debug (be careful this generates lots of output)

To enable debugging at IOC startup time add this to your startup script:

```
var bsread_debug 1 # Enables debugging and sets debug level of bsread
```

### Retrieve Actual bsread Configuration on the IOC shell - bsreadInfo

__bsread_info__ prints the actual configuration and status of bsread on the IOC shell.

```
bsreadInfo <instance_name>
```

While having no special bsread setup __instance_name__ need to be set to __"default"__.

```
bsreadInfo "default"
```


### PulseId Offset

Different systems may provide data to the EPICS records at different points in time relative to the reception of `BSREAD_EVENT` trigger event. Due to this the actual semantic meaning of the data captured by the bsread may have a offset of +,- x pulse_ids.

PULSEID_OFFSET can be set as part of the module configuration script via the macro __PULSEID_OFFSET__ or at runtime by setting the EPICS channel `$(SYS):READ.D`



# Development

## Components

A bsread consists of 3 base components, bsdata, bsread and epics_bsread.

### bsdata Library

bsdata is the base library for (efficiently) serializing data according to the bsread specification. The library has an external dependency to _libZMQ_. To allow creation of _Windows shared libraries_, by default, the library also depends on _EPICS OSI_ . If you are not on Windows, this can be disabled by defining the symbol `WITHOUT_EPICS`.

```
g++ xyz.c bsdata.cc -DWITHOUT_EPICS
```

Or with cmake

```
add_definitions(-DWITHOUT_EPICS)
```

Note that it is recommended to statically link the library. See `src/bsdata_example` for more info on use of the bsdata.


### bsread Library

The bsread library provides a higher level API than bsdata and adds a BSREAD RPC protocol that allows remote configuration of the bsread system.


See `src/bsread_example` for more info on use of the bsread library.


### epics_bsread

__epics_bsread__ is an EPICS module (driver) that integrates the bsread library into the EPICS IOC. This module consists of an ASUB record implementation, record templates and configuration scripts. Also, for each channel/record to be read out a thin wrapper is created that performs record locking and data copying.

To compile the _bsread_ module within the PSI EPICS infrastructure use:

```
make
```

To install within the PSI EPICS infrastructure use:

```
make install
```

__Note__: For spotting problems easier and quicker it is recommended to prepend the __dye__ command before the actual make statement.

__Note__: To compile the driver using a vanilla EPICS build system, set the correct parameters (`EPICS_BASE`, etc) in `configure/RELEASE`, change directory to the `src` and run `make`. The resulting files will be located in `$TOP/lib/$arch`


## ZMQ RPC

Beside the data socket, bsread also provides a second ZMQ socket for configuration and status retrieval.
Within _epics_bsread_ this is always BSREAD_PORT + 1, i.e. 10000 by default.
Currently following functions are implemented:

__Configuration__

Request:
```json
{"cmd": "config", "config": "<configuration string>"}
```

Response:
```json
{"status": ["ok", "error"], "error": "<error description>"}
```

__Introspection__

Request:
```json
{"cmd":"introspect"}
```

Response:
```json
{"status": "ok", "channels": ["<enumeration of all channels>"], "config": "<current configuration>"}
```

__Inhibit__

Request - set:
```json
{"cmd": "inhibit", "val": true}
```

Request - clear:
```json
{"cmd": "inhibit", "val": false}
```

Request - query:
```json
{"cmd": "inhibit"}
```

To use the ZMQ RPC it is recommended to use the official [Python Client](https://git.psi.ch/sf_daq/bsread_commandline).


## bsreadConfigure IOC Shell Command.

The _bsreadConfigure_ IOC shell function creates a new instance of bsread. In general there is no need to have more than one bsread instance on an IOC. However while calling the function twice additional instances can be created.

```
bsreadConfigure <instance_name> <ZMQ address> <PUSH|PUB> <high watermark>  
```

However additional instances need to be triggered "manually" through the bsread api.

_Note:_ The default instance should be called `default`. Currently the module provided default templates and asub record used to trigger the readout of channels is hardcoded to use `default` bsread instance.

_Note:_ The function can be used either on startup (before iocInit) or during operation. Note that the function will always create a new bsread instance, so invoking it multiple times with same port is illegal and will raise an exception.


## (Re)Generate Json Sources

The `json.cc` and `json.h` files where generated from the json-cpp project. To generate these these file, download json-cpp from the [project page](https://github.com/open-source-parsers/jsoncpp), enter the json-cpp folder and run:

```
python amalgamate.py
```

This will generate `json.cc` and `json.h` files in `./dist`. See the README in json-cpp sources for more details.
