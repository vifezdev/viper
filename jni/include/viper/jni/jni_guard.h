#pragma once

#include <jni.h>
#include <string>

namespace viper {
namespace jni {

class JNIThreadGuard {
public:
    JNIThreadGuard();
    ~JNIThreadGuard();

    JNIThreadGuard(const JNIThreadGuard&) = delete;
    JNIThreadGuard& operator=(const JNIThreadGuard&) = delete;
};

class GlobalRefGuard {
public:
    GlobalRefGuard(JNIEnv* env, jobject obj);
    ~GlobalRefGuard();

    GlobalRefGuard(const GlobalRefGuard&) = delete;
    GlobalRefGuard& operator=(const GlobalRefGuard&) = delete;

    GlobalRefGuard(GlobalRefGuard&& other) noexcept;
    GlobalRefGuard& operator=(GlobalRefGuard&& other) noexcept;

    jobject get() const { return ref_; }

private:
    JNIEnv* env_;
    jobject ref_;
};

class LocalRefGuard {
public:
    LocalRefGuard(JNIEnv* env, jobject obj);
    ~LocalRefGuard();

    LocalRefGuard(const LocalRefGuard&) = delete;
    LocalRefGuard& operator=(const LocalRefGuard&) = delete;

    jobject get() const { return ref_; }

private:
    JNIEnv* env_;
    jobject ref_;
};

class ExceptionGuard {
public:
    explicit ExceptionGuard(JNIEnv* env);
    ~ExceptionGuard();

    ExceptionGuard(const ExceptionGuard&) = delete;
    ExceptionGuard& operator=(const ExceptionGuard&) = delete;

private:
    JNIEnv* env_;
};

class JNIStringGuard {
public:
    JNIStringGuard(JNIEnv* env, jstring jstr);
    ~JNIStringGuard();

    JNIStringGuard(const JNIStringGuard&) = delete;
    JNIStringGuard& operator=(const JNIStringGuard&) = delete;

    const char* c_str() const;
    std::string str() const;

private:
    JNIEnv* env_;
    jstring jstr_;
    const char* chars_;
};

} 
} 
