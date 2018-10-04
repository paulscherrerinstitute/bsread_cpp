#ifndef BSREAD_COMPRESSION_H
#define BSREAD_COMPRESSION_H

#include <cstddef>
#include <cstdint>
#include "constants.h"

namespace bsread{

    size_t get_compression_buffer_size(compression_type compression, size_t n_elements, size_t element_size);

    size_t compress_buffer(compression_type compression, const char* data, size_t n_elements, size_t element_size,
                           char* buffer, size_t buffer_size);

    size_t compress_lz4(const char* data, size_t n_elements, size_t element_size,
                        char* buffer, size_t buffer_size);

    size_t decompress_lz4(const char* compressed_data, size_t compressed_size, char* data);

    size_t compress_bitshuffle(const char* data, size_t n_elements, size_t element_size, char* buffer);
};

#endif //BSREAD_COMPRESSION_H
