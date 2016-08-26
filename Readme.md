# Overview
__bsread__ provides synchronized, fast, IOC based readout functionality for the SwissFEL data acquisition system. It reads configured channels from IOC at timing system provided trigger and streams out the data via ZMQ.

The ZMQ data stream is served in a ZMQ PUSH/PULL delivery scheme. The default port is 9999. The stream consists of messages consisting of several sub-messages.

The specification can be be found [here](https://git.psi.ch/sf_daq/bsread_specification)

# Usage

The __bsread__ module provides all you need to bring beam synchronous data acquisition on your IOC.

It consists of following main parts:

1. Driver
 - __bsread__
    Core bsread libraries and templates.

2. Startup Scripts
  - __bsread_sim.cmd__ used for simulation and testing
  - __bsread_evr.cmd__ used for production systems with EVR

Also see [_example_](example) directory.

## bsread_sim

In order to run bsread without a EVR receiver (e.g. for testing or for use with softIOC and systems without timing) a bsread_sim module can be used. This module creates a 100Hz scan record and connects it to bsread and free running pulseid counter. Note that any data obtained in this way is not (and can not be) synchronized with data obtained from bsread systems that are connected to timing system.

Simply append the following line to startup script:

```
require "bsread"
runScript "$(bsread_DIR)/bsread_sim.cmd", "SYS=TEST-IOC"
```

Parameters that can be passed to the module are:

__mandatory__
  - `SYS` - System prefix (e.g. my IOC0), is expanded to $(SYS)-BSREAD:xx

__optional__
  - `BSREAD_PULSEID` -  Record used to obtain pulse id
  - `BSREAD_TS_SEC` -  Record used to obtain global timestamp sec
  - `BSREAD_TS_NSEC` -  Record used to obtain global timestamp sec

  - `READ_FLNK` -  Forward link for the :READ record

  - `BSREAD_PORT` - bsread primary port to use (default = 9999), configuration port is always primary + 1
  - `BSREAD_MODE` - zmq mode of operation (PUSH or PUB)


## bsread_evr

Majority of systems will use bsread in connection with hardware timing receiver (EVR). To simplify setup a bsread_evr module is provided. This module loads bsread and connects it to correct EVR records (so that it is triggered by hardware timing event and that pulse_id and global timestamp are obtained from databuffer)


Simply append the following line to startup script:

```
require "bsread"
runScript "$(bsread_DIR)/bsread_evr.cmd", "SYS=TEST-IOC,EVR=EVR0"
```

Parameters that can be passed to the module are:

__mandatory__
  - `SYS` - System prefix (e.g. my IOC0), is expanded to $(SYS)-BSREAD:xx

__optional__ [default]

  - `EVR` - Id of EVR to be used [EVR0]
  - `BSREAD_EVENT` - Timing event that should be used to trigger bsread acquisition

  - `BSREAD_PULSEID` - Record used to obtain pulse id
  - `BSREAD_TS_SEC` - Record used to obtain global timestamp sec
  - `BSREAD_TS_NSEC` -  Record used to obtain global timestamp sec

  - `READ_FLNK` -  Forward link for the :READ record

  - `BSREAD_PORT` - bsread primary port to use (default = 9999), configuration port is always primary + 1
  - `BSREAD_MODE` - zmq mode of operation (PUSH or PUB)

__deprecated__

  - `NO_EVR` - Set this macro to `#` to disable EVR. This will be replaced with a special startup script



# Python client

A python client was developed that allows both a configuration as well as readout of bsread data. See the following
GIT page for details: https://git.psi.ch/sf_daq/bsread_commandline


# Monitoring bsread operation

There are following channels to configure, control and monitor __bsread__:

  * __$(P):CONFIGURATION__ - Configuration - i.e. channels to be read out (supported but deprecated, use ZMQ rpc instead)

  * __$(P):READ.FTVA__ - Time in seconds required for last readout (double)
  * __$(P):READ.FTVB__ - Number of times time for read was > 1ms (i.e. FTVA > 1ms) (ulong)
  * __$(P):INHIBIT__ - Displays the status of inhibit bit.


# Advanced Usage

## Enabling debug output

A variable `bsread_debug` is provided that, when set to a positive integer enables debug output.
To enable debugging append (or prepend) this statement to your startup script:

    var bsread_debug 6 #Enables full debugging of bsread


Debug levels:

1. Info
2. More info
3. Debug
4. More debug
5. Periodic debug (careful! Loads of output)


## bsreadConfigure IOCSH command.

While using the bsreadConfigure iocsh function creates a new instance of bsread. In general there is no need to have more than one bsread instance on an IOC. To allow future compatibility this instance should be called `default`. Currently the asub is record used to trigger the readout is hardcoded to use `default` bsread instance.

Additional instances may be created, however they  have to be triggered programatically, trough BSREAD API.

```
bsreadConfigure <instance_name> <ZMQ address> <PUSH|PUB> <high watermark>  
```

e.g.

```
bsreadConfigure "default" "tcp://*:9090" PUSH 100
```

Function can be used either on startup (before iocInit) or during operation. Note that the function creates a new bsread instance, so invoking it multiple times with same port is illegal and will raise an exception.

## bsreadApply IOCSH command

```
bsreadApply <instance_name> <config_file>
```

bsreadApply load a json configuration file (conating same json message as passed via ZMQ RPC or via configuration record) and apply it to desired bsread instancee.

e.g.

```
bsreadApply default myConfigFile.json
```


###Example: 

Lets say for the sake of example that you are running a system that was configured via `bs` command (e.g. `bs config -a` to enable all channels) and you would now like to persist this configuration so that it is loaded on IOC startup. 

First lets fetch the existing configuration: 

     #In IOCSH
     bsreadInfo default #default is the name of the instance, it rarely needs be different than default 

This will output some bsread information as well as current configuration json. The output will look similar to this: 


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


To create a configuration file simply copy the json part of the output into the file (filename and location is up to you, but for clarity it is recommended to store it in cfg/bsread.json)


Contents of cfg/bsread.json: 

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



Now you can restart the ioc and try to apply this configuration using `bsreadApply` command 

    bsreadApply default cfg/bsread.json


And confirm that the configuration was applied by running `bsreadInfo default` 

This allows you to change the configurations on the fly, but to load configuration on boot we __cannot__ just insert `bsreadApply default cfg/bsread.json` into the startup script as this needs to happen after ioc is initialized (since only than channels are available). The bsreadApply command needs to be scheduled for execution after init, this can be done `afterInit` command. 


Finally the startup script that initializes bsread and loads configuration looks like this: 


    runScript $(bsread_DIR)/bsread_sim.cmd, "SYS=SLEJKO,BSREAD_PORT=9999"

    afterInit bsreadApply default cfg/bsread.json

                       


## bsreadInfo IOCSH command
```
bsreadInfo <instance_name>
```

Prints some debug info into iocsh. It also includes a current configuration that can be used as starting point for configuration file consumed by bsreadApply


## ZMQ RPC

BSREAD also creates a second ZMQ socket (within EPICS bsread this is always zmq port + 1 [10000 by default]) that can is used to preform a RPC calls.
Currently only 2 functions are implemented; introspection and configuration.

To use ZMQ RPC an offical [Python client is recommended](https://git.psi.ch/sf_daq/bsread_commandline)

- Configuration:

  request:
      ```
      {"cmd":"config","config":<configuration string>}
      ```

  response:
      ```
      {"status":["ok","error"], "error":<error description>}
      ```

- Introspection:

  request:
      ```
      {"cmd":"introspect"}
      ```

  response:
      ```
      {"status": "ok", "channels": [<enumeration of all channels>], "config":<current configuration>}
      ```

- Inhibit:

  set:
      ```
      {"cmd":"inhibit","val":true}
      ```

  clear:
      ```
      {"cmd":"inhibit","val":false}
      ```

  query:
      ```
      {"cmd":"inhibit"}
      ```



## Using configuration record (deprecated, use ZMQ RPC instead)

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


# Development

## Code organization and components

A bsread EPICS module consists of 3 parts:


###BSDATA library:

Dependency free serialization library, used to efficently serialzie data according to BSREAD specification. Libraries only dependency mandatory dependency is libZMQ. By default library depends on EPICS OSI to allow creation of Windows shared libraries. This can be disabled by defining symbol `WITHOUT_EPICS`.

for example:

```
g++ xyz.c bsdata.cc -DWITHOUT_EPICS
```

or in cmake:

add_definitions(-DWITHOUT_EPICS)


Note that it is recommended to staticaly link the library. See `src/bsdata_example` for more info on use of the bsdata.


###BSREAD library:

A more high level libray built on top of BSDATA and EPICS OSI. Library provides a higher level API and adds a BSREAD RPC protocol that allows remote configuration of the bsread system.


See examples in `src/bsread_example`


###EPICS_BSREAD

EPICS module (driver) that integrates a bsread library into EPICS ioc. This module consists of ASUB record implementation and BSREAD integration.
On creation of bsread instances (using iocsh function, see above for details) all IOCs recrods are added to bsread. For each record a thin wrapper is created that performs record locking and data copying.


To compile the _bsread_ module within the PSI EPICS infrastructure use:

```
make
```

To install within the PSI EPICS infrastructure use:

```
make install
```

__Note__: For spotting problems easier and quicker it is recommended to prepend the __dye__ command before the actual make statement.


__Note__: To compile the driver using a vanilla EPICS build system, set the correct parameters (`EPICS_BASE`, etc) in `configure/RELEASE`, change directory to `src` and run `make`. The resulting files will be located in `$TOP/lib/$arch`

### Miscelaneous
#### JSON
The `json.cc` and `json.h` files where generated from the json-cpp project. To generate these these file, download json-cpp from the [project page](https://github.com/open-source-parsers/jsoncpp), enter the json-cpp folder and run:

```
python amalgamate.py
```

This will generate `json.cc` and `json.h` files in `./dist`. See the README in json-cpp sources for more details.
