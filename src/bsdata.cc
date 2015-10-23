#include "bsdata.h"

//const char*bsdata_type_name[] = {"Double","Float","Long","ULong","Int","UInt","Short","UShort","String"};
//const size_t bsdata_type_size[] = { 8,4,8,8,4,4,2,2,1 };

Json::Value bsread::BSDataChannel::get_data_header(){
    Json::Value root;
    root["name"]=m_name;
    root["type"]=bsdata_type_name[m_type];
    root["shape"][0]=static_cast<int>(m_len); //shape is array of dimensions, scalar = [1]
    root["encoding"]= m_encoding_le ? "little" : "big";

    return root;
}

std::string bsread::BSDataChannel::dump_header(){
    return get_data_header().toStyledString();
}

void bsread::BSDataChannel::set_timestamp(){
    clock_gettime(CLOCK_REALTIME,&m_timestamp);
}

void bsread::BSDataChannel::set_enabled(bool enabled){
    m_enabled = enabled;
}

bool bsread::BSDataChannel::get_enabled(){
    return m_enabled;
}

size_t bsread::BSDataChannel::set_data(void *data, size_t len){
    m_data=data;
    m_len=len;
    return get_len();
}

void bsread::BSDataChannel::set_timestamp(timespec timestamp){
    m_timestamp = timestamp;
}

bsread::BSDataSenderZmq::BSDataSenderZmq(string address, int sndhwm, int sock_type):
    m_ctx(1),
    m_sock(m_ctx,sock_type),
    m_address(address.c_str())
{
    m_sock.bind(address.c_str());
    m_sock.setsockopt(ZMQ_SNDHWM, &sndhwm,sizeof(sndhwm));
}

size_t bsread::BSDataSenderZmq::send_message(bsread::BSDataMessage &message, zmq::socket_t& sock){
    const char* header;
    size_t msg_len=0;
    size_t part_len;

    //Send main header
    header = message.get_main_header().c_str();
    part_len = sock.send(header,strlen(header),ZMQ_SNDMORE|ZMQ_NOBLOCK);
    if(!part_len) return 0;
    msg_len+=part_len;

    //Send dataheader
    header = message.get_data_header().c_str();
    part_len = sock.send(header,strlen(header),ZMQ_SNDMORE|ZMQ_NOBLOCK);
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
            long int rtimestamp[2];
            chan->get_timestamp(rtimestamp);

            part_len = sock.send(data,len,ZMQ_SNDMORE | ZMQ_NOBLOCK);
            msg_len+=part_len;


            //Last part
            if(i==channels->size()-1)
                part_len = sock.send(rtimestamp,2*sizeof(long int),ZMQ_NOBLOCK);
            else
                part_len = sock.send(rtimestamp,2*sizeof(long int),ZMQ_SNDMORE | ZMQ_NOBLOCK);

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
