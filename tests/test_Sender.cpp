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

    for (int i=0; i < n_messages; i++) {
        // Simulate 100Hz.
        usleep(10000);

        auto result = sender.send_message(i, {});
        EXPECT_TRUE(result != 0);
    }

    for (int i=0; i < n_messages; i++) {
        receiver.receive();
    }
}