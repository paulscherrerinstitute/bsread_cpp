#ifndef LIB_BSREAD_CONSTANTS_H
#define LIB_BSREAD_CONSTANTS_H

#include <map>
#include <string>

namespace bsread {

    enum bsdata_compression_type{
        compression_none,
        compression_lz4,
        compression_bslz4
    };

    static const std::map<bsdata_compression_type, std::string> compression_names = {
            {compression_none, "none"},
            {compression_lz4, "lz4"},
            {compression_bslz4, "bitshuffle_lz4"}
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
