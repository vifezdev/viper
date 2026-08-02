#pragma once








#include <viper/common/types.h>
#include <string>
#include <vector>
#include <functional>
#include <nlohmann/json.hpp>

namespace viper {
namespace ipc {






inline constexpr u32 PROTOCOL_MAGIC = 0x52504956; 


inline constexpr u32 MAX_PAYLOAD_SIZE = 65536;


inline constexpr u32 HEARTBEAT_INTERVAL_MS = 5000;


inline constexpr u32 HEARTBEAT_TIMEOUT_MS = 15000;


inline constexpr const wchar_t* PIPE_NAME_PREFIX = L"\\\\.\\pipe\\viper-ipc-";





enum class MessageType : u16 {
    
    ClientHello     = 0x0001,   
    ServerHello     = 0x0002,   

    
    Heartbeat       = 0x0100,   
    HeartbeatAck    = 0x0101,   
    Disconnect      = 0x0102,   

    
    EnvRequest      = 0x0200,   
    EnvResponse     = 0x0201,   

    
    Command         = 0x0300,   
    CommandResponse = 0x0301,   

    
    StatusRequest   = 0x0400,   
    StatusResponse  = 0x0401,   

    
    ModuleList      = 0x0500,   
    ModuleToggle    = 0x0501,   
    ModuleUpdate    = 0x0502,   

    
    Error           = 0xFF00,   
};

const char* messageTypeToString(MessageType type) noexcept;





#pragma pack(push, 1)
struct MessageHeader {
    u32 magic;          
    u16 version;        
    u16 type;           
    u32 payloadSize;    
    u32 sequenceId;     
    u16 reserved;       
};
#pragma pack(pop)

static_assert(sizeof(MessageHeader) == 18, "MessageHeader must be exactly 18 bytes");





struct Message {
    MessageHeader header{};
    nlohmann::json payload;

    
    static Message create(MessageType type, const nlohmann::json& payload = {});

    
    std::vector<u8> serialize() const;

    
    static Message deserialize(const std::vector<u8>& data);
    static Message deserialize(const u8* data, usize size);

    
    bool isValid() const noexcept;
};


using MessageCallback = std::function<void(const Message&)>;





enum class ProtocolState : u8 {
    Disconnected    = 0,
    Handshake       = 1,    
    Negotiation     = 2,    
    Ready           = 3,    
    Active          = 4,    
    Disconnecting   = 5,    
};

const char* protocolStateToString(ProtocolState state) noexcept;





namespace payloads {


inline nlohmann::json clientHello(const std::string& clientVersion) {
    return {
        {"client_version", clientVersion},
        {"protocol_version", 1},
        {"client_type", "external"},
    };
}


inline nlohmann::json serverHello(const std::string& serverVersion,
                                   const EnvironmentInfo& env) {
    return {
        {"server_version", serverVersion},
        {"protocol_version", 1},
        {"minecraft_version", env.minecraftVersion},
        {"launcher", launcherToString(env.launcher)},
        {"java_version", env.javaVersion},
        {"process_id", env.processId},
    };
}


inline nlohmann::json error(ErrorCode code, const std::string& message) {
    return {
        {"error_code", static_cast<u32>(code)},
        {"error_string", errorCodeToString(code)},
        {"message", message},
    };
}

} 

} 
} 
