#include <iostream>
#include <sstream>     // cout, ios
#include <string>
#include <chrono>
#include <thread>

#include "unistd.h"

#include <zmq.h>
#include "../bsread.h"

#include "utils.h"

using namespace std;
#define debug(M, ...) fprintf(stderr, "DEBUG[%s:%d]: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define info(M, ...) fprintf(stderr, "INFO: " M "\n", ##__VA_ARGS__)



// Configuration variables
struct{
    char* addr = "tcp://*:9090";
    int sock_type = ZMQ_PUSH;
    int hwm = 1024;
    int linger = 10;
    char* rpc_addr = "tcp://*:9091";

    int num_chan = 100;         //number of channels to simulate
    int num_elem = 1024;   //number of elements (type double) per channel

    int num_runs = 100; //number of pulses to simulate before reconstructing the bsread instance

    bool single_run = true; //True if only a single iteration is needed
    bool auto_enable = false; //True for channel auto enable

}conf;


const char* usage = "bsread test application\n"
                    "optional flags:\n"
                    "\n"
                    "   -addr [tcp://*:9090]\n"
                    "   -rpc_addr [tcp://*:9091]\n"
                    "   -num_chan [100]   num of channels\n"
                    "   -num_elem [1024]  num of elements per channels [type: double]\n"
                    "   -num_runs [100]   stop sending and destroy bsread after num_runs\n"
                    "   -single           if set the bsread instance is not recreated after num_runs\n"
                    "   -auto_enable      enable all channels on start\n";


void parse_args(int argc, char *argv[]){
    ArgsParser parser(argc,argv);

    if( parser.cmdOptionExists("-addr") ){
        conf.addr = strdup(parser.getCmdOption("-addr").c_str());
    }

    if( parser.cmdOptionExists("-rpc_addr") ){
        conf.rpc_addr = strdup(parser.getCmdOption("-rpc_addr").c_str());
    }

    if( parser.cmdOptionExists("-num_chan") ){
        conf.num_chan = std::stoi(parser.getCmdOption("-num_chan"));
    }

    if( parser.cmdOptionExists("-num_elem") ){
        conf.num_elem = std::stoi(parser.getCmdOption("-num_elem"));
    }

    if( parser.cmdOptionExists("-num_runs") ){
        conf.num_runs = std::stoi(parser.getCmdOption("-num_runs"));
    }


    if( parser.cmdOptionExists("-single") ) conf.single_run = true;
    else conf.single_run = false;

    if( parser.cmdOptionExists("-auto_enable") ) conf.auto_enable = true;
    else conf.auto_enable = false;

}

// Globals
vector<tuple<double*,int>> buffers;

bsread::BSRead* bsread_instance;

void configure_bsread(){
    info("Creating bsread instance");
    bsread_instance = new bsread::BSRead();

    info("Configuring zmq data %s",conf.addr);
    bsread_instance->confiugre_zmq(conf.addr,conf.sock_type,conf.hwm,conf.linger);

    info("Configuring zmq rpc %s", conf.rpc_addr);
    bsread_instance->confiugre_zmq_config(conf.rpc_addr);

    info("Adding channels %d",buffers.size());
    for(int i=0;i<buffers.size();i++){
        bsread::BSDataChannel* chan = new bsread::BSDataChannel("test"+i,bsread::BSDATA_FLOAT64);
        chan->set_data(std::get<0>(buffers[i]),std::get<1>(buffers[i]));
        bsread_instance->add_channel(chan);
    }

    if(conf.auto_enable){
        info("Enabling all channels since auto_enable is set");
        bsread_instance->enable_all_channels();
    }

}

void run_bsread(){
    uint64_t pulse_id = 0;
    bsread::timestamp global_timestamp;
    int number_of_runs_ = conf.num_runs;

    info("Starting run %d",conf.num_runs);
    std::chrono::time_point<std::chrono::steady_clock> t_start,t_end;
    std::chrono::duration<double> t_send_duration;

    while (number_of_runs_!= 0) {
        t_start = std::chrono::steady_clock::now();

        bsread_instance->send(pulse_id,global_timestamp);

        t_end = std::chrono::steady_clock::now();
        t_send_duration = t_end - t_start;

        info("Send took %f ms, zmq overflows %d",t_send_duration*1000,bsread_instance->zmq_overflows());
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        number_of_runs_ --;
    }

}

void cleanup_bsread(){
    info("Cleaning up!");
    delete bsread_instance;
}

int main(int argc, char *argv[])
{
    info("%s",usage);

    parse_args(argc,argv);

    debug("Creating [%d] buffers",conf.num_chan);
    for(int i=0;i<conf.num_chan;i++){
        double* buff = new double[conf.num_elem];
        buffers.push_back(std::make_tuple(buff,conf.num_elem));
    }


    while(true){
        configure_bsread();
        run_bsread();
        cleanup_bsread();

        //exit if sinlge run is enabled
        if(conf.single_run) break;
    }


    for(int i=0;i < buffers.size(); i++){
        delete std::get<0>(buffers[i]);
    }

    return 0;
}

