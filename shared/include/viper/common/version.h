#pragma once





#include <string>

namespace viper {

#ifndef VIPER_VERSION_MAJOR
#define VIPER_VERSION_MAJOR 1
#endif

#ifndef VIPER_VERSION_MINOR
#define VIPER_VERSION_MINOR 0
#endif

#ifndef VIPER_VERSION_PATCH
#define VIPER_VERSION_PATCH 0
#endif

inline constexpr int VERSION_MAJOR = VIPER_VERSION_MAJOR;
inline constexpr int VERSION_MINOR = VIPER_VERSION_MINOR;
inline constexpr int VERSION_PATCH = VIPER_VERSION_PATCH;

inline const std::string VERSION_STRING =
    std::to_string(VERSION_MAJOR) + "." +
    std::to_string(VERSION_MINOR) + "." +
    std::to_string(VERSION_PATCH);

inline constexpr const char* PROJECT_NAME = "Viper Client";


inline constexpr const char* TARGET_MC_VERSION = "1.8.9";


inline constexpr int IPC_PROTOCOL_VERSION = 1;

} 
