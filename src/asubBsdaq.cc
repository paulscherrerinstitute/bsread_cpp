#include <stdio.h>
#include <cstring>
#include <string>
#include <stdexcept>
#include <iostream>

#include <aSubRecord.h>
#include <registryFunction.h>
#include <dbStaticLib.h>
#include <epicsString.h>
#include <errlog.h>
#include <recSup.h>

#include "bsdaq.h"
#include <stdlib.h>

#include <epicsExport.h>
#include <time.h>


long bsdaqConfigureInit(aSubRecord*);
long bsdaqConfigure(aSubRecord*);
long bsdaqTriggerInit(aSubRecord*);
long bsdaqTrigger(aSubRecord*);


using namespace std;


long bsdaqConfigureInit(aSubRecord* prec){

  if (prec->fta != DBF_CHAR) {
    errlogPrintf("FTA has invalid type. Must be a CHAR");
    prec->brsv=-1;
    return -1;
  }

  return 0;
}


long bsdaqConfigure(aSubRecord* prec){
    /* Reading from a string waveform */
    char* config = (char*) prec->a;
    try{
        BSDAQ::get()->configureBSDAQ(string(config));
    }
    catch(runtime_error & e){
        errlogPrintf("Problem parsing BSDAQ configuration: %s\n", e.what());
        prec->brsv=-1;
        return -1;
    }

    return 0;
}


long bsdaqTriggerInit(aSubRecord* prec){
    return 0;
}


long bsdaqTrigger(aSubRecord* prec){

    long* a = (long*)(prec->a);
    long bunchId = a[0];

    //Serialization performance measurment
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    BSDAQ::get()->snapshot(bunchId);

    clock_gettime(CLOCK_MONOTONIC, &t1);
    long int timeSpan = (t1.tv_sec*1e9+t1.tv_nsec)-(t0.tv_sec*1e9+t0.tv_nsec);


    //Put serialization time into VALA
    long *vala = (long *)prec->vala;
    *vala = timeSpan;
    return 0;
}

extern "C"{
epicsRegisterFunction(bsdaqTriggerInit);
epicsRegisterFunction(bsdaqTrigger);
epicsRegisterFunction(bsdaqConfigureInit);
epicsRegisterFunction(bsdaqConfigure);
}
