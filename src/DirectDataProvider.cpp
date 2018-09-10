#include "DirectDataProvider.h"

using namespace std;
using namespace bsread;

DirectDataProvider::DirectDataProvider(void* data, size_t data_len) :
        m_data(data),
        m_data_len(data_len)
{}

channel_data DirectDataProvider::get_data() {
    return {m_data, m_data_len};
}