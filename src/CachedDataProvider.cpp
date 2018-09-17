#include "CachedDataProvider.h"

using namespace std;
using namespace bsread;

bsread::CachedDataProvider::CachedDataProvider(void* data, size_t data_len) :
        DirectDataProvider(data, data_len)
{
    m_data_cache.reset(new char[data_len]);
}

void bsread::CachedDataProvider::cache_data() {
    memcpy(m_data_cache.get(), m_data, m_data_len);
}

channel_data bsread::CachedDataProvider::get_data() {
    return {m_data_cache.get(), m_data_len};
}