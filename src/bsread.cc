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
#include <epicsTime.h>
#include <epicsTypes.h>
#include <epicsEndian.h>


#include "md5.h"
#include "json.h"  // jsoncpp
// #include "bunchData.pb.h" // protocol buffer serialization


using namespace std;
using namespace zmq;


/**
 * Create a zmq context, create and connect a zmq push socket
 * If a zmq context or zmq socket creation failed, an zmq exception will be thrown
 */
BSRead::BSRead():
    zmq_context_(new zmq::context_t(1)),
    zmq_socket_(new socket_t(*zmq_context_, ZMQ_PUSH)),
    zmq_overflows_(0),
    mutex_(),
    applyConfiguration_(false),
    configuration_()
{
    const char * const address = "tcp://*:9999";
    int high_water_mark = 1000;
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
       errlogPrintf(msg.c_str());
       throw runtime_error(msg);
    }

    const Json::Value channels = root["channels"];
    if (channels.isNull()) {    // Only throw error if attribute 'channels' is not found in the JSON root. It might be empty, which is OK.
        string msg = "Invalid configuration - missing mandatory channels attribute. ";
        errlogPrintf(msg.c_str());
        throw runtime_error(msg);
    } else {
        //Parsing was successful so we can drop existing configuration

        //This mutex is only protecting configuration_incoming, not actuall configuration_ 
        //configuration_incoming_ is only a placeholder for configuration that is waiting to be applied
        //Thread that samples the data will have to acquire this mutex before accessing configuration_incoming_ 
        //In order to avoid locking the sampling thread a non-blocking try_lock is used.
        epicsGuard < epicsMutex > guard(mutex_);

        configuration_incoming_.clear();

        // Parsing success, iterate over per-record configuration
        for (Json::Value::const_iterator iterator = channels.begin(); iterator != channels.end(); ++iterator)  {

            BSReadChannelConfig config;
            const Json::Value current_channel = *iterator;
            config.channel_name = current_channel["name"].asString(); //TODO: Add grep support.

            if (current_channel["offset"] != Json::Value::null) {
                int offset = 0;
                if (!(istringstream(current_channel["offset"].asString()) >> offset)) {
                    errlogPrintf("Invalid offset for channel: %s\n", config.channel_name.c_str());
                }
                else {
                    config.offset = offset;
                }
            }

            if (current_channel["modulo"] != Json::Value::null) {
                int modulo = 0;
                if (!(istringstream(current_channel["modulo"].asString()) >> modulo)) {
                    errlogPrintf("Invalid modulo for channel: %s\n", config.channel_name.c_str());
                }
                else {
                    if (modulo > 0) {
                        config.modulo = modulo;
                    }
                    else {
                        errlogPrintf("Invalid modulo for channel: %s . [modulo<=0] \n", config.channel_name.c_str()); // TODO Need to throw exception
                    }
                }
            }

            //Find address
            if(dbNameToAddr(config.channel_name.c_str(), &(config.address))) {
                //Could not find desired record
                errlogPrintf("Channel %s does not exist!", config.channel_name.c_str()); // TODO Need to throw exception
                continue;
            }

            // determine if the DBR type is supported
            if(config.address.dbr_field_type == DBR_DOUBLE){
                config.type="Double";
            }
            else if(config.address.dbr_field_type == DBR_FLOAT){
                config.type="Float";
            }
            else if(config.address.dbr_field_type == DBR_STRING){
                config.type="String";
            }
            else if(config.address.dbr_field_type == DBR_LONG){
                config.type="Long";
            }
            else if(config.address.dbr_field_type == DBR_ULONG){
                config.type="ULong";
            }
            else if(config.address.dbr_field_type == DBR_SHORT){
                config.type="Short";
            }
            else if(config.address.dbr_field_type == DBR_USHORT){
                config.type="UShort";
            }
            else{
                errlogPrintf("BSREAD: Channel %s has unsuporrted type: %d\n",config.channel_name.c_str(), config.address.dbr_field_type); // TODO Need to throw exception
                continue;
            }

            configuration_incoming_.push_back(config);
            
            Debug(1,"Added channel %s offset: %d  modulo: %d\n", config.channel_name.c_str(), config.offset, config.modulo);
        }

        applyConfiguration_ = true;
    }
}


void BSRead::read(long pulse_id, struct timespec t)
{
    //Skip read if configuration is not available yet...
    if (configuration_.empty()) {
      return;
    }

//    bsdaqPB::BunchData pb_data_message;

    // Set global values
//    pb_data_message.set_pulse_id(pulse_id);

    try {
        // Construct main header
        // std::ostringstream main_header;
        Json::Value main_header,main_header_global_timestamp;
        main_header["htype"] = "bsr_m-1.0";
        main_header["pulse_id"] = static_cast<Json::UInt64>(pulse_id);
        main_header["hash"] = md5(data_header_);

        main_header_global_timestamp["epoch"]=static_cast<Json::UInt64>(t.tv_sec + 631152000); //Offset epics EPOCH into POSIX epich 
        main_header_global_timestamp["ns"]=static_cast<Json::UInt64>(t.tv_nsec);

        main_header["global_timestamp"]=main_header_global_timestamp;

        // Check https://bobobobo.wordpress.com/2010/10/17/md5-c-implementation/ for MD5 Hash ...

        // Check wheter there is any channel that needs to be read out for this pulse
        vector<BSReadChannelConfig>::iterator iterator;
        BSReadChannelConfig *channel_config;
        unsigned modulo;
        int offset;

        for( iterator = configuration_.begin(); iterator != configuration_.end(); ++iterator){
            channel_config = &(*iterator);
            modulo = channel_config->modulo;
            offset = channel_config->offset;

            if (modulo == 0) break;
            if (modulo > 0) {
                if ( ((pulse_id+offset) % modulo ) != 0) {
                    continue;   // channel won't be send in this pulse, moving on...
                }
                else {
                    break;      // we found a channel to send!
                }
            }
        }

        if (iterator != configuration_.end()) { // there are messages to send, so let's send them
            // Send main header
            string main_header_serialized = writer_.write(main_header);
            size_t bytes_sent =zmq_socket_->send(main_header_serialized.c_str(), main_header_serialized.size(), ZMQ_NOBLOCK|ZMQ_SNDMORE);
            if (bytes_sent == 0) {
                zmq_overflows_++;
                Debug(2,"ZMQ message [main header] NOT send. [%ld]\n",zmq_overflows_);
            }

            // Send data header
            bytes_sent =zmq_socket_->send(data_header_.c_str(), data_header_.size(), ZMQ_NOBLOCK|ZMQ_SNDMORE);
            if (bytes_sent == 0) {
                zmq_overflows_++;
                Debug(2,"ZMQ message [data header] NOT send. [%ld]\n",zmq_overflows_);
            }

            // Read channels and send sub-messages
            for(iterator = configuration_.begin(); iterator != configuration_.end(); ++iterator){

                channel_config = &(*iterator);

                //Last element?
                int zmq_flags = ZMQ_NOBLOCK | ZMQ_SNDMORE;
                if( iterator == (--configuration_.end())){
                    zmq_flags = ZMQ_NOBLOCK;
                }

                // Check whether channel needs to be read out for this pulse
                modulo = channel_config->modulo;
                offset = channel_config->offset;

                if (modulo > 0) {
                    if ( ((pulse_id+offset) % modulo ) != 0) {
                      bytes_sent = zmq_socket_->send(0, 0, zmq_flags);
                      if (bytes_sent == 0) {
                        zmq_overflows_++;
                        Debug(2,"ZMQ message [NODAT] NOT send.[%ld]\n",zmq_overflows_);
                      }
                      continue;
                    }
                }

                struct dbCommon* precord = channel_config->address.precord;
                //Lock the record beeing read
                dbScanLock(precord);
                //All values are treated in the same way, data header contains information about
                // their types, packing and endianess

                void* val = channel_config->address.pfield; //Data pointer
                long no_elements = channel_config->address.no_elements;
                long element_size = channel_config->address.field_size;

                bytes_sent = zmq_socket_->send(val, element_size*no_elements, ZMQ_NOBLOCK|ZMQ_SNDMORE);
                if (bytes_sent == 0) {
                        zmq_overflows_++;
                        Debug(2,"ZMQ message [data] NOT send. [%ld]\n",zmq_overflows_);
                }

                //Add timestamp binary blob
                //Current timestamp is packed into a two 64bit unsigned integers where:
                //[0] seconds past POSIX epoch (00:00 1.1.1970)
                //[1] nanoseconds since last full second.
                struct timespec t;
                epicsTimeToTimespec (&t, &(precord->time)); //Convert to unix time

                uint64_t rtimestamp[2];
                rtimestamp[0] = t.tv_sec;
                rtimestamp[1] = t.tv_nsec;

                bytes_sent = zmq_socket_->send(rtimestamp, sizeof(rtimestamp), zmq_flags);
                if (bytes_sent == 0) {
                        zmq_overflows_++;
                        Debug(2,"ZMQ message [timestamp] NOT send. [%ld]\n",zmq_overflows_);
                }

                dbScanUnlock(precord);
            }
        }

    } catch(zmq::error_t &e ){
        Debug(2,"ZMQ send failed: %s  \n", e.what());
    }

}

std::string BSRead::generateDataHeader(){
    Json::Value root,channels,channel;
    root["htype"] = "bsr_d-1.0";
    root["encoding"] = (EPICS_BYTE_ORDER == EPICS_ENDIAN_BIG) ? "big" : "little";


    //Iterate over channels and create data header channel entires
    for(vector<BSReadChannelConfig>::iterator iterator = configuration_.begin(); iterator != configuration_.end(); ++iterator){
        BSReadChannelConfig *channel_config = &(*iterator);

        channel["name"]=channel_config->channel_name;
        channel["type"]=channel_config->type;
        channel["offset"]=channel_config->offset;
        channel["modulo"]=channel_config->modulo;

        channels.append(channel);
    }

    root["channels"] = channels;

    //Serialize to string
    // Debug(2,"%s",writer_.write(root).c_str());
    return writer_.write(root);

}

bool BSRead::applyConfiguration(){
    bool newConfig = false;
    
    //Never block! 
    if(mutex_.tryLock()){
       if(applyConfiguration_){
           Debug(1,"Apply new configuration\n");
           
           // Todo Could be more efficient
           // could be, should be? Regardless of implementation we will always need
           // to iterate over vector at least once. Since configuarition is small 
           // it will be difficult to be faster than a direct copy.
           configuration_ = configuration_incoming_;
           applyConfiguration_ = false;
           newConfig = true;

           // New configuration! Data needs to be updated...
           data_header_ = generateDataHeader();
       }
       mutex_.unlock();
    }

    return newConfig;
}


unsigned long BSRead::numberOfZmqOverflows(){
    return zmq_overflows_;
}

/**
 * Get singleton instance of this class
 */
BSRead* BSRead::get_instance()
{
    static BSRead instance_;
    return &instance_;
}

int bsread_debug=0;

#include <epicsExport.h>
extern "C"{
    epicsExportAddress(int,bsread_debug);
}
