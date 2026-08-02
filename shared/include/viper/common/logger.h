#pragma once








#include <string>
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>

namespace viper {
namespace log {





inline constexpr const char* LOGGER_BOOT    = "BOOT";
inline constexpr const char* LOGGER_RUNTIME = "RUNTIME";
inline constexpr const char* LOGGER_JNI     = "JNI";
inline constexpr const char* LOGGER_MC      = "MINECRAFT";
inline constexpr const char* LOGGER_IPC     = "IPC";
inline constexpr const char* LOGGER_CLIENT  = "CLIENT";









void init(const std::string& logDir = "logs",
          spdlog::level::level_enum level = spdlog::level::debug);


void shutdown();






std::shared_ptr<spdlog::logger> get(const char* name);


inline std::shared_ptr<spdlog::logger> boot()     { return get(LOGGER_BOOT); }
inline std::shared_ptr<spdlog::logger> runtime()  { return get(LOGGER_RUNTIME); }
inline std::shared_ptr<spdlog::logger> jni()      { return get(LOGGER_JNI); }
inline std::shared_ptr<spdlog::logger> mc()       { return get(LOGGER_MC); }
inline std::shared_ptr<spdlog::logger> ipc()      { return get(LOGGER_IPC); }
inline std::shared_ptr<spdlog::logger> client()   { return get(LOGGER_CLIENT); }

} 
} 







#define VIPER_LOG_BOOT(...)    viper::log::boot()->info(__VA_ARGS__)
#define VIPER_LOG_RUNTIME(...) viper::log::runtime()->info(__VA_ARGS__)
#define VIPER_LOG_JNI(...)     viper::log::jni()->info(__VA_ARGS__)
#define VIPER_LOG_MC(...)      viper::log::mc()->info(__VA_ARGS__)
#define VIPER_LOG_IPC(...)     viper::log::ipc()->info(__VA_ARGS__)
#define VIPER_LOG_CLIENT(...)  viper::log::client()->info(__VA_ARGS__)

#define VIPER_LOG_BOOT_ERR(...)    viper::log::boot()->error(__VA_ARGS__)
#define VIPER_LOG_RUNTIME_ERR(...) viper::log::runtime()->error(__VA_ARGS__)
#define VIPER_LOG_JNI_ERR(...)     viper::log::jni()->error(__VA_ARGS__)
#define VIPER_LOG_MC_ERR(...)      viper::log::mc()->error(__VA_ARGS__)
#define VIPER_LOG_IPC_ERR(...)     viper::log::ipc()->error(__VA_ARGS__)
#define VIPER_LOG_CLIENT_ERR(...)  viper::log::client()->error(__VA_ARGS__)

#define VIPER_LOG_BOOT_WARN(...)    viper::log::boot()->warn(__VA_ARGS__)
#define VIPER_LOG_RUNTIME_WARN(...) viper::log::runtime()->warn(__VA_ARGS__)
#define VIPER_LOG_JNI_WARN(...)     viper::log::jni()->warn(__VA_ARGS__)
#define VIPER_LOG_MC_WARN(...)      viper::log::mc()->warn(__VA_ARGS__)
#define VIPER_LOG_IPC_WARN(...)     viper::log::ipc()->warn(__VA_ARGS__)
#define VIPER_LOG_CLIENT_WARN(...)  viper::log::client()->warn(__VA_ARGS__)

#define VIPER_LOG_BOOT_DEBUG(...)    viper::log::boot()->debug(__VA_ARGS__)
#define VIPER_LOG_RUNTIME_DEBUG(...) viper::log::runtime()->debug(__VA_ARGS__)
#define VIPER_LOG_JNI_DEBUG(...)     viper::log::jni()->debug(__VA_ARGS__)
#define VIPER_LOG_MC_DEBUG(...)      viper::log::mc()->debug(__VA_ARGS__)
#define VIPER_LOG_IPC_DEBUG(...)     viper::log::ipc()->debug(__VA_ARGS__)
#define VIPER_LOG_CLIENT_DEBUG(...)  viper::log::client()->debug(__VA_ARGS__)
