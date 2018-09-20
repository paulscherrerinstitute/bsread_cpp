#ifndef BSREAD_CACHEMANAGER_H
#define BSREAD_CACHEMANAGER_H

#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include "CachedDataProvider.h"

namespace bsread {
    class CacheManager {
    protected:
        std::mutex m_ref_count_lock;
        std::vector<std::shared_ptr<CachedDataProvider>> m_data_providers;
        int m_current_ref_count;

        void decrease_ref_count();

    public:
        CacheManager();
        CacheManager(std::vector<std::shared_ptr<CachedDataProvider>> data_providers);
        void add_data_provider(std::shared_ptr<CachedDataProvider> data_provider);
        bool cache_all();
        void release_all();

        static void release_cache(void* data, void* manager);
    };

}

#endif //BSREAD_CACHEMANAGER_H
