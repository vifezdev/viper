#include <viper/minecraft/event_system.h>

namespace viper {
namespace minecraft {

u64 EventSystem::subscribe(EventType type, EventHandler handler) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    u64 id = next_id_++;
    listeners_[type][id] = std::move(handler);
    listener_types_[id] = type;
    return id;
}

void EventSystem::unsubscribe(u64 id) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    auto it = listener_types_.find(id);
    if (it != listener_types_.end()) {
        EventType type = it->second;
        listeners_[type].erase(id);
        listener_types_.erase(it);
    }
}

void EventSystem::emit(EventType type, const EventData& data) {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = listeners_.find(type);
    if (it != listeners_.end()) {
        for (const auto& [id, handler] : it->second) {
            handler(data);
        }
    }
}

} 
} 
