#ifndef BSREAD_H
#define BSREAD_H

//std
#include <string>
#include <vector>

//EPICS includes
#include <dbAccess.h>
#include <epicsThread.h>

//External includes
#include "zmq.hpp"
#include "json.h"



extern int bsread_debug;



#ifdef DEBUG
 #ifdef _WIN32
  #define Debug(level,...) if(bsread_debug >= level) printf(__VA_ARGS__); ;
 #else
  #define Debug(level,args...) if(bsread_debug >= level) printf(args); ;
 #endif
#else
 #ifdef _WIN32
  #define Debug(level,...)
 #else
  #define Debug(level,args...)
 #endif
#endif




// Class holding the configuration for one channel
class BSReadChannelConfig {

public:

    BSReadChannelConfig(): modulo(0), offset(0) {}

    std::string channel_name;
    dbAddr address;
    unsigned int modulo;
    int offset;
};


// Singleton object holding the bsread business logic
class BSRead
{

public:

    //Configuration method accepting a json string. Function will throw runtime error in case json is invalid.
    void configure(const std::string & json);

    // Read all currently configured channels and send values out via ZMQ;
    void read(long pulse_id, struct timespec timestamp);

    //This function has to be called from the same thread as read(). It will check wether new configuration is
    //available and apply it. Returns true if new configuration was applied.
    bool applyConfiguration();

    unsigned long numberOfZmqOverflows();
    // Get singleton instance of this class
    static BSRead* get_instance();

private:

    //ZMQ related fields
    zmq::context_t* zmq_context_;
    zmq::socket_t*  zmq_socket_;
    unsigned long zmq_overflows_;   //Number of zmq send errors

    epicsMutex mutex_;          //synchornisation between config/read thread
    Json::FastWriter writer_;   //Json writer instance used for generating data headers
    std::string data_header_;

    std::vector<BSReadChannelConfig> configuration_;
    // Contains next configuration. Incoming configuration is stored
    // here and than copied into configuration_ within BSRead::read method.
    // This prevents blocking of read method.
    std::vector<BSReadChannelConfig> configuration_incoming_;


    BSRead();
    std::string generateDataHeader();
};

#endif // BSREAD_H
