#ifndef BSREAD_RECEIVER_H
#define BSREAD_RECEIVER_H

#include <zmq.hpp>
#include <string>

namespace bsread {

    class DummyReceiver {

        zmq::context_t m_ctx;
        zmq::socket_t m_sock;
        std::string m_address;

    public:
        DummyReceiver(std::string address, int rcvhwm=10, int sock_typ=ZMQ_PULL);
        void receive();
        virtual ~DummyReceiver() = default;
    };
}

#endif //BSREAD_RECEIVER_H
