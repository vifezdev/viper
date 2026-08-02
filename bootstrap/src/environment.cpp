



#include <viper/bootstrap/environment.h>
#include <viper/common/logger.h>
#include <viper/common/platform.h>

#include <Windows.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <filesystem>
#include <fstream>
#include <algorithm>

namespace fs = std::filesystem;

namespace viper {
namespace bootstrap {





Result<JVMInfo> EnvironmentDiscovery::discoverJVM() {
    VIPER_LOG_BOOT("Searching for Java 8 x64 JVM...");

    
    std::string javaPath;

    
    auto envResult = findJavaViaEnvVar();
    if (envResult.isOk()) {
        javaPath = envResult.value();
        VIPER_LOG_BOOT("  Found via JAVA_HOME: {}", javaPath);
    }

    
    if (javaPath.empty()) {
        auto regResult = findJavaViaRegistry();
        if (regResult.isOk()) {
            javaPath = regResult.value();
            VIPER_LOG_BOOT("  Found via Registry: {}", javaPath);
        }
    }

    
    if (javaPath.empty()) {
        auto pathResult = findJavaViaPath();
        if (pathResult.isOk()) {
            javaPath = pathResult.value();
            VIPER_LOG_BOOT("  Found via PATH: {}", javaPath);
        }
    }

    
    if (javaPath.empty()) {
        auto commonResult = findJavaViaCommonPaths();
        if (commonResult.isOk()) {
            javaPath = commonResult.value();
            VIPER_LOG_BOOT("  Found in common path: {}", javaPath);
        }
    }

    if (javaPath.empty()) {
        return Result<JVMInfo>(err_tag,
            std::string("Java 8 x64 not found. Searched: JAVA_HOME, Registry, PATH, common paths."));
    }

    
    bool isX64 = validateArchitecture(javaPath);

    JVMInfo info;
    info.javaExePath = javaPath;
    info.isX64 = isX64;
    info.jdkHome = fs::path(javaPath).parent_path().parent_path().string();

    if (!isX64) {
        VIPER_LOG_BOOT_WARN("  [WARN] Java found but is NOT x64: {}", javaPath);
    }

    
    info.javaVersion = "1.8";  

    return Result<JVMInfo>(std::move(info));
}

Result<std::string> EnvironmentDiscovery::findJavaViaEnvVar() {
    wchar_t buffer[MAX_PATH];
    DWORD len = GetEnvironmentVariableW(L"JAVA_HOME", buffer, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        return Result<std::string>(err_tag, std::string("JAVA_HOME not set"));
    }

    fs::path javaHome(buffer);
    fs::path javaExe = javaHome / "bin" / "java.exe";
    if (fs::exists(javaExe)) {
        return Result<std::string>(javaExe.string());
    }

    return Result<std::string>(err_tag, std::string("java.exe not found in JAVA_HOME"));
}

Result<std::string> EnvironmentDiscovery::findJavaViaRegistry() {
    
    const wchar_t* regPaths[] = {
        L"SOFTWARE\\JavaSoft\\Java Runtime Environment",
        L"SOFTWARE\\JavaSoft\\Java Development Kit",
        L"SOFTWARE\\JavaSoft\\JDK",
    };

    for (const auto* regPath : regPaths) {
        HKEY hKey = nullptr;
        LSTATUS status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, regPath,
                                        0, KEY_READ | KEY_WOW64_64KEY, &hKey);
        if (status != ERROR_SUCCESS) continue;

        
        wchar_t version[64];
        DWORD versionSize = sizeof(version);
        status = RegQueryValueExW(hKey, L"CurrentVersion", nullptr, nullptr,
                                   reinterpret_cast<LPBYTE>(version), &versionSize);

        if (status == ERROR_SUCCESS) {
            
            HKEY hVersionKey = nullptr;
            status = RegOpenKeyExW(hKey, version, 0, KEY_READ | KEY_WOW64_64KEY,
                                    &hVersionKey);
            if (status == ERROR_SUCCESS) {
                wchar_t javaHome[MAX_PATH];
                DWORD homeSize = sizeof(javaHome);
                status = RegQueryValueExW(hVersionKey, L"JavaHome", nullptr, nullptr,
                                           reinterpret_cast<LPBYTE>(javaHome), &homeSize);
                RegCloseKey(hVersionKey);

                if (status == ERROR_SUCCESS) {
                    fs::path javaExe = fs::path(javaHome) / "bin" / "java.exe";
                    RegCloseKey(hKey);
                    if (fs::exists(javaExe)) {
                        return Result<std::string>(javaExe.string());
                    }
                }
            }
        }
        RegCloseKey(hKey);
    }

    return Result<std::string>(err_tag, std::string("Java not found in registry"));
}

Result<std::string> EnvironmentDiscovery::findJavaViaPath() {
    wchar_t foundPath[MAX_PATH];
    wchar_t* filePart = nullptr;
    DWORD result = SearchPathW(nullptr, L"java.exe", nullptr,
                                MAX_PATH, foundPath, &filePart);
    if (result > 0 && result < MAX_PATH) {
        std::wstring wpath(foundPath);
        return Result<std::string>(std::string(wpath.begin(), wpath.end()));
    }

    return Result<std::string>(err_tag, std::string("java.exe not found in PATH"));
}

Result<std::string> EnvironmentDiscovery::findJavaViaCommonPaths() {
    const fs::path commonRoots[] = {
        "C:\\Program Files\\Java",
        "C:\\Program Files\\Eclipse Adoptium",
        "C:\\Program Files\\AdoptOpenJDK",
        "C:\\Program Files\\Zulu",
    };

    for (const auto& root : commonRoots) {
        if (!fs::exists(root)) continue;

        for (const auto& entry : fs::directory_iterator(root)) {
            if (!entry.is_directory()) continue;
            std::string dirName = entry.path().filename().string();

            
            if (dirName.find("1.8") != std::string::npos ||
                dirName.find("jdk8") != std::string::npos ||
                dirName.find("jre8") != std::string::npos) {
                fs::path javaExe = entry.path() / "bin" / "java.exe";
                if (fs::exists(javaExe)) {
                    return Result<std::string>(javaExe.string());
                }
            }
        }
    }

    return Result<std::string>(err_tag, std::string("Java not found in common paths"));
}





bool EnvironmentDiscovery::validateArchitecture(const std::string& javaExePath) {
    return platform::isExecutable64Bit(javaExePath);
}





Result<MinecraftInstallInfo> EnvironmentDiscovery::discoverMinecraft() {
    VIPER_LOG_BOOT("Searching for Minecraft installation...");

    std::string mcDir = findMinecraftDir();
    if (mcDir.empty()) {
        return Result<MinecraftInstallInfo>(err_tag,
            std::string("Minecraft directory not found"));
    }

    VIPER_LOG_BOOT("  .minecraft: {}", mcDir);

    MinecraftInstallInfo info;
    info.minecraftDir = mcDir;
    info.librariesDir = (fs::path(mcDir) / "libraries").string();
    info.assetsDir = (fs::path(mcDir) / "assets").string();

    
    fs::path versionDir = fs::path(mcDir) / "versions" / "1.8.9";
    fs::path versionJar = versionDir / "1.8.9.jar";

    if (fs::exists(versionJar)) {
        info.versionJar = versionJar.string();
        VIPER_LOG_BOOT("  Version JAR: {}", info.versionJar);
    } else {
        VIPER_LOG_BOOT_WARN("  [WARN] 1.8.9.jar not found at: {}", versionJar.string());
    }

    
    fs::path nativesDir = versionDir / "natives";
    if (fs::exists(nativesDir)) {
        info.nativesDir = nativesDir.string();
    }

    return Result<MinecraftInstallInfo>(std::move(info));
}

std::string EnvironmentDiscovery::findMinecraftDir() {
    
    wchar_t appdata[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appdata))) {
        fs::path mcDir = fs::path(appdata) / ".minecraft";
        if (fs::exists(mcDir)) {
            return mcDir.string();
        }
    }

    return "";
}

MinecraftLauncher EnvironmentDiscovery::detectLauncher() {
    std::string mcDir = findMinecraftDir();
    if (mcDir.empty()) return MinecraftLauncher::Unknown;

    
    fs::path profilesPath = fs::path(mcDir) / "launcher_profiles.json";
    if (fs::exists(profilesPath)) {
        std::ifstream file(profilesPath);
        std::string content((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

        
        if (content.find("forge") != std::string::npos ||
            content.find("Forge") != std::string::npos) {
            return MinecraftLauncher::Forge;
        }
        
        if (content.find("fabric") != std::string::npos ||
            content.find("Fabric") != std::string::npos) {
            return MinecraftLauncher::Fabric;
        }
    }

    
    wchar_t localAppData[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, localAppData))) {
        fs::path lunarDir = fs::path(localAppData) / ".lunarclient";
        if (fs::exists(lunarDir)) {
            return MinecraftLauncher::LunarClient;
        }
    }

    return MinecraftLauncher::Vanilla;
}

} 
} 
