#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <viper/common/types.h>

namespace viper {
namespace minecraft {

enum class EventType {
    Tick,
    Render,
    KeyPress,
    KeyRelease,
    PacketSend,
    PacketReceive,
    WorldLoad,
    WorldUnload
};

struct EventData {
    EventType type;
    u64 timestamp;
    std::string payload; 
};

using EventHandler = std::function<void(const EventData&)>;

class EventSystem {
public:
    u64 subscribe(EventType type, EventHandler handler);
    void unsubscribe(u64 id);
    void emit(EventType type, const EventData& data);

private:
    std::unordered_map<EventType, std::unordered_map<u64, EventHandler>> listeners_;
    std::unordered_map<u64, EventType> listener_types_;
    std::shared_mutex mutex_;
    u64 next_id_ = 1;
};

} 
} 
