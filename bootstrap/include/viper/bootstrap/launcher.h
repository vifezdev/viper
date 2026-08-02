#pragma once





#include <viper/common/types.h>
#include <viper/common/result.h>
#include <viper/bootstrap/environment.h>

#include <Windows.h>
#include <string>
#include <vector>

namespace viper {
namespace bootstrap {


struct MinecraftProcess {
    u32 pid = 0;
    std::string exeName;
    std::string commandLine;
    std::string jvmDllPath;
    bool isX64 = false;
};

class ProcessLauncher {
public:


    
    Result<void> injectIntoRunning(u32 pid, const std::string& runtimeDllPath);

    
    std::vector<MinecraftProcess> findMinecraftProcesses();

    
    u32 monitorProcess(HANDLE hProcess);

private:


    
    std::wstring getProcessCommandLine(HANDLE hProcess);

    
    bool hasJvmModule(HANDLE hProcess, std::string& outPath);
};

} 
} 
