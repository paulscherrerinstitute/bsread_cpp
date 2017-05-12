#include "bsdata.h"

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
    m_enabled(true),
    m_callback(0),
    m_meta_modulo(1),
    m_meta_offset(0),
    m_compression(compression_none)
{}

size_t bsread::BSDataChannel::set_data(void *data, size_t len){
    m_data=data;
    m_len=len;
    return get_len();
}

void bsread::BSDataChannel::set_timestamp(timestamp timestamp){
    m_timestamp = timestamp;
}

bsread::BSDataSenderZmq::BSDataSenderZmq(zmq::context_t &ctx, string address, int sndhwm, int sock_type, int linger):
    m_ctx(ctx),
    m_sock(m_ctx,sock_type),
    m_address(address.c_str())
{
    m_sock.setsockopt(ZMQ_LINGER, &linger,sizeof(linger));
    m_sock.setsockopt(ZMQ_SNDHWM, &sndhwm,sizeof(sndhwm));
    m_sock.bind(address.c_str());
}

size_t bsread::BSDataSenderZmq::send_message(bsread::BSDataMessage &message, zmq::socket_t& sock){
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

            //Acquire data
            const void* data = chan->acquire();
            size_t len = chan->get_len();

            //Fetch timestamp
            uint64_t rtimestamp[2];
            chan->get_timestamp(rtimestamp);

            part_len = sock.send(data,len,zmq_flags);
            msg_len+=part_len;


            //Last part
            if(i==n-1) zmq_flags &= ~ZMQ_SNDMORE;
            part_len = sock.send(rtimestamp,sizeof(rtimestamp),zmq_flags);

            msg_len+=part_len;


            //Done with sending, release the data
            chan->release();

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
    if(m_dh_compression == BSDataChannel::compression_lz4) compression = "lz4";
    if(m_dh_compression == BSDataChannel::compression_bslz4) compression = "bitshuffle_lz4";


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
        if(m_dh_compression == BSDataChannel::compression_lz4){
            //Compresssing data header            
            size_t header_len = m_dataheader.length();
            char* compressed = new char[LZ4_compressBound(header_len)]; //Temporary compression buffer
            int compressed_len = LZ4_compress_default(m_dataheader.c_str(),compressed,header_len,header_len);

            if(!compressed_len){
                throw runtime_error("Could not compress data header...\n");
            }

            m_dataheader = string(compressed,compressed_len);
            delete compressed;
            printf("dataheader: %d %d %d\n",header_len, compressed_len,m_dataheader.length());
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
