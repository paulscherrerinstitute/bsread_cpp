#include "Sender.h"

using namespace std;

bsread::Sender::Sender(zmq::context_t &ctx, string address, int sndhwm, int sock_type, int linger):
        m_compress_buffer_size(0),
        m_ctx(ctx),
        m_sock(m_ctx, sock_type),
        m_address(address.c_str())
{
    m_sock.setsockopt(ZMQ_LINGER, &linger,sizeof(linger));
    m_sock.setsockopt(ZMQ_SNDHWM, &sndhwm,sizeof(sndhwm));
    m_sock.bind(address.c_str());
}

bsread::Sender::~Sender() {}

size_t bsread::Sender::send_message(bsread::Message &message){
    size_t msg_len=0;
    size_t part_len;

    //Send main header
    const string* mainheader = message.get_main_header();
    part_len = m_sock.send(mainheader->c_str(),mainheader->length(),ZMQ_SNDMORE|ZMQ_NOBLOCK);
    if(!part_len) return 0;
    msg_len+=part_len;


    //Send dataheader
    const string* dataheader = message.get_data_header(true);
    part_len = m_sock.send(dataheader->c_str(),dataheader->length(),ZMQ_SNDMORE|ZMQ_NOBLOCK);
    if(!part_len) return 0;
    msg_len+=part_len;

    //Send data for each channel
    const vector<Channel*>* channels = message.get_channels();
    size_t n = channels->size();
    for(size_t i=0; i<n; i++){
        int zmq_flags = ZMQ_SNDMORE | ZMQ_NOBLOCK;
        Channel* chan = channels->at(i);

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
            data_len = chan->acquire_compressed(m_compress_buffer,m_compress_buffer_size);
            if(data_len){
                part_len = m_sock.send(m_compress_buffer,data_len,zmq_flags);
                msg_len+=part_len;
            }
            else{ //Compressing disabled
                data = chan->acquire();
                data_len = chan->get_len();

                part_len = m_sock.send(data,data_len,zmq_flags);
                msg_len+=part_len;
            }


            //Send timestamp, take care of last part
            if(i==n-1) zmq_flags &= ~ZMQ_SNDMORE;
            part_len = m_sock.send(rtimestamp,sizeof(rtimestamp),zmq_flags);

            msg_len+=part_len;

        }
            //Not enabled channels are replaced with empty submessages
        else{
            m_sock.send(NULL,0,ZMQ_SNDMORE | ZMQ_NOBLOCK);

            //Last part
            if(i==n-1) zmq_flags &= ~ZMQ_SNDMORE;
            part_len = m_sock.send(NULL,0,zmq_flags);

        }

    }

    return msg_len;
}

