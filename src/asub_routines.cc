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

#include "bsread.h"
#include <stdlib.h>

#include <epicsExport.h>
#include <time.h>


long bsread_configure_init(aSubRecord*);
long bsread_configure(aSubRecord*);
long bsread_read_init(aSubRecord*);
long bsread_read(aSubRecord*);


using namespace std;


long bsread_configure_init(aSubRecord* record){
    if (record->fta != DBF_CHAR) {
        Debug("FTA has invalid type. Must be a CHAR");
        record->brsv=-1;
        return -1;
    }

  return 0;
}


long bsread_configure(aSubRecord* prec){
    /* Reading from a string waveform */
    char* configuration = (char*) prec->a;
    try{
        BSRead::get_instance()->configure(string(configuration));
    }
    catch(runtime_error & e){
        Debug("Problem parsing BSDAQ configuration: %s\n", e.what());
        prec->brsv=-1;
        return -1;
    }

    return 0;
}


long bsread_read_init(aSubRecord* prec){
    return 0;
}


long bsread_read(aSubRecord* prec){

    long* a = (long*)(prec->a);
    long pulse_id = a[0];

    //Serialization performance measurment
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    BSRead::get_instance()->read(pulse_id);

    clock_gettime(CLOCK_MONOTONIC, &t1);
    long int timeSpan = (t1.tv_sec*1e9+t1.tv_nsec)-(t0.tv_sec*1e9+t0.tv_nsec);


    //Put serialization time into VALA
    long *vala = (long *)prec->vala;
    *vala = timeSpan;
    return 0;
}


extern "C"{
    epicsRegisterFunction(bsread_configure_init);
    epicsRegisterFunction(bsread_configure);
    epicsRegisterFunction(bsread_read_init);
    epicsRegisterFunction(bsread_read);
}
