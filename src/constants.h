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
}


#endif //LIB_BSREAD_CONSTANTS_H
