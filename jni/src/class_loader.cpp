#include <viper/jni/class_loader.h>
#include <viper/jni/jvm_manager.h>
#include <viper/common/logger.h>
#include <algorithm>

namespace viper {
namespace jni {

ClassLoader& ClassLoader::getInstance() {
    static ClassLoader instance;
    return instance;
}

ClassLoader::~ClassLoader() {
    
    
}

void ClassLoader::shutdown() {
    clearCache();
}

void ClassLoader::setClassLoaderMode(MinecraftLauncher mode) {
    mode_ = mode;
}

void ClassLoader::clearCache() {
    JNIEnv* env = JVMManager::getInstance().getEnv();
    if (env) {
        for (auto& pair : classCache_) {
            if (pair.second) {
                env->DeleteGlobalRef(pair.second);
            }
        }
    }
    classCache_.clear();
    methodCache_.clear();
    fieldCache_.clear();
}

std::string ClassLoader::toDotName(const std::string& slashName) {
    std::string res = slashName;
    std::replace(res.begin(), res.end(), '/', '.');
    return res;
}

std::string ClassLoader::toSlashName(const std::string& dotName) {
    std::string res = dotName;
    std::replace(res.begin(), res.end(), '.', '/');
    return res;
}

jclass ClassLoader::findClassDirect(JNIEnv* env, const std::string& slashName) {
    jclass cls = env->FindClass(slashName.c_str());
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    return cls;
}

jclass ClassLoader::findClassViaForge(JNIEnv* env, const std::string& dotName) {
    jclass launchClass = env->FindClass("net/minecraft/launchwrapper/Launch");
    if (!launchClass || env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }

    jfieldID classLoaderField = env->GetStaticFieldID(launchClass, "classLoader", "Lnet/minecraft/launchwrapper/LaunchClassLoader;");
    if (!classLoaderField || env->ExceptionCheck()) {
        env->ExceptionClear();
        env->DeleteLocalRef(launchClass);
        return nullptr;
    }

    jobject classLoaderObj = env->GetStaticObjectField(launchClass, classLoaderField);
    if (!classLoaderObj || env->ExceptionCheck()) {
        env->ExceptionClear();
        env->DeleteLocalRef(launchClass);
        return nullptr;
    }

    jclass launchClassLoaderClass = env->GetObjectClass(classLoaderObj);
    jmethodID findClassMethod = env->GetMethodID(launchClassLoaderClass, "findClass", "(Ljava/lang/String;)Ljava/lang/Class;");
    if (!findClassMethod || env->ExceptionCheck()) {
        env->ExceptionClear();
        env->DeleteLocalRef(launchClassLoaderClass);
        env->DeleteLocalRef(classLoaderObj);
        env->DeleteLocalRef(launchClass);
        return nullptr;
    }

    jstring jDotName = env->NewStringUTF(dotName.c_str());
    jclass result = static_cast<jclass>(env->CallObjectMethod(classLoaderObj, findClassMethod, jDotName));
    
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        result = nullptr;
    }

    env->DeleteLocalRef(jDotName);
    env->DeleteLocalRef(launchClassLoaderClass);
    env->DeleteLocalRef(classLoaderObj);
    env->DeleteLocalRef(launchClass);

    return result;
}

jclass ClassLoader::findClassViaFabric(JNIEnv* env, const std::string& dotName) {
    jclass threadClass = env->FindClass("java/lang/Thread");
    if (!threadClass || env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }

    jmethodID currentThreadMethod = env->GetStaticMethodID(threadClass, "currentThread", "()Ljava/lang/Thread;");
    if (!currentThreadMethod || env->ExceptionCheck()) {
        env->ExceptionClear();
        env->DeleteLocalRef(threadClass);
        return nullptr;
    }

    jobject currentThread = env->CallStaticObjectMethod(threadClass, currentThreadMethod);
    if (!currentThread || env->ExceptionCheck()) {
        env->ExceptionClear();
        env->DeleteLocalRef(threadClass);
        return nullptr;
    }

    jmethodID getContextClassLoaderMethod = env->GetMethodID(threadClass, "getContextClassLoader", "()Ljava/lang/ClassLoader;");
    if (!getContextClassLoaderMethod || env->ExceptionCheck()) {
        env->ExceptionClear();
        env->DeleteLocalRef(currentThread);
        env->DeleteLocalRef(threadClass);
        return nullptr;
    }

    jobject classLoader = env->CallObjectMethod(currentThread, getContextClassLoaderMethod);
    if (!classLoader || env->ExceptionCheck()) {
        env->ExceptionClear();
        env->DeleteLocalRef(currentThread);
        env->DeleteLocalRef(threadClass);
        return nullptr;
    }

    jclass classLoaderClass = env->GetObjectClass(classLoader);
    jmethodID loadClassMethod = env->GetMethodID(classLoaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
    if (!loadClassMethod || env->ExceptionCheck()) {
        env->ExceptionClear();
        env->DeleteLocalRef(classLoaderClass);
        env->DeleteLocalRef(classLoader);
        env->DeleteLocalRef(currentThread);
        env->DeleteLocalRef(threadClass);
        return nullptr;
    }

    jstring jDotName = env->NewStringUTF(dotName.c_str());
    jclass result = static_cast<jclass>(env->CallObjectMethod(classLoader, loadClassMethod, jDotName));

    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        result = nullptr;
    }

    env->DeleteLocalRef(jDotName);
    env->DeleteLocalRef(classLoaderClass);
    env->DeleteLocalRef(classLoader);
    env->DeleteLocalRef(currentThread);
    env->DeleteLocalRef(threadClass);

    return result;
}

Result<jclass> ClassLoader::findClass(const std::string& name) {
    JNIEnv* env = JVMManager::getInstance().getEnv();
    if (!env) {
        return {ResultError("JNIEnv not found")};
    }

    std::string slashName = toSlashName(name);
    auto it = classCache_.find(slashName);
    if (it != classCache_.end()) {
        return {it->second};
    }

    jclass localClass = findClassDirect(env, slashName);
    if (!localClass) {
        std::string dotName = toDotName(name);
        if (mode_ == MinecraftLauncher::Forge) {
            localClass = findClassViaForge(env, dotName);
        } else if (mode_ == MinecraftLauncher::Fabric) {
            localClass = findClassViaFabric(env, dotName);
        } else if (mode_ == MinecraftLauncher::Lunar) {
            localClass = findClassViaFabric(env, dotName); 
        } else {
            
            localClass = findClassViaForge(env, dotName);
            if (!localClass) {
                localClass = findClassViaFabric(env, dotName);
            }
        }
    }

    if (!localClass) {
        return {ResultError("Could not find class: " + name)};
    }

    jclass globalClass = static_cast<jclass>(env->NewGlobalRef(localClass));
    env->DeleteLocalRef(localClass);
    
    classCache_[slashName] = globalClass;
    return {globalClass};
}

Result<jmethodID> ClassLoader::getMethod(jclass cls, const std::string& name, const std::string& sig) {
    JNIEnv* env = JVMManager::getInstance().getEnv();
    if (!env || !cls) {
        return {ResultError("Invalid env or class")};
    }

    std::string key = name + sig; 
    auto it = methodCache_.find(key);
    if (it != methodCache_.end()) {
        return {it->second};
    }

    jmethodID method = env->GetMethodID(cls, name.c_str(), sig.c_str());
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        return {ResultError("Could not find method: " + name + sig)};
    }
    
    if (!method) {
        return {ResultError("Method ID is null: " + name + sig)};
    }

    methodCache_[key] = method;
    return {method};
}

Result<jmethodID> ClassLoader::getStaticMethod(jclass cls, const std::string& name, const std::string& sig) {
    JNIEnv* env = JVMManager::getInstance().getEnv();
    if (!env || !cls) {
        return {ResultError("Invalid env or class")};
    }

    std::string key = "static_" + name + sig;
    auto it = methodCache_.find(key);
    if (it != methodCache_.end()) {
        return {it->second};
    }

    jmethodID method = env->GetStaticMethodID(cls, name.c_str(), sig.c_str());
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        return {ResultError("Could not find static method: " + name + sig)};
    }
    
    if (!method) {
        return {ResultError("Static method ID is null: " + name + sig)};
    }

    methodCache_[key] = method;
    return {method};
}

Result<jfieldID> ClassLoader::getField(jclass cls, const std::string& name, const std::string& sig) {
    JNIEnv* env = JVMManager::getInstance().getEnv();
    if (!env || !cls) {
        return {ResultError("Invalid env or class")};
    }

    std::string key = name + sig;
    auto it = fieldCache_.find(key);
    if (it != fieldCache_.end()) {
        return {it->second};
    }

    jfieldID field = env->GetFieldID(cls, name.c_str(), sig.c_str());
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        return {ResultError("Could not find field: " + name + sig)};
    }
    
    if (!field) {
        return {ResultError("Field ID is null: " + name + sig)};
    }

    fieldCache_[key] = field;
    return {field};
}

Result<jfieldID> ClassLoader::getStaticField(jclass cls, const std::string& name, const std::string& sig) {
    JNIEnv* env = JVMManager::getInstance().getEnv();
    if (!env || !cls) {
        return {ResultError("Invalid env or class")};
    }

    std::string key = "static_" + name + sig;
    auto it = fieldCache_.find(key);
    if (it != fieldCache_.end()) {
        return {it->second};
    }

    jfieldID field = env->GetStaticFieldID(cls, name.c_str(), sig.c_str());
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        return {ResultError("Could not find static field: " + name + sig)};
    }
    
    if (!field) {
        return {ResultError("Static field ID is null: " + name + sig)};
    }

    fieldCache_[key] = field;
    return {field};
}

} 
} 
