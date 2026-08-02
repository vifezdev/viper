






#include <viper/ipc/message_handler.h>
#include <viper/common/logger.h>

namespace viper {
namespace ipc {

MessageHandler::MessageHandler() = default;

void MessageHandler::registerHandler(MessageType type, HandlerFunc handler) {
    std::lock_guard<std::mutex> lock(m_handlerMutex);
    m_handlers[type] = std::move(handler);
}

void MessageHandler::unregisterHandler(MessageType type) {
    std::lock_guard<std::mutex> lock(m_handlerMutex);
    m_handlers.erase(type);
}

bool MessageHandler::dispatch(const Message& msg) {
    MessageType type = static_cast<MessageType>(msg.header.type);

    std::lock_guard<std::mutex> lock(m_handlerMutex);
    auto it = m_handlers.find(type);
    if (it != m_handlers.end()) {
        try {
            it->second(msg);
            return true;
        } catch (const std::exception& e) {
            VIPER_LOG_IPC_ERR("Handler exception for {}: {}",
                              messageTypeToString(type), e.what());
            return false;
        }
    }

    VIPER_LOG_IPC_DEBUG("No handler registered for message type: {}",
                        messageTypeToString(type));
    return false;
}

ProtocolState MessageHandler::getState() const noexcept {
    return m_state.load(std::memory_order_acquire);
}

void MessageHandler::setState(ProtocolState state) {
    auto old = m_state.exchange(state, std::memory_order_release);
    if (old != state) {
        VIPER_LOG_IPC("Protocol state: {} -> {}",
                      protocolStateToString(old),
                      protocolStateToString(state));
    }
}

u32 MessageHandler::nextSequenceId() noexcept {
    return m_sequenceCounter.fetch_add(1, std::memory_order_relaxed);
}

void MessageHandler::reset() {
    std::lock_guard<std::mutex> lock(m_handlerMutex);
    m_handlers.clear();
    m_state.store(ProtocolState::Disconnected, std::memory_order_release);
    m_sequenceCounter.store(0, std::memory_order_release);
}

} 
} 
