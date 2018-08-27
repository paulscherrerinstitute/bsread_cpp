#ifndef BSREAD_COMPRESSION_H
#define BSREAD_COMPRESSION_H

#include <cstddef>
#include <cstdint>

namespace bsread{
    size_t compress_lz4(const char* uncompressed_data, int32_t uncompressed_data_len, char*& buffer,
                        size_t& buffer_size, bool network_order);

    size_t compress_bitshuffle(const char* uncompressed_data, size_t nelm, size_t elm_size,
                               char*& buffer, size_t& buffer_size);
};


#endif //BSREAD_COMPRESSION_H
