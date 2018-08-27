#include "compression.h"

#include <stdexcept>

extern "C"{
    #include "lz4.h"
    #include "bitshuffle.h"
}

using namespace std;

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
size_t bsread::compress_lz4(const char* uncompressed_data, int32_t uncompressed_data_len, char*& buffer, size_t& buffer_size, bool network_order){

    size_t compressed_size;

    // Ensure output buffer is large enough
    if(buffer_size < (size_t)(LZ4_compressBound(uncompressed_data_len)+4) ){
        // Free existing buffer if it exists
        if(buffer_size) free(buffer);
        //New output buffer
        buffer_size = LZ4_compressBound(uncompressed_data_len)+4;
        buffer = (char*) malloc(buffer_size);
    }

    //Set the uncompressed blob length
    if(network_order){
        ((int32_t*)buffer)[0] = htonl(uncompressed_data_len);
    }
    else{
        ((int32_t*)buffer)[0] = uncompressed_data_len;
    }

    //Compress the data
    compressed_size = LZ4_compress_default((const char*)uncompressed_data,&buffer[4],uncompressed_data_len,buffer_size-4);

    if(!compressed_size) throw runtime_error("Error while compressing [LZ4] channel:");
    return compressed_size+4;

}

size_t bsread::compress_bitshuffle(const char* uncompressed_data, size_t nelm, size_t elm_size, char*& buffer, size_t& buffer_size){

    size_t compressed_size;
    size_t block_size = bshuf_default_block_size(elm_size);
    size_t buf_min_size=bshuf_compress_lz4_bound(nelm,elm_size,0)+12; //12byte header at the start

    // Ensure output buffer is large enough
    if(buffer_size < buf_min_size ){
        // Free existing buffer if it exists
        if(buffer_size) free(buffer);
        //New output buffer
        buffer_size = buf_min_size;
        buffer = (char*) malloc(buffer_size);
    }

    uint64_t uncompressed_data_len = (uint64_t) nelm*elm_size;

    // The system is little endian, convert the 64bit value to big endian (network order).
    if (htonl(1) != 1) {
        uint32_t high_bytes = htonl((uint32_t)(uncompressed_data_len >> 32));
        uint32_t low_bytes = htonl((uint32_t)(uncompressed_data_len & 0xFFFFFFFFLL));
        uncompressed_data_len = (((uint64_t)low_bytes) << 32) | high_bytes;
    }

    //Set the uncompressed blob length
    ((int64_t*)buffer)[0] = uncompressed_data_len;

    //The block size has to be multiplied by the elm_size before inserting it into the binary header.
    //https://github.com/kiyo-masui/bitshuffle/blob/04e58bd553304ec26e222654f1d9b6ff64e97d10/src/bshuf_h5filter.c#L167
    uint32_t header_block_size = (uint32_t) block_size * elm_size;

    //Set the subblock size length
    ((int32_t*)buffer)[2] = htonl(header_block_size);

    //Compress the data
    compressed_size = bshuf_compress_lz4((const char*)uncompressed_data,&buffer[12],nelm,elm_size,block_size);

    if(!compressed_size) throw runtime_error("Error while compressing [LZ4] channel:");
    return compressed_size+12;

    return 0;
}