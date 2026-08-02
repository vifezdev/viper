#pragma once







#include <viper/common/types.h>
#include <viper/common/result.h>

#include <string>
#include <vector>

namespace viper {
namespace bootstrap {


struct JVMInfo {
    std::string javaExePath;     
    std::string javaVersion;     
    std::string jdkHome;         
    bool isX64 = false;
};


struct MinecraftInstallInfo {
    std::string minecraftDir;    
    std::string versionJar;      
    std::string librariesDir;    
    std::string assetsDir;       
    std::string nativesDir;      
    std::vector<std::string> classpath;  
};

class EnvironmentDiscovery {
public:
    
    Result<JVMInfo> discoverJVM();

    
    Result<MinecraftInstallInfo> discoverMinecraft();

    
    MinecraftLauncher detectLauncher();

    
    bool validateArchitecture(const std::string& javaExePath);

private:
    
    Result<std::string> findJavaViaRegistry();
    Result<std::string> findJavaViaEnvVar();
    Result<std::string> findJavaViaPath();
    Result<std::string> findJavaViaCommonPaths();

    
    std::string findMinecraftDir();
};

} 
} 
