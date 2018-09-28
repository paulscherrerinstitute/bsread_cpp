#include "gtest/gtest.h"
#include "../src/ZeroCopySender.h"
#include "../src/DirectDataProvider.h"
#include "DummyReceiver.h"
#include "../src/CacheManager.h"
#include <unistd.h>

using namespace std;
using namespace bsread;

TEST(ZeroCopySender, deallocation_callback) {

    size_t n_messages = 50;

    ZeroCopySender sender("tcp://127.0.0.1:12345");

    float test_float = 12.34;
    sender.add_channel(make_shared<Channel>("test_channel_float",
                                            make_shared<DirectDataProvider>(&test_float, sizeof(float)),
                                            BSDATA_FLOAT32));

    uint64_t test_long = 1234;
    sender.add_channel(make_shared<Channel>("test_channel_long",
                                            make_shared<DirectDataProvider>(&test_long, sizeof(uint64_t)),
                                            BSDATA_UINT64));

    DummyReceiver receiver("tcp://0.0.0.0:12345");
    sleep(1);

    atomic<bool> buffer_lock(true);

    auto release_channels = [](void* data, void* buffer_lock) {
        (static_cast<atomic<bool>*>(buffer_lock))->store(false);
    };

    for (int i=0; i < n_messages; i++) {
        // Simulate 100Hz.
        usleep(10000);
        EXPECT_TRUE(sender.send_message(i, {}, release_channels, &buffer_lock) == SENT);
    }

    for (int i=0; i < n_messages; i++) {
        auto message = receiver.receive();
        EXPECT_EQ(message.main_header->pulse_id, i);
    }

    EXPECT_FALSE(buffer_lock.load());
}

TEST(ZeroCopySender, cache_manager) {
    size_t n_messages = 50;

    ZeroCopySender sender("tcp://127.0.0.1:12345");

    float test_float = 12.34;
    auto float_provider = make_shared<CachedDataProvider>(&test_float, sizeof(float));
    sender.add_channel(make_shared<Channel>("test_channel_float", float_provider, BSDATA_FLOAT32));

    uint64_t test_long = 1234;
    auto long_provider = make_shared<CachedDataProvider>(&test_long, sizeof(uint64_t));
    sender.add_channel(make_shared<Channel>("test_channel_long", long_provider, BSDATA_UINT64));

    DummyReceiver receiver("tcp://0.0.0.0:12345");
    sleep(1);

    CacheManager cache_manager({float_provider,
                                long_provider});

    for (int i=0; i < n_messages; i++) {
        // Simulate 100Hz.
        usleep(10000);

        EXPECT_TRUE(cache_manager.cache_all());
        // This should fail because the cache is already loaded.
        EXPECT_FALSE(cache_manager.cache_all());

        EXPECT_TRUE(sender.send_message(i, {}, cache_manager.release_cache, &cache_manager) == SENT);
    }

    for (int i=0; i < n_messages; i++) {
        auto message = receiver.receive();
        EXPECT_EQ(message.main_header->pulse_id, i);
    }
}

TEST(ZeroCopySender, cannot_send_anymore) {
    ZeroCopySender sender("tcp://127.0.0.1:12345");

    float test_float = 12.34;
    auto float_provider = make_shared<CachedDataProvider>(&test_float, sizeof(float));
    sender.add_channel(make_shared<Channel>("test_channel_float", float_provider, BSDATA_FLOAT32));

    CacheManager cache_manager({float_provider});

    EXPECT_TRUE(cache_manager.cache_all());

    EXPECT_EQ(FAILED, sender.send_message(0, {}, cache_manager.release_cache, &cache_manager));

    EXPECT_FALSE(cache_manager.cache_all());

    cache_manager.release_all();

    EXPECT_TRUE(cache_manager.cache_all());

    DummyReceiver receiver("tcp://0.0.0.0:12345");
    sleep(1);

    EXPECT_EQ(SENT, sender.send_message(1, {}, cache_manager.release_cache, &cache_manager));

    EXPECT_EQ(BUSY, sender.send_message(2, {}, cache_manager.release_cache, &cache_manager));

    EXPECT_FALSE(cache_manager.cache_all());

    // Only the pulse_id=1 sent should have been sent.
    EXPECT_EQ(receiver.receive().main_header->pulse_id, 1);

    usleep(1*1000);

    EXPECT_TRUE(cache_manager.cache_all());

    EXPECT_EQ(SENT, sender.send_message(3, {}, cache_manager.release_cache, &cache_manager));

    EXPECT_EQ(BUSY, sender.send_message(4, {}, cache_manager.release_cache, &cache_manager));

    EXPECT_EQ(receiver.receive().main_header->pulse_id, 3);

}