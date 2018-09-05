#include "compression.h"

#include <stdexcept>

extern "C"{
    #include "lz4.h"
    #include "bitshuffle.h"
}

using namespace std;

#define is_little_endian htonl(1) != 1

size_t bsread::get_compression_buffer_size(compression_type compression, size_t n_elements, size_t element_size) {
    switch (compression) {

        case compression_lz4: {
            size_t n_bytes = n_elements * element_size;
            return static_cast<size_t>(LZ4_compressBound(n_bytes)) + 4;
        }

        case compression_bslz4:
            return bshuf_compress_lz4_bound(n_elements, element_size, 0) + 12;

        default:
            throw runtime_error("Cannot determine compression buffer size for unknown compression type.");
    }
}

size_t bsread::compress_buffer(compression_type compression, const char* data, size_t n_elements, size_t element_size,
                               char* buffer, size_t buffer_size) {
    switch (compression) {

        case compression_lz4: {
            return compress_lz4(data, n_elements, element_size, buffer, buffer_size);
        }

        case compression_bslz4:
            return compress_bitshuffle(data, n_elements, element_size, buffer);

        default:
            throw runtime_error("Cannot compress with unknown compression type.");
    }
}



 /**
 * @brief compress auxilary function that wraps lz4 so that they are bs compatible (prepends length, etc..)
 *
 * The routine accepts uncompressed data and lvalue pointing to buffer pointer. If buffer_size is 0 than new buffer is alocated. If not,
 * the buffer will be reuesed if possible, otherwise it will be replaced by a larger buffer.
 *
 * @param uncompressed_data
 * @param uncompressed_data_len
 * @param buffer
 * @param buffer_size
 * @param network_order
 * @return
 */
size_t bsread::compress_lz4(const char* data, size_t n_elements, size_t element_size,
                            char* buffer, size_t buffer_size){


    size_t data_len = n_elements * element_size;

    // The bytes should be in big endian (network order).
    if(is_little_endian){
        ((uint32_t*)buffer)[0] = htonl(data_len);
    } else {
        ((uint32_t*)buffer)[0] = data_len;
    }

    size_t compressed_size = LZ4_compress_default(data, &buffer[4], data_len, buffer_size-4);

    if(!compressed_size) throw runtime_error("Error while compressing [LZ4] channel:");
    return compressed_size+4;

}

size_t bsread::compress_bitshuffle(const char* data, size_t n_elements, size_t element_size, char* buffer){

    size_t compressed_size;
    size_t block_size = bshuf_default_block_size(element_size);


    uint64_t uncompressed_data_len = (uint64_t) n_elements * element_size;

    // The system is little endian, convert the 64bit value to big endian (network order).
    if (is_little_endian) {
        uint32_t high_bytes = htonl((uint32_t)(uncompressed_data_len >> 32));
        uint32_t low_bytes = htonl((uint32_t)(uncompressed_data_len & 0xFFFFFFFFLL));
        uncompressed_data_len = (((uint64_t)low_bytes) << 32) | high_bytes;
    }

    ((int64_t*)buffer)[0] = uncompressed_data_len;

    // The block size has to be multiplied by the elm_size before inserting it into the binary header.
    // https://github.com/kiyo-masui/bitshuffle/blob/04e58bd553304ec26e222654f1d9b6ff64e97d10/src/bshuf_h5filter.c#L167
    uint32_t header_block_size = (uint32_t) block_size * element_size;

    ((int32_t*)buffer)[2] = htonl(header_block_size);

    compressed_size = bshuf_compress_lz4(data, &buffer[12], n_elements, element_size, block_size);

    if(!compressed_size) throw runtime_error("Error while compressing [LZ4] channel:");
    return compressed_size+12;
}