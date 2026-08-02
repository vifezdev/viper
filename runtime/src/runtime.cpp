





#include <viper/runtime/runtime.h>
#include <viper/runtime/thread_pool.h>
#include <viper/common/logger.h>
#include <viper/common/platform.h>
#include <viper/common/version.h>
#include <viper/jni/jvm_manager.h>
#include <viper/jni/class_loader.h>
#include <viper/minecraft/detector.h>
#include <viper/minecraft/minecraft_core.h>
#include <viper/minecraft/renderer.h>
#include <viper/minecraft/event_system.h>
#include <viper/minecraft/module_system.h>
#include <viper/minecraft/notification_manager.h>
#include <viper/ipc/pipe_server.h>
#include <viper/ipc/message_handler.h>
#include <viper/ipc/protocol.h>

#include <filesystem>
#include <chrono>

namespace viper {





namespace {

const char* mcLauncherToString(minecraft::MinecraftLauncher l) {
    switch (l) {
        case minecraft::MinecraftLauncher::Vanilla: return "Vanilla";
        case minecraft::MinecraftLauncher::Forge:   return "Forge";
        case minecraft::MinecraftLauncher::Fabric:  return "Fabric";
        case minecraft::MinecraftLauncher::Lunar:   return "Lunar";
        default:                                     return "Unknown";
    }
}

const char* mcMappingToString(minecraft::MappingStyle m) {
    switch (m) {
        case minecraft::MappingStyle::Notch:         return "Notch";
        case minecraft::MappingStyle::MCP:           return "MCP";
        case minecraft::MappingStyle::Intermediary:  return "Intermediary";
        default:                                      return "Unknown";
    }
}

MinecraftLauncher convertLauncher(minecraft::MinecraftLauncher l) {
    switch (l) {
        case minecraft::MinecraftLauncher::Vanilla: return MinecraftLauncher::Vanilla;
        case minecraft::MinecraftLauncher::Forge:   return MinecraftLauncher::Forge;
        case minecraft::MinecraftLauncher::Fabric:  return MinecraftLauncher::Fabric;
        case minecraft::MinecraftLauncher::Lunar:   return MinecraftLauncher::LunarClient;
        default:                                     return MinecraftLauncher::Unknown;
    }
}

MappingStyle convertMapping(minecraft::MappingStyle m) {
    switch (m) {
        case minecraft::MappingStyle::Notch:         return MappingStyle::Notch;
        case minecraft::MappingStyle::MCP:           return MappingStyle::Mcp;
        case minecraft::MappingStyle::Intermediary:  return MappingStyle::Intermediary;
        default:                                      return MappingStyle::Unknown;
    }
}

} 





Runtime& Runtime::instance() {
    static Runtime s_instance;
    return s_instance;
}

Runtime::Runtime() {
    m_shutdownEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
}

Runtime::~Runtime() {
    if (m_shutdownEvent) {
        CloseHandle(m_shutdownEvent);
    }
}





void Runtime::run(HMODULE hModule) {
    if (m_running.load()) {
        return;  
    }

    m_running.store(true);
    m_hModule = hModule;

    if (!initialize(hModule))    { cleanup(); return; }
    if (!attachJVM())            { cleanup(); return; }
    if (!detectMinecraft())      { cleanup(); return; }
    if (!initializeFramework())  { cleanup(); return; }
    if (!startIPC())             { cleanup(); return; }

    mainLoop();
    cleanup();
}

void Runtime::shutdown() {
    if (m_running.load()) {
        VIPER_LOG_RUNTIME("Shutdown requested");
        m_running.store(false);
        if (m_shutdownEvent) {
            SetEvent(m_shutdownEvent);
        }
    }
}

bool Runtime::isRunning() const noexcept {
    return m_running.load();
}

const EnvironmentInfo& Runtime::getEnvironment() const noexcept {
    return m_envInfo;
}

HANDLE Runtime::getShutdownEvent() const noexcept {
    return m_shutdownEvent;
}





bool Runtime::initialize(HMODULE hModule) {
    
    wchar_t dllPath[MAX_PATH];
    GetModuleFileNameW(hModule, dllPath, MAX_PATH);
    std::filesystem::path dllDir = std::filesystem::path(dllPath).parent_path();
    std::string logDir = (dllDir / "logs").string();

    log::init(logDir);

    log::runtime()->info("============================================================");
    log::runtime()->info(" {} v{} — Runtime Starting", PROJECT_NAME, VERSION_STRING);
    log::runtime()->info("============================================================");

    platform::logPlatformInfo();

    
    m_threadPool = std::make_unique<ThreadPool>();
    VIPER_LOG_RUNTIME("Thread pool created ({} workers)", m_threadPool->size());

    return true;
}





bool Runtime::attachJVM() {
    VIPER_LOG_JNI("Searching for JVM...");

    auto& jvm = jni::JVMManager::getInstance();
    auto findResult = jvm.findJVM();
    if (findResult.isErr()) {
        VIPER_LOG_JNI_ERR("[ERROR] JVM not found: {}", findResult.error());
        return false;
    }
    VIPER_LOG_JNI("[OK] JVM found (jvm.dll located)");

    auto attachResult = jvm.attach();
    if (attachResult.isErr()) {
        VIPER_LOG_JNI_ERR("[ERROR] JVM attach failed: {}", attachResult.error());
        return false;
    }
    VIPER_LOG_JNI("[OK] Attached to JVM");

    std::string version = jvm.getJavaVersion();
    VIPER_LOG_JNI("[OK] Java version: {}", version);

    m_envInfo.javaVersion = version;
    m_envInfo.isX64 = (sizeof(void*) == 8);
    m_envInfo.processId = GetCurrentProcessId();

    return true;
}





bool Runtime::detectMinecraft() {
    VIPER_LOG_MC("Detecting Minecraft environment...");

    auto& jvm = jni::JVMManager::getInstance();
    JNIEnv* env = jvm.getEnv();
    if (!env) {
        VIPER_LOG_MC_ERR("[ERROR] JNIEnv not available");
        return false;
    }

    auto result = minecraft::MinecraftDetector::detect(env);
    if (result.isErr()) {
        VIPER_LOG_MC_ERR("[ERROR] Detection failed: {}", result.error());
        return false;
    }

    const auto& mcEnv = result.value();

    
    m_envInfo.minecraftVersion = mcEnv.version;
    m_envInfo.launcher = convertLauncher(mcEnv.launcher);
    m_envInfo.mappingStyle = convertMapping(mcEnv.mappingStyle);
    m_envInfo.processId = GetCurrentProcessId();

    
    m_detectedMcLauncher = mcEnv.launcher;
    m_detectedMcMapping = mcEnv.mappingStyle;

    VIPER_LOG_MC("============================================================");
    VIPER_LOG_MC(" Minecraft Environment Detected");
    VIPER_LOG_MC("============================================================");
    VIPER_LOG_MC("  Version:  {}", m_envInfo.minecraftVersion);
    VIPER_LOG_MC("  Launcher: {}", mcLauncherToString(mcEnv.launcher));
    VIPER_LOG_MC("  Mappings: {}", mcMappingToString(mcEnv.mappingStyle));
    VIPER_LOG_MC("  Java:     {}", m_envInfo.javaVersion);
    VIPER_LOG_MC("  Arch:     {}", m_envInfo.isX64 ? "x64" : "x86");
    VIPER_LOG_MC("  PID:      {}", m_envInfo.processId);
    VIPER_LOG_MC("============================================================");

    return true;
}





bool Runtime::initializeFramework() {
    VIPER_LOG_RUNTIME("Initializing framework subsystems...");

    
    m_eventSystem = std::make_unique<minecraft::EventSystem>();
    VIPER_LOG_RUNTIME("[OK] Event system initialized");

    
    m_moduleManager = std::make_unique<minecraft::ModuleManager>();
    VIPER_LOG_RUNTIME("[OK] Module system initialized");

    
    m_minecraftCore = minecraft::createMinecraftCore(
        m_detectedMcLauncher, m_detectedMcMapping);
    if (m_minecraftCore) {
        VIPER_LOG_RUNTIME("[OK] Minecraft core created for {}",
                          mcLauncherToString(m_detectedMcLauncher));
    } else {
        VIPER_LOG_RUNTIME_WARN("[WARN] Could not create MinecraftCore (unknown launcher)");
    }

    
    m_renderer = std::make_unique<minecraft::Renderer>();
    auto& jvm = jni::JVMManager::getInstance();
    JNIEnv* env = jvm.getEnv();
    if (env) {
        auto renderResult = m_renderer->initialize(env);
        if (renderResult.isOk()) {
            VIPER_LOG_RUNTIME("[OK] Renderer initialized");
        } else {
            VIPER_LOG_RUNTIME_WARN("[WARN] Renderer init: {}", renderResult.error());
        }
    }

    
    m_messageHandler = std::make_unique<ipc::MessageHandler>();
    setupMessageHandlers();
    VIPER_LOG_RUNTIME("[OK] Message handlers registered");

    return true;
}





bool Runtime::startIPC() {
    VIPER_LOG_IPC("Starting IPC server...");

    u32 pid = GetCurrentProcessId();
    m_pipeServer = std::make_unique<ipc::PipeServer>(pid);

    m_pipeServer->setMessageHandler([this](const ipc::Message& msg) {
        if (m_messageHandler) {
            m_messageHandler->dispatch(msg);
        }
    });

    auto result = m_pipeServer->start();
    if (result.isErr()) {
        VIPER_LOG_IPC_ERR("[ERROR] Failed to start IPC: {}", result.error());
        return false;
    }

    VIPER_LOG_IPC("[OK] IPC server listening (PID: {})", pid);
    return true;
}





void Runtime::mainLoop() {
    VIPER_LOG_RUNTIME("[READY] Viper runtime fully initialized");

    
    minecraft::NotificationManager::instance().notifySuccess("Viper Client Injected", "Viper Client");

    auto lastHeartbeat = std::chrono::steady_clock::now();

    while (m_running.load()) {
        
        DWORD waitResult = WaitForSingleObject(m_shutdownEvent, 100);

        if (waitResult == WAIT_OBJECT_0) {
            
            break;
        }

        
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - lastHeartbeat).count();

        if (elapsed >= ipc::HEARTBEAT_INTERVAL_MS) {
            if (m_pipeServer && m_pipeServer->isConnected()) {
                auto hb = ipc::Message::create(ipc::MessageType::Heartbeat);
                m_pipeServer->send(hb);
            }
            lastHeartbeat = now;
        }

        
        if (m_moduleManager) {
            m_moduleManager->tickAll();
        }
    }
}





void Runtime::cleanup() {
    VIPER_LOG_RUNTIME("Cleaning up...");

    
    if (m_pipeServer) {
        m_pipeServer->stop();
        m_pipeServer.reset();
    }

    
    if (m_threadPool) {
        m_threadPool->shutdown();
        m_threadPool.reset();
    }

    
    m_renderer.reset();
    m_minecraftCore.reset();
    m_moduleManager.reset();
    m_eventSystem.reset();
    m_messageHandler.reset();

    
    jni::ClassLoader::getInstance().shutdown();

    
    auto& jvm = jni::JVMManager::getInstance();
    if (jvm.isAttached()) {
        jvm.detach();
        VIPER_LOG_JNI("Detached from JVM");
    }

    m_running.store(false);
    VIPER_LOG_RUNTIME("Cleanup complete");

    log::shutdown();
}





void Runtime::setupMessageHandlers() {
    using namespace ipc;

    
    m_messageHandler->registerHandler(MessageType::ClientHello,
        [this](const Message& msg) {
            VIPER_LOG_IPC("Received CLIENT_HELLO");
            m_messageHandler->setState(ProtocolState::Ready);

            auto response = Message::create(
                MessageType::ServerHello,
                payloads::serverHello(VERSION_STRING, m_envInfo));
            m_pipeServer->send(response);

            m_messageHandler->setState(ProtocolState::Active);
        });

    
    m_messageHandler->registerHandler(MessageType::Heartbeat,
        [this](const Message&) {
            auto ack = Message::create(MessageType::HeartbeatAck);
            m_pipeServer->send(ack);
        });

    
    m_messageHandler->registerHandler(MessageType::StatusRequest,
        [this](const Message& msg) { handleStatusRequest(msg); });

    
    m_messageHandler->registerHandler(MessageType::EnvRequest,
        [this](const Message& msg) { handleEnvRequest(msg); });

    
    m_messageHandler->registerHandler(MessageType::Command,
        [this](const Message& msg) { handleCommand(msg); });

    
    m_messageHandler->registerHandler(MessageType::ModuleToggle,
        [this](const Message& msg) { handleModuleToggle(msg); });
}

void Runtime::handleStatusRequest(const ipc::Message& msg) {
    nlohmann::json payload = {
        {"status", m_running.load() ? "running" : "stopped"},
        {"uptime_ms", 0},
        {"ipc_connected", m_pipeServer ? m_pipeServer->isConnected() : false},
        {"modules_loaded", m_moduleManager
            ? static_cast<int>(m_moduleManager->getModules().size()) : 0},
    };

    auto response = ipc::Message::create(ipc::MessageType::StatusResponse, payload);
    m_pipeServer->send(response);
}

void Runtime::handleEnvRequest(const ipc::Message& msg) {
    auto response = ipc::Message::create(
        ipc::MessageType::EnvResponse,
        ipc::payloads::serverHello(VERSION_STRING, m_envInfo));
    m_pipeServer->send(response);
}

void Runtime::handleCommand(const ipc::Message& msg) {
    std::string command = msg.payload.value("command", "");
    VIPER_LOG_RUNTIME("Received command: {}", command);

    nlohmann::json response;
    if (command == "ping") {
        response = {{"result", "pong"}};
    } else if (command == "modules") {
        
        if (m_moduleManager) {
            std::string stateStr = m_moduleManager->serializeState();
            try {
                response = nlohmann::json::parse(stateStr);
            } catch (...) {
                response = {{"modules", stateStr}};
            }
        }
    } else {
        response = {{"error", "Unknown command: " + command}};
    }

    auto resp = ipc::Message::create(ipc::MessageType::CommandResponse, response);
    m_pipeServer->send(resp);
}

void Runtime::handleModuleToggle(const ipc::Message& msg) {
    std::string moduleName = msg.payload.value("module", "");
    if (m_moduleManager) {
        auto* mod = m_moduleManager->getModule(moduleName);
        if (mod) {
            mod->toggle();
            bool enabled = mod->isEnabled();
            nlohmann::json update = {
                {"module", moduleName},
                {"enabled", enabled},
            };
            auto resp = ipc::Message::create(ipc::MessageType::ModuleUpdate, update);
            m_pipeServer->send(resp);

            
            if (enabled) {
                minecraft::NotificationManager::instance().notifySuccess(moduleName + " Enabled", "Module Toggle");
            } else {
                minecraft::NotificationManager::instance().notifyWarning(moduleName + " Disabled", "Module Toggle");
            }
        }
    }
}

} 
