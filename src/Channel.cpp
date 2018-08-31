#include "Channel.h"

#include "compression.h"

using namespace std;
using namespace bsread;

bsread::Channel::Channel(const string &name, bsread::bsdata_type type, vector<size_t> shape,
                         endianess endian, compression_type compression, int modulo, int offset):
        m_name(name),
        m_type(type),
        m_endianess(endian),
        m_endianess_name(endianess_names.at(endian)),
        m_compression(compression),
        m_compression_name(compression_type_names.at(compression)),
        m_shape(shape),
        m_modulo(modulo),
        m_offset(offset)
{
    if (m_modulo < 1) {
        throw runtime_error("Modulo cannot be less than 1.");
    }

    if (shape.size() == 0) {
        throw runtime_error("Shape must have at least 1 value.");
    }
}

string bsread::Channel::get_name() const {
    return m_name;
}

Json::Value bsread::Channel::get_channel_data_header() const {
    Json::Value root;
    root["name"] = m_name;
    root["type"]= bsdata_type_name[m_type];
    root["encoding"]= m_endianess_name;
    root["compression"] = m_compression_name;
    root["modulo"] = m_modulo;
    root["offset"] = m_offset;

    for (uint16_t i = 0; i<m_shape.size(); i++) {
        root["shape"][i] = static_cast<int>(m_shape[i]);
    }

    return root;
}

channel_data bsread::Channel::get_data_for_pulse_id(uint64_t pulse_id) {

    if (!is_enabled_for_pulse_id(pulse_id)) {
        return channel_data();
    }

    // TODO: Return the value from the value provider.
    return channel_data();
}

bool bsread::Channel::is_enabled_for_pulse_id(uint64_t pulse_id) const {
    return ((pulse_id-m_offset) % m_modulo) == 0;
}