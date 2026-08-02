#pragma once

#include <jni.h>
#include <memory>
#include <viper/minecraft/detector.h>

namespace viper {
namespace minecraft {

class MinecraftCore {
public:
    virtual ~MinecraftCore() = default;
    
    virtual jobject getMinecraftInstance(JNIEnv* env) = 0;
    virtual jobject getPlayer(JNIEnv* env) = 0;
    virtual jobject getWorld(JNIEnv* env) = 0;
    virtual int getDisplayWidth(JNIEnv* env) = 0;
    virtual int getDisplayHeight(JNIEnv* env) = 0;
    virtual bool isInGame(JNIEnv* env) = 0;
};

class VanillaMinecraftCore : public MinecraftCore {
public:
    jobject getMinecraftInstance(JNIEnv* env) override;
    jobject getPlayer(JNIEnv* env) override;
    jobject getWorld(JNIEnv* env) override;
    int getDisplayWidth(JNIEnv* env) override;
    int getDisplayHeight(JNIEnv* env) override;
    bool isInGame(JNIEnv* env) override;
};

class ForgeMinecraftCore : public MinecraftCore {
public:
    jobject getMinecraftInstance(JNIEnv* env) override;
    jobject getPlayer(JNIEnv* env) override;
    jobject getWorld(JNIEnv* env) override;
    int getDisplayWidth(JNIEnv* env) override;
    int getDisplayHeight(JNIEnv* env) override;
    bool isInGame(JNIEnv* env) override;
};

std::unique_ptr<MinecraftCore> createMinecraftCore(MinecraftLauncher launcher, MappingStyle style);

} 
} 
