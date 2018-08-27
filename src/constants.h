#ifndef LIB_BSREAD_CONSTANTS_H
#define LIB_BSREAD_CONSTANTS_H

namespace bsread {

    #define BSREAD_MAIN_HEADER_VERSION "bsr_m-1.1"
    #define BSREAD_DATA_HEADER_VERSION "bsr_d-1.1"

    enum bsdata_compression_type{
        compression_none,
        compression_lz4,
        compression_bslz4
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
}


#endif //LIB_BSREAD_CONSTANTS_H
