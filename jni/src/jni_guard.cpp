#include <viper/jni/jni_guard.h>
#include <viper/jni/jvm_manager.h>
#include <viper/common/logger.h>

namespace viper {
namespace jni {

JNIThreadGuard::JNIThreadGuard() {
    JVMManager::getInstance().attach();
}

JNIThreadGuard::~JNIThreadGuard() {
    JVMManager::getInstance().detach();
}


GlobalRefGuard::GlobalRefGuard(JNIEnv* env, jobject obj) : env_(env), ref_(nullptr) {
    if (env_ && obj) {
        ref_ = env_->NewGlobalRef(obj);
    }
}

GlobalRefGuard::~GlobalRefGuard() {
    if (env_ && ref_) {
        env_->DeleteGlobalRef(ref_);
    }
}

GlobalRefGuard::GlobalRefGuard(GlobalRefGuard&& other) noexcept : env_(other.env_), ref_(other.ref_) {
    other.ref_ = nullptr;
}

GlobalRefGuard& GlobalRefGuard::operator=(GlobalRefGuard&& other) noexcept {
    if (this != &other) {
        if (env_ && ref_) {
            env_->DeleteGlobalRef(ref_);
        }
        env_ = other.env_;
        ref_ = other.ref_;
        other.ref_ = nullptr;
    }
    return *this;
}


LocalRefGuard::LocalRefGuard(JNIEnv* env, jobject obj) : env_(env), ref_(obj) {
}

LocalRefGuard::~LocalRefGuard() {
    if (env_ && ref_) {
        env_->DeleteLocalRef(ref_);
    }
}


ExceptionGuard::ExceptionGuard(JNIEnv* env) : env_(env) {
}

ExceptionGuard::~ExceptionGuard() {
    if (env_ && env_->ExceptionCheck()) {
        env_->ExceptionDescribe();
        env_->ExceptionClear();
    }
}


JNIStringGuard::JNIStringGuard(JNIEnv* env, jstring jstr) : env_(env), jstr_(jstr), chars_(nullptr) {
    if (env_ && jstr_) {
        chars_ = env_->GetStringUTFChars(jstr_, nullptr);
    }
}

JNIStringGuard::~JNIStringGuard() {
    if (env_ && jstr_ && chars_) {
        env_->ReleaseStringUTFChars(jstr_, chars_);
    }
}

const char* JNIStringGuard::c_str() const {
    return chars_ ? chars_ : "";
}

std::string JNIStringGuard::str() const {
    return chars_ ? std::string(chars_) : std::string();
}

} 
} 
