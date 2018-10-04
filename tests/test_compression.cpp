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

TEST(compression, decompress_lz4) {

    size_t image_pixel_size = 2160 * 2560;
    size_t n_element_bytes = sizeof(uint16_t);
    size_t image_bytes = image_pixel_size * n_element_bytes;

    auto populate_buffer = [](char* buffer, size_t buffer_size, uint16_t init_value) {
        for (size_t index=0; index<buffer_size/2; index++) {
            ((uint16_t*)buffer)[index] = init_value;
        }
    };

    auto compare_buffers = [](char* original_buffer, char* new_buffer, size_t buffer_size) {
        for (size_t index=0; index<buffer_size; index++) {
            SCOPED_TRACE(index);
            EXPECT_EQ(original_buffer[index], new_buffer[index]);

            if (original_buffer[index] != new_buffer[index]) {
                return;
            }
        }
    };

    auto compression_buffer_size = get_compression_buffer_size(compression_lz4, image_pixel_size, n_element_bytes);

    unique_ptr<char[]> original_buffer(new char[image_bytes]);
    unique_ptr<char[]> compressed_buffer(new char[compression_buffer_size]);
    unique_ptr<char[]> decompressed_buffer(new char[image_bytes*2]);

    populate_buffer(original_buffer.get(), image_bytes, 65000);

    auto compressed_size = compress_lz4(original_buffer.get(), image_pixel_size, sizeof(uint16_t),
            compressed_buffer.get(), compression_buffer_size);

    auto decompressed_size = decompress_lz4(compressed_buffer.get(), compressed_size, decompressed_buffer.get());

    EXPECT_EQ(image_bytes, decompressed_size);

    compare_buffers(original_buffer.get(), decompressed_buffer.get(), image_bytes);
}