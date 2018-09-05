#ifndef LIB_BSREAD_BSDATACHANNEL_H
#define LIB_BSREAD_BSDATACHANNEL_H

#include <vector>
#include <string>
#include <memory>

#include "json.h"
#include "constants.h"
#include "DataProvider.h"

namespace bsread {

    class Channel {

    protected:
        const std::string m_name;
        std::unique_ptr<bsread::DataProvider> m_data_provider;
        const bsdata_type m_type;
        const size_t m_type_size;
        const std::vector<size_t> m_shape;
        const endianess m_endianess;
        const std::string m_endianess_name;
        const compression_type m_compression;
        const std::string m_compression_name;
        const int m_modulo;
        const int m_offset;


    public:
        Channel(const std::string &name, std::unique_ptr<DataProvider> data_provider, bsread::bsdata_type type,
                std::vector<size_t> shape, endianess endian, compression_type compression=compression_none,
                int modulo=1, int offset=0);

        std::string get_name() const;

        Json::Value get_channel_data_header() const;

        channel_data get_data_for_pulse_id(uint64_t pulse_id);

    private:
        bool is_enabled_for_pulse_id(uint64_t pulse_id) const;

        std::unique_ptr<char[]> m_compression_buffer;
        size_t m_compression_buffer_len;

        size_t m_n_elements;
    };
}

#endif //LIB_BSREAD_BSDATACHANNEL_H
