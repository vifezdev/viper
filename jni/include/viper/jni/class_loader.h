#pragma once

#include <jni.h>
#include <viper/common/result.h>
#include <viper/common/types.h>
#include <string>
#include <unordered_map>

namespace viper {
namespace jni {

enum class MinecraftLauncher {
    Vanilla,
    Forge,
    Fabric,
    Lunar
};

class ClassLoader {
public:
    static ClassLoader& getInstance();

    void shutdown();
    void setClassLoaderMode(MinecraftLauncher mode);
    void clearCache();

    Result<jclass> findClass(const std::string& name);
    Result<jmethodID> getMethod(jclass cls, const std::string& name, const std::string& sig);
    Result<jmethodID> getStaticMethod(jclass cls, const std::string& name, const std::string& sig);
    Result<jfieldID> getField(jclass cls, const std::string& name, const std::string& sig);
    Result<jfieldID> getStaticField(jclass cls, const std::string& name, const std::string& sig);

private:
    ClassLoader() = default;
    ~ClassLoader();
    ClassLoader(const ClassLoader&) = delete;
    ClassLoader& operator=(const ClassLoader&) = delete;

    jclass findClassDirect(JNIEnv* env, const std::string& slashName);
    jclass findClassViaForge(JNIEnv* env, const std::string& dotName);
    jclass findClassViaFabric(JNIEnv* env, const std::string& dotName);

    MinecraftLauncher mode_ = MinecraftLauncher::Vanilla;

    std::unordered_map<std::string, jclass> classCache_;
    std::unordered_map<std::string, jmethodID> methodCache_;
    std::unordered_map<std::string, jfieldID> fieldCache_;

    std::string toDotName(const std::string& slashName);
    std::string toSlashName(const std::string& dotName);
};

} 
} 
