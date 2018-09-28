#include "gtest/gtest.h"
#include "../src/Sender.h"
#include "../src/DirectDataProvider.h"
#include "Receiver.h"
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

    Receiver receiver("tcp://0.0.0.0:12345");
    sleep(1);

    for (int i=0; i < n_messages; i++) {
        // Simulate 100Hz.
        usleep(10000);
        EXPECT_TRUE(sender.send_message(i, {}) == SENT);
    }

    for (int i=0; i < n_messages; i++) {
        auto message = receiver.receive();
        EXPECT_EQ(message.main_header->pulse_id, i);
    }
}

TEST(Sender, disable_sending) {

    size_t n_messages = 10;

    Sender sender("tcp://127.0.0.1:12345", n_messages);
    Receiver receiver("tcp://0.0.0.0:12345");

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
        EXPECT_EQ(message.main_header->pulse_id, i);
    }
}

TEST(Sender, data_header) {
    Sender sender("tcp://127.0.0.1:12345");
    Receiver receiver("tcp://0.0.0.0:12345");

    sender.add_channel(make_shared<Channel>("default_channel", nullptr));
    sender.add_channel(make_shared<Channel>("complete_channel", nullptr,
                                            BSDATA_INT8, vector<size_t>({1,2,3,4}),
                                            compression_bslz4, 2, 3, big));

    // Wait for the connection to happen.
    sleep(1);

    auto status = sender.send_message(0, {});
    EXPECT_EQ(status, SENT);

    auto message = receiver.receive();

    auto& default_channel_definition = message.data_header->channels[0];
    EXPECT_EQ(default_channel_definition.name, "default_channel");
    EXPECT_EQ(default_channel_definition.type, BSDATA_FLOAT64);
    EXPECT_EQ(default_channel_definition.shape, vector<uint32_t>({1}));
    EXPECT_EQ(default_channel_definition.compression, compression_none);
    EXPECT_EQ(default_channel_definition.endianess, little);
    EXPECT_EQ(default_channel_definition.modulo, 1);
    EXPECT_EQ(default_channel_definition.offset, 0);

    auto& complete_channel_definition = message.data_header->channels[1];
    EXPECT_EQ(complete_channel_definition.name, "complete_channel");
    EXPECT_EQ(complete_channel_definition.type, BSDATA_INT8);
    EXPECT_EQ(complete_channel_definition.shape, vector<uint32_t>({1,2,3,4}));
    EXPECT_EQ(complete_channel_definition.compression, compression_bslz4);
    EXPECT_EQ(complete_channel_definition.endianess, big);
    EXPECT_EQ(complete_channel_definition.modulo, 2);
    EXPECT_EQ(complete_channel_definition.offset, 3);
}