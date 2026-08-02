#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <jni.h>
#include <viper/common/result.h>

namespace viper {
namespace minecraft {

class Renderer {
public:
    static Renderer* getInstance();

    Result<void> initialize(JNIEnv* env);
    
    void onFrameStart();
    void onFrameEnd(HDC hdc = nullptr);
    
    HWND getDisplayHandle();
    bool isContextReady();
    bool isReady() const noexcept { return ready_; }
    void shutdown();
    
private:
    HWND hwnd_ = nullptr;
    bool ready_ = false;
};

} 
} 
