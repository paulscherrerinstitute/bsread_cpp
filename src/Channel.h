#ifndef LIB_BSREAD_BSDATACHANNEL_H
#define LIB_BSREAD_BSDATACHANNEL_H

#include <vector>
#include <string>
#include <memory>

#include "json.h"
#include "constants.h"

namespace bsread {

    class Channel {

    protected:
        const std::string m_name;
        const bsdata_type m_type;
        const std::vector<size_t> m_shape;
        const endianess m_endianess;
        const std::string m_endianess_name;
        const compression_type m_compression;
        const std::string m_compression_name;
        const int m_modulo;
        const int m_offset;

    public:
        Channel(const std::string &name, bsread::bsdata_type type, std::vector<size_t> shape,
                endianess endian, compression_type compression=compression_none, int modulo=1, int offset=0);

        std::string get_name() const;

        Json::Value get_channel_data_header() const;

        channel_data get_data_for_pulse_id(uint64_t pulse_id);

    private:
        bool is_enabled_for_pulse_id(uint64_t pulse_id) const;

    };
}

#endif //LIB_BSREAD_BSDATACHANNEL_H
