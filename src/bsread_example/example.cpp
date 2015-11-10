#include "../bsread.h"
#include "zmq.hpp"


#include <iostream>
#include <sstream>     // cout, ios
#include <string>
#include <streambuf>
#include <time.h>
#include <vector>
#include <unistd.h>

using namespace std;


///// Utilities /////////

/**
 * @brief time_get same as clock_gettime except that it returns time in seconds
 * @param clock_type
 * @return
 */
inline double dbltime_get(int clock_type=CLOCK_REALTIME){
    struct timespec t;
    clock_gettime(clock_type,&t);
    return t.tv_sec + t.tv_nsec/1e9;
}

/**
 * @brief time_nanosleep same as clock_nanosleep, except that it takes argument in seconds
 * @param sec
 * @param clock_type
 */
inline void time_nanosleep(double sec, int clock_type=CLOCK_REALTIME){
    struct timespec t;
    t.tv_sec=int(sec);
    t.tv_nsec= (sec-t.tv_sec)*1e9;

    clock_nanosleep(clock_type,0,&t,0);
}



void test_chan_cb(bsread::BSDataChannel* chan,bool acquire, void* pvt){
    if(acquire){
        //Update data
        double* data = static_cast<double*>(chan->get_data());
        data[0]=dbltime_get(CLOCK_PROCESS_CPUTIME_ID);

        chan->set_timestamp();
    }
}


int main(int argc, char *argv[])
{
    bsread_debug = 10;

    //DAQ buffer
    size_t buffer_len = 1024;
    unsigned int* buffer = new unsigned int[buffer_len];

    //Create a channel for this buffer, 4byte long unsigned int data
    bsread::BSDataChannel daq_channel("BSREADTEST:DAQ_DATA",bsread::BSDATA_INT);

    //Since the daq_channel data and the bsread sending is preformed from the same
    //thread no locking of data is needed, we only need to set the channels data.
    //Whenever a message that contians this channel will be sent, the data will be
    //fetched from buffer and sent out.
    daq_channel.set_data(buffer,buffer_len);

    //Lets create a second channel that will hold time needed to send the last message

    double time_spent;
    bsread::BSDataChannel time_channel("BSREADTEST:TIME_SPENT",bsread::BSDATA_DOUBLE);
    time_channel.set_data(&time_spent,1);
    time_channel.m_meta_modulo=1;

    //Now we need to create a bsdata message that will capture our channels
    bsread::BSDataMessage message;

    bsread::BSRead sender;
    sender.add_channel(&daq_channel);
    sender.add_channel(&time_channel);

    sender.enable_all_channels();

    sender.confiugre_zmq("tcp://*:9991",ZMQ_PUSH,100);
    sender.confiugre_zmq_config("tcp://*:9995");

//    while(1);

    //Lets send one message every 10ms, as in SwissFEL
    long pulse_id=0;
    timespec global_timestamp;

    double t; //Used for time measurment
    size_t sent;

    while(true){
        pulse_id++;


        t = dbltime_get();
        //Send the message
        sender.send(pulse_id,global_timestamp);

        time_spent = (dbltime_get() - t)*1e3;
        //time_spent, which is a buffer for time_channel was updated
        //it makes sense to update its timestamp as well
        time_channel.set_timestamp();

//        cout << "Send " << sent/1024 <<" kb " << "took " << time_spent<< "ms" << endl;

        //Sleep 10ms
        time_nanosleep(1000/1e3);

    }

    return 0;
}
