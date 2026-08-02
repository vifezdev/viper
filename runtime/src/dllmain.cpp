






#include <viper/runtime/runtime.h>
#include <viper/common/types.h>

#include <Windows.h>
#include <jni.h>





static HANDLE g_shutdownEvent = nullptr;
static HMODULE g_hModule = nullptr;
static HANDLE g_workerThread = nullptr;





static DWORD WINAPI WorkerThread(LPVOID lpParam) {
    HMODULE hModule = reinterpret_cast<HMODULE>(lpParam);
    viper::Runtime::instance().run(hModule);
    return 0;
}





BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
    switch (reason) {
        case DLL_PROCESS_ATTACH: {
            
            DisableThreadLibraryCalls(hModule);

            g_hModule = hModule;

            
            g_shutdownEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

            
            
            HANDLE hThread = CreateThread(
                nullptr,    
                0,          
                WorkerThread,
                hModule,    
                0,          
                nullptr     
            );

            if (hThread) {
                g_workerThread = hThread;
            }
            break;
        }

        case DLL_PROCESS_DETACH: {
            
            viper::Runtime::instance().shutdown();

            if (g_shutdownEvent) {
                SetEvent(g_shutdownEvent);
            }

            
            if (g_workerThread) {
                WaitForSingleObject(g_workerThread, 2000);
                CloseHandle(g_workerThread);
                g_workerThread = nullptr;
            }

            if (g_shutdownEvent) {
                CloseHandle(g_shutdownEvent);
                g_shutdownEvent = nullptr;
            }
            break;
        }
    }

    return TRUE;
}








extern "C" JNIEXPORT jint JNICALL Agent_OnLoad(
    JavaVM* vm, char* options, void* reserved)
{
    
    HMODULE hModule = nullptr;
    GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&Agent_OnLoad),
        &hModule);

    g_hModule = hModule;
    g_shutdownEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

    
    HANDLE hThread = CreateThread(nullptr, 0, WorkerThread, hModule, 0, nullptr);
    if (hThread) {
        g_workerThread = hThread;
    }

    return JNI_OK;
}

extern "C" JNIEXPORT void JNICALL Agent_OnUnload(JavaVM* vm) {
    viper::Runtime::instance().shutdown();
    if (g_shutdownEvent) {
        SetEvent(g_shutdownEvent);
    }
    
    if (g_workerThread) {
        WaitForSingleObject(g_workerThread, 2000);
        CloseHandle(g_workerThread);
        g_workerThread = nullptr;
    }

    if (g_shutdownEvent) {
        CloseHandle(g_shutdownEvent);
        g_shutdownEvent = nullptr;
    }
}
