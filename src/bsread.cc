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


#include "md5.h"
#include "json.h"  // jsoncpp
// #include "bunchData.pb.h" // protocol buffer serialization


using namespace std;
using namespace zmq;


/**
 * Create a zmq context, create and connect a zmq push socket
 * If a zmq context or zmq socket creation failed, an zmq exception will be thrown
 */
BSRead::BSRead(): zmq_context_(new zmq::context_t(1)), zmq_socket_(new socket_t(*zmq_context_, ZMQ_PUSH)), mutex_(), configuration_()
{
    const char * const address = "tcp://*:9999";
    int high_water_mark = 100;
    zmq_socket_->setsockopt(ZMQ_SNDHWM, &high_water_mark, sizeof(high_water_mark));
    zmq_socket_->bind(address);
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
       Debug(msg.c_str());
       throw runtime_error(msg);
    }

    const Json::Value channels = root["channels"];
    if (channels.empty()) {
        string msg = "Invalid configuration - missing mandatory channels attribute";
        Debug(msg.c_str());
        throw runtime_error(msg);
    } else {
        //Parsing was successful so we can drop existing configuration

        epicsGuard < epicsMutex > guard(mutex_); // TODO Wrong place ... - must not affect reading of values !!!!

        configuration_.clear();
        configuration_incoming_.clear();
        std::ostringstream data_header_stream;
        data_header_stream << "{  \"htype\":\"bsr_d-1.0\" \"channels\":[";

        // Parsing success, iterate over per-record configuration
        for (Json::Value::const_iterator iterator = channels.begin(); iterator != channels.end(); ++iterator)  {

            BSReadChannelConfig config;
            const Json::Value current_channel = *iterator;
            config.channel_name = current_channel["name"].asString(); //TODO: Add grep support.

            if (current_channel["offset"] != Json::Value::null) {
                int offset = 0;
                if (!(istringstream(current_channel["offset"].asString()) >> offset)) {
                    Debug("Invalid offset for channel: %s\n", config.channel_name.c_str());
                }
                else {
                    config.offset = offset;
                }
            }

            if (current_channel["frequency"] != Json::Value::null) {
                int frequency = 0;
                if (!(istringstream(current_channel["frequency"].asString()) >> frequency)) {
                    Debug("Invalid frequency for channel: %s\n", config.channel_name.c_str());
                }
                else {
                    if (frequency > 0) {
                        config.frequency = frequency;
                    }
                    else {
                        Debug("Invalid frequency for channel: %s . [frequency<=0] \n", config.channel_name.c_str()); // TODO Need to throw exception
                    }
                }
            }

            //Find address
            if(dbNameToAddr(config.channel_name.c_str(), &(config.address))) {
                //Could not find desired record
                Debug("Channel %s does not exist!", config.channel_name.c_str()); // TODO Need to throw exception
                continue;
            }

            configuration_incoming_.push_back(config);
            Debug("Added channel %s offset: %d  frequency: %d\n", config.channel_name.c_str(), config.offset, config.frequency);

            data_header_stream << "{ \"name\":\"" << config.channel_name << "\", \"type\":\"";
            if(config.address.dbr_field_type == DBR_DOUBLE){
                data_header_stream << "Double";
            }
            else if(config.address.dbr_field_type == DBR_STRING){
                data_header_stream << "String";
            }
            data_header_stream << "\"}";

            data_header_stream << "]}";
            data_header_ = data_header_stream.str();
        }
    }
}


void BSRead::read(long pulse_id)
{
    epicsGuard < epicsMutex > guard(mutex_); // TODO Need to be revised

    if (configuration_.empty()) {
      return;
    }

//    bsdaqPB::BunchData pb_data_message;

    // Set global values
//    pb_data_message.set_pulse_id(pulse_id);

    try {
        // Construct main header
        std::ostringstream main_header;
        main_header << "{ \"htype\":\"bsr_m-1.0\" \"pulse-id\":" << pulse_id << " \"hash\":\"" << md5(data_header_) << "\"" << " }";

        // Check https://bobobobo.wordpress.com/2010/10/17/md5-c-implementation/ for MD5 Hash ...


        // Send main header
        string main_header_serialized = main_header.str();
        const char* pointer_main_header_serialized = main_header_serialized.c_str();
        size_t bytes_sent =zmq_socket_->send(pointer_main_header_serialized, main_header_serialized.size(), ZMQ_NOBLOCK)|ZMQ_SNDMORE;
        if (bytes_sent == 0) {
            Debug("ZMQ message [main header] NOT send.\n");
        }

        // Send data header
        bytes_sent =zmq_socket_->send(data_header_.c_str(), data_header_.size(), ZMQ_NOBLOCK);
        if (bytes_sent == 0) {
            Debug("ZMQ message [data header] NOT send.\n");
        }

        // Read channels and send sub-messages
        for(vector<BSReadChannelConfig>::iterator iterator = configuration_.begin(); iterator != configuration_.end(); ++iterator){

            BSReadChannelConfig *channel_config = &(*iterator);

            // Check whether channel needs to be read out for this pulse
            unsigned frequency = channel_config->frequency;
            int offset = channel_config->offset;

            if (frequency > 0) {
                frequency = 100/frequency;
                if ( ((pulse_id-offset) % frequency ) != 0) {
                  continue;
                }
            }

            // Read channel value
    //        bsdaqPB::BunchData_Record* channel_data = pb_data_message.add_record();
    //        channel_data->set_record_name(channel_config->channel_name);

            if(channel_config->address.dbr_field_type == DBR_DOUBLE){
                epicsFloat64 val;
                dbGetField(&(channel_config->address), DBR_DOUBLE, &val, NULL, NULL, NULL);
                printf("%f\n",val);
    //            channel_data->add_double_val(val);

                // Todo: Enable Waveforms
                bytes_sent =zmq_socket_->send(&val, sizeof(epicsFloat64), ZMQ_NOBLOCK);
                if (bytes_sent == 0) {
                    Debug("ZMQ message [data header] NOT send.\n");
                }
            }
            else if(channel_config->address.dbr_field_type == DBR_STRING){
                char c_val[255];
                dbGetField(&(channel_config->address), DBR_STRING, &c_val, NULL, NULL, NULL);
    //            channel_data->add_string_val()->append(c_val);
                printf("%s\n",c_val);
                // Todo: Implement String + Waveform sending
            }
        }

    } catch(zmq::error_t &e ){
        Debug("ZMQ send failed: %s  \n", e.what());
    }

    // Apply new configuration if available. This is done at the end of the
    // Read routine as
    // Todo Actually this belongs outside timing
    if(configuration_incoming_.size()){
        Debug("Apply new configuration\n");
        // Todo Could be more efficient
        configuration_ = configuration_incoming_;
        configuration_incoming_.clear();
    }

}


/**
 * Get singleton instance of this class
 */
BSRead* BSRead::get_instance()
{
    static BSRead instance_;
    return &instance_;
}
