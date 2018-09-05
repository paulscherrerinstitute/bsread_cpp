#ifndef LIB_BSREAD_CONSTANTS_H
#define LIB_BSREAD_CONSTANTS_H

#include <map>
#include <string>

namespace bsread {

    // 200 characters for 200 channels.
    static const size_t MAX_DATA_HEADER_LEN = 200 * 200;

    enum compression_type {
        compression_none,
        compression_lz4,
        compression_bslz4
    };

    static const std::string compression_type_name[] = {
            "none",
            "lz4",
            "bitshuffle_lz4"
    };

    enum endianess {
        little,
        big
    };

    static const std::string endianess_name[] = {
            "little",
            "big",
    };

    struct timestamp{

        /**
         * Constructor
         */
        timestamp(void):
                sec(0),
                nsec(0)
        {};

        /**
         * @brief sec seconds past UNIX epoch (1/1/1970)
         */
        uint64_t sec;

        /**
         * @brief ns nanosecond offset since last full second
         */
        uint64_t nsec;
    };

    struct channel_data{

        // Empty channel_data can be sent over ZMQ as an empty message.
        channel_data():
                data(nullptr),
                data_len(0),
                timestamp(nullptr),
                timestamp_len(0)
        {}

        // Channel data can also be sent without a timestamp.
        channel_data(void* data, size_t data_len) : data(data), data_len(data_len),
                                                    timestamp(nullptr), timestamp_len(0)
        {}

        channel_data(void* data, size_t data_len, void* timestamp, size_t timestamp_len):
                data(data),
                data_len(data_len),
                timestamp(timestamp),
                timestamp_len(timestamp_len)
        {}

        void* data;
        size_t data_len;
        void* timestamp;
        size_t timestamp_len;
    };


    enum bsdata_type {BSDATA_STRING,
        BSDATA_BOOl,
        BSDATA_FLOAT64,
        BSDATA_FLOAT32,
        BSDATA_INT8,
        BSDATA_UINT8,
        BSDATA_INT16,
        BSDATA_UINT16,
        BSDATA_INT32,
        BSDATA_UINT32,
        BSDATA_INT64,
        BSDATA_UINT64};

    static const size_t bsdata_type_size[] = {  1,
                                                1,
                                                8,
                                                4,
                                                1,
                                                1,
                                                2,
                                                2,
                                                4,
                                                4,
                                                8,
                                                8
    };

    static const char* const bsdata_type_name[] = {
            "string",
            "bool",
            "float64",
            "float32",
            "int8",
            "uint8",
            "int16",
            "uint16",
            "int32",
            "uint32",
            "int64",
            "uint64"};
}


#endif //LIB_BSREAD_CONSTANTS_H
