





#include <viper/ipc/protocol.h>
#include <viper/common/logger.h>

#include <cstring>
#include <atomic>

namespace viper {
namespace ipc {





const char* messageTypeToString(MessageType type) noexcept {
    switch (type) {
        case MessageType::ClientHello:     return "CLIENT_HELLO";
        case MessageType::ServerHello:     return "SERVER_HELLO";
        case MessageType::Heartbeat:       return "HEARTBEAT";
        case MessageType::HeartbeatAck:    return "HEARTBEAT_ACK";
        case MessageType::Disconnect:      return "DISCONNECT";
        case MessageType::EnvRequest:      return "ENV_REQUEST";
        case MessageType::EnvResponse:     return "ENV_RESPONSE";
        case MessageType::Command:         return "COMMAND";
        case MessageType::CommandResponse: return "COMMAND_RESPONSE";
        case MessageType::StatusRequest:   return "STATUS_REQUEST";
        case MessageType::StatusResponse:  return "STATUS_RESPONSE";
        case MessageType::ModuleList:      return "MODULE_LIST";
        case MessageType::ModuleToggle:    return "MODULE_TOGGLE";
        case MessageType::ModuleUpdate:    return "MODULE_UPDATE";
        case MessageType::Error:           return "ERROR";
        default:                           return "UNKNOWN";
    }
}

const char* protocolStateToString(ProtocolState state) noexcept {
    switch (state) {
        case ProtocolState::Disconnected:  return "DISCONNECTED";
        case ProtocolState::Handshake:     return "HANDSHAKE";
        case ProtocolState::Negotiation:   return "NEGOTIATION";
        case ProtocolState::Ready:         return "READY";
        case ProtocolState::Active:        return "ACTIVE";
        case ProtocolState::Disconnecting: return "DISCONNECTING";
        default:                           return "UNKNOWN";
    }
}





namespace {
    std::atomic<u32> g_sequenceCounter{1};
}





Message Message::create(MessageType type, const nlohmann::json& payload) {
    Message msg;
    msg.header.magic = PROTOCOL_MAGIC;
    msg.header.version = 1;
    msg.header.type = static_cast<u16>(type);
    msg.header.sequenceId = g_sequenceCounter.fetch_add(1, std::memory_order_relaxed);
    msg.header.reserved = 0;
    msg.payload = payload;

    
    if (payload.is_null() || payload.empty()) {
        msg.header.payloadSize = 0;
    } else {
        std::string jsonStr = payload.dump();
        msg.header.payloadSize = static_cast<u32>(jsonStr.size());
    }

    return msg;
}

std::vector<u8> Message::serialize() const {
    std::string jsonStr;
    u32 payloadSize = 0;

    if (!payload.is_null() && !payload.empty()) {
        jsonStr = payload.dump();
        payloadSize = static_cast<u32>(jsonStr.size());
    }

    
    MessageHeader hdr = header;
    hdr.payloadSize = payloadSize;

    std::vector<u8> buffer(sizeof(MessageHeader) + payloadSize);

    
    std::memcpy(buffer.data(), &hdr, sizeof(MessageHeader));

    
    if (payloadSize > 0) {
        std::memcpy(buffer.data() + sizeof(MessageHeader), jsonStr.data(), payloadSize);
    }

    return buffer;
}

Message Message::deserialize(const std::vector<u8>& data) {
    return deserialize(data.data(), static_cast<usize>(data.size()));
}

Message Message::deserialize(const u8* data, usize size) {
    Message msg;

    if (size < sizeof(MessageHeader)) {
        VIPER_LOG_IPC_ERR("Message too small: {} bytes (need {})", size, sizeof(MessageHeader));
        return msg;
    }

    
    std::memcpy(&msg.header, data, sizeof(MessageHeader));

    
    if (msg.header.payloadSize > 0) {
        if (size < sizeof(MessageHeader) + msg.header.payloadSize) {
            VIPER_LOG_IPC_ERR("Message payload truncated: have {} bytes, need {}",
                              size - sizeof(MessageHeader), msg.header.payloadSize);
            return msg;
        }

        std::string jsonStr(
            reinterpret_cast<const char*>(data + sizeof(MessageHeader)),
            msg.header.payloadSize);

        try {
            msg.payload = nlohmann::json::parse(jsonStr);
        } catch (const nlohmann::json::exception& e) {
            VIPER_LOG_IPC_ERR("Failed to parse message payload: {}", e.what());
            msg.payload = nullptr;
        }
    }

    return msg;
}

bool Message::isValid() const noexcept {
    return header.magic == PROTOCOL_MAGIC &&
           header.version >= 1 &&
           header.payloadSize <= MAX_PAYLOAD_SIZE;
}

} 
} 
