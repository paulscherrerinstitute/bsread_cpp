#include "Sender.h"

using namespace std;

bsread::Sender::Sender(zmq::context_t &ctx, string address, int sndhwm, int sock_type, int linger,
                       bsdata_compression_type data_header_compression):
        m_compress_buffer_size(0),
        m_ctx(ctx),
        m_sock(m_ctx, sock_type),
        m_address(address.c_str()),
        m_data_header_compression(data_header_compression),
        m_data_header_compression_name(compression_names[m_data_header_compression])
{
    m_sock.setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
    m_sock.setsockopt(ZMQ_SNDHWM, &sndhwm, sizeof(sndhwm));
    m_sock.bind(address.c_str());
}

bsread::Sender::~Sender() {}

size_t bsread::Sender::send_message(const uint64_t pulse_id, const bsread::timestamp global_timestamp,
                                    const bsread::Message &message){

    lock_guard<std::recursive_mutex> lock(m_data_lock);

    size_t msg_len=0;
    size_t part_len;

    //Send main header
    auto mainheader = get_main_header(pulse_id, global_timestamp);
    part_len = m_sock.send(mainheader.c_str(),mainheader.length(),ZMQ_SNDMORE|ZMQ_NOBLOCK);
    if(!part_len) return 0;
    msg_len+=part_len;


    //Send dataheader
    auto dataheader = get_data_header();
    part_len = m_sock.send(dataheader.c_str(),dataheader.length(), ZMQ_SNDMORE|ZMQ_NOBLOCK);
    if(!part_len) return 0;
    msg_len+=part_len;

    //Send data for each channel
    size_t n_channels = m_channels.size();
    for(size_t i=0; i<n_channels; i++){

        auto chan = m_channels.at(i);

        //Only send enabled channels
        if(!chan->get_enabled()) {

        }
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



void bsread::Sender::build_data_header(){
    lock_guard<std::recursive_mutex> lock(m_data_lock);
    //TODO: Build data header.
}

virtual const std::string& bsread::Sender::get_data_header(){
    if (m_data_header.empty()) {
        build_data_header();
    }

    return m_data_header;
}

virtual const std::string& bsread::Sender::get_data_header_hash(){
    if (m_data_header.empty()) {
        build_data_header();
    }

    return m_data_header_hash;
}

const string bsread::Sender::get_main_header(uint64_t pulse_id, timestamp global_timestamp){
    Json::Value root;

    root["htype"] = BSREAD_MAIN_HEADER_VERSION;
    root["pulse_id"] = static_cast<Json::Int64>(pulse_id);
    root["global_timestamp"]["sec"] = static_cast<Json::Int64>(global_timestamp.sec);
    root["global_timestamp"]["ns"] = static_cast<Json::Int64>(global_timestamp.nsec);
    root["dh_compression"] = m_data_header_compression_name;
    root["hash"] = get_data_header_hash();

    return m_writer.write(root);
}