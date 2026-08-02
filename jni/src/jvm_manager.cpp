#include <viper/jni/jvm_manager.h>
#include <viper/common/logger.h>
#include <viper/jni/jni_guard.h>

namespace viper {
namespace jni {

struct ThreadEnvWrapper {
    JNIEnv* env = nullptr;
    bool attached_by_us = false;

    ~ThreadEnvWrapper() {
        if (env && attached_by_us) {
            auto* vm = JVMManager::getInstance().getJavaVM();
            if (vm) {
                vm->DetachCurrentThread();
            }
        }
    }
};
thread_local ThreadEnvWrapper tls_env_wrapper;

JVMManager& JVMManager::getInstance() {
    static JVMManager instance;
    return instance;
}

Result<void> JVMManager::findJVM() {
    jvmModule_ = GetModuleHandleA("jvm.dll");
    if (!jvmModule_) {
        return {ResultError("Could not find jvm.dll. Is it loaded in the current process?")};
    }

    getCreatedJavaVMs_ = reinterpret_cast<JNI_GetCreatedJavaVMs_t>(
        GetProcAddress(jvmModule_, "JNI_GetCreatedJavaVMs"));

    if (!getCreatedJavaVMs_) {
        return {ResultError("Could not find JNI_GetCreatedJavaVMs in jvm.dll")};
    }

    jsize numVMs = 0;
    jint status = getCreatedJavaVMs_(&javaVM_, 1, &numVMs);
    if (status != JNI_OK || numVMs == 0 || !javaVM_) {
        return {ResultError("Failed to get created Java VMs")};
    }

    return {};
}

Result<void> JVMManager::attach() {
    if (!javaVM_) {
        return {ResultError("JavaVM not initialized")};
    }

    bool attached_by_us = false;
    JNIEnv* env = nullptr;
    jint status = javaVM_->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_8);

    if (status == JNI_EDETACHED) {
        status = javaVM_->AttachCurrentThread(reinterpret_cast<void**>(&env), nullptr);
        if (status != JNI_OK) {
            return {ResultError("Failed to attach current thread to JavaVM")};
        }
        attached_by_us = true;
    } else if (status != JNI_OK) {
        return {ResultError("Failed to get JNIEnv")};
    }

    tls_env_wrapper.env = env;
    tls_env_wrapper.attached_by_us = attached_by_us;
    return {};
}

void JVMManager::detach() {
    if (javaVM_ && tls_env_wrapper.env) {
        if (tls_env_wrapper.attached_by_us) {
            javaVM_->DetachCurrentThread();
        }
        tls_env_wrapper.env = nullptr;
        tls_env_wrapper.attached_by_us = false;
    }
}

JNIEnv* JVMManager::getEnv() {
    if (tls_env_wrapper.env) {
        return tls_env_wrapper.env;
    }

    if (javaVM_) {
        JNIEnv* env = nullptr;
        if (javaVM_->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_8) == JNI_OK) {
            tls_env_wrapper.env = env;
            tls_env_wrapper.attached_by_us = false;
            return env;
        } else if (javaVM_->AttachCurrentThread(reinterpret_cast<void**>(&env), nullptr) == JNI_OK) {
            tls_env_wrapper.env = env;
            tls_env_wrapper.attached_by_us = true;
            return env;
        }
    }

    return nullptr;
}

JavaVM* JVMManager::getJavaVM() {
    return javaVM_;
}

std::string JVMManager::getJavaVersion() {
    JNIEnv* env = getEnv();
    if (!env) {
        return "Unknown (No JNIEnv)";
    }

    jclass sysClass = env->FindClass("java/lang/System");
    if (!sysClass) {
        env->ExceptionClear();
        return "Unknown (No System class)";
    }

    jmethodID getPropMethod = env->GetStaticMethodID(sysClass, "getProperty", "(Ljava/lang/String;)Ljava/lang/String;");
    if (!getPropMethod) {
        env->ExceptionClear();
        return "Unknown (No getProperty method)";
    }

    jstring propName = env->NewStringUTF("java.version");
    jstring versionStr = static_cast<jstring>(env->CallStaticObjectMethod(sysClass, getPropMethod, propName));
    
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        env->DeleteLocalRef(propName);
        env->DeleteLocalRef(sysClass);
        return "Unknown (Exception during getProperty)";
    }

    JNIStringGuard strGuard(env, versionStr);
    std::string result = strGuard.c_str() ? strGuard.c_str() : "Unknown";

    env->DeleteLocalRef(propName);
    env->DeleteLocalRef(versionStr);
    env->DeleteLocalRef(sysClass);

    return result;
}

bool JVMManager::isAttached() {
    return getEnv() != nullptr;
}

} 
} 
