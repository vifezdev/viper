







#include <viper/minecraft/renderer.h>
#include <viper/minecraft/notification_manager.h>
#include <viper/jni/class_loader.h>
#include <viper/common/logger.h>

#include <MinHook.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <gl/GL.h>
#include <cstring>
#include <atomic>
#include <thread>

namespace viper {
namespace minecraft {


static Renderer* s_rendererInstance = nullptr;

Renderer* Renderer::getInstance() {
    return s_rendererInstance;
}





typedef BOOL(WINAPI* wglSwapBuffers_t)(HDC hdc);
static wglSwapBuffers_t g_targetWglSwapBuffers = nullptr;
static wglSwapBuffers_t g_originalWglSwapBuffers = nullptr;
static bool g_hookInstalled = false;

static std::atomic<int> g_activeFrames{0};
static std::atomic<bool> g_isShuttingDown{false};

struct OpenGLStateGuard {
    OpenGLStateGuard() {
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
        
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        
        glMatrixMode(GL_TEXTURE);
        glPushMatrix();
    }
    
    ~OpenGLStateGuard() {
        glMatrixMode(GL_TEXTURE);
        glPopMatrix();
        
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
        
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        
        glPopClientAttrib();
        glPopAttrib();
    }
    
    
    OpenGLStateGuard(const OpenGLStateGuard&) = delete;
    OpenGLStateGuard& operator=(const OpenGLStateGuard&) = delete;
};

static void executeRenderOverlay(HDC hdc) {
    if (wglGetCurrentContext() != nullptr) {
        auto* renderer = Renderer::getInstance();
        if (renderer && renderer->isReady()) {
            OpenGLStateGuard guard;
            renderer->onFrameEnd(hdc);
        }
    }
}

static void logCrashError() {
    static bool s_crashLogged = false;
    if (!s_crashLogged) {
        s_crashLogged = true;
        VIPER_LOG_MC_ERR("[CRASH_PREVENTED] Caught SEH exception in hkWglSwapBuffers. Render overlay safely isolated.");
    }
}


static void safeRenderOverlay(HDC hdc) {
    __try {
        executeRenderOverlay(hdc);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        logCrashError();
    }
}


static BOOL WINAPI hkWglSwapBuffers(HDC hdc) {
    if (g_isShuttingDown.load(std::memory_order_acquire)) {
        if (g_originalWglSwapBuffers) return g_originalWglSwapBuffers(hdc);
        return FALSE;
    }

    g_activeFrames.fetch_add(1, std::memory_order_acquire);
    
    
    if (g_isShuttingDown.load(std::memory_order_acquire)) {
        g_activeFrames.fetch_sub(1, std::memory_order_release);
        if (g_originalWglSwapBuffers) return g_originalWglSwapBuffers(hdc);
        return FALSE;
    }

    static bool s_firstFrameLogged = false;
    if (!s_firstFrameLogged) {
        s_firstFrameLogged = true;
        VIPER_LOG_MC("[HOOK] First wglSwapBuffers call intercepted on render thread (HDC: {})", static_cast<void*>(hdc));
    }

    
    safeRenderOverlay(hdc);

    BOOL result = FALSE;
    if (g_originalWglSwapBuffers) {
        result = g_originalWglSwapBuffers(hdc);
    }
    
    g_activeFrames.fetch_sub(1, std::memory_order_release);
    return result;
}

static bool installWglSwapBuffersHook() {
    if (g_hookInstalled) return true;

    
    MH_STATUS initStatus = MH_Initialize();
    VIPER_LOG_MC("[HOOK] MinHook initialize status: {}", MH_StatusToString(initStatus));
    if (initStatus != MH_OK && initStatus != MH_ERROR_ALREADY_INITIALIZED) {
        VIPER_LOG_MC_ERR("[HOOK] MinHook initialization failed: {}", MH_StatusToString(initStatus));
        return false;
    }

    
    HMODULE hOpengl32 = GetModuleHandleA("opengl32.dll");
    if (!hOpengl32) {
        hOpengl32 = LoadLibraryA("opengl32.dll");
    }
    if (!hOpengl32) {
        VIPER_LOG_MC_ERR("[HOOK] Failed to load opengl32.dll");
        return false;
    }

    g_targetWglSwapBuffers = reinterpret_cast<wglSwapBuffers_t>(
        GetProcAddress(hOpengl32, "wglSwapBuffers"));
    VIPER_LOG_MC("[HOOK] Target wglSwapBuffers address: {}", static_cast<void*>(g_targetWglSwapBuffers));

    if (!g_targetWglSwapBuffers) {
        VIPER_LOG_MC_ERR("[HOOK] GetProcAddress(wglSwapBuffers) failed");
        return false;
    }

    
    MH_STATUS createStatus = MH_CreateHook(
        reinterpret_cast<LPVOID>(g_targetWglSwapBuffers),
        reinterpret_cast<LPVOID>(&hkWglSwapBuffers),
        reinterpret_cast<LPVOID*>(&g_originalWglSwapBuffers));

    VIPER_LOG_MC("[HOOK] MinHook CreateHook status: {}", MH_StatusToString(createStatus));
    if (createStatus != MH_OK) {
        VIPER_LOG_MC_ERR("[HOOK] MH_CreateHook failed: {}", MH_StatusToString(createStatus));
        return false;
    }

    
    MH_STATUS enableStatus = MH_EnableHook(reinterpret_cast<LPVOID>(g_targetWglSwapBuffers));
    VIPER_LOG_MC("[HOOK] MinHook EnableHook status: {}", MH_StatusToString(enableStatus));
    if (enableStatus != MH_OK) {
        VIPER_LOG_MC_ERR("[HOOK] MH_EnableHook failed: {}", MH_StatusToString(enableStatus));
        return false;
    }

    g_hookInstalled = true;
    VIPER_LOG_MC("[OK] MinHook wglSwapBuffers detour active and ready.");
    return true;
}

static void removeWglSwapBuffersHook() {
    if (!g_hookInstalled || !g_targetWglSwapBuffers) return;

    MH_DisableHook(reinterpret_cast<LPVOID>(g_targetWglSwapBuffers));
    MH_RemoveHook(reinterpret_cast<LPVOID>(g_targetWglSwapBuffers));

    g_originalWglSwapBuffers = nullptr;
    g_hookInstalled = false;
    VIPER_LOG_MC("[HOOK] MinHook wglSwapBuffers detour removed.");
}





struct EnumWindowData {
    DWORD pid;
    HWND hWnd;
};

static BOOL CALLBACK EnumWindowsCallback(HWND hwnd, LPARAM lParam) {
    auto* data = reinterpret_cast<EnumWindowData*>(lParam);
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == data->pid && IsWindowVisible(hwnd)) {
        if (GetWindow(hwnd, GW_OWNER) == nullptr) {
            data->hWnd = hwnd;
            return FALSE; 
        }
    }
    return TRUE;
}

static HWND findProcessMainWindow() {
    EnumWindowData data{ GetCurrentProcessId(), nullptr };
    EnumWindows(EnumWindowsCallback, reinterpret_cast<LPARAM>(&data));
    return data.hWnd;
}





Result<void> Renderer::initialize(JNIEnv* env) {
    s_rendererInstance = this;

    
    if (env) {
        auto displayRes = jni::ClassLoader::getInstance().findClass("org/lwjgl/opengl/Display");
        if (displayRes.isOk() && displayRes.value()) {
            jclass displayClass = displayRes.value();
            jmethodID isCreatedMethod = env->GetStaticMethodID(displayClass, "isCreated", "()Z");
            if (isCreatedMethod) {
                jboolean created = env->CallStaticBooleanMethod(displayClass, isCreatedMethod);
                VIPER_LOG_MC("LWJGL Display class resolved (isCreated: {})", created ? "true" : "false");
            }
        } else {
            VIPER_LOG_MC_WARN("[WARN] Could not resolve org/lwjgl/opengl/Display via ClassLoader. Proceeding with native HWND & MinHook discovery.");
        }
    }

    
    hwnd_ = FindWindowA("LWJGL", nullptr);
    if (!hwnd_) {
        hwnd_ = FindWindowW(L"LWJGL", nullptr);
    }
    if (!hwnd_) {
        hwnd_ = findProcessMainWindow();
    }

    if (hwnd_) {
        VIPER_LOG_MC("[OK] Minecraft Window Handle found (HWND: {})", static_cast<void*>(hwnd_));
    } else {
        VIPER_LOG_MC_WARN("[WARN] Window handle not immediately found; will resolve dynamically on frame callback.");
    }

    
    if (!installWglSwapBuffersHook()) {
        return Result<void>::createError("Failed to install MinHook wglSwapBuffers hook");
    }

    ready_ = true;
    VIPER_LOG_MC("[OK] Renderer initialized and frame pipeline active.");
    return Result<void>::createSuccess();
}

void Renderer::onFrameStart() {
    if (!ready_) return;
}

void Renderer::onFrameEnd(HDC hdc) {
    if (!ready_) return;

    
    if (!hwnd_) {
        if (hdc) {
            hwnd_ = WindowFromDC(hdc);
        }
        if (!hwnd_) {
            hwnd_ = findProcessMainWindow();
        }
        if (!hwnd_) return;
    }

    RECT rect{};
    if (GetClientRect(hwnd_, &rect)) {
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        if (width > 0 && height > 0) {
            NotificationManager::instance().render(width, height);
        }
    }
}

HWND Renderer::getDisplayHandle() {
    return hwnd_;
}

bool Renderer::isContextReady() {
    return ready_ && g_hookInstalled;
}

void Renderer::shutdown() {
    g_isShuttingDown.store(true, std::memory_order_release);
    
    
    while (g_activeFrames.load(std::memory_order_acquire) > 0) {
        std::this_thread::yield();
    }

    ready_ = false;
    removeWglSwapBuffersHook();
    hwnd_ = nullptr;
    s_rendererInstance = nullptr;
}

} 
} 
