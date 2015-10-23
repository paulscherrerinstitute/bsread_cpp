#ifndef BSREAD_H_GURAD
#define BSREAD_H_GURAD

#include <iostream>
#include <string>
#include <time.h>
#include <vector>
#include <zmq.hpp>


#include "json.h"


using namespace std;

namespace bsread{

enum bsdata_type {BSDATA_DOUBLE, BSDATA_FLOAT, BSDATA_LONG, BSDATA_ULONG, BSDATA_INT, BSDATA_UINT, BSDATA_SHORT,BSDATA_USHORT,BSDATA_CHAR,BSDATA_UCHAR,BSDATA_STRING};
static const char*bsdata_type_name[] = {"Double","Float","Long","ULong","Integer","UInt","Short","UShort","Char","UChar","String"};
static const size_t bsdata_type_size[] = { 8,4,8,8,4,4,2,2,1,1,1 };

class BSDataChannel;
typedef void (*BSDataCallaback)(BSDataChannel* chan,bool acquire, void* pvt);

class BSDataChannel{
    bsdata_type     m_type;
    void*           m_data;
    timespec        m_timestamp;
    size_t          m_len;
    string          m_name;
    bool            m_encoding_le;
    bool            m_enabled;

    BSDataCallaback m_callback;
    void* m_callback_pvt;


public:

    /* meta data variables */
    int             m_meta_modulo;
    int             m_meta_offset;

    BSDataChannel(const string& name,bsdata_type type):
        m_type(type),
        m_data(0),
        m_len(0),
        m_name(name),
        m_encoding_le(true),
        m_enabled(true),
        m_callback(0),
        m_meta_modulo(0),
        m_meta_offset(0)
    {}

    /**
     * @brief set_data set message data. length is specified in number of
     * elements and not size in bytes. It is considered that data points
     * to an array of elements of type specified in constructor.
     * @param data
     * @param len number of elements of type m_type.
     * @return expected size of data (e.g. len*sizeof(m_type))
     */
    size_t set_data(void* data, size_t len);

    void* get_data(){
        return m_data;
    }

    /**
     * @brief set_callback set channel callback.
     * The callback is invoked when acquire is called and allows user to dynamically set data pointer,
     * data shape, or to preform lock to a data structure. The callback is invoked again after the sender
     * has finished using the data.
     *
     * @param c
     */
    void set_callback(BSDataCallaback c,void* pvt){
        m_callback = c;
        m_callback_pvt = pvt;
    }


    void clear_callback(){
        m_callback = 0;
        m_callback_pvt =0;
    }

    /**
     * @brief acquire function has to be called before attempting to access data
     */
    const void* acquire(){
        if(m_callback) m_callback(this,true,m_callback_pvt);
        return m_data;
    }

    /**
     * @brief release has to be called when consumer no longer requires the data
     */
    void release(){
        if(m_callback) m_callback(this,false,m_callback_pvt);
    }

    /**
     * @brief get_len
     * @return size of data returned by acquire in bytes
     */
    size_t get_len(){
        return m_len*bsdata_type_size[m_type];
    }

    void set_timestamp(timespec timestamp);

    /**
     * @brief set_timestamp sets the timestamp to a current system time
     */
    void set_timestamp();

    timespec get_timestamp(){
        return m_timestamp;
    }

    void get_timestamp(long int dest[2]){
        dest[0] = m_timestamp.tv_sec;
        dest[1] = m_timestamp.tv_nsec;
    }

    void set_enabled(bool enabled);
    bool get_enabled();

    string get_name(){
        return m_name;
    }

    string dump_header();

    /**
     * @brief get_data_header
     * @return Json::Value representing data header for this channel
     */
    Json::Value get_data_header();

};


class BSDataMessage{
    //Message metadata
    long        m_pulseid;
    timespec    m_globaltimestamp;
    string      m_datahash;

    //Actual members
    vector<BSDataChannel*> m_channels;
    bool m_changed; //Indicates that data header has to be reconstructed TODO: do me! [thats what she said :D ]

    Json::FastWriter m_writer;   //Json writer instance used for generating data headers


public:

    /**
     * @brief add_channel add bsdata channel to the message
     * @param c
     */
    void add_channel(BSDataChannel* c){
        m_channels.push_back(c);
    }

    void clear_channels(){
        m_channels.clear();
    }

    /**
     * sets the message pulseid and global timestamp metadata.if set_enable = true
     * than BSDataChannel enable flag will be modified according to offset and modulo.
     * @brief set
     * @param pulseid
     * @param timestamp
     * @param calc_enable
     */
    void set(long pulseid, timespec timestamp,bool set_enable=true){
        m_pulseid = pulseid;
        m_globaltimestamp = timestamp;


        if(set_enable){
            for(size_t i=0;i<m_channels.size();i++){
                BSDataChannel* c = m_channels[i];

                int modulo = c->m_meta_modulo;
                int offset = c->m_meta_offset;

                //Calculate modulo if set
                if (modulo > 0) {
                    if ( ((pulseid-offset) % modulo ) == 0) {
                        c->set_enabled(true);
                    }
                    else{
                        c->set_enabled(false);
                    }
                }
            }
        }
    }

    string get_main_header(){
        Json::Value root;
        root["htype"] = "bsr_m-1.0";
        root["pulse_id"] = static_cast<Json::Int64>(m_pulseid);
        root["global_timestamp"]["epoch"] = static_cast<Json::Int64>(m_globaltimestamp.tv_sec);
        root["global_timestamp"]["ns"] = static_cast<Json::Int64>(m_globaltimestamp.tv_nsec);
        root["hash"]="0";
        return m_writer.write(root);;
    }

    string get_data_header(){
        Json::Value root;
        root["htype"] = "bsr_d-1.0";

        for(size_t i=0;i<m_channels.size();i++){
            root["channels"][(int)i]=m_channels[i]->get_data_header();
        }

        return m_writer.write(root);
    }

    const vector<BSDataChannel*>* get_channels(){
        return &m_channels;
    }
};


/**
 * @brief The BSDataSenderZmq class
 *
 * Class that takes BSData messages and transmits them via ZMQ socket
 * By default the class will create its own context and socket. A static
 * method exists that perorms the same, but requires ZMQ socket to be passed
 */
class BSDataSenderZmq{

public:

    /**
     * @brief BSDataSenderZmq Holds all infrastructre needed for BSREAD, this includes zmq context and zmq socket.
     * Parameters passed to constructor are used to create this infrastructure.
     * @param address
     * @param sndhwm
     * @param sock_type
     */
    BSDataSenderZmq(string address,int sndhwm=10,int sock_type=ZMQ_PUSH);


    size_t send_message(BSDataMessage& message){
        //TODO: add stats
        return send_message(message,m_sock);
    }

    static size_t send_message(BSDataMessage& message, zmq::socket_t &sock);
private:

    zmq::context_t  m_ctx;
    zmq::socket_t   m_sock;
    string          m_address;
};




class BSDataSenderDebug{
    ostream& m_os;

public:

    BSDataSenderDebug(ostream& os):m_os(os){};


    void send_message(BSDataMessage& message){

        m_os << "#MAIN_HEADER" << endl;
        m_os << message.get_main_header() << endl;

        m_os << "#DATA_HEADER" << endl;
        m_os << message.get_data_header() << endl;

        const vector<BSDataChannel*>* channels = message.get_channels();




        for(size_t i=0;i<channels->size();i++){
            BSDataChannel* chan = channels->at(i);

            m_os << "#" << channels->at(i)->get_name() << endl;

            const void* data = chan->acquire();
            size_t len = chan->get_len();

            long int rtimestamp[2];
            chan->get_timestamp(rtimestamp);

            m_os.write(static_cast<const char*>(data),len);
            m_os.put('#');

            m_os.write((char*)(rtimestamp),sizeof(long int)*2);

            chan->release();

            m_os << "END" << endl;
        }
    }


};


}// End of namespace bsread


#endif
