#ifndef BSREAD_RECEIVER_H
#define BSREAD_RECEIVER_H

#include <zmq.hpp>
#include <string>
#include <memory>
#include "json.h"

#include "../src/constants.h"

namespace bsread {

    struct main_header {
        compression_type dh_compression;
        timestamp global_timestamp;
        std::string hash;
        std::string htype;
        uint64_t pulse_id;
    };

    struct bsread_message {
        bsread_message(std::shared_ptr<main_header> main_header): main_header(main_header) {};

        std::shared_ptr<main_header> main_header;
    };

    class DummyReceiver {

        zmq::context_t m_ctx;
        zmq::socket_t m_sock;
        std::string m_address;

        Json::Reader json_reader;

    public:
        DummyReceiver(std::string address, int rcvhwm=10, int sock_typ=ZMQ_PULL);
        std::shared_ptr<bsread::bsread_message> receive();
        virtual ~DummyReceiver() = default;

    private:
        std::shared_ptr<main_header> get_main_header(void* data, size_t data_len);
    };
}

#endif //BSREAD_RECEIVER_H
