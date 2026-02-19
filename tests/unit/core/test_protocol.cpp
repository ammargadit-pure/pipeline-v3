#include <gtest/gtest.h>
#include <cstring>
#include <arpa/inet.h>

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
    EXPECT_EQ(out.version, OTA_PROTOCOL_VERSION);
    EXPECT_EQ(out.msg_type, OTA_MSG_HEARTBEAT);
    EXPECT_EQ(out.payload_length, 42u);
    EXPECT_EQ(out.sequence_num, 7u);
}

TEST(Protocol, HeaderNetworkByteOrder)
{
    ota_header_t hdr = {};
    hdr.magic = OTA_PROTOCOL_MAGIC;
    hdr.version = OTA_PROTOCOL_VERSION;
    hdr.msg_type = OTA_MSG_REGISTER_REQ;
    hdr.payload_length = 0x01020304;
    hdr.sequence_num = 0x05060708;

    uint8_t buf[OTA_HEADER_SIZE];
    ASSERT_EQ(ota_header_serialize(&hdr, buf, sizeof(buf)), OTA_OK);

    /* Magic in big-endian: 0x4F54 → buf[0]=0x4F, buf[1]=0x54 */
    EXPECT_EQ(buf[0], 0x4F);
    EXPECT_EQ(buf[1], 0x54);

    /* Version byte */
    EXPECT_EQ(buf[2], OTA_PROTOCOL_VERSION);

    /* Msg type byte */
    EXPECT_EQ(buf[3], OTA_MSG_REGISTER_REQ);

    /* payload_length in big-endian */
    EXPECT_EQ(buf[4], 0x01);
    EXPECT_EQ(buf[5], 0x02);
    EXPECT_EQ(buf[6], 0x03);
    EXPECT_EQ(buf[7], 0x04);
}

TEST(Protocol, HeaderValidateBadMagic)
{
    ota_header_t hdr = {};
    hdr.magic = 0xFFFF;
    hdr.version = OTA_PROTOCOL_VERSION;
    hdr.payload_length = 0;

    EXPECT_EQ(ota_header_validate(&hdr), OTA_ERR_PROTOCOL_MAGIC);
    EXPECT_NE(ota_header_validate(&hdr), OTA_OK);
}

TEST(Protocol, HeaderValidateBadVersion)
{
    ota_header_t hdr = {};
    hdr.magic = OTA_PROTOCOL_MAGIC;
    hdr.version = 0xFF;
    hdr.payload_length = 0;

    EXPECT_EQ(ota_header_validate(&hdr), OTA_ERR_PROTOCOL_VERSION);
    EXPECT_NE(ota_header_validate(&hdr), OTA_OK);
}

TEST(Protocol, HeaderValidateOversizedPayload)
{
    ota_header_t hdr = {};
    hdr.magic = OTA_PROTOCOL_MAGIC;
    hdr.version = OTA_PROTOCOL_VERSION;
    hdr.payload_length = OTA_MAX_PAYLOAD_SIZE + 1;

    EXPECT_EQ(ota_header_validate(&hdr), OTA_ERR_PROTOCOL_PAYLOAD_SIZE);
    EXPECT_NE(ota_header_validate(&hdr), OTA_OK);
}

TEST(Protocol, HeaderValidateZeroPayload)
{
    ota_header_t hdr = {};
    hdr.magic = OTA_PROTOCOL_MAGIC;
    hdr.version = OTA_PROTOCOL_VERSION;
    hdr.payload_length = 0;

    EXPECT_EQ(ota_header_validate(&hdr), OTA_OK);
    EXPECT_NE(ota_header_validate(&hdr), OTA_ERR_PROTOCOL_PAYLOAD_SIZE);
}

TEST(Protocol, MsgIsBinaryTransferData)
{
    EXPECT_TRUE(ota_msg_is_binary(OTA_MSG_TRANSFER_DATA));
    EXPECT_FALSE(ota_msg_is_binary(OTA_MSG_HEARTBEAT));
}

TEST(Protocol, MsgIsNotBinaryForOthers)
{
    const ota_msg_type_t non_binary[] = {
        OTA_MSG_REGISTER_REQ, OTA_MSG_REGISTER_ACK,
        OTA_MSG_HEARTBEAT, OTA_MSG_HEARTBEAT_ACK,
        OTA_MSG_TRANSFER_START, OTA_MSG_TRANSFER_ACK,
        OTA_MSG_TRANSFER_NACK, OTA_MSG_TRANSFER_COMPLETE,
        OTA_MSG_INSTALL_CMD, OTA_MSG_ACTIVATE_CMD, OTA_MSG_ROLLBACK_CMD,
        OTA_MSG_STATUS_REPORT, OTA_MSG_HEALTH_REPORT, OTA_MSG_CAMPAIGN_STATUS,
    };

    for (size_t i = 0; i < sizeof(non_binary) / sizeof(non_binary[0]); i++) {
        EXPECT_FALSE(ota_msg_is_binary(non_binary[i]))
            << "Expected false for msg type 0x" << std::hex << (int)non_binary[i];
    }
    EXPECT_TRUE(ota_msg_is_binary(OTA_MSG_TRANSFER_DATA));
}

TEST(Protocol, MsgTypeStrReturnsNonNull)
{
    const ota_msg_type_t all_types[] = {
        OTA_MSG_REGISTER_REQ, OTA_MSG_REGISTER_ACK,
        OTA_MSG_HEARTBEAT, OTA_MSG_HEARTBEAT_ACK,
        OTA_MSG_TRANSFER_START, OTA_MSG_TRANSFER_DATA,
        OTA_MSG_TRANSFER_ACK, OTA_MSG_TRANSFER_NACK, OTA_MSG_TRANSFER_COMPLETE,
        OTA_MSG_INSTALL_CMD, OTA_MSG_ACTIVATE_CMD, OTA_MSG_ROLLBACK_CMD,
        OTA_MSG_STATUS_REPORT, OTA_MSG_HEALTH_REPORT, OTA_MSG_CAMPAIGN_STATUS,
    };

    for (size_t i = 0; i < sizeof(all_types) / sizeof(all_types[0]); i++) {
        const char *s = ota_msg_type_str(all_types[i]);
        ASSERT_NE(s, nullptr);
        EXPECT_STRNE(s, "UNKNOWN") << "Got UNKNOWN for type 0x" << std::hex << (int)all_types[i];
    }

    /* Verify specific known values */
    EXPECT_STREQ(ota_msg_type_str(OTA_MSG_HEARTBEAT), "HEARTBEAT");
    EXPECT_STREQ(ota_msg_type_str(OTA_MSG_TRANSFER_DATA), "TRANSFER_DATA");
}

TEST(Protocol, BufferTooSmallForSerialize)
{
    ota_header_t hdr = {};
    hdr.magic = OTA_PROTOCOL_MAGIC;
    hdr.version = OTA_PROTOCOL_VERSION;

    uint8_t buf[4]; /* Too small — need 12 bytes */
    EXPECT_EQ(ota_header_serialize(&hdr, buf, sizeof(buf)), OTA_ERR_INVALID_ARGUMENT);
    EXPECT_NE(ota_header_serialize(&hdr, buf, sizeof(buf)), OTA_OK);
}
