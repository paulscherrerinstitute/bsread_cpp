#ifndef LIB_BSREAD_BSDATASENDERZMQ_H
#define LIB_BSREAD_BSDATASENDERZMQ_H

#include <cstddef>
#include <string>
#include <zmq.hpp>

#include "BSDataMessage.h"

namespace bsread{
    /**
     * @brief The BSDataSenderZmq class
     *
     * Class that takes BSData messages and transmits them via ZMQ socket
     * By default the class will create its own context and socket. A static
     * method exists that perorms the same, but requires ZMQ socket to be passed
     */
    class BSDataSenderZmq{

    protected:
        char* m_compress_buffer;
        size_t m_compress_buffer_size;
        zmq::context_t& m_ctx;
        zmq::socket_t m_sock;
        std::string m_address;

    public:

        /**
         * @brief BSDataSenderZmq Holds all infrastructre needed for BSREAD, this includes zmq context and zmq socket.
         * Parameters passed to constructor are used to create this infrastructure.
         * @param address
         * @param sndhwm
         * @param sock_type
         * @param linger
         */
        BSDataSenderZmq(zmq::context_t& ctx, std::string address,
                        int sndhwm=10, int sock_type=ZMQ_PUSH, int linger=1000);

        virtual ~BSDataSenderZmq();

        virtual size_t send_message(BSDataMessage& message);
    };
}

#endif //LIB_BSREAD_BSDATASENDERZMQ_H
