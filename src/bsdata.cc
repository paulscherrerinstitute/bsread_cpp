//#include <dbAccess.h>
//#include <epicsThread.h>

//#include <epicsExport.h>
#include "bsdata.h"

//Simple code to allow runtime detection of system endianess
bool isLittleEndian()
{
    short int number = 0x1;
    char *numPtr = (char*)&number;
    return (numPtr[0] == 1);
}

Json::Value bsread::BSDataChannel::get_data_header(){
    Json::Value root;
    root["name"]=m_name;
    root["type"]= bsdata_type_name[m_type];
    root["shape"][0]=static_cast<int>(m_len); //shape is array of dimensions, scalar = [1]
    root["encoding"]= m_encoding_le ? "little" : "big";    

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
    m_len(0),
    m_name(name),
    m_encoding_le(isLittleEndian()),
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

void bsread::BSDataChannel::set_timestamp(timestamp timestamp){
    m_timestamp = timestamp;
}

bsread::BSDataSenderZmq::BSDataSenderZmq(zmq::context_t &ctx, string address, int sndhwm, int sock_type):
    m_ctx(ctx),
    m_sock(m_ctx,sock_type),
    m_address(address.c_str())
{
    m_sock.bind(address.c_str());
    m_sock.setsockopt(ZMQ_SNDHWM, &sndhwm,sizeof(sndhwm));
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

            part_len = sock.send(data,len,ZMQ_SNDMORE | ZMQ_NOBLOCK);
            msg_len+=part_len;


            //Last part
            if(i==channels->size()-1)
                part_len = sock.send(rtimestamp,2*sizeof(long long),ZMQ_NOBLOCK);
            else
                part_len = sock.send(rtimestamp,2*sizeof(long long),ZMQ_SNDMORE | ZMQ_NOBLOCK);

            msg_len+=part_len;


            //Done with sending, release the data
            chan->release();

        }
        //Not enabled channels are replaced with empty submessages
        else{
            sock.send((void*)0,0,ZMQ_SNDMORE | ZMQ_NOBLOCK);

            //Last part
            if(i==channels->size()-1)
                part_len = sock.send((void*)0,0,ZMQ_NOBLOCK);
            else
                part_len = sock.send((void*)0,0,ZMQ_SNDMORE | ZMQ_NOBLOCK);


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

void bsread::BSDataMessage::set(uint64_t pulseid, bsread::timestamp timestamp, bool set_enable){
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

const string *bsread::BSDataMessage::get_main_header(){
    Json::Value root;
    root["htype"] = BSREAD_MAIN_HEADER_VERSION;
    root["pulse_id"] = static_cast<Json::Int64>(m_pulseid);
    root["global_timestamp"]["epoch"] = static_cast<Json::Int64>(m_globaltimestamp.sec);
    root["global_timestamp"]["ns"] = static_cast<Json::Int64>(m_globaltimestamp.nsec);

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

const string* bsread::BSDataMessage::get_data_header(){
    if(m_datahash.empty()){

        m_datasize = 0;
        Json::Value root;
        root["htype"] = BSREAD_DATA_HEADER_VERSION;

        for(size_t i=0;i<m_channels.size();i++){
            root["channels"][(int)i]=m_channels[i]->get_data_header();
            m_datasize += m_channels[i]->get_len() + 2*sizeof(long long); //Size of data + timestamp
        }

        m_dataheader = m_writer.write(root);
        m_datahash = md5(m_dataheader);
    }

    return &m_dataheader;

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
            memcpy((void*)((char*)data_part.data()+offset),data,len);
//            t=dbltime_get() - t;
//            printf("%4.4f us, %4.4f Gb/s\n",t*1e6,(len/1024.0/1024.0/1024.0)/t);

            offset+=len;
            memcpy((void*)((char*)data_part.data()+offset),rtimestamp,2*sizeof(long long));
            offset+=2*sizeof(long long);

            //Done with sending, release the data
            chan->release();

        }
        //Not enabled channels are replaced with empty submessages
        else{
//            sock.send((void*)0,0,ZMQ_SNDMORE | ZMQ_NOBLOCK);
            //?
        }


    }

    //Do the send
    sock.send(data_part,ZMQ_NOBLOCK);
    data_part.rebuild();


    return msg_len+data_part.size();
}
