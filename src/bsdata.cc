#include "bsdata.h"

#if defined(_MSC_VER)
 #include <winsock2.h>
#else
 #include <arpa/inet.h>
#endif

extern "C"{
#include "compression/lz4.h"
#include "compression/bitshuffle.h"
}


//Simple code to allow runtime detection of system endianess
bool isLittleEndian()
{
    short int number = 0x1;
    char *numPtr = (char*)&number;
    return (numPtr[0] == 1);
}

Json::Value bsread::BSDataChannel::get_data_header(bool config_only){
    Json::Value root;
    root["name"]=m_name;
    root["offset"] = this->m_meta_offset;
    root["modulo"] = this->m_meta_modulo;

    if(!config_only){

        root["type"]= bsdata_type_name[m_type];
        root["encoding"]= m_encoding_le ? "little" : "big";

        if(m_shape.size()){
            for(unsigned int i=0;i<m_shape.size();i++){
                root["shape"][i]=m_shape[i];
            }
        }
        else{
            root["shape"][0]=static_cast<int>(m_len); //shape is array of dimensions, scalar = [1]
        }

        string compression = "none";

        if(m_compression == compression_lz4) compression = "lz4";
        if(m_compression == compression_bslz4) compression = "bitshuffle_lz4";


        root["compression"] = compression;

    }

    return root;
}

std::string bsread::BSDataChannel::dump_header(){
    return get_data_header().toStyledString();
}

void bsread::BSDataChannel::set_enabled(bool enabled){
    m_enabled = enabled;
}

bool bsread::BSDataChannel::get_enabled(){
    return m_enabled;
}

string bsread::BSDataChannel::get_name(){
    return m_name;
}

bsread::BSDataChannel::BSDataChannel(const string &name, bsread::bsdata_type type):
    m_type(type),
    m_data(0),
    m_timestamp(),
    m_len(0),
    m_name(name),
    m_encoding_le(isLittleEndian()),
    m_compression(compression_none),
    m_enabled(true),
    m_callback(0),
    m_meta_modulo(1),
    m_meta_offset(0)
{}

size_t bsread::BSDataChannel::set_data(void *data, size_t len){
    m_data=data;
    m_len=len;
    return get_len();
}

/**
 * @brief compress auxilary function that wraps lz4 so that they are bs compatible (prepends length, etc..)
 *
 * The routine accepts uncompressed data and lvalue pointing to buffer pointer. If buffer_size is 0 than new buffer is alocated. If not,
 * the buffer will be reuesed if possible, otherwise it will be replaced by a larger buffer.
 *
 * @param uncompressed_data
 * @param uncompressed_data_len
 * @param buffer
 * @param buffer_size
 * @param network_order
 * @return
 */
size_t compress_lz4(const char* uncompressed_data, int32_t uncompressed_data_len, char*& buffer, size_t& buffer_size, bool network_order){

    size_t compressed_size;

    // Ensure output buffer is large enough
    if(buffer_size < (size_t)(LZ4_compressBound(uncompressed_data_len)+4) ){
        // Free existing buffer if it exists
        if(buffer_size) free(buffer);
        //New output buffer
        buffer_size = LZ4_compressBound(uncompressed_data_len)+4;
        buffer = (char*) malloc(buffer_size);
    }

    //Set the uncompressed blob length
    if(network_order){
        ((int32_t*)buffer)[0] = htonl(uncompressed_data_len);
    }
    else{
        ((int32_t*)buffer)[0] = uncompressed_data_len;
    }

    //Compress the data
    compressed_size = LZ4_compress_default((const char*)uncompressed_data,&buffer[4],uncompressed_data_len,buffer_size-4);

    if(!compressed_size) throw runtime_error("Error while compressing [LZ4] channel:");
    return compressed_size+4;

}

size_t compress_bitshuffle(const char* uncompressed_data, size_t nelm, size_t elm_size, char*& buffer, size_t& buffer_size){

    size_t compressed_size;
    size_t block_size = bshuf_default_block_size(elm_size);
    size_t buf_min_size=bshuf_compress_lz4_bound(nelm,elm_size,0)+12; //12byte header at the start

    // Ensure output buffer is large enough
    if(buffer_size < buf_min_size ){
        // Free existing buffer if it exists
        if(buffer_size) free(buffer);
        //New output buffer
        buffer_size = buf_min_size;
        buffer = (char*) malloc(buffer_size);
    }

    //Set the uncompressed blob length
    ((int64_t*)buffer)[0] = htonl(nelm*elm_size);
    //Set the subblock size length
    ((int32_t*)buffer)[2] = htonl(block_size);



    //Compress the data
    compressed_size = bshuf_compress_lz4((const char*)uncompressed_data,&buffer[12],nelm,elm_size,block_size);

    if(!compressed_size) throw runtime_error("Error while compressing [LZ4] channel:");
    return compressed_size+12;



    return 0;
}


size_t bsread::BSDataChannel::acquire_compressed(char*& buffer, size_t& buffer_size){
    if(m_compression==compression_none) return 0;

    const void* uncompressed_data = acquire(); //Do not forget to release before exiting this function
    int32_t uncompressed_data_len = get_len();
    size_t compressed_size=0;

    if(m_compression==compression_lz4){
        compressed_size=compress_lz4((const char*)uncompressed_data,uncompressed_data_len,buffer,buffer_size,false);
    }
    if(m_compression==compression_bslz4){
        compressed_size=compress_bitshuffle((const char*)uncompressed_data,get_nelm(),get_elem_size(),buffer,buffer_size);
    }

    release();
    return compressed_size;
}

void bsread::BSDataChannel::set_timestamp(timestamp timestamp){
    m_timestamp = timestamp;
}

bsread::BSDataSenderZmq::BSDataSenderZmq(zmq::context_t &ctx, string address, int sndhwm, int sock_type, int linger):
    m_compress_buffer_size(0),
    m_ctx(ctx),
    m_sock(m_ctx,sock_type),
    m_address(address.c_str())
{
    m_sock.setsockopt(ZMQ_LINGER, &linger,sizeof(linger));
    m_sock.setsockopt(ZMQ_SNDHWM, &sndhwm,sizeof(sndhwm));
    m_sock.bind(address.c_str());
}

size_t bsread::BSDataSenderZmq::send_message(bsread::BSDataMessage &message, zmq::socket_t& sock,char*& compress_buffer, size_t &compress_buffer_size){
    size_t msg_len=0;
    size_t part_len;

    //Send main header
    const string* mainheader = message.get_main_header();
    part_len = sock.send(mainheader->c_str(),mainheader->length(),ZMQ_SNDMORE|ZMQ_NOBLOCK);
    if(!part_len) return 0;
    msg_len+=part_len;


    //Send dataheader
    const string* dataheader = message.get_data_header();
    part_len = sock.send(dataheader->c_str(),dataheader->length(),ZMQ_SNDMORE|ZMQ_NOBLOCK);
    if(!part_len) return 0;
    msg_len+=part_len;

    //Send data for each channel
    const vector<BSDataChannel*>* channels = message.get_channels();
    size_t n = channels->size();
    for(size_t i=0; i<n; i++){
        int zmq_flags = ZMQ_SNDMORE | ZMQ_NOBLOCK;
        BSDataChannel* chan = channels->at(i);

        //Only send enabled channels
        if(chan->get_enabled()){
            const void* data;
            size_t data_len;
            //Fetch timestamp
            uint64_t rtimestamp[2];
            chan->get_timestamp(rtimestamp);


            //Acquire and send data

            //Check if compressing is enabled
            /* When sending compressed data we are sending from our buffer (acquire_compressed makes a
             * compressed copy of the source data. As such it does not require unlocking of channel.
             * This is the reason for different handling of compressed vs uncompressed data here */
            data_len = chan->acquire_compressed(compress_buffer,compress_buffer_size);
            if(data_len){
                part_len = sock.send(compress_buffer,data_len,zmq_flags);
                msg_len+=part_len;
//                printf("Compressed data! new buffer size %d, data sent %d/%d, header: %d, orig: %d \n",
//                       compress_buffer_size,
//                       data_len,
//                       part_len,
//                       ntohl(*(int32_t*)compress_buffer),
//                       chan->get_len());
            }
            else{ //Compressing disabled
                data = chan->acquire();
                data_len = chan->get_len();

                part_len = sock.send(data,data_len,zmq_flags);
                msg_len+=part_len;
                chan->release();
            }


            //Send timestamp, take care of last part
            if(i==n-1) zmq_flags &= ~ZMQ_SNDMORE;
            part_len = sock.send(rtimestamp,sizeof(rtimestamp),zmq_flags);

            msg_len+=part_len;

        }
        //Not enabled channels are replaced with empty submessages
        else{
            sock.send(NULL,0,ZMQ_SNDMORE | ZMQ_NOBLOCK);

            //Last part
            if(i==n-1) zmq_flags &= ~ZMQ_SNDMORE;
            part_len = sock.send(NULL,0,zmq_flags);

        }

    }

    return msg_len;
}

void bsread::BSDataMessage::add_channel(bsread::BSDataChannel *c){
    m_channels.push_back(c);
    //Update MD5 datahash since data header has changed
    m_datahash.clear();
}

void bsread::BSDataMessage::clear_channels(){
    m_channels.clear();
}

bool bsread::BSDataMessage::set(uint64_t pulseid, bsread::timestamp timestamp, bool set_enable){
    m_pulseid = pulseid;
    m_globaltimestamp = timestamp;

    bool any_channels_enabled = false;

    if(set_enable){
        for(size_t i=0;i<m_channels.size();i++){
            BSDataChannel* c = m_channels[i];

            int modulo = c->m_meta_modulo;

            int offset = c->m_meta_offset;

            //Calculate modulo if set
            if (modulo > 0) {
                if ( ((pulseid-offset) % modulo ) == 0) {
                    c->set_enabled(true);
                    any_channels_enabled = true;
                }
                else{
                    c->set_enabled(false);
                }
            }
        }
    }

    return any_channels_enabled;
}

const string *bsread::BSDataMessage::get_main_header(){
    Json::Value root;
    root["htype"] = BSREAD_MAIN_HEADER_VERSION;
    root["pulse_id"] = static_cast<Json::Int64>(m_pulseid);
    root["global_timestamp"]["sec"] = static_cast<Json::Int64>(m_globaltimestamp.sec);
    root["global_timestamp"]["ns"] = static_cast<Json::Int64>(m_globaltimestamp.nsec);

    string compression = "none";
    if(m_dh_compression == compression_lz4) compression = "lz4";
    if(m_dh_compression == compression_bslz4) compression = "bitshuffle_lz4";


    root["dh_compression"] = compression;

    /* Empty datahash indicates that the data_header was not yet constructed,
     * which is needed to calculate datahash. m_datahash is updated whenever a
     * new dataheader needs to be constructed. Here we simply overcome the lazy loading...
     */
    if(!m_datahash.empty()){
        this->get_data_header();
    }

    root["hash"]=m_datahash;
    m_mainheader = m_writer.write(root);

    return &m_mainheader;
}

const string* bsread::BSDataMessage::get_data_header(bool force_build_header){
    if(m_datahash.empty() || force_build_header){

        m_datasize = 0;
        Json::Value root;
        root["htype"] = BSREAD_DATA_HEADER_VERSION;

        for(size_t i=0;i<m_channels.size();i++){
            root["channels"][(int)i]=m_channels[i]->get_data_header();
            m_datasize += m_channels[i]->get_len() + sizeof(uint64_t[2]); //Size of data + timestamp
        }

        m_dataheader = m_writer.write(root);

        // Compress data header with LZ4
        if(m_dh_compression == compression_lz4){

            char* compressed=0;
            size_t compressed_buf_size=0;
            size_t compressed_len = compress_lz4(m_dataheader.c_str(),m_dataheader.length(),compressed,compressed_buf_size,true);
            m_dataheader = string(compressed,compressed_len);

            delete compressed;            
        }

        // Compress data header with LZ4 bitshuffle
        if(m_dh_compression == compression_bslz4){

            char* compressed=0;
            size_t compressed_buf_size=0;
            size_t compressed_len = compress_bitshuffle(m_dataheader.c_str(),m_dataheader.length(),sizeof(char),compressed,compressed_buf_size);
            m_dataheader = string(compressed,compressed_len);

            delete compressed;
        }



        m_datahash = md5(m_dataheader);
    }       

    return &m_dataheader;

}

const bool bsread::BSDataMessage::is_empty(){
    const vector<BSDataChannel*>* channels = this->get_channels();
    vector<BSDataChannel*>::const_iterator iter;

    for(iter=channels->begin();iter != channels->end();iter++){
        if((*iter)->get_enabled()){
            return false;
        }
    }
    return true;

}

bsread::BSDataChannel *bsread::BSDataMessage::find_channel(const string &name){
    for(size_t i=0;i<m_channels.size();i++){
        if(m_channels[i]->get_name() == name) return m_channels[i];
    }

    return NULL;
}

size_t bsread::BSDataMessage::get_datasize(){
    get_data_header();
    return m_datasize;
}

size_t bsread::BSDataSenderZmqOnepart::send_message(bsread::BSDataMessage &message, zmq::socket_t &sock)
{
    size_t msg_len=0;
    size_t part_len;

    //Send main header
    const string* mainheader = message.get_main_header();
    part_len = sock.send(mainheader->c_str(),mainheader->length(),ZMQ_SNDMORE|ZMQ_NOBLOCK);
    if(!part_len) return 0;
    msg_len+=part_len;


    //Send dataheader
    const string* dataheader = message.get_data_header();
    part_len = sock.send(dataheader->c_str(),dataheader->length(),ZMQ_SNDMORE|ZMQ_NOBLOCK);
    if(!part_len) return 0;
    msg_len+=part_len;


    //Allocate buffer
    zmq::message_t data_part(message.get_datasize());

    //Copy data into a message
    size_t offset=0;
    const vector<BSDataChannel*>* channels = message.get_channels();
    for(size_t i=0;i<channels->size();i++){
        BSDataChannel* chan = channels->at(i);


        //Only send enabled channels
        if(chan->get_enabled()){

            //Acquire data
            const void* data = chan->acquire();
            size_t len = chan->get_len();

            //Fetch timestamp
            uint64_t rtimestamp[2];
            chan->get_timestamp(rtimestamp);



//            double t = dbltime_get();
            memcpy((char*)data_part.data()+offset,data,len);
//            t=dbltime_get() - t;
//            printf("%4.4f us, %4.4f Gb/s\n",t*1e6,(len/1024.0/1024.0/1024.0)/t);

            offset+=len;
            memcpy((char*)data_part.data()+offset,rtimestamp,sizeof(rtimestamp));
            offset+=sizeof(rtimestamp);

            //Done with sending, release the data
            chan->release();

        }
        //Not enabled channels are replaced with empty submessages
        else{
//            sock.send(NULL,0,ZMQ_SNDMORE | ZMQ_NOBLOCK);
            //?
        }


    }

    //Do the send
    sock.send(data_part,ZMQ_NOBLOCK);
    data_part.rebuild();


    return msg_len+data_part.size();
}
