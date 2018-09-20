#include "CacheManager.h"
#include <utility>

using namespace std;
using namespace bsread;

CacheManager::CacheManager() : m_current_ref_count(0)
{}

CacheManager::CacheManager(vector<shared_ptr<CachedDataProvider>> data_providers) :
        m_current_ref_count(0), m_data_providers(move(data_providers))
{}

void CacheManager::add_data_provider(std::shared_ptr<CachedDataProvider> data_provider) {
    lock_guard<mutex> lock(m_ref_count_lock);
    m_data_providers.push_back(data_provider);
}

bool CacheManager::cache_all() {
    lock_guard<mutex> lock(m_ref_count_lock);

    // We cannot cache the memory until everything is released.
    if (m_current_ref_count > 0) {
        return false;
    }

    for (auto& data_provider : m_data_providers) {
        data_provider->cache_data();

        auto data = data_provider->get_data();

        if (data.data != nullptr) {
            m_current_ref_count++;
        }

        if (data.timestamp != nullptr) {
            m_current_ref_count++;
        }
    }

    return true;
}

void CacheManager::release_cache(void* data, void* manager) {
    auto cache_manager = static_cast<CacheManager*>(manager);

    if (cache_manager == nullptr) {
        throw runtime_error("CacheManager is nullptr. Pass it to the send_message as free_buffer_hint.");
    }

    // We can receive null pointers in case of empty ZMQ messages.
    if (data != nullptr){
        cache_manager->decrease_ref_count();
    }
}

void CacheManager::decrease_ref_count() {
    lock_guard<mutex> lock(m_ref_count_lock);

    m_current_ref_count--;

    if (m_current_ref_count < 0) {
        throw runtime_error("Cache reference count below zero?! Memory corrupted.");
    }
}

void CacheManager::release_all() {
    lock_guard<mutex> lock(m_ref_count_lock);

    m_current_ref_count = 0;
}