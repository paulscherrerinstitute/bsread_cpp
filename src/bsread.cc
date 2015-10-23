#include "bsread.h"
#include "bsdata.h"

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

#include <zmq.hpp>
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
    zmq_overflows_(0),
    mutex_(),
    applyConfiguration_(false)
{
    const char * const address = "tcp://*:9990";
    int high_water_mark = 1000;

    confiugre_zmq(address,ZMQ_PUSH,high_water_mark);
    applyConfiguration();
}



void BSRead::confiugre_zmq(const char *address, int socket_type, int hwm)
{
    printf("Creating new bsread ZMQ context: %s of type %d HWM set to %d messages\n",address,socket_type,hwm);
    epicsGuard <epicsMutex> guard(mutex_);
    bsread::BSDataSenderZmq* zmq = new bsread::BSDataSenderZmq(address,hwm,socket_type);
    sender_new_ = zmq;
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
                config.type=bsread::BSDATA_DOUBLE;
            }
            else if(config.address.dbr_field_type == DBR_FLOAT){
                config.type=bsread::BSDATA_FLOAT;
            }
            else if(config.address.dbr_field_type == DBR_STRING){
                config.type=bsread::BSDATA_STRING;
            }
            else if(config.address.dbr_field_type == DBR_LONG){
                config.type=bsread::BSDATA_INT;
            }
            else if(config.address.dbr_field_type == DBR_ULONG){
                config.type=bsread::BSDATA_UINT;
            }
            else if(config.address.dbr_field_type == DBR_SHORT){
                config.type=bsread::BSDATA_SHORT;
            }
            else if(config.address.dbr_field_type == DBR_USHORT){
                config.type=bsread::BSDATA_USHORT;
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
    if(message_.get_channels()->empty() || sender_ == NULL){
        return;
    }

    //Set message pulse id and global timestamp
    message_.set(pulse_id,t);

    Debug(1,"Sending id %ld\n",pulse_id);

    //Send message
    if(!sender_->send_message(message_)){
        zmq_overflows_++;
        Debug(2,"BSread not sent :( [%ld]\n",numberOfZmqOverflows());
    }


}

/*
 * Callback function invoked by bsdata used to lock the record and apply the
 * timestamp
 */
void lock_record(bsread::BSDataChannel* chan, bool acquire, void* pvt){
    struct dbCommon* precord = static_cast<dbCommon*>(pvt);

    if(acquire){
        dbScanLock(precord);

        //Set channels timestamp
        struct timespec t;
        epicsTimeToTimespec (&t, &(precord->time)); //Convert to unix time
        chan->set_timestamp(t);
    }
    else{
        dbScanUnlock(precord);
    }
}


bool BSRead::applyConfiguration(){
    bool newConfig = false;

    //Never block!
    if(mutex_.tryLock()){
       if(applyConfiguration_){
           Debug(1,"Apply new configuration\n");

           //Clear exisiting message
           message_.clear_channels();

           //Construct new BSData message
           for(size_t i=0; i< configuration_incoming_.size(); i++){
               BSReadChannelConfig& conf = configuration_incoming_[i];

               //Create a new channel
               bsread::BSDataChannel* chan = new bsread::BSDataChannel(conf.channel_name,conf.type);

               //Fetch data and set data for the channel
               struct dbCommon* precord = conf.address.precord;
               chan->set_data(conf.address.pfield,conf.address.no_elements);

               //Set callback, callback is used to lock and unlock the data
               chan->set_callback(lock_record,precord);

               //Set channel metadata
               chan->m_meta_modulo = conf.modulo;
               chan->m_meta_offset = conf.offset;

               message_.add_channel(chan);

           }

           applyConfiguration_ = false;
           newConfig = true;


       }

       if(sender_new_){
           if(sender_)
            delete sender_;

           sender_=sender_new_;
           sender_new_ = 0;
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


#include <iocsh.h>

static const iocshArg bsreadConfigureArg0 = { "address",iocshArgString};
static const iocshArg bsreadConfigureArg1 = { "type",iocshArgString};
static const iocshArg bsreadConfigureArg2 = { "hwm",iocshArgInt};

static const iocshArg *const bsreadConfigureArgs[3] =
    {&bsreadConfigureArg0,&bsreadConfigureArg1,&bsreadConfigureArg2};

static const iocshFuncDef bsreadConfigureFuncDef =
    {"bsreadConfigure",3,bsreadConfigureArgs};

static void bsreadConfigFunc(const iocshArgBuf *args)
{

    if(!args[0].sval | !args[1].sval){
        printf("not enough arguments\n");
        return;
    }

    int socket_type=-1;
    if(!strcmp(args[1].sval,"PUSH")) socket_type = ZMQ_PUSH;
    if(!strcmp(args[1].sval,"PUB")) socket_type = ZMQ_PUB;

    if(socket_type == -1){
        errlogPrintf("Invalid type arguemnt, avialbale socket types: PUSH, PUB\n");
        return;
    }

    BSRead* bsread = BSRead::get_instance();

    try{
        bsread->confiugre_zmq(args[0].sval,socket_type,args[2].ival);
    }
    catch(zmq::error_t& e){
        errlogPrintf("Could not configure the port\nRuntime excetion occured: %s\n",e.what());
    }

}

static int doRegister(void)
{
    iocshRegister(&bsreadConfigureFuncDef,bsreadConfigFunc);
    return 1;
}
static int done = doRegister();

