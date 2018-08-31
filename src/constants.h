#ifndef LIB_BSREAD_CONSTANTS_H
#define LIB_BSREAD_CONSTANTS_H

#include <map>
#include <string>

namespace bsread {

    enum compression_type {
        compression_none,
        compression_lz4,
        compression_bslz4
    };

    static const std::map<compression_type, std::string> compression_type_names = {
            {compression_none, "none"},
            {compression_lz4, "lz4"},
            {compression_bslz4, "bitshuffle_lz4"}
    };

    enum endianess {
        little,
        big
    };

    static const std::map<endianess, std::string> endianess_names = {
            {little, "little"},
            {big, "big"}
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

        //Empty channel_data can be sent over ZMQ as an empty message.
        channel_data():
                data(nullptr),
                n_data_bytes(0),
                timestamp(nullptr),
                n_timestamp_bytes(0)
        {}

        void* data;
        size_t n_data_bytes;
        void* timestamp;
        size_t n_timestamp_bytes;
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
