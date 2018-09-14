#include "DummyReceiver.h"

#include <iostream>

using namespace std;

bsread::DummyReceiver::DummyReceiver(string address, int rcvhwm, int sock_type) :
        m_ctx(1),
        m_sock(m_ctx, sock_type),
        m_address(address)
{
    m_sock.setsockopt(ZMQ_RCVHWM, &rcvhwm, sizeof(rcvhwm));
    m_sock.connect(address.c_str());
}

shared_ptr<bsread::bsread_message> bsread::DummyReceiver::receive() {
    zmq::message_t msg;
    int more;
    size_t more_size = sizeof(more);

    m_sock.recv(&msg);
    m_sock.getsockopt(ZMQ_RCVMORE, &more, &more_size);
    if (!more) throw runtime_error("Data header expected after main header.");

    auto main_header = get_main_header(msg.data(), msg.size());

    m_sock.recv(&msg);
    m_sock.getsockopt(ZMQ_RCVMORE, &more, &more_size);
    auto data_header = get_data_header(msg.data(), msg.size());

    while (more) {
        m_sock.recv(&msg);
        m_sock.getsockopt(ZMQ_RCVMORE, &more, &more_size);
    }

    return make_shared<bsread::bsread_message>(main_header, data_header);
}


std::shared_ptr<bsread::main_header> bsread::DummyReceiver::get_main_header(void* data, size_t data_len) {

    Json::Value root;
    auto json_string = string(static_cast<char*>(data), data_len);
    json_reader.parse(json_string, root);

    auto main_header = make_shared<bsread::main_header>();
    main_header->pulse_id = root["pulse_id"].asUInt64();
    main_header->dh_compression = compression_type_mapping.at(root["dh_compression"].asString());
    main_header->hash = root["htype"].asString();
    main_header->htype = root["htype"].asString();
    main_header->global_timestamp = timestamp(root["global_timestamp"]["sec"].asUInt64(),
                                              root["global_timestamp"]["ns"].asUInt64());

    return main_header;
}

std::shared_ptr<bsread::data_header> bsread::DummyReceiver::get_data_header(void* data, size_t data_len) {
    Json::Value root;
    auto json_string = string(static_cast<char*>(data), data_len);
    json_reader.parse(json_string, root);

    auto data_header = make_shared<bsread::data_header>();
    data_header->htype = root["htype"].asString();

    // Even if your IDE shows an error, it actually works.
    for(auto& channel : root["channels"]) {

        data_channel channel_definition;
        channel_definition.name = channel["name"].asString();
        channel_definition.type = bsdata_type_mapping.at(channel.get("type", "float64").asString());

        for(auto& dimension : channel.get("shape", {1})) {
            channel_definition.shape.push_back(dimension.asUInt());
        }

        channel_definition.compression = compression_type_mapping.at(channel.get("compression", "none").asString());
        channel_definition.endianess = endianess_mapping.at(channel.get("endianess", "little").asString());
        channel_definition.modulo = channel.get("modulo", 1).asUInt();
        channel_definition.offset = channel.get("offset", 0).asUInt();

        data_header->channels[channel_definition.name] = channel_definition;
    }

    return data_header;
}