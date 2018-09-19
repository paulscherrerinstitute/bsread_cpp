#include "gtest/gtest.h"
#include "../src/compression.h"

using namespace std;
using namespace bsread;

TEST(compression, get_compression_buffer_size) {

    EXPECT_THROW({
        auto size = get_compression_buffer_size(compression_none, 0, 0);
    }, runtime_error);

    auto lz4_size = get_compression_buffer_size(compression_lz4, 0, 0);
    // 16 bytes (lz4 overhead) + 4 bytes (bsread overhead)
    EXPECT_EQ(lz4_size, 20);

    // Calculation depends on element_size -> must not be 0.
    auto bslz4_size = get_compression_buffer_size(compression_bslz4, 0, 1);
    // 0 bytes (bslz4 overhead) + 12 bytes (bsread overhead)
    EXPECT_EQ(bslz4_size, 12);
}

TEST(compression, compress_buffer) {

    EXPECT_THROW({
        compress_buffer(compression_none, nullptr, 0, 0, nullptr, 0);
    }, runtime_error);

    size_t n_elements = 1024;
    size_t element_size = sizeof(uint16_t);
    auto uncompressed_size = n_elements * element_size;
    char data[uncompressed_size];

    auto buffer_length = get_compression_buffer_size(compression_lz4, n_elements, element_size);
    char buffer[buffer_length];

    auto size_lz4 = compress_buffer(compression_lz4, data, n_elements, element_size, buffer, buffer_length);

    EXPECT_TRUE(size_lz4 <= uncompressed_size);
    EXPECT_TRUE(size_lz4 > 0);

    auto size_bslz4 = compress_buffer(compression_bslz4, data, n_elements, element_size, buffer, buffer_length);

    EXPECT_TRUE(size_bslz4 <= uncompressed_size);
    EXPECT_TRUE(size_bslz4 > 0);
}