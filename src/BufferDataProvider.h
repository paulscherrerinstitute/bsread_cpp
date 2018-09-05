#ifndef BSREAD_SIMPLEDATAPROVIDER_H
#define BSREAD_SIMPLEDATAPROVIDER_H

#include "DataProvider.h"

namespace bsread {
    class BufferDataProvider: public DataProvider{

    protected:
        void* const m_data;
        const size_t m_data_len;

    public:
        BufferDataProvider(void* data, size_t data_len);
        bsread::channel_data get_data() override;
    };
};

#endif //BSREAD_SIMPLEDATAPROVIDER_H
