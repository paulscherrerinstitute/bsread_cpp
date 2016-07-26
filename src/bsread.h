#ifndef BSREAD_H
#define BSREAD_H

//std
#include <string>
#include <vector>

//EPICS includes
#include <shareLib.h>
#include <epicsThread.h>

//External includes
#include "zmq.hpp"
#include "json.h"

#include "bsdata.h"

extern int bsread_debug;

//Debug macro for consistency...
#define bsread_debug(level,M, ...) if(bsread_debug >= level) fprintf(stderr, "BSREAD_DEBUG: (%s:%d) " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)


namespace bsread{

class epicsShareClass BSRead
{

public:

    BSRead();


    /**
     * @brief confiugre_zmq Reconfigure bsread sender. Function must be invoked before first message can be send.
     * This can be safely invoked during runtime and sender hangover is ensured to complete without data loss.
     * @param address
     * @param socket_type
     * @param hwm
     */
    void confiugre_zmq(const char* address,int socket_type,int hwm,int linger=1000);


    /**
     * @brief confiugre_zmq_config enable ZMQ RPC socket allowing one to send
     * JSON confiugration string via ZMQ. This function should not be called more than once!
     * @param address
     */
    void confiugre_zmq_config(const char* address);

    /**
     * @brief add_channel Add a channel to BSRead instance. Note that by default
     * the channel is NOT enabled. It has be enabled via JSON configuration string
     * or by calling enable_all_channels();
     *
     * Note that the channel should not be shared between mutliple instances of BSREAD.
     * Note that once the channel is added, its lifecycle is managed by BSRead class,
     * the channel will be destroyed when the BSRead instance is destroyed...
     * @param chan
     */
    void add_channel(BSDataChannel* chan){
        m_channels.push_back(chan);
    }

    /**
     * @brief configure method accepting a json string. Function will throw runtime error in case json is invalid.
     * Configuration will only be applied after applyConfiguration was called. (periodically after read)
     * @param json
     */
    void configure(Json::Value config);
    void configure(const string & json_string);


    //TODO: remove me!
    void enable_all_channels(){
        m_message_new = new BSDataMessage();

        for(size_t i=0;i<m_channels.size();i++){
            m_message_new->add_channel(m_channels[i]);
        }

    }

    /**
     * @brief read read all currently configured channels and send values out via ZMQ;
     * @param pulse_id
     * @param timestamp
     */
    void send(uint64_t pulse_id, timestamp tst);

    static BSDataMessage parse_json_config(const vector<BSDataChannel*>& all_channels, Json::Value config);

    /**
     * @brief generate_json_config generates a Json object containig current configuration.
     * Note that the configuration is recreated from current state and is not a simple cached value
     * of applied configuration.
     *
     * @return
     */
    Json::Value generate_json_config();


    ~BSRead();

    uint32_t zmq_overflows() const;


private:

    static void zmq_config_thread(void* param);

    vector<BSDataChannel*> m_channels;

    bsread::BSDataMessage* m_message;
    bsread::BSDataMessage* m_message_new;  //Used as 1-length sync queue (protced by m_mutex_config)

    bsread::BSDataSenderZmq* m_sender;
    bsread::BSDataSenderZmq* m_sender_new; //Used as 1-length sync queue (protced by m_mutex_config)

    volatile bool m_inhibit; /*Setting this member to True will disable any sendout from send method (but will still apply new configurations...)
                              *
                              * Bit can be manipulated vial 0RPC interface by sending the following command
                              * {"cmd":"inhibit","val":<true/false>}
                              *
                              *  Bit can be read by sending the following command
                              * {"cmd":"inhibit"}
                              */

    zmq::context_t m_zmq_ctx;
    zmq::socket_t* m_zmq_sock_config;  //Socket for configuration

    uint32_t zmq_overflows_;   //Number of zmq send errors

    epicsMutex m_mutex_config;          //synchornisation between config/read thread

    epicsThreadId   m_thread_rpc;  //ZMQ remote configuration thread
    epicsMutex      m_thread_rpc_running;   //Held while thread is running

};


}
#endif // BSREAD_H
