#include "gtest/gtest.h"
#include "../src/CachedDataProvider.h"

using namespace std;
using namespace bsread;

TEST(data_providers, CachedDataProvider) {
    float some_data = 123.45;
    CachedDataProvider cached_data_provider(&some_data, sizeof(float));

    auto random_data = cached_data_provider.get_data();
    EXPECT_FALSE(random_data.data == &some_data);
    EXPECT_FALSE(*(static_cast<float*>(random_data.data)) != some_data);

    cached_data_provider.cache_data();

    auto cached_data = cached_data_provider.get_data();
    EXPECT_FALSE(cached_data.data == &some_data);
    EXPECT_TRUE(*(static_cast<float*>(cached_data.data)) == some_data);
}