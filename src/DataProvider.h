#ifndef BSREAD_DATAPROVIDER_H
#define BSREAD_DATAPROVIDER_H

#include "constants.h"

namespace bsread {
    class DataProvider {
    public:
        virtual ~DataProvider() = 0;
        virtual bsread::channel_data get_data() = 0;
    };

    inline DataProvider::~DataProvider() {}
};

#endif //BSREAD_DATAPROVIDER_H
