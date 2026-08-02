#pragma once








#include <viper/common/types.h>
#include <viper/ipc/protocol.h>

#include <functional>
#include <unordered_map>
#include <mutex>
#include <atomic>

namespace viper {
namespace ipc {


struct MessageTypeHash {
    std::size_t operator()(MessageType t) const noexcept {
        return std::hash<u16>{}(static_cast<u16>(t));
    }
};

using HandlerFunc = std::function<void(const Message&)>;

class MessageHandler {
public:
    MessageHandler();
    ~MessageHandler() = default;

    
    void registerHandler(MessageType type, HandlerFunc handler);

    
    void unregisterHandler(MessageType type);

    
    
    bool dispatch(const Message& msg);

    
    ProtocolState getState() const noexcept;

    
    void setState(ProtocolState state);

    
    u32 nextSequenceId() noexcept;

    
    void reset();

private:
    std::unordered_map<MessageType, HandlerFunc, MessageTypeHash> m_handlers;
    std::mutex m_handlerMutex;

    std::atomic<ProtocolState> m_state{ProtocolState::Disconnected};
    std::atomic<u32> m_sequenceCounter{0};
};

} 
} 
