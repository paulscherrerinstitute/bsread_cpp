#ifndef BSREAD_CACHEDDATAPROVIDER_H
#define BSREAD_CACHEDDATAPROVIDER_H

#include "DirectDataProvider.h"
#include <memory>

namespace bsread {

    class CachedDataProvider : public DirectDataProvider {
    protected:
        std::unique_ptr<char[]> m_data_cache;

    public:
        CachedDataProvider(void* data, size_t data_len);
        void cache_data();
        bsread::channel_data get_data() override;
    };
}

#endif //BSREAD_CACHEDDATAPROVIDER_H
