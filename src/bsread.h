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


#ifdef DEBUG
#define Debug(args...) printf(args)
#else
#define Debug(args...)
#endif

// Class holding the configuration for one channel
class BSReadChannelConfig {

public:

    BSReadChannelConfig(): frequency(0), offset(0) {}

    std::string channel_name;
    dbAddr address;
    unsigned int frequency;
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
    // Get singleton instance of this class
    static BSRead* get_instance();

private:

    BSRead();

    zmq::context_t* zmq_context_;
    zmq::socket_t*  zmq_socket_;
    epicsMutex mutex_;
    Json::FastWriter writer_;   //Json writer instance used for generating data headers
    std::string data_header_;
    std::vector<BSReadChannelConfig> configuration_;
    // Contains next configuration. Incoming configuration is stored
    // here and than copied into configuration_ within BSRead::read method.
    // This prevents blocking of read method.
    std::vector<BSReadChannelConfig> configuration_incoming_;

    std::string generateDataHeader();
};

#endif // BSREAD_H
