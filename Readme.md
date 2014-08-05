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

The ZMQ message send out for each readout is split in a header and body part. The header part contains JSON encoded data containing the `htype` and the number of (double) `elements`
contained in the body part. The body part contains, for each configured readout resource its double value (8 bytes) in BIG_ENDIAN format.

The data stream is served in a ZMQ PUB/SUB delivery scheme.


# Installation
To configure CRLOGIC on your IOC continue as follows:

  * Copy latest `crlogic.template` file from the CRLOGIC project to your IOC configuration folder
  * Create substitution file `crlogic.subs` for crlogic.template ((Example)[doc/EXAMPLE_crlogic.subs])
 
```
file crlogic.template {
	{
		P = "YOUR-IOC-CRLOGIC-PREFIX"
	} 
} 
```


# Development


* Compilation

```
make
```

* Installation

```
make install
```

# Debugging
Start medm panel:
```
medm -x -macro "P=MTEST-HW3-CRL" /work/sls/config/medm/G_CRLOGIC_expert.adl
```

