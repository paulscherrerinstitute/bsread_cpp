#include "../bsdata.h"

#include <iostream>
#include <sstream>     // cout, ios
#include <string>
#include <streambuf>
#include <time.h>
#include <vector>
#include <unistd.h>

#include <zmq.hpp>
#include "json.h"

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


/**
 * @brief example Example showing the correct of BSDATA in application that manually manages
 * its databuffer. (e.g. buffer access is local to thread that is sending the data)
 *
 * @param buffer_len length of integer buffer to be sent. a sensible value is 300e3 which corresponds to 1.2Mb per message
 * or about 960MBps bandwidth
 *
 * @param sleep time in seconds to wait before sending a next message
 */
void example(size_t buffer_len, double sleep=0.01){
    //DAQ buffer
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
    bsread::BSDataChannel time_channel("BSREADTESTS:TIME_SPENT",bsread::BSDATA_DOUBLE);
    time_channel.set_data(&time_spent,1);
    time_channel.m_meta_modulo=100;

    //Now we need to create a bsdata message that will capture our channels
    bsread::BSDataMessage message;

    //Add both channels to the message
    message.add_channel(&daq_channel);
    message.add_channel(&time_channel);


    //We now need to create a bsread sender that will serialize and send out the message
    bsread::BSDataSenderZmq zmq_sender("tcp://*:9999",4096*4096);

    //Lets send one message every 10ms, as in SwissFEL
    long pulse_id=0;
    timespec global_timestamp;

    double t; //Used for time measurment
    size_t sent;

    while(true){
        pulse_id++;
        //Set a message, assign it a pulse id and prepare it for sending
        message.set(pulse_id,global_timestamp);

        t = dbltime_get();
        //Send the message
        sent = zmq_sender.send_message(message);

        time_spent = (dbltime_get() - t)*1e3;
        //time_spent, which is a buffer for time_channel was updated
        //it makes sense to update its timestamp as well
        time_channel.set_timestamp();

        cout << "Send " << sent/1024 <<" kb " << "took " << time_spent<< "ms" << endl;

        //Sleep 10ms
        time_nanosleep(10/1e3);

    }

}


int main(int argc, char *argv[])
{

    example(300e3); //300e3*4b = ~1.2 mb * 100Hz = 120Mb/s = 960MBps

    double dbl_test[2048];
    long int int_test[2048];
    string str_test = "Hello world string";
    const char* chr_test = "Hello world char";

    bsread::BSDataChannel chan1("testChan",bsread::BSDATA_DOUBLE);
    bsread::BSDataChannel chan2("testChan2",bsread::BSDATA_LONG);
    bsread::BSDataChannel chan3("testChan3",bsread::BSDATA_STRING);
    bsread::BSDataChannel chan4("testChan4",bsread::BSDATA_STRING);
    bsread::BSDataChannel cpu_time("cputime",bsread::BSDATA_DOUBLE);

    chan1.set_data(dbl_test,2048);
    chan1.set_timestamp();


    chan2.set_data(int_test,2048);
    chan2.set_timestamp();
    chan2.m_meta_modulo = 0;

    chan3.set_data(strdup(str_test.c_str()),str_test.size());
    chan3.m_meta_modulo = 10;

    chan4.set_data(strdup(chr_test),strlen(chr_test));

    cpu_time.set_data(new double,1);
    cpu_time.set_timestamp();
    cpu_time.set_callback(test_chan_cb,0);


    bsread::BSDataMessage msg;
    msg.add_channel(&chan1);
    msg.add_channel(&chan2);
//    msg.add_channel(&chan3);
    msg.add_channel(&cpu_time);
    msg.add_channel(&chan4);



//    bsread::BSDataSenderDebug sender(cout);
//    sender.send_message(msg);

    /* Create zmq context */

    bsread::BSDataSenderZmq bsread_zmq("tcp://*:9999",10,ZMQ_PUSH);

    timespec t;
    int i=1;
    while(i++){
        chan1.set_timestamp();
        dbl_test[0]=i/10.0;
        int_test[0]=i;

        msg.set(i,t);

        double t0 = dbltime_get(CLOCK_REALTIME);
        size_t a=bsread_zmq.send_message(msg);
        t0 = dbltime_get() - t0;
        cout << "Send " << a <<" b " << "took " << t0*1e3 << "ms" << endl;
        time_nanosleep(0.01);
    }





    return 0;
}
