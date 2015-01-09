#include "bsread.h"

#include <stdexcept>
#include <iostream>
#include <sstream>

#include <registryFunction.h>
#include <dbStaticLib.h>
#include <dbAccess.h>
#include <epicsString.h>
#include <epicsGuard.h>
#include <errlog.h>
#include <recSup.h>

#include "json.h"  // jsoncpp
#include "bunchData.pb.h" // protocol buffer serialization


using namespace std;
using namespace zmq;


/**
 * Create a zmq context, create and connect a zmq push socket
 * If a zmq context or zmq socket creation failed, an zmq exception will be thrown
 */
BSRead::BSRead(): zmq_context_(new zmq::context_t(1)), zmq_socket_(new socket_t(*_zmqCtx, ZMQ_PUSH)), mutex_(), configuration_()
{
    // TODO Need to be started as server
    //TODO should be part of the configuration
    const char * const address = "tcp://10.5.1.215:9999";
    int high_water_mark = 100;
    zmq_socket_->setsockopt(ZMQ_SNDHWM, &high_water_mark, sizeof(high_water_mark));
    zmq_socket_->connect(address);
}


/**
 * Configure bsread. Accepts a json string. Function will throw runtime error in case json is invalid.
 * Parsed json is divided into per record configuration that is contained in a vector of BSReadChannelConfig.
 * This per-record configurations are later used this->read() method that records appropriate values.
 */
void BSRead::configure(const string & json_string)
{
    Json::Value root;
    Json::Reader reader;

    // Try to parse the incoming json string
    bool parsingSuccessful = reader.parse( json_string, root );
    if ( !parsingSuccessful ){
       string msg = "Could not parse JSON:" + reader.getFormatedErrorMessages();
       errlogPrintf(msg.c_str());
       throw runtime_error(msg);
    }

    const Json::Value channels = root["channels"];
    if (channels.empty()) {
        string msg = "Invalid configuration - missing mandatory channels attribute";
        errlogPrintf(msg.c_str());
        throw runtime_error(msg);
    } else {
        //Parsing was successful so we can drop existing configuration

        epicsGuard < epicsMutex > guard(_mutex); // TODO Wrong place ... - must not affect reading of values !!!!

        configuration_.clear();

        // Parsing success, iterate over per-record configuration
        for (Json::Value::const_iterator it = channels.begin(); it != channels.end(); ++it)  {

            BSDAQRecordConfig config;
            const Json::Value currentRecord = *it;
            config.name = currentRecord["name"].asString(); //TODO: Add grep support.

            if (currentRecord["offset"] != Json::Value::null) {
              int offset = 0;
              if (!(istringstream(currentRecord["offset"].asString()) >> offset)) {
                errlogPrintf("offset is invalid for a record name: %s . Should be an int \n", config.name.c_str());
              }
              else {
                config.offset = offset;
              }
            }

            if (currentRecord["freq"] != Json::Value::null) {
              int freq = 0;
              if (!(istringstream(currentRecord["freq"].asString()) >> freq)) {
                 errlogPrintf("freq is invalid for a record name: %s . Should be an unsigned int \n", config.name.c_str());
              }
              else {
                if (freq > 0) {
                  config.frequency = freq;
                }
                else {
                  errlogPrintf("freq is invalid for a record name: %s . Should be greater then 0 \n", config.name.c_str());
                }
              }
            }

            //Find address
            if(dbNameToAddr(config.name.c_str(),&(config.addr))){
                //Could not find desired record
                errlogPrintf("Record %s does not exist!", config.name.c_str());
                continue;
            }

            configuration_.push_back(config);
            cout << "Added record " << config.name << " offset:" << config.offset << " freq:" << config.frequency << endl;

        }//end of configuration iteration
    }
}


void BSRead::read(long pulse_id)
{
    epicsGuard < epicsMutex > guard(_mutex);

    if (configuration_.empty()) {
      return;
    }

    bsdaqPB::BunchData bunchData;

    bunchData.set_pulse_id(bunchId);

    //Iterate over all records in configuration
    for(vector<BSDAQRecordConfig>::iterator it = configuration_.begin(); it != configuration_.end(); ++it){

        BSDAQRecordConfig *recordConfig = &(*it);

        unsigned freq = recordConfig->frequency;
        int offset = recordConfig->offset;

        //Check if record should be snapshoted.
        if (freq > 0) {
            freq = 100/freq;
            if ( ((bunchId-offset) % freq ) != 0) {
              continue;
            }
        }

        //Preapre protoBuf entry
        bsdaqPB::BunchData_Record* recordData = bunchData.add_record();
        recordData->set_record_name(recordConfig->name);

        //Fill protoBuf
        if(recordConfig->addr.dbr_field_type == DBR_DOUBLE){
            epicsFloat64 val;
            dbGetField(&(recordConfig->addr), DBR_DOUBLE, &val, NULL, NULL, NULL);
            recordData->add_double_val(val);
        }
        else if(recordConfig->addr.dbr_field_type == DBR_STRING){
            char c_val[255];
            dbGetField(&(recordConfig->addr), DBR_STRING, &c_val, NULL, NULL, NULL);
            recordData->add_string_val()->append(c_val);
        }
    }


    // Serialize to protocol buffer
    string serialized_data;
    bunchData.SerializeToString(&serialized_data);

    //Deserialize the data back to human readble format, this is used for diagnostic purposes only
//    google::protobuf::TextFormat::PrintToString(bunchData, &output); //Comment out this line if you would like to have an actual PB on as output

    try {
        size_t bytes_sent =zmq_socket->send(serialized_data.c_str(), serialized_data.size(), ZMQ_NOBLOCK);

        if (bytes_sent == 0) {
            Debug("zmq socket queue full. Message NOT send.\n");
        }
    } catch(zmq::error_t &e ){
        Debug("zmq send failed: %s  \n", e.what());
    }
}


/**
 * Get singleton instance of this class
 */
BSRead BSRead& ::get_instance()
{
    static BSRead instance_;

//    if(!instance_){
//      instance_ = new BSRead();
//    }

    return instance_;
}