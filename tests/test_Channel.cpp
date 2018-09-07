#include "gtest/gtest.h"
#include "../src/Channel.h"
#include "../src/BufferDataProvider.h"
#include "../src/constants.h"

using namespace std;
using namespace bsread;

TEST(Channel, constructor) {

    int32_t data = 12345;
    auto data_provider = make_shared<BufferDataProvider>(&data, sizeof(data));

    auto channel_name = "integer_channel";
    Channel channel(channel_name, data_provider, BSDATA_INT32, {1});

    EXPECT_EQ(channel_name, channel.get_name());

    auto channel_data = channel.get_data_for_pulse_id(0);

    // They should point to the same buffer.
    EXPECT_EQ(channel_data.data, &data);
    EXPECT_EQ(channel_data.data_len, sizeof(data));

    // BufferDataProvider does not use a timestamp.
    EXPECT_EQ(channel_data.timestamp, nullptr);
    EXPECT_EQ(channel_data.timestamp_len, 0);
}

TEST(Channel, endian_test) {
    auto data_provider = make_shared<BufferDataProvider>(nullptr, 0);

    auto expected_endianess = htonl(1) == 1 ? "big" : "little";

    // Automatic system endianes detection.
    Channel channel_auto("auto_detect", data_provider, BSDATA_INT32, {1});
    EXPECT_EQ(expected_endianess, channel_auto.get_channel_data_header()["encoding"].asString());

    // Forced automatic system endianes detection.
    Channel channel_auto_explicit("little", data_provider, BSDATA_INT32, {1}, compression_none, 1, 0, auto_detect);
    EXPECT_EQ(expected_endianess, channel_auto_explicit.get_channel_data_header()["encoding"].asString());

    Channel channel_little("little", data_provider, BSDATA_INT32, {1}, compression_none, 1, 0, little);
    EXPECT_EQ("little", channel_little.get_channel_data_header()["encoding"].asString());

    Channel channel_big("big", data_provider, BSDATA_INT32, {1}, compression_none, 1, 0, big);
    EXPECT_EQ("big", channel_big.get_channel_data_header()["encoding"].asString());
}