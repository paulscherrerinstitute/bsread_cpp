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
#include <epicsExport.h>

#include "bsread.h"

#include <zmq.hpp>
#include "md5.h"
#include "json.h"  // jsoncpp


using namespace std;
using namespace zmq;

int bsread_debug;

namespace bsread{

/**
 * Create a zmq context, create and connect a zmq push socket
 * If a zmq context or zmq socket creation failed, an zmq exception will be thrown
 */
BSRead::BSRead():
    m_message(0),
    m_message_new(0),
    m_sender(0),
    m_sender_new(0),    
    m_inhibit(false),
    m_zmq_ctx(1),
    zmq_overflows_(0)
{    
}



void BSRead::confiugre_zmq(const char *address, int socket_type, int hwm, int linger)
{

    bsread_debug(1,"Creating new bsread ZMQ context for BSREAD: %s of type %d HWM set to %d messages and linger to %d ms",address,socket_type,hwm,linger);
    epicsGuard <epicsMutex> guard(m_mutex_config);
    bsread::BSDataSenderZmq* zmq = new bsread::BSDataSenderZmq(m_zmq_ctx,address,hwm,socket_type,linger);

    if(!m_sender) m_sender = zmq;

    else m_sender_new = zmq;


}


void BSRead::confiugre_zmq_config(const char *address)
{

    if(m_thread_rpc){
        bsread_debug(1,"ZMQ RPC already running! Will not reconfigure...");
    }

    bsread_debug(1,"starting bsread ZMQ RPC thread on %s",address);
    //Configuration socket
    m_zmq_sock_config = new socket_t(m_zmq_ctx,ZMQ_REP);
    m_zmq_sock_config->bind(address);

   //Start thread    
    m_thread_rpc = epicsThreadCreate("bsread_config_thread",epicsThreadPriorityLow,epicsThreadGetStackSize(epicsThreadStackMedium),&BSRead::zmq_config_thread,this);

}


/**
 * Configure bsread. Accepts a json string. Function will throw runtime error in case json is invalid.
 * Parsed json is divided into per record configuration that is contained in a vector of BSReadChannelConfig.
 * This per-record configurations are later used this->read() method that records appropriate values.
 */
void BSRead::configure(const string & json_string)
{
    Json::Reader json_reader;
    Json::Value config;

    //Try to decode the message
    if(!json_reader.parse(json_string,config)){
        throw std::runtime_error("Could not parse JSON: " + json_reader.getFormatedErrorMessages());
    }

    configure(config);

}

void BSRead::set_inhibit(bool inhibit){
    this->m_inhibit = inhibit;
    bsread_debug(2,"ZMQ RPC: setting inhibit bit to: %d",this->m_inhibit);
}

bool BSRead::get_inhibit(){
    return this->m_inhibit;
}

void BSRead::configure(Json::Value config)
{
    epicsGuard<epicsMutex> guard(m_mutex_config);

    if(m_message_new) delete m_message_new;
    m_message_new = new BSDataMessage(parse_json_config(m_channels,config));
}


void BSRead::send(uint64_t pulse_id, bsread::timestamp t)
{
    //Startup condition, guard will block here...
    if(!m_sender){
        epicsGuard<epicsMutex> guard(m_mutex_config);

        if(!m_sender_new)
            throw std::runtime_error("ZMQ socket not configured!");
        else m_sender = m_sender_new;

        m_sender_new = 0;
    }

    //Try to apply new configuration
    if(m_mutex_config.tryLock()){

        if(m_message_new){
           bsread_debug(2,"Applying new configuration");

           //Clear exisiting message
           delete m_message;
           m_message = m_message_new;
           m_message_new = 0;
       }

       if(m_sender_new){
           if(m_sender)
            delete m_sender;

           bsread_debug(2,"Applying new ZMQ socket");

           m_sender=m_sender_new;
           m_sender_new = 0;
       }

       m_mutex_config.unlock();
    }

    //Skip read if configuration is not available yet...
    if(!m_message ||
        m_message->get_channels()->empty() ||
        this->m_inhibit){

        bsread_debug(5,"Skipping message");
        return;
    }


    //Set message pulse id and global timestamp
    bool empty_message = !m_message->set(pulse_id,t);

    if(empty_message){
        bsread_debug(5,"Skipping message since no channels are enabled\n");
        return;
    }

    bsread_debug(5,"Sending id %lld",(long long)pulse_id);

    //Send message
    if(!m_sender->send_message(*m_message)){
        zmq_overflows_++;
        bsread_debug(5,"not sent :( [%d]",zmq_overflows_);
    }


}

BSDataMessage BSRead::parse_json_config(const vector<BSDataChannel *> &all_channels, Json::Value root){
    BSDataMessage outMsg;

    if(root["grep"]!=Json::nullValue){
        bsread_debug(4,"Enabling all channels [%lld]",(long long)all_channels.size());
        for(size_t i=0;i<all_channels.size();i++){
            outMsg.add_channel(all_channels[i]);
        }
        bsread_debug(4,"Done, returning...");
        return outMsg;
    }

    const Json::Value channels = root["channels"];
    if (channels.isNull()) {    // Only throw error if attribute 'channels' is not found in the JSON root. It might be empty, which is OK.
        throw runtime_error("Invalid configuration - missing mandatory channels attribute.");
    }


    // Parsing success, iterate over per-record configuration
    for (Json::Value::const_iterator iterator = channels.begin(); iterator != channels.end(); ++iterator)  {

        const Json::Value current_channel = *iterator;
        string name = current_channel["name"].asString(); //TODO: Add grep support.

        //Check if the channel exists
        BSDataChannel* chan = 0;

        for(size_t i=0;i<all_channels.size();i++){
            if(all_channels[i]->get_name() == name){
                chan=all_channels[i];
                break;
            }
        }

        if(!chan) throw runtime_error("Channel "+name+" does not exist!");

        int offset=0;   //Default offset
        int modulo=1;   //Default modulo

        if (current_channel["offset"] != Json::Value::null) {
            offset = current_channel["offset"].asInt();
        }

        if (current_channel["modulo"] != Json::Value::null) {
            modulo = current_channel["modulo"].asInt();

            if(modulo < 1){
                throw runtime_error("Invalid modulo specfied for channel: "+name);
            }
        }


        chan->m_meta_modulo = modulo;
        chan->m_meta_offset = offset;
        outMsg.add_channel(chan);
    }

    return outMsg;
}

//Generates current json configuration...
Json::Value BSRead::generate_json_config(){

    Json::Value root;
    Json::Value channels;

    //No configuration has been set yet
    if(!m_message){
        root["channels"] = Json::nullValue;
        return root;
    }

    for(size_t i=0;i<m_message->get_channels()->size();i++){
        BSDataChannel* chan =m_message->get_channels()->at(i);
        channels.append(chan->get_data_header(true));

    }

    root["channels"] = channels;
    return root;
}



BSRead::~BSRead(){
    //Senders need to be deleted first to release ZMQ sockets
    if(m_sender) delete m_sender;
    if(m_sender_new) delete m_sender_new;

    //close context, this will cause it to unblock and throw exception
    //this exception is later used to cleanly terminate the configuration thread...
    m_zmq_ctx.close();


    //Wait until RPC thread terminated
    m_thread_rpc_running.lock();
    m_thread_rpc_running.unlock();

    if(m_message_new) delete m_message_new;
    if(m_message) delete m_message;

    for(size_t i=0;i<m_channels.size();i++){
        delete m_channels[i];
    }

}

/**
 * @brief BSRead::zmq_config_thread A thead that handles configuration ZMQ port. It allows set-ing and retriving
 * a new configuration...
 * @param param
 */
void BSRead::zmq_config_thread(void *param)
{
    BSRead* self = static_cast<BSRead*>(param);
    //Epics thread does not have join, this emulates it...
    epicsGuard<epicsMutex> running_guard(self->m_thread_rpc_running);

    while(true){
        bsread_debug(3,"ZMQ RPC: waiting for message!");

        zmq::message_t  msg;
        Json::Value     json_request;
        Json::Value     json_response;
        Json::Reader    json_reader;

        try{
            self->m_zmq_sock_config->recv(&msg);
        }
        catch(zmq::error_t& err){
            bsread_debug(1,"ZMQ RPC: terminating configuration thread: %s",err.what());
            break;
        }


        bsread_debug(3,"ZMQ RPC: received message, parsing...!");

        //Try to decode the message
        if(!json_reader.parse(string((char*)(msg.data())),json_request)){
            json_response["status"]="error";
            json_response["error"]="Could not parse JSON: " + json_reader.getFormatedErrorMessages();
        }
        else{ //JSON correctly decoded

            //For beckward compatilibity
            //Check if `channels` or `grep` elements are present in
            //request. If they are it indicates that this is a configuration message
            if(json_request.isMember("grep") || json_request.isMember("channels")){
                try{
                    self->configure(json_request);
                    bsread_debug(2,"ZMQ RPC: new configuration applied!");                    
                    json_response["status"]="ok";
                }
                catch(std::runtime_error& e){
                    json_response["status"]="error";
                    json_response["error"]=e.what();
                    bsread_debug(2,"ZMQ RPC: invalid configuration received: %s",e.what());
                }
            }

            //RPC approach
            else if(json_request.isMember("cmd")){
                string cmd = json_request["cmd"].asString();

                try{
                    if(cmd=="config"){
                        self->configure(json_request["config"]);
                        bsread_debug(2,"ZMQ RPC: new configuration applied!");                        
                    }
                    //Introspect
                    if(cmd=="introspect"){
                        json_response["config"] = self->generate_json_config();

                        Json::Value all_channels;
                        for(size_t i=0;i<self->m_channels.size();i++){
                                all_channels.append(self->m_channels[i]->get_name());
                        }

                        json_response["channels"]=all_channels;
                        json_response["inhibit"]=self->m_inhibit;
                    }

                    //inhibit
                    if(cmd=="inhibit"){
                        if(json_request.isMember("val")){
                            bool val = json_request["val"].asBool();
                            self->set_inhibit(val);
                        }
                        json_response["inhibit"] = self->m_inhibit;
                    }


                    json_response["status"]="ok";

                }
                catch(std::runtime_error& e){
                    json_response["status"]="error";
                    json_response["error"]=e.what();
                    bsread_debug(2,"ZMQ RPC: exception while processing: %s",e.what());
                }
            }

            //No cmd found
            else{
                json_response["status"]="error";
                json_response["error"]="Did not understand this JSON, expecting cmd member..";
            }
        } //END of while(true)

        //Encode JSON to string
        string response = json_response.toStyledString();

        try{
            self->m_zmq_sock_config->send(response.c_str(),response.length());
        }
        catch(zmq::error_t& err){
            bsread_debug(1,"ZMQ RPC: terminating configuration thread: %s",err.what());
            break;
        }

    }

    self->m_zmq_sock_config->close();
    bsread_debug(1,"ZMQ RPC: configuration thread terminated!");
}

uint32_t BSRead::zmq_overflows() const
{
    return zmq_overflows_;
}


int bsread_debug=0;


}
