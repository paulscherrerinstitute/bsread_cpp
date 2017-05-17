#ifndef BSREAD_H_GURAD
#define BSREAD_H_GURAD

#include <iostream>
#include <string>
#include <vector>
#include <stdint.h>
#include <zmq.hpp>


/**
  Allow builds without EPICS depenency.
  Note that this can be used for all builds except
  for shared libraries on windows systems.
**/
#ifndef WITHOUT_EPICS
    #include <shareLib.h>
#else
    #define epicsShareClass
#endif


#include "json.h"
#include "md5.h"


using namespace std;

#define BSREAD_MAIN_HEADER_VERSION "bsr_m-1.1"
#define BSREAD_DATA_HEADER_VERSION "bsr_d-1.1"

namespace bsread{

enum bsdata_type {BSDATA_STRING,
                  BSDATA_BOOl,
                  BSDATA_FLOAT64,
                  BSDATA_FLOAT32,
                  BSDATA_INT8,
                  BSDATA_UINT8,
                  BSDATA_INT16,
                  BSDATA_UINT16,
                  BSDATA_INT32,
                  BSDATA_UINT32,
                  BSDATA_INT64,
                  BSDATA_UINT64};

static const char* const bsdata_type_name[] = {
                                        "string",
                                        "bool",
                                        "float64",
                                        "float32",
                                        "int8",
                                        "uint8",
                                        "int16",
                                        "uint16",
                                        "int32",
                                        "uint32",
                                        "int64",
                                        "uint64"};

static const size_t bsdata_type_size[] = {  1,
                                            1,
                                            8,
                                            4,
                                            1,
                                            1,
                                            2,
                                            2,
                                            4,
                                            4,
                                            8,
                                            8
                                         };

class epicsShareClass BSDataChannel;

enum bsdata_compression_type{
    compression_none,
    compression_lz4,
    compression_bslz4
};


/**
 * Callback that is invoked when BSDataChannel.acquire() is called.
 *
 * chan     points to BSDataChannel instance
 * acquire  is 1 if channel is beeing acquired or 0 if its beeing released
 * pvt      private callback data
 */
typedef void (*BSDataCallaback)(BSDataChannel* chan,bool acquire, void* pvt);

/**
 * @brief The timestamp struct
 *
 * platform independant structure for storing timestamps
 */
struct timestamp{

    /**
     * Constructor
     */
    timestamp(void):
      sec(0),
      nsec(0)
      {};

    /**
     * @brief sec seconds past UNIX epoch (1/1/1970)
     */
    uint64_t sec;

    /**
     * @brief ns nanosecond offset since last full second
     */
    uint64_t nsec;
};

class BSDataChannel{
    bsdata_type     m_type;
    void*           m_data;
    timestamp       m_timestamp;
    size_t          m_len;
    string          m_name;
    bool            m_encoding_le;
    bool            m_enabled;
    vector<unsigned int> m_shape;

    BSDataCallaback m_callback;
    void* m_callback_pvt;


public:



    /* standard meta data variables */
    int             m_meta_modulo;
    int             m_meta_offset;
    bsdata_compression_type    m_compression;


    /* extra metadata variables */
    Json::Value     m_meta;

    BSDataChannel(const string& name,bsdata_type type);

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
     * @brief acquire function has to be called before attempting to access data,
     */
    const void* acquire(){
        if(m_callback) m_callback(this,true,m_callback_pvt);
        return m_data;
    }

    /**
     * @brief acquire function has to be called before attempting to access data
     * @param temp_buffer; temporary buffer to use for compression. If temp_buffer
     * is not nullptr than existing buffer is used and the result of compression is
     * placed in it. Note that if buffer is too small it will be silently resized by
     * realloc.
     *
     * If temp_buffer is nullptr than new buffer is allocated. It is responsiblity of
     * the caller to free this buffer.
     *
     */
    size_t acquire_compressed(char *&buffer, size_t &buffer_size);

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
    inline size_t get_len(){
        return get_nelm()*get_elem_size();
    }

    /**
     * @brief get_nelm
     * @return number of elements in the channel
     */
    inline size_t get_nelm(){
        return m_len;
    }

    /**
     * @brief get_elem_size
     * @return size of each element in bytes
     */
    inline size_t get_elem_size(){
        return bsdata_type_size[m_type];
    }

    void set_timestamp(timestamp timestamp);

    void set_timestamp(int64_t sec, int64_t nsec){
        m_timestamp.sec=sec;
        m_timestamp.nsec = nsec;
    }


    timestamp get_timestamp(){
        return m_timestamp;
    }

    void get_timestamp(uint64_t dest[2]){
        dest[0] = m_timestamp.sec;
        dest[1] = m_timestamp.nsec;
    }

    void set_enabled(bool enabled);
    bool get_enabled();

    /**
     * @brief set_shape sets the shape of the channel. If not specified (or if given an empty vector)
     * than the shape is deducted automatically from size of data. Note that total size must remain
     * unchanged or else the clients will not be able to parse the data.
     *
     * @param shape
     */
    void set_shape(const vector<unsigned int>& shape){
        m_shape = shape;
    }


    /**
     * @brief set_shape same as set_shape(); but with classic array interface that is more suitable for
     * static configurations
     * @param shape
     * @param ndim
     */
    void set_shape(unsigned int shape[], size_t ndim){
        m_shape.clear();
        for(size_t i=0;i<ndim;i++){
            m_shape.push_back(shape[i]);
        }
    }

    string get_name();

    string dump_header();

    /**
     * @brief get_data_header
     * @param config_only if set to true the data header returend contains
     *  only configuration fields (name, modulo, offset)
     * @return Json::Value representing data header for this channel
     */
    Json::Value get_data_header(bool config_only=false);

};


class BSDataMessage{
    //Message metadata
    uint64_t        m_pulseid;
    timestamp   m_globaltimestamp;
    size_t      m_datasize;     //raw size of data
    string      m_datahash;
    string      m_dataheader;    //Copy of dataheader JSON string (to avoid reconstructing JSON on every iteration)
    string      m_mainheader;
    bsdata_compression_type m_dh_compression; //Data header compression

    //Actual members
    vector<BSDataChannel*> m_channels;

    Json::FastWriter m_writer;   //Json writer instance used for generating data headers


public:

	BSDataMessage(){};
    /**
     * @brief add_channel add bsdata channel to the message. Note that the channels are not
     * deleted when message is cleared. It's users application responsibility to deallocate
     * channels.
     * @param c
     */
    void add_channel(BSDataChannel* c);

    void clear_channels();

    /**
     * sets the message pulseid and global timestamp metadata.if set_enable = true
     * than BSDataChannel enable flag will be modified according to offset and modulo.
     * @brief set
     * @param pulseid
     * @param timestamp
     * @param calc_enable
     * @return true if at least one channel was enabled, false otherwise. If
     * set_enable is false than false is always returned
     */
    bool set(uint64_t pulseid, bsread::timestamp tst, bool set_enable=true);

    const string* get_main_header();

    const string* get_data_header(bool force_build_header = false);

    const vector<BSDataChannel*>* get_channels(){
        return &m_channels;
    }


    /**
     * @brief is_empty checks if any of the channels within this message are enabled
     * @return true if no channels are enabled, false otherwise
     */
    const bool is_empty();

    BSDataChannel* find_channel(const string& name);

    size_t get_datasize();

    void set_dh_compression(bsdata_compression_type type){
        this->m_dh_compression = type;
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
     * @param linger
     */
    BSDataSenderZmq(zmq::context_t& ctx, string address,int sndhwm=10, int sock_type=ZMQ_PUSH, int linger=1000);


    virtual size_t send_message(BSDataMessage& message){
        //TODO: add stats
        return send_message(message,m_sock,m_compress_buffer,m_compress_buffer_size);
    }

    static size_t send_message(BSDataMessage& message, zmq::socket_t &sock, char*& compress_buffer, size_t &compress_buffer_size);


    virtual ~BSDataSenderZmq(){
        if(m_compress_buffer_size) free(m_compress_buffer);
    };

protected:
    char*           m_compress_buffer;
    size_t          m_compress_buffer_size;
    zmq::context_t&  m_ctx;
    zmq::socket_t   m_sock;
    string          m_address;
};


/**
 * @brief The BSDataSenderZmqOnepart class Experimental class
 * in order to evaluate performance of sending all payload data as
 * single part...
 */
class BSDataSenderZmqOnepart: public BSDataSenderZmq{
public:

    BSDataSenderZmqOnepart(zmq::context_t& ctx, string address,int sndhwm=10,int sock_type=ZMQ_PUSH):BSDataSenderZmq(ctx,address,sndhwm,sock_type){}


    virtual size_t send_message(BSDataMessage& message){
        return send_message(message,m_sock);
    }

    static size_t send_message(BSDataMessage& message, zmq::socket_t &sock);

};



class BSDataSenderDebug{
    ostream& m_os;

public:

    BSDataSenderDebug(ostream& os):m_os(os){}


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

            uint64_t rtimestamp[2];
            chan->get_timestamp(rtimestamp);

            m_os.write(static_cast<const char*>(data),len);
            m_os.put('#');

            m_os.write((char*)(rtimestamp),sizeof(uint64_t)*2);

            chan->release();

            m_os << "END" << endl;
        }
    }


};




}// End of namespace bsread


#endif
