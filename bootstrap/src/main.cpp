










#include <viper/bootstrap/environment.h>
#include <viper/bootstrap/launcher.h>
#include <viper/bootstrap/validator.h>
#include <viper/common/logger.h>
#include <viper/common/platform.h>
#include <viper/common/version.h>

#include <Windows.h>
#include <filesystem>
#include <string>
#include <iostream>
#include <vector>

namespace fs = std::filesystem;

using namespace viper;
using namespace viper::bootstrap;





enum class BootMode {
    Auto,       
    Inject,     
};

struct BootConfig {
    BootMode mode = BootMode::Auto;
    u32 targetPid = 0;
    std::string runtimeDllPath;
};

BootConfig parseArgs(int argc, char* argv[]) {
    BootConfig config;

    
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    fs::path exeDir = fs::path(exePath).parent_path();
    config.runtimeDllPath = (exeDir / "viper-runtime.dll").string();

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);

        if (arg == "--inject") {
            config.mode = BootMode::Inject;
            
            if (i + 1 < argc) {
                try {
                    config.targetPid = std::stoul(argv[i + 1]);
                    ++i;
                } catch (...) {
                    
                }
            }
        } else if (arg == "--dll") {
            if (i + 1 < argc) {
                config.runtimeDllPath = argv[++i];
            }
        }
    }

    return config;
}





int main(int argc, char* argv[]) {
    
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hConsole, &dwMode);
    SetConsoleMode(hConsole, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    
    log::init("logs");

    
    auto logger = log::boot();
    logger->info("");
    logger->info("  ██╗   ██╗██╗██████╗ ███████╗██████╗ ");
    logger->info("  ██║   ██║██║██╔══██╗██╔════╝██╔══██╗");
    logger->info("  ██║   ██║██║██████╔╝█████╗  ██████╔╝");
    logger->info("  ╚██╗ ██╔╝██║██╔═══╝ ██╔══╝  ██╔══██╗");
    logger->info("   ╚████╔╝ ██║██║     ███████╗██║  ██║");
    logger->info("    ╚═══╝  ╚═╝╚═╝     ╚══════╝╚═╝  ╚═╝");
    logger->info("");
    logger->info("  {} v{}", PROJECT_NAME, VERSION_STRING);
    logger->info("  Modern Minecraft 1.8.9 Framework");
    logger->info("");

    
    platform::logPlatformInfo();

    
    BootConfig config = parseArgs(argc, argv);

    
    
    
    logger->info("");
    SystemValidator validator;

    
    EnvironmentDiscovery discovery;
    auto jvmResult = discovery.discoverJVM();
    std::string javaPath;
    if (jvmResult.isOk()) {
        javaPath = jvmResult.value().javaExePath;
    }

    auto validationResults = validator.validateAll(javaPath, config.runtimeDllPath);
    SystemValidator::logResults(validationResults);

    
    bool hasFatalErrors = false;
    for (const auto& r : validationResults) {
        if (r.status == ValidationStatus::Error) {
            hasFatalErrors = true;
        }
    }

    
    
    
    logger->info("");
    logger->info("Discovering environment...");

    if (jvmResult.isErr()) {
        logger->error("[ERROR] {}", jvmResult.error());
        logger->error("Cannot continue without Java 8 x64.");
        logger->error("Set JAVA_HOME or install Java 8 JDK x64.");
        log::shutdown();
        return 1;
    }

    auto& jvmInfo = jvmResult.value();
    logger->info("[OK] Java: {} ({})", jvmInfo.javaExePath,
                 jvmInfo.isX64 ? "x64" : "x86");

    auto mcResult = discovery.discoverMinecraft();
    MinecraftInstallInfo mcInfo;
    if (mcResult.isOk()) {
        mcInfo = mcResult.value();
        logger->info("[OK] Minecraft: {}", mcInfo.minecraftDir);
    } else {
        logger->warn("[WARN] {}", mcResult.error());
    }

    MinecraftLauncher detectedLauncher = discovery.detectLauncher();
    logger->info("[OK] Launcher: {}", launcherToString(detectedLauncher));

    
    
    
    logger->info("");
    ProcessLauncher launcher;

    switch (config.mode) {
        case BootMode::Auto: {
            logger->info("Mode: AUTO (inject into running process)");
            logger->info("");

            
            auto processes = launcher.findMinecraftProcesses();
            if (!processes.empty()) {
                auto& proc = processes[0];
                logger->info("Found running Minecraft (PID: {}, exe: {})",
                             proc.pid, proc.exeName);

                auto injectResult = launcher.injectIntoRunning(
                    proc.pid, config.runtimeDllPath);
                if (injectResult.isOk()) {
                    logger->info("[OK] Injected successfully. Viper is running.");
                    log::shutdown();
                    return 0;
                } else {
                    logger->error("[ERROR] Injection failed: {}", injectResult.error());
                    log::shutdown();
                    return 1;
                }
            } else {
                logger->error("[ERROR] No running Minecraft process found. Please launch Minecraft first.");
                log::shutdown();
                return 1;
            }
            break;
        }

        case BootMode::Inject: {
            logger->info("Mode: INJECT");

            u32 pid = config.targetPid;

            if (pid == 0) {
                
                auto processes = launcher.findMinecraftProcesses();
                if (processes.empty()) {
                    logger->error("[ERROR] No running Minecraft process found");
                    log::shutdown();
                    return 1;
                }

                
                logger->info("Found {} Minecraft process(es):", processes.size());
                for (const auto& p : processes) {
                    logger->info("  PID: {} | {} | {}",
                                 p.pid, p.exeName, p.isX64 ? "x64" : "x86");
                }

                pid = processes[0].pid;
                logger->info("Targeting PID: {}", pid);
            }

            auto injectResult = launcher.injectIntoRunning(pid, config.runtimeDllPath);
            if (injectResult.isOk()) {
                logger->info("[OK] Injected successfully into PID {}", pid);
            } else {
                logger->error("[ERROR] {}", injectResult.error());
                log::shutdown();
                return 1;
            }
            break;
        }
    }

    logger->info("");
    logger->info("[DONE] Viper bootstrap complete.");
    log::shutdown();
    return 0;
}
