



#include <viper/common/platform.h>
#include <viper/common/types.h>
#include <viper/common/logger.h>

#include <Windows.h>
#include <winternl.h>
#include <TlHelp32.h>
#include <string>
#include <sstream>
#include <fstream>

namespace viper {





const char* errorCodeToString(ErrorCode code) noexcept {
    switch (code) {
        case ErrorCode::Success:                    return "Success";
        case ErrorCode::PlatformNotSupported:       return "Platform not supported";
        case ErrorCode::ArchitectureMismatch:       return "Architecture mismatch";
        case ErrorCode::CompilerMismatch:           return "Compiler mismatch";
        case ErrorCode::JvmNotFound:                return "JVM not found";
        case ErrorCode::JvmAttachFailed:            return "JVM attach failed";
        case ErrorCode::JvmVersionMismatch:         return "JVM version mismatch";
        case ErrorCode::JvmArchMismatch:            return "JVM architecture mismatch";
        case ErrorCode::JniError:                   return "JNI error";
        case ErrorCode::ClassNotFound:              return "Class not found";
        case ErrorCode::MethodNotFound:             return "Method not found";
        case ErrorCode::FieldNotFound:              return "Field not found";
        case ErrorCode::JniExceptionThrown:         return "JNI exception thrown";
        case ErrorCode::MinecraftNotFound:          return "Minecraft not found";
        case ErrorCode::MinecraftVersionMismatch:   return "Minecraft version mismatch";
        case ErrorCode::LauncherNotDetected:        return "Launcher not detected";
        case ErrorCode::ClassLoaderNotFound:        return "ClassLoader not found";
        case ErrorCode::IpcPipeCreateFailed:        return "IPC pipe creation failed";
        case ErrorCode::IpcConnectFailed:           return "IPC connection failed";
        case ErrorCode::IpcDisconnected:            return "IPC disconnected";
        case ErrorCode::IpcSendFailed:              return "IPC send failed";
        case ErrorCode::IpcReceiveFailed:           return "IPC receive failed";
        case ErrorCode::IpcHandshakeFailed:         return "IPC handshake failed";
        case ErrorCode::IpcTimeout:                 return "IPC timeout";
        case ErrorCode::IpcProtocolError:           return "IPC protocol error";
        case ErrorCode::RuntimeInitFailed:          return "Runtime initialization failed";
        case ErrorCode::RuntimeAlreadyRunning:      return "Runtime already running";
        case ErrorCode::RuntimeShutdownFailed:      return "Runtime shutdown failed";
        case ErrorCode::ThreadPoolError:            return "Thread pool error";
        case ErrorCode::BootstrapJavaNotFound:      return "Bootstrap: Java not found";
        case ErrorCode::BootstrapMinecraftNotFound: return "Bootstrap: Minecraft not found";
        case ErrorCode::BootstrapLaunchFailed:      return "Bootstrap: Launch failed";
        case ErrorCode::BootstrapInjectFailed:      return "Bootstrap: Injection failed";
        case ErrorCode::BootstrapValidationFailed:  return "Bootstrap: Validation failed";
        case ErrorCode::Unknown:                    return "Unknown error";
        default:                                    return "Unrecognized error code";
    }
}

const char* launcherToString(MinecraftLauncher launcher) noexcept {
    switch (launcher) {
        case MinecraftLauncher::Vanilla:     return "Vanilla";
        case MinecraftLauncher::Forge:       return "Forge";
        case MinecraftLauncher::Fabric:      return "Fabric";
        case MinecraftLauncher::LunarClient: return "Lunar Client";
        case MinecraftLauncher::Unknown:     return "Unknown";
        default:                             return "Unknown";
    }
}

const char* mappingStyleToString(MappingStyle style) noexcept {
    switch (style) {
        case MappingStyle::Notch:        return "Notch (Obfuscated)";
        case MappingStyle::Srg:          return "SRG (Searge)";
        case MappingStyle::Mcp:          return "MCP (Human-Readable)";
        case MappingStyle::Intermediary: return "Intermediary (Fabric)";
        case MappingStyle::Unknown:      return "Unknown";
        default:                         return "Unknown";
    }
}

namespace platform {

std::string getWindowsVersion() {
    
    using RtlGetVersionPtr = NTSTATUS(WINAPI*)(PRTL_OSVERSIONINFOW);
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (!hNtdll) return "Windows (unknown version)";

    auto rtlGetVersion = reinterpret_cast<RtlGetVersionPtr>(
        GetProcAddress(hNtdll, "RtlGetVersion"));
    if (!rtlGetVersion) return "Windows (unknown version)";

    RTL_OSVERSIONINFOW osvi{};
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    rtlGetVersion(&osvi);

    std::ostringstream ss;
    ss << "Windows " << osvi.dwMajorVersion << "." << osvi.dwMinorVersion
       << " Build " << osvi.dwBuildNumber;
    return ss.str();
}

std::string getCompilerVersion() {
    std::ostringstream ss;
    ss << "MSVC " << _MSC_VER
       << " (Full: " << _MSC_FULL_VER << ")";
    return ss.str();
}

bool isProcess64Bit() {
    
    return sizeof(void*) == 8;
}

bool isProcess64Bit(unsigned long pid) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProcess) return false;

    BOOL isWow64 = FALSE;
    
    BOOL result = IsWow64Process(hProcess, &isWow64);
    CloseHandle(hProcess);

    if (!result) return false;

    
    return !isWow64;
}

bool isExecutable64Bit(const std::string& path) {
    
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    
    IMAGE_DOS_HEADER dosHeader{};
    file.read(reinterpret_cast<char*>(&dosHeader), sizeof(dosHeader));
    if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE) return false;

    
    file.seekg(dosHeader.e_lfanew);
    DWORD peSignature = 0;
    file.read(reinterpret_cast<char*>(&peSignature), sizeof(peSignature));
    if (peSignature != IMAGE_NT_SIGNATURE) return false;

    
    IMAGE_FILE_HEADER fileHeader{};
    file.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));

    return fileHeader.Machine == IMAGE_FILE_MACHINE_AMD64;
}

unsigned long getCurrentProcessId() {
    return GetCurrentProcessId();
}

unsigned long getCurrentThreadId() {
    return GetCurrentThreadId();
}

void logPlatformInfo() {
    auto logger = log::boot();
    logger->info("========================================");
    logger->info(" Viper Client — Platform Information");
    logger->info("========================================");
    logger->info("  OS:       {}", getWindowsVersion());
    logger->info("  Compiler: {}", getCompilerVersion());
    logger->info("  Arch:     x64 ({})", isProcess64Bit() ? "confirmed" : "MISMATCH");
    logger->info("  PID:      {}", getCurrentProcessId());
    logger->info("  C++:      {}", __cplusplus);
    logger->info("========================================");
}

} 
} 
