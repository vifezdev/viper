#include <viper/minecraft/detector.h>
#include <viper/common/logger.h>

namespace viper {
namespace minecraft {

Result<EnvironmentInfo> MinecraftDetector::detect(JNIEnv* env) {
    if (!env) {
        return Result<EnvironmentInfo>::createError("JNIEnv is null");
    }

    EnvironmentInfo info;
    info.version = detectVersion(env);
    info.launcher = detectLauncher(env);
    info.mappingStyle = detectMappingStyle(env, info.launcher);

    return Result<EnvironmentInfo>::createSuccess(info);
}

std::string MinecraftDetector::detectVersion(JNIEnv* env) {
    
    jclass sharedConstantsClass = env->FindClass("net/minecraft/util/SharedConstants");
    if (sharedConstantsClass) {
        jfieldID versionField = env->GetStaticFieldID(sharedConstantsClass, "VERSION_STRING", "Ljava/lang/String;");
        if (versionField) {
            jstring versionStr = (jstring)env->GetStaticObjectField(sharedConstantsClass, versionField);
            if (versionStr) {
                const char* chars = env->GetStringUTFChars(versionStr, nullptr);
                std::string result(chars);
                env->ReleaseStringUTFChars(versionStr, chars);
                return result;
            }
        }
    }
    env->ExceptionClear();

    
    auto args = getJVMArguments(env);
    for (const auto& arg : args) {
        if (arg.find("--version") != std::string::npos && arg.find("1.8.9") != std::string::npos) {
            return "1.8.9";
        }
    }

    return "1.8.9"; 
}

MinecraftLauncher MinecraftDetector::detectLauncher(JNIEnv* env) {
    
    auto args = getJVMArguments(env);
    for (const auto& arg : args) {
        if (arg.find("lunar") != std::string::npos) {
            return MinecraftLauncher::Lunar;
        }
    }

    jclass lunarClass = env->FindClass("com/moonsworth/lunar/client/LunarClient");
    if (lunarClass) {
        return MinecraftLauncher::Lunar;
    }
    env->ExceptionClear();

    
    jclass forgeLoader = env->FindClass("net/minecraftforge/fml/common/Loader");
    jclass launchWrapper = env->FindClass("net/minecraft/launchwrapper/Launch");
    if (forgeLoader && launchWrapper) {
        return MinecraftLauncher::Forge;
    }
    env->ExceptionClear();

    
    jclass fabricLoader = env->FindClass("net/fabricmc/loader/api/FabricLoader");
    if (fabricLoader) {
        return MinecraftLauncher::Fabric;
    }
    env->ExceptionClear();

    
    jclass vanillaClass = env->FindClass("net/minecraft/client/Minecraft");
    jclass notchClass = env->FindClass("ave"); 
    if (vanillaClass || notchClass) {
        return MinecraftLauncher::Vanilla;
    }
    env->ExceptionClear();

    return MinecraftLauncher::Unknown;
}

MappingStyle MinecraftDetector::detectMappingStyle(JNIEnv* env, MinecraftLauncher launcher) {
    if (launcher == MinecraftLauncher::Forge) {
        jclass mcpClass = env->FindClass("net/minecraft/client/Minecraft");
        if (mcpClass) {
            return MappingStyle::MCP;
        }
        env->ExceptionClear();
    }
    
    if (launcher == MinecraftLauncher::Fabric) {
        return MappingStyle::Intermediary;
    }

    jclass notchClass = env->FindClass("ave");
    if (notchClass) {
        return MappingStyle::Notch;
    }
    env->ExceptionClear();

    return MappingStyle::Unknown;
}

std::vector<std::string> MinecraftDetector::getJVMArguments(JNIEnv* env) {
    std::vector<std::string> args;
    jclass managementFactory = env->FindClass("java/lang/management/ManagementFactory");
    if (!managementFactory) {
        env->ExceptionClear();
        return args;
    }

    jmethodID getRuntimeBean = env->GetStaticMethodID(managementFactory, "getRuntimeMXBean", "()Ljava/lang/management/RuntimeMXBean;");
    if (!getRuntimeBean) {
        env->ExceptionClear();
        return args;
    }

    jobject runtimeBean = env->CallStaticObjectMethod(managementFactory, getRuntimeBean);
    jclass runtimeBeanClass = env->GetObjectClass(runtimeBean);
    jmethodID getInputArgs = env->GetMethodID(runtimeBeanClass, "getInputArguments", "()Ljava/util/List;");
    
    jobject argsList = env->CallObjectMethod(runtimeBean, getInputArgs);
    jclass listClass = env->GetObjectClass(argsList);
    jmethodID listSize = env->GetMethodID(listClass, "size", "()I");
    jmethodID listGet = env->GetMethodID(listClass, "get", "(I)Ljava/lang/Object;");

    jint size = env->CallIntMethod(argsList, listSize);
    for (jint i = 0; i < size; ++i) {
        jstring argStr = (jstring)env->CallObjectMethod(argsList, listGet, i);
        const char* chars = env->GetStringUTFChars(argStr, nullptr);
        args.push_back(std::string(chars));
        env->ReleaseStringUTFChars(argStr, chars);
    }
    
    return args;
}

} 
} 
