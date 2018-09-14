#include "gtest/gtest.h"
#include "../src/Sender.h"
#include "../src/DirectDataProvider.h"
#include "DummyReceiver.h"
#include <unistd.h>

using namespace std;
using namespace bsread;

TEST(Sender, basic_workflow) {

    size_t n_messages = 50;

    Sender sender("tcp://127.0.0.1:12345", n_messages);

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

    for (int i=0; i < n_messages; i++) {
        // Simulate 100Hz.
        usleep(10000);
        EXPECT_TRUE(sender.send_message(i, {}) == SENT);
    }

    for (int i=0; i < n_messages; i++) {
        auto message = receiver.receive();
        EXPECT_EQ(message->main_header->pulse_id, i);
    }
}

TEST(Sender, disable_sending) {

    size_t n_messages = 10;

    Sender sender("tcp://127.0.0.1:12345", n_messages);
    DummyReceiver receiver("tcp://0.0.0.0:12345");

    // There are no channels, all messages should be SKIPPED.
    for (int i=0; i < n_messages; i++) {
        // Simulate 100Hz.
        usleep(10000);

        EXPECT_TRUE(sender.send_message(i, {})==SKIPPED);
    }

    float test_float = 12.34;
    sender.add_channel(make_shared<Channel>("test_channel_float",
                                            make_shared<DirectDataProvider>(&test_float, sizeof(float)),
                                            BSDATA_FLOAT32));

    sender.set_sending_enabled(false);

    // Sending is disabled, all messages should be skipped.
    for (int i=0; i < n_messages; i++) {
        // Simulate 100Hz.
        usleep(10000);

        EXPECT_TRUE(sender.send_message(i, {})==SKIPPED);
    }

    sender.set_sending_enabled(true);

    for (int i=0; i < n_messages; i++) {
        // Simulate 100Hz.
        usleep(10000);

        EXPECT_TRUE(sender.send_message(i, {})==SENT);
    }

    for (int i=0; i < n_messages; i++) {
        auto message = receiver.receive();
        EXPECT_EQ(message->main_header->pulse_id, i);
    }
}