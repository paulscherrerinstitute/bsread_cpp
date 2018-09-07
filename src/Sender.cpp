#include "Sender.h"

#include "md5.h"
#include "compression.h"

using namespace std;

bsread::Sender::Sender(string address, int sndhwm, int sock_type, int linger,
                       compression_type data_header_compression, int n_io_threads):
        m_ctx(n_io_threads),
        m_sock(m_ctx, sock_type),
        m_address(address),
        m_sending_enabled(true),
        m_dh_compression(data_header_compression),
        m_dh_compression_name(compression_type_name[m_dh_compression])
{
    m_sock.setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
    m_sock.setsockopt(ZMQ_SNDHWM, &sndhwm, sizeof(sndhwm));
    m_sock.bind(address.c_str());

    if (m_dh_compression) {
        m_dh_compression_buffer_len = get_compression_buffer_size(m_dh_compression, MAX_DATA_HEADER_LEN, sizeof(char));
        m_dh_compression_buffer.reset(new char[m_dh_compression_buffer_len]);
    }
}

bsread::Sender::~Sender() = default;

size_t bsread::Sender::send_channel(Channel& channel, uint64_t pulse_id, bool last_channel) {

    auto channel_data = channel.get_data_for_pulse_id(pulse_id);

    size_t msg_len=0;
    size_t part_len=0;

    part_len = m_sock.send(channel_data.data, channel_data.data_len, ZMQ_SNDMORE|ZMQ_NOBLOCK);

    if (!part_len) return 0;
    msg_len += part_len;

    if (last_channel) {
        part_len = m_sock.send(channel_data.timestamp, channel_data.timestamp_len, ZMQ_NOBLOCK);
    } else {
        part_len = m_sock.send(channel_data.timestamp, channel_data.timestamp_len, ZMQ_SNDMORE | ZMQ_NOBLOCK);
    }
    if (!part_len) return 0;
    msg_len += part_len;

    return msg_len;
}

size_t bsread::Sender::send_message(const uint64_t pulse_id, const bsread::timestamp global_timestamp){

    lock_guard<std::recursive_mutex> lock(m_sender_lock);

    if (!m_sending_enabled) {
        return 0;
    }

    size_t msg_len=0;
    size_t part_len=0;

    // We have to construct the main header each time, because it contains the pulse_id and global timestamp.
    auto mainheader = get_main_header(pulse_id, global_timestamp);
    part_len = m_sock.send(mainheader.c_str(), mainheader.length(), ZMQ_SNDMORE|ZMQ_NOBLOCK);

    if (!part_len) return 0;
    msg_len += part_len;

    auto dataheader = get_data_header();
    part_len = m_sock.send(dataheader.c_str(), dataheader.length(), ZMQ_SNDMORE|ZMQ_NOBLOCK);

    if (!part_len) return 0;
    msg_len += part_len;

    size_t n_channels = m_channels.size();
    for(size_t i=0; i<n_channels; i++){

        bool last_channel = (i == n_channels-1);
        auto channel = m_channels.at(i);

        part_len = send_channel(*channel, pulse_id, last_channel);

        if (!part_len) return 0;
        msg_len += part_len;
    }

    return msg_len;
}



void bsread::Sender::build_data_header(){
    lock_guard<std::recursive_mutex> lock(m_sender_lock);

    Json::Value root;
    root["htype"] = BSREAD_DATA_HEADER_VERSION;

    for(size_t i=0; i<m_channels.size(); i++){
        root["channels"][(int)i]=m_channels[i]->get_channel_data_header();
    }

    m_data_header = m_writer.write(root);

    if (m_dh_compression) {
        size_t compressed_size = compress_buffer(m_dh_compression,
                                                 m_data_header.c_str(),
                                                 m_data_header.size(),
                                                 sizeof(char),
                                                 m_dh_compression_buffer.get(),
                                                 m_dh_compression_buffer_len);

        m_data_header = string(m_dh_compression_buffer.get(), compressed_size);
    }

    m_data_header_hash = md5(m_data_header);
}

const std::string& bsread::Sender::get_data_header(){
    if (m_data_header.empty()) {
        build_data_header();
    }

    return m_data_header;
}

const string& bsread::Sender::get_data_header_hash() {
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
    root["dh_compression"] = m_dh_compression_name;
    root["hash"] = get_data_header_hash();

    return m_writer.write(root);
}

void bsread::Sender::set_sending_enabled(bool enable) {
    lock_guard<std::recursive_mutex> lock(m_sender_lock);

    m_sending_enabled = enable;
}

void bsread::Sender::add_channel(shared_ptr<Channel> channel){
    lock_guard<std::recursive_mutex> lock(m_sender_lock);

    m_channels.push_back(move(channel));

    // A new data header needs to be constructed when a new channel is added.
    m_data_header.clear();
    m_data_header_hash.clear();
}
