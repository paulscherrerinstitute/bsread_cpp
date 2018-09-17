#ifndef LIB_BSREAD_BSDATASENDERZMQ_H
#define LIB_BSREAD_BSDATASENDERZMQ_H

#include <cstddef>
#include <string>
#include <zmq.hpp>
#include <memory>
#include "json.h"

#include "Channel.h"

#define BSREAD_MAIN_HEADER_VERSION "bsr_m-1.1"
#define BSREAD_DATA_HEADER_VERSION "bsr_d-1.1"

namespace bsread {

    enum send_status {
        FAILED,
        SENT,
        SKIPPED,
        BUSY
    };

    class Sender{

    protected:
        zmq::context_t m_ctx;
        zmq::socket_t m_sock;
        const std::string m_address;
        bool m_sending_enabled;

        const compression_type m_dh_compression;
        // String representation of the data header compression (for main header)
        const std::string m_dh_compression_name;

        std::string m_data_header;
        std::string m_data_header_hash;

        virtual void build_data_header();

        virtual const std::string get_main_header(uint64_t pulse_id, timestamp global_timestamp);
        virtual const std::string& get_data_header();
        virtual const std::string& get_data_header_hash();

        std::vector<std::shared_ptr<Channel>> m_channels;
        virtual size_t send_channel(channel_data& channel_data, bool last_channel);

        Json::FastWriter m_writer;
        std::recursive_mutex m_sender_lock;

    public:

        Sender(std::string address, int sndhwm=10, int sock_type=ZMQ_PUSH, int linger=1000,
               compression_type data_header_compression=compression_none, int n_io_threads=1);

        virtual ~Sender() = default;

        virtual void add_channel(std::shared_ptr<Channel> channel);

        virtual send_status send_message(uint64_t pulse_id, bsread::timestamp global_timestamp);

        virtual void set_sending_enabled(bool enable);

    private:
        std::unique_ptr<char[]> m_dh_compression_buffer;
        size_t m_dh_compression_buffer_len;
    };
}

#endif //LIB_BSREAD_BSDATASENDERZMQ_H
