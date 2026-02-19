#include "ota/core/protocol.h"
#include <arpa/inet.h>
#include <string.h>

ota_error_t ota_header_serialize(const ota_header_t *hdr, uint8_t *buf, size_t buf_len)
{
    if (!hdr || !buf) {
        return OTA_ERR_INVALID_ARGUMENT;
    }
    if (buf_len < OTA_HEADER_SIZE) {
        return OTA_ERR_INVALID_ARGUMENT;
    }

    uint16_t magic_net = htons(hdr->magic);
    uint32_t payload_net = htonl(hdr->payload_length);
    uint32_t seq_net = htonl(hdr->sequence_num);

    memcpy(buf + 0, &magic_net, 2);
    buf[2] = hdr->version;
    buf[3] = hdr->msg_type;
    memcpy(buf + 4, &payload_net, 4);
    memcpy(buf + 8, &seq_net, 4);

    return OTA_OK;
}

ota_error_t ota_header_deserialize(const uint8_t *buf, size_t buf_len, ota_header_t *out)
{
    if (!buf || !out) {
        return OTA_ERR_INVALID_ARGUMENT;
    }
    if (buf_len < OTA_HEADER_SIZE) {
        return OTA_ERR_INVALID_ARGUMENT;
    }

    uint16_t magic_net;
    uint32_t payload_net;
    uint32_t seq_net;

    memcpy(&magic_net, buf + 0, 2);
    out->magic = ntohs(magic_net);
    out->version = buf[2];
    out->msg_type = buf[3];
    memcpy(&payload_net, buf + 4, 4);
    out->payload_length = ntohl(payload_net);
    memcpy(&seq_net, buf + 8, 4);
    out->sequence_num = ntohl(seq_net);

    return OTA_OK;
}

ota_error_t ota_header_validate(const ota_header_t *hdr)
{
    if (!hdr) {
        return OTA_ERR_INVALID_ARGUMENT;
    }
    if (hdr->magic != OTA_PROTOCOL_MAGIC) {
        return OTA_ERR_PROTOCOL_MAGIC;
    }
    if (hdr->version != OTA_PROTOCOL_VERSION) {
        return OTA_ERR_PROTOCOL_VERSION;
    }
    if (hdr->payload_length > OTA_MAX_PAYLOAD_SIZE) {
        return OTA_ERR_PROTOCOL_PAYLOAD_SIZE;
    }
    return OTA_OK;
}

bool ota_msg_is_binary(ota_msg_type_t type)
{
    return type == OTA_MSG_TRANSFER_DATA;
}

const char *ota_msg_type_str(ota_msg_type_t type)
{
    switch (type) {
    case OTA_MSG_REGISTER_REQ:      return "REGISTER_REQ";
    case OTA_MSG_REGISTER_ACK:      return "REGISTER_ACK";
    case OTA_MSG_HEARTBEAT:         return "HEARTBEAT";
    case OTA_MSG_HEARTBEAT_ACK:     return "HEARTBEAT_ACK";
    case OTA_MSG_TRANSFER_START:    return "TRANSFER_START";
    case OTA_MSG_TRANSFER_DATA:     return "TRANSFER_DATA";
    case OTA_MSG_TRANSFER_ACK:      return "TRANSFER_ACK";
    case OTA_MSG_TRANSFER_NACK:     return "TRANSFER_NACK";
    case OTA_MSG_TRANSFER_COMPLETE: return "TRANSFER_COMPLETE";
    case OTA_MSG_INSTALL_CMD:       return "INSTALL_CMD";
    case OTA_MSG_ACTIVATE_CMD:      return "ACTIVATE_CMD";
    case OTA_MSG_ROLLBACK_CMD:      return "ROLLBACK_CMD";
    case OTA_MSG_STATUS_REPORT:     return "STATUS_REPORT";
    case OTA_MSG_HEALTH_REPORT:     return "HEALTH_REPORT";
    case OTA_MSG_CAMPAIGN_STATUS:   return "CAMPAIGN_STATUS";
    }
    return "UNKNOWN";
}
