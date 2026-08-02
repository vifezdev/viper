#pragma once








#include <viper/common/types.h>
#include <viper/common/result.h>

#include <Windows.h>
#include <atomic>
#include <memory>


namespace viper {
    namespace jni   { class JVMManager; }
    namespace ipc   { class PipeServer; class MessageHandler; struct Message; }
    namespace minecraft {
        class MinecraftDetector;
        class MinecraftCore;
        class Renderer;
        class EventSystem;
        class ModuleManager;

        
        enum class MinecraftLauncher;
        enum class MappingStyle;
    }
}

namespace viper {

class ThreadPool;

class Runtime {
public:
    
    static Runtime& instance();

    
    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;

    
    
    void run(HMODULE hModule);

    
    void shutdown();

    
    bool isRunning() const noexcept;

    
    const EnvironmentInfo& getEnvironment() const noexcept;

    
    HANDLE getShutdownEvent() const noexcept;

private:
    Runtime();
    ~Runtime();

    
    bool initialize(HMODULE hModule);
    bool attachJVM();
    bool detectMinecraft();
    bool initializeFramework();
    bool startIPC();
    void mainLoop();
    void cleanup();

    
    void setupMessageHandlers();
    void handleStatusRequest(const ipc::Message& msg);
    void handleEnvRequest(const ipc::Message& msg);
    void handleCommand(const ipc::Message& msg);
    void handleModuleToggle(const ipc::Message& msg);

    
    HMODULE m_hModule = nullptr;
    HANDLE m_shutdownEvent = nullptr;
    std::atomic<bool> m_running{false};
    EnvironmentInfo m_envInfo;

    
    std::unique_ptr<ipc::PipeServer> m_pipeServer;
    std::unique_ptr<ipc::MessageHandler> m_messageHandler;
    std::unique_ptr<minecraft::EventSystem> m_eventSystem;
    std::unique_ptr<minecraft::ModuleManager> m_moduleManager;
    std::unique_ptr<minecraft::MinecraftCore> m_minecraftCore;
    std::unique_ptr<minecraft::Renderer> m_renderer;
    std::unique_ptr<ThreadPool> m_threadPool;

    
    
    minecraft::MinecraftLauncher m_detectedMcLauncher{};
    minecraft::MappingStyle m_detectedMcMapping{};
};

} 
