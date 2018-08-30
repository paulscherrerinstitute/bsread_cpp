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

namespace bsread{

    /**
     * @brief The BSDataSenderZmq class
     *
     * Class that takes BSData messages and transmits them via ZMQ socket
     * By default the class will create its own context and socket. A static
     * method exists that perorms the same, but requires ZMQ socket to be passed
     */
    class Sender{

    protected:
        char* m_compress_buffer;
        size_t m_compress_buffer_size;
        zmq::context_t& m_ctx;
        zmq::socket_t m_sock;
        const std::string m_address;

        const bsdata_compression_type m_data_header_compression;
        // String representation of the data header compression (for main header)
        const std::string m_data_header_compression_name;

        std::string m_data_header;
        std::string m_data_header_hash;

        virtual void build_data_header();

        virtual const std::string get_main_header(uint64_t pulse_id, timestamp global_timestamp);
        virtual const std::string& get_data_header();
        virtual const std::string& bsread::Sender::get_data_header_hash();

        std::vector<Channel*> m_channels;

        Json::FastWriter m_writer;
        std::recursive_mutex m_data_lock;

    public:

        /**
         * @brief BSDataSenderZmq Holds all infrastructre needed for BSREAD, this includes zmq context and zmq socket.
         * Parameters passed to constructor are used to create this infrastructure.
         * @param address
         * @param sndhwm
         * @param sock_type
         * @param linger
         */
        Sender(zmq::context_t& ctx, std::string address, int sndhwm=10, int sock_type=ZMQ_PUSH, int linger=1000,
               bsdata_compression_type data_header_compression=compression_none);

        virtual ~Sender();

        virtual size_t send_message(const uint64_t pulse_id, const bsread::timestamp);
    };
}

#endif //LIB_BSREAD_BSDATASENDERZMQ_H
