#include "DummyReceiver.h"

#include <iostream>

using namespace std;
using namespace bsread;

DummyReceiver::DummyReceiver(string address, int rcvhwm, int sock_type) :
        m_ctx(1),
        m_sock(m_ctx, sock_type),
        m_address(address)
{
    m_sock.setsockopt(ZMQ_RCVHWM, &rcvhwm, sizeof(rcvhwm));
    m_sock.connect(address.c_str());
}

bsread_message DummyReceiver::receive() {
    zmq::message_t msg;
    int more;
    size_t more_size = sizeof(more);

    m_sock.recv(&msg);
    m_sock.getsockopt(ZMQ_RCVMORE, &more, &more_size);
    if (!more) throw runtime_error("Data header expected after main header.");

    auto main_header = get_main_header(msg.data(), msg.size());

    m_sock.recv(&msg);
    m_sock.getsockopt(ZMQ_RCVMORE, &more, &more_size);
    auto data_header = get_data_header(msg.data(), msg.size(), main_header->dh_compression);

    auto channels_value = make_shared<unordered_map<string, data_channel_value>>();

    for (auto& channel_header : data_header->channels) {

        if (!more) throw runtime_error("Invalid message format. The multipart message terminated prematurely.");

        m_sock.recv(&msg);
        m_sock.getsockopt(ZMQ_RCVMORE, &more, &more_size);
        auto channel_value = get_channel_data(msg.data(), msg.size(), channel_header.compression);

        if (!more) throw runtime_error("Invalid message format. The multipart message terminated prematurely.");

        m_sock.recv(&msg);
        m_sock.getsockopt(ZMQ_RCVMORE, &more, &more_size);
        channel_value.timestamp = get_channel_timestamp(msg.data(), msg.size());

        channels_value->emplace(channel_header.name, channel_value);
    }

    if (more) throw runtime_error("Invalid message format. The multipart message has too many parts. Check sender.");

    return bsread_message(main_header, data_header, channels_value);
}


std::shared_ptr<main_header> DummyReceiver::get_main_header(void* data, size_t data_len) {

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

std::shared_ptr<data_header> DummyReceiver::get_data_header(void* data, size_t data_len, compression_type compression) {

    if (compression != compression_none) {
        throw runtime_error("Data header de-compression not implemented yet. Use compression 'none'.");
    }

    Json::Value root;
    auto json_string = string(static_cast<char*>(data), data_len);
    json_reader.parse(json_string, root);

    auto data_header = make_shared<bsread::data_header>();
    data_header->htype = root["htype"].asString();

    for(Json::Value& channel : root["channels"]) {

        data_channel_header channel_definition;
        channel_definition.name = channel["name"].asString();
        channel_definition.type = bsdata_type_mapping.at(channel.get("type", "float64").asString());

        for(Json::Value& dimension : channel.get("shape", {1})) {
            channel_definition.shape.push_back(dimension.asUInt());
        }

        channel_definition.compression = compression_type_mapping.at(channel.get("compression", "none").asString());
        channel_definition.endianess = endianess_mapping.at(channel.get("encoding", "little").asString());
        channel_definition.modulo = channel.get("modulo", 1).asUInt();
        channel_definition.offset = channel.get("offset", 0).asUInt();

        data_header->channels.push_back(channel_definition);
    }

    return data_header;
}

data_channel_value DummyReceiver::get_channel_data(void* data, size_t data_len, compression_type compression) {
    if (data_len == 0) {
        return data_channel_value(nullptr, 0);
    }

    if (compression != compression_none) {
        throw runtime_error("Data de-compression not implemented yet. Use compression 'none'.");
    }

    char* buffer_data = new char[data_len];
    memcpy(buffer_data, data, data_len);

    return data_channel_value(buffer_data, data_len);
}

shared_ptr<timestamp> DummyReceiver::get_channel_timestamp(void* data, size_t data_len) {
    if (data_len == 0) {
        return nullptr;
    }

    if (data_len != sizeof(timestamp)) {
        throw runtime_error("Timestamp length incorrect. Check sender.");
    }

    auto channel_timestamp = static_cast<uint64_t*>(data);

    return make_shared<timestamp>(channel_timestamp[0], channel_timestamp[1]);
}