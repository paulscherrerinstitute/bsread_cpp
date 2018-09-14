#ifndef BSREAD_DATAPROVIDER_H
#define BSREAD_DATAPROVIDER_H

#include "constants.h"

namespace bsread {

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

    class DataProvider {
    public:
        virtual ~DataProvider() = 0;
        virtual bsread::channel_data get_data() = 0;
    };

    inline DataProvider::~DataProvider() {}
};

#endif //BSREAD_DATAPROVIDER_H
