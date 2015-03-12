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

// Setting PACT ensures that the record is not processed if init failed.
static long fail_init(aSubRecord *prec)
{
    prec->brsv = 1;
    prec->pact = 1;
    return -1;
}

static long fail_process(aSubRecord *prec)
{
    prec->brsv = 1;
    return -1;
}

long bsread_configure_init(aSubRecord* record){
    Debug("configure init\n");
    if (record->fta != DBF_CHAR) {
        Debug("FTA has invalid type. Must be a CHAR");
        return fail_init(record);
    }

  return 0;
}


long bsread_configure(aSubRecord* prec){
    Debug("configure\n");
    /* Reading from a string waveform */
    char const *configuration = (char const *) prec->a;
        if (strnlen(configuration, prec->noa) == prec->noa) {
            Debug("Config is not null terminated!\n");
            return fail_process(prec);
        }
    try{
        BSRead::get_instance()->configure(string(configuration));
    }
    catch(runtime_error & e){
        Debug("Problem parsing BSDAQ configuration: %s\n", e.what());
        return fail_process(prec);
    }

    return 0;
}


long bsread_read_init(aSubRecord* prec){
    Debug("read init\n");
    // INPA = bunch ID
    if (prec->fta != DBF_ULONG) {
        Debug("FTA must be ULONG.\n");
        return fail_init(prec);
    }
    if (prec->noa != 1) {
        Debug("INPA must be a scalar.\n");
        return fail_init(prec);
    }
    //INPB is master timestamp seconds
    if (prec->ftb != DBF_ULONG) {
        Debug("FTB must be ULONG.\n");
        return fail_init(prec);
    }
    if (prec->nob != 1) {
        Debug("INPB must be a scalar.\n");
        return fail_init(prec);
    }
    //INPC is master timestamp nsec
    if (prec->ftc != DBF_ULONG) {
        Debug("FTA must be ULONG.\n");
        return fail_init(prec);
    }
    if (prec->noc != 1) {
        Debug("INPA must be a scalar.\n");
        return fail_init(prec);
    }

    // VALA = snapshot duration
    if (prec->ftva != DBF_DOUBLE) {
        Debug("FTVA must be DOUBLE.\n");
        return fail_init(prec);
    }
    if (prec->nova != 1) {
        Debug("NOVA must be 1.\n");
        return fail_init(prec);
    }

    // VALB = number of timeouts
    if (prec->ftvb != DBF_ULONG) {
        Debug("FTVB must be ULONG\n");
        return fail_init(prec);
    }
    if (prec->novb != 1){
        Debug("NOVB must be 1\n");
        return fail_init(prec);
    }

    // VALC = number of dropped messages
    if (prec->ftvb != DBF_ULONG) {
        Debug("FTVB must be ULONG\n");
        return fail_init(prec);
    }
    if (prec->novb != 1){
        Debug("NOVB must be 1\n");
        return fail_init(prec);
    }
    return 0;
}


long bsread_read(aSubRecord* prec){
    //Debug("read\n");
    //Extract pulse id
    unsigned long* a = (unsigned long*)(prec->a);
    unsigned long pulse_id = a[0];

    //Extract timestamp
    struct timespec t;
    t.tv_sec = ((unsigned long*)(prec->b))[0];
    t.tv_nsec = ((unsigned long*)(prec->c))[0];


    //Serialization performance measurment
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    BSRead::get_instance()->read(pulse_id,t);

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double timeSpan = (t1.tv_sec * 1e9 + t1.tv_nsec) - (t0.tv_sec * 1e9 + t0.tv_nsec);
    timeSpan/=1e6; //time in ms

    // Put the serialization time into VALA.
    *(double *)prec->vala = timeSpan;
    prec->neva = 1;

    //If it takes more than 1 ms to sample the data than we have an error
    if(timeSpan > 1.0){
        (*(unsigned long*)prec->valb)++; //increase number of "timeouts"
        return -1; //Throw record into an alarm state
    }

    //Update the overflow count
    (*(unsigned long*)prec->valc) = BSRead::get_instance()->numberOfZmqOverflows();
    //Check if new configuration is available
    BSRead::get_instance()->applyConfiguration();
    return 0;
}


extern "C"{
    epicsRegisterFunction(bsread_configure_init);
    epicsRegisterFunction(bsread_configure);
    epicsRegisterFunction(bsread_read_init);
    epicsRegisterFunction(bsread_read);
}
