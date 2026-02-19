#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "ota/core/protocol.h"
}

TEST(Protocol, HeaderSerializeDeserializeRoundtrip)
{
    ota_header_t hdr = {};
    hdr.magic = OTA_PROTOCOL_MAGIC;
    hdr.version = OTA_PROTOCOL_VERSION;
    hdr.msg_type = OTA_MSG_HEARTBEAT;
    hdr.payload_length = 42;
    hdr.sequence_num = 7;

    uint8_t buf[OTA_HEADER_SIZE];
    ASSERT_EQ(ota_header_serialize(&hdr, buf, sizeof(buf)), OTA_OK);

    ota_header_t out = {};
    ASSERT_EQ(ota_header_deserialize(buf, sizeof(buf), &out), OTA_OK);

    EXPECT_EQ(out.magic, OTA_PROTOCOL_MAGIC);
    EXPECT_EQ(out.payload_length, 42u);
}

TEST(Protocol, MsgIsBinaryTransferData)
{
    EXPECT_TRUE(ota_msg_is_binary(OTA_MSG_TRANSFER_DATA));
    EXPECT_FALSE(ota_msg_is_binary(OTA_MSG_HEARTBEAT));
}
