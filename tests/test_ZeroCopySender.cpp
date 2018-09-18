#include "gtest/gtest.h"
#include "../src/ZeroCopySender.h"
#include "../src/DirectDataProvider.h"
#include "DummyReceiver.h"
#include <unistd.h>

using namespace std;
using namespace bsread;

TEST(ZeroCopySender, deallocation_callback) {

    size_t n_messages = 1;

    ZeroCopySender sender("tcp://127.0.0.1:12345", n_messages);

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
        EXPECT_EQ(message->main_header->pulse_id, i);
    }

    EXPECT_FALSE(buffer_lock.load());
}