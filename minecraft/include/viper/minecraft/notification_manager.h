#pragma once








#include <viper/common/types.h>

#include <string>
#include <vector>
#include <chrono>
#include <mutex>

namespace viper {
namespace minecraft {

enum class NotificationType : u8 {
    Info    = 0,
    Success = 1,
    Warning = 2,
    Error   = 3,
};

struct Notification {
    u64 id = 0;
    std::string title;
    std::string message;
    NotificationType type = NotificationType::Info;
    u32 durationMs = 4000;
    std::chrono::steady_clock::time_point startTime;

    
    float getProgress() const;

    
    float getAlpha() const;

    
    float getSlideOffset() const;
};

class NotificationManager {
public:
    static NotificationManager& instance();

    
    NotificationManager(const NotificationManager&) = delete;
    NotificationManager& operator=(const NotificationManager&) = delete;

    
    void add(const std::string& title,
             const std::string& message,
             NotificationType type = NotificationType::Info,
             u32 durationMs = 4000);

    
    void notifySuccess(const std::string& message, const std::string& title = "Viper Client");
    void notifyInfo(const std::string& message, const std::string& title = "Viper Client");
    void notifyWarning(const std::string& message, const std::string& title = "Viper Client");
    void notifyError(const std::string& message, const std::string& title = "Viper Client");

    
    void render(int screenWidth, int screenHeight);

    
    void update();

    
    void clear();

    
    bool hasActiveNotifications() const;

private:
    NotificationManager() = default;
    ~NotificationManager() = default;

    
    void ensureFontInitialized();

    
    void drawRect(float x, float y, float width, float height, u8 r, u8 g, u8 b, u8 a);

    
    void drawOutlineRect(float x, float y, float width, float height, float lineWidth, u8 r, u8 g, u8 b, u8 a);

    
    void drawText(float x, float y, const std::string& text, u8 r, u8 g, u8 b, u8 a);

    std::vector<Notification> m_notifications;
    mutable std::mutex m_mutex;
    u64 m_nextId = 1;

    
    bool m_fontInitialized = false;
    unsigned int m_fontListBase = 0;
};

} 
} 
