#pragma once








#include <jni.h>
#include <viper/common/result.h>
#include <viper/common/types.h>
#include <string>
#include <windows.h>

namespace viper {
namespace jni {

class JVMManager {
public:
    static JVMManager& getInstance();

    Result<void> findJVM();
    Result<void> attach();
    void detach();

    JNIEnv* getEnv();
    JavaVM* getJavaVM();
    std::string getJavaVersion();
    bool isAttached();

private:
    JVMManager() = default;
    ~JVMManager() = default;

    JVMManager(const JVMManager&) = delete;
    JVMManager& operator=(const JVMManager&) = delete;

    using JNI_GetCreatedJavaVMs_t = jint(JNICALL*)(JavaVM**, jsize, jsize*);

    JavaVM* javaVM_ = nullptr;
    HMODULE jvmModule_ = nullptr;
    JNI_GetCreatedJavaVMs_t getCreatedJavaVMs_ = nullptr;
};

} 
} 
