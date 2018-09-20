#include "ZeroCopySender.h"
#include <sstream>
#include <iostream>

using namespace std;
using namespace bsread;

ZeroCopySender::ZeroCopySender(string& address, int sock_type, int linger,
                               compression_type data_header_compression, int n_io_threads,
                               size_t max_header_len, size_t max_data_header_len):
        Sender(address, 1, sock_type, linger, data_header_compression, n_io_threads)
{
    m_header_buffer_len = max_header_len;
    m_header_buffer.reset(new char[max_header_len]);
    m_header_buffer_free.store(true);

    m_data_header_buffer_len = max_data_header_len;
    m_data_header_buffer.reset(new char[max_data_header_len]);
    m_data_header_buffer_free.store(true);
}

size_t bsread::ZeroCopySender::send_channel(channel_data& channel_data, bool last_channel,
                                            zmq_free_fn free_buffer_func,
                                            void* free_buffer_hint) {

    size_t msg_len=0;
    bool sent;

    zmq::message_t data_message(channel_data.data, channel_data.data_len, free_buffer_func, free_buffer_hint);
    sent = m_sock.send(data_message, ZMQ_SNDMORE|ZMQ_NOBLOCK);

    if (!sent) return 0;
    msg_len += channel_data.data_len;

    zmq::message_t timestamp_message(channel_data.timestamp, channel_data.timestamp_len,
                                     free_buffer_func, free_buffer_hint);

    if (last_channel) {
        sent = m_sock.send(data_message, ZMQ_NOBLOCK);
    } else {
        sent = m_sock.send(data_message, ZMQ_SNDMORE|ZMQ_NOBLOCK);
    }
    if (!sent) return 0;
    msg_len += channel_data.timestamp_len;

    return msg_len;
}

bsread::send_status bsread::ZeroCopySender::send_message(const uint64_t pulse_id,
                                                         const bsread::timestamp global_timestamp,
                                                         zmq_free_fn free_buffer_func,
                                                         void* free_buffer_hint){
    lock_guard<std::recursive_mutex> lock(m_sender_lock);

    size_t n_channels = m_channels.size();

    if (!m_sending_enabled || !n_channels) {
        return SKIPPED;
    }

    if (!m_header_buffer_free.load() || !m_data_header_buffer_free.load()) {
        return BUSY;
    }

    // We have to construct the main header each time, because it contains the pulse_id and global timestamp.
    auto mainheader = get_main_header(pulse_id, global_timestamp);
    if (mainheader.length() >= m_header_buffer_len) {
        stringstream error_message;
        error_message << "Main header does not fit into the allocated buffer. ";
        error_message << "Increase max_header_len (current: " << m_header_buffer_len << ") ";
        error_message << "to fit header of length " << mainheader.length() << ".";

        throw runtime_error(error_message.str());
    }
    memcpy(m_header_buffer.get(), mainheader.c_str(), mainheader.length());
    m_header_buffer_free.store(false);

    auto dataheader = get_data_header();
    if (dataheader.length() >= m_data_header_buffer_len) {
        m_header_buffer_free.store(true);

        stringstream error_message;
        error_message << "Data header does not fit into the allocated buffer. ";
        error_message << "Increase max_data_header_len (current: " << m_data_header_buffer_len << ") ";
        error_message << "to fit data header of length " << dataheader.length() << ".";

        throw runtime_error(error_message.str());
    }
    memcpy(m_data_header_buffer.get(), dataheader.c_str(), dataheader.length());
    m_data_header_buffer_free.store(false);


    size_t msg_len = 0;
    size_t part_len = 0;
    bool sent;

    zmq::message_t header_message(m_header_buffer.get(), mainheader.length(), release_buffer, &m_header_buffer_free);
    sent = m_sock.send(header_message, ZMQ_SNDMORE|ZMQ_NOBLOCK);

    if (!sent) {
        m_header_buffer_free.store(true);
        m_data_header_buffer_free.store(true);

        return FAILED;
    }
    msg_len += mainheader.length();

    zmq::message_t data_header_message(m_data_header_buffer.get(), dataheader.length(),
                                       release_buffer, &m_data_header_buffer_free);
    sent = m_sock.send(data_header_message, ZMQ_SNDMORE|ZMQ_NOBLOCK);

    if (!sent) {
        m_header_buffer_free.store(true);
        m_data_header_buffer_free.store(true);

        return FAILED;
    }
    msg_len += dataheader.length();

    for(size_t i=0; i<n_channels; i++){

        bool last_channel = (i == n_channels-1);
        auto channel_data = m_channels.at(i)->get_data_for_pulse_id(pulse_id);

        part_len = send_channel(channel_data, last_channel, free_buffer_func, free_buffer_hint);

        if (part_len != (channel_data.data_len + channel_data.timestamp_len)) {
            m_header_buffer_free.store(true);
            m_data_header_buffer_free.store(true);

            return FAILED;
        }

        msg_len += part_len;
    }

    return SENT;
}

void ZeroCopySender::release_buffer(void* data, void* buffer_lock){
    auto buffer_free = static_cast<atomic<bool>*>(buffer_lock);
    buffer_free->store(true);
}