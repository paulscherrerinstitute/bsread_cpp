#include "gtest/gtest.h"
#include "../src/Channel.h"
#include "../src/DirectDataProvider.h"
#include "../src/constants.h"

using namespace std;
using namespace bsread;

TEST(Channel, data_provider) {

    int32_t data = 12345;
    auto data_provider = make_shared<DirectDataProvider>(&data, sizeof(data));

    auto channel_name = "integer_channel";
    Channel channel(channel_name, data_provider, BSDATA_INT32);

    EXPECT_EQ(channel_name, channel.get_name());

    auto channel_data = channel.get_data_for_pulse_id(0);

    // They should point to the same buffer.
    EXPECT_EQ(channel_data.data, &data);
    EXPECT_EQ(channel_data.data_len, sizeof(data));

    // DirectDataProvider does not use a timestamp.
    EXPECT_EQ(channel_data.timestamp, nullptr);
    EXPECT_EQ(channel_data.timestamp_len, 0);
}

TEST(Channel, endian) {
    auto data_provider = make_shared<DirectDataProvider>(nullptr, 0);

    auto expected_endianess = htonl(1) == 1 ? "big" : "little";

    // Automatic system endianes detection.
    Channel channel_auto("auto_detect", data_provider, BSDATA_INT32, {1});
    EXPECT_EQ(expected_endianess, channel_auto.get_channel_data_header()["encoding"].asString());

    // Forced automatic system endianess detection.
    Channel channel_auto_explicit("little", data_provider, BSDATA_INT32, {1}, compression_none, 1, 0, auto_detect);
    EXPECT_EQ(expected_endianess, channel_auto_explicit.get_channel_data_header()["encoding"].asString());

    Channel channel_little("little", data_provider, BSDATA_INT32, {1}, compression_none, 1, 0, little);
    EXPECT_EQ("little", channel_little.get_channel_data_header()["encoding"].asString());

    Channel channel_big("big", data_provider, BSDATA_INT32, {1}, compression_none, 1, 0, big);
    EXPECT_EQ("big", channel_big.get_channel_data_header()["encoding"].asString());
}

TEST(Channel, data_header) {

    Channel scalar_channel("test", nullptr, BSDATA_FLOAT32);
    auto scalar_channel_dh = scalar_channel.get_channel_data_header();
    auto expected_endianess = htonl(1) == 1 ? "big" : "little";

    // Expected default values
    EXPECT_EQ("test", scalar_channel_dh["name"].asString());
    EXPECT_EQ("float32", scalar_channel_dh["type"].asString());
    EXPECT_EQ(expected_endianess, scalar_channel_dh["encoding"].asString());
    EXPECT_EQ(1, scalar_channel_dh["modulo"].asInt());
    EXPECT_EQ(0, scalar_channel_dh["offset"].asInt());
    EXPECT_EQ(1, scalar_channel_dh["shape"][0].asInt());

    Channel array_channel("test", nullptr, BSDATA_UINT16, {1024, 512}, compression_bslz4, 5, 3, big);
    auto array_channel_dh = array_channel.get_channel_data_header();

    // Information on left taken from Channel constructor.
    EXPECT_EQ("test", array_channel_dh["name"].asString());
    EXPECT_EQ("uint16", array_channel_dh["type"].asString());
    EXPECT_EQ("big", array_channel_dh["encoding"].asString());
    EXPECT_EQ(5, array_channel_dh["modulo"].asInt());
    EXPECT_EQ(3, array_channel_dh["offset"].asInt());
    EXPECT_EQ(1024, array_channel_dh["shape"][0].asInt());
    EXPECT_EQ(512, array_channel_dh["shape"][1].asInt());
}

TEST(Channel, modulo_and_offset) {
    Channel channel_auto("test", nullptr, BSDATA_UINT16, {1}, compression_none, 3, 0);
}