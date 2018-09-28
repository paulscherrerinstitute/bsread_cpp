#include "gtest/gtest.h"
#include "../src/Sender.h"
#include "../src/DirectDataProvider.h"
#include "../src/Receiver.h"
#include <unistd.h>

using namespace std;
using namespace bsread;

TEST(Receiver, check_headers) {

    Sender sender("tcp://127.0.0.1:12345", 1);

    float test_float = 12.34;
    sender.add_channel(make_shared<Channel>("test_channel_float",
                                            make_shared<DirectDataProvider>(&test_float, sizeof(float)),
                                            BSDATA_FLOAT32));

    uint64_t test_long_array[] = {123, 456};
    vector<size_t> shape = {2};
    sender.add_channel(make_shared<Channel>("test_channel_long",
                                            make_shared<DirectDataProvider>(&test_long_array, sizeof(uint64_t)*2),
                                            BSDATA_UINT64, shape));

    Receiver receiver("tcp://0.0.0.0:12345");
    sleep(1);

    uint64_t pulse_id = 1234567890;
    uint64_t global_timestamp_sec = 13579;
    uint64_t global_timestamp_nsec = 24680;

    EXPECT_TRUE(sender.send_message(pulse_id, {global_timestamp_sec, global_timestamp_nsec}) == SENT);

    auto message = receiver.receive();

    EXPECT_EQ(message.main_header->pulse_id, pulse_id);
    EXPECT_EQ(message.main_header->htype, BSREAD_MAIN_HEADER_VERSION);
    EXPECT_FALSE(message.main_header->hash.empty());
    EXPECT_EQ(message.main_header->dh_compression, compression_none);
    EXPECT_EQ(message.main_header->global_timestamp.sec, global_timestamp_sec);
    EXPECT_EQ(message.main_header->global_timestamp.nsec, global_timestamp_nsec);

    EXPECT_EQ(message.data_header->htype, BSREAD_DATA_HEADER_VERSION);
    EXPECT_EQ(message.data_header->channels.size(), 2);

    auto& float_header = message.data_header->channels.at(0);
    EXPECT_EQ(float_header.name, "test_channel_float");
    EXPECT_EQ(float_header.type, BSDATA_FLOAT32);
    EXPECT_EQ(float_header.shape[0], 1);
    EXPECT_EQ(float_header.compression, compression_none);
    EXPECT_EQ(float_header.endianess, little);
    EXPECT_EQ(float_header.modulo, 1);
    EXPECT_EQ(float_header.offset, 0);

    auto& long_array_header = message.data_header->channels.at(1);
    EXPECT_EQ(long_array_header.name, "test_channel_long");
    EXPECT_EQ(long_array_header.type, BSDATA_UINT64);
    EXPECT_EQ(long_array_header.shape[0], 2);
    EXPECT_EQ(long_array_header.compression, compression_none);
    EXPECT_EQ(long_array_header.endianess, little);
    EXPECT_EQ(long_array_header.modulo, 1);
    EXPECT_EQ(long_array_header.offset, 0);

    EXPECT_EQ(message.channels_value->size(), 2);

    auto& float_value = message.channels_value->at("test_channel_float");
    EXPECT_NE(float_value.data, nullptr);
    EXPECT_EQ(float_value.timestamp, nullptr);

    float* received_float_value = (float*)float_value.data.get();
    EXPECT_EQ(*received_float_value, test_float);

    auto& long_array_value = message.channels_value->at("test_channel_long");
    EXPECT_NE(long_array_value.data, nullptr);
    EXPECT_EQ(long_array_value.timestamp, nullptr);

    uint64_t* received_long_array_value = (uint64_t*)long_array_value.data.get();
    EXPECT_EQ(received_long_array_value[0], 123);
    EXPECT_EQ(received_long_array_value[1], 456);
}