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
#include "utils.h"
#include "sconf.h"



using namespace std;


//void test_chan_cb(bsread::BSDataChannel* chan,bool acquire, void* pvt){
//    if(acquire){
//        //Update data
//        double* data = static_cast<double*>(chan->get_data());
//        data[0]=dbltime_get(CLOCK_PROCESS_CPUTIME_ID);

////        chan->set_timestamp();
//    }
//}


void message_send_perf(bsread::BSDataSenderZmq& sender, bsread::BSDataMessage& msg){
    double t_set,t_send;
    dbl_stats t_set_stat,t_send_stat,t_all_stat;
    dbl_stats_init(t_set_stat,20);
    dbl_stats_init(t_send_stat,20);
    dbl_stats_init(t_all_stat,300);

    long long pulse_id = 0;
    bsread::timestamp t;
    struct timespec global_timestamp;
    size_t no_of_channels = msg.get_channels()->size();
    size_t payload_size = msg.get_datasize();

    while(true){
        pulse_id++;
        clock_gettime(CLOCK_REALTIME,&global_timestamp);
        t.sec = (int64_t)global_timestamp.tv_sec;
        t.nsec = (int64_t)global_timestamp.tv_nsec;



        t_set = dbltime_get();
        msg.set(pulse_id,t);
        t_set = (dbltime_get() - t_set)*1e6;

        t_send = dbltime_get();
        size_t size = sender.send_message(msg);
        t_send = (dbltime_get() - t_send)*1e6;


        dbl_stats_add(t_set_stat,t_set);
        dbl_stats_add(t_send_stat,t_send);
        dbl_stats_add(t_all_stat,t_send+t_set);


        if(!(pulse_id%100)){
            dbl_stats_print("set[us]",t_set_stat);
            dbl_stats_print("send[us]",t_send_stat);
            dbl_stats_print("all[us]",t_all_stat);
            printf("\nsize: %fkb\n",size/1024.0);
            printf("payload: %fkb\n",payload_size/1024.0);
            printf("channels: %d time per channel[us]: %f\n",no_of_channels,t_all_stat.avg/no_of_channels);
            printf("\n\n");

            //Reset stats
            dbl_stats_init(t_set_stat,20);
            dbl_stats_init(t_send_stat,20);
        }


        time_nanosleep(0.01);
    }



}


bsread::BSDataMessage example_perf_scalar(size_t no_of){

    //Message
    bsread::BSDataMessage msg;

    //Create scalars...
    for(int i=0; i < no_of; i++){
        double* val = new double;
        char name[255];
        snprintf(name,255,"TEST%d",i);
        bsread::BSDataChannel* chan = new bsread::BSDataChannel(name,bsread::BSDATA_FLOAT64);
        chan->set_data(val,1);
        chan->m_meta_modulo = 1;
        chan->m_meta_offset = 0;
        msg.add_channel(chan);
    }

    cout << "Created " << no_of << " channels!" << endl;


    return msg;
}


bsread::BSDataMessage example_perf_buff(size_t len, int port=9999){


    //Message
    bsread::BSDataMessage msg;

    double* buffer = new double[len];
    bsread::BSDataChannel* chan = new bsread::BSDataChannel("TEST_BUF",bsread::BSDATA_FLOAT64);
    chan->set_data(buffer,len);
    chan->m_meta_modulo=1;

    msg.add_channel(chan);

    return msg;
}



int main(int argc, char *argv[])
{
    int port = 9999;
    int mode = 1;
    int no_of = 5000;
    int scalars = 1;
    int hwm;

    conf_item_t* config = \
    conf_create_int_item(0,     &port,      "port",     9999,   "ZMQ PORT to use");
    conf_create_int_item(config,&mode,      "mode",     0,      "zmq sender to test, 0: normal, 1: oneshot, 2: nan");
    conf_create_int_item(config,&no_of,     "size",     1000,   "no of items in message");
    conf_create_int_item(config,&scalars,   "scalar",   1,      "1: use scalar message, 0: use array message");
    conf_create_int_item(config,&hwm,       "hwm",      1024,    "zmq HWM");

    conf_load_cmdline(argc,argv,config);

    cout << "Current config!" << endl;
    conf_dump(config);
    conf_savefs("test.conf",config);
    cout << endl << endl;


    //Argument parser



    //Create context
    zmq::context_t ctx(1);
    //We now need to create a bsread sender that will serialize and send out the message
    char address[255];
    snprintf(address,255,"tcp://*:%d",port);
    bsread::BSDataSenderZmq* zmq_sender;


    switch (mode) {
    case 0:
        cout << "Creating normal BSREAD sender" << endl;
        zmq_sender = new bsread::BSDataSenderZmq(ctx,address,hwm);
        break;
    case 1:
        cout << "Creating onpart BSREAD sender" << endl;
        zmq_sender = new bsread::BSDataSenderZmqOnepart(ctx,address,hwm);
        break;
    default:
        break;
    }


    bsread::BSDataMessage msg;

    switch (scalars) {
    case 0:
        cout << "Creating message with arrray with " << no_of << " doubles" << endl;
        msg = example_perf_buff(no_of);
        cout << "Payload size: " << msg.get_datasize() << endl;

        break;
    case 1:
        cout << "Creating message with " << no_of << " scalars" << endl;
        msg = example_perf_scalar(no_of);
        cout << "Payload size: " << msg.get_datasize() << endl;
        break;
    default:
        break;
    }

    message_send_perf(*zmq_sender,msg);

    return 0;
}
