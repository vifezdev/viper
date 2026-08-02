#pragma once

#include <jni.h>
#include <string>
#include <vector>
#include <viper/common/result.h>
#include <viper/common/types.h>

namespace viper {
namespace minecraft {

enum class MinecraftLauncher {
    Vanilla,
    Forge,
    Fabric,
    Lunar,
    Unknown
};

enum class MappingStyle {
    Notch,
    MCP,
    Intermediary,
    Unknown
};

struct EnvironmentInfo {
    std::string version;
    MinecraftLauncher launcher;
    MappingStyle mappingStyle;
};

class MinecraftDetector {
public:
    static Result<EnvironmentInfo> detect(JNIEnv* env);
    static std::string detectVersion(JNIEnv* env);
    static MinecraftLauncher detectLauncher(JNIEnv* env);
    static MappingStyle detectMappingStyle(JNIEnv* env, MinecraftLauncher launcher);
    static std::vector<std::string> getJVMArguments(JNIEnv* env);
};

} 
} 
