






#include <viper/bootstrap/launcher.h>
#include <viper/common/logger.h>
#include <viper/common/platform.h>

#include <TlHelp32.h>
#include <Psapi.h>
#include <winternl.h>
#include <filesystem>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

namespace viper {
namespace bootstrap {




using NtQueryInformationProcessFn = NTSTATUS(WINAPI*)(
    HANDLE, ULONG, PVOID, ULONG, PULONG);







Result<void> ProcessLauncher::injectIntoRunning(
    u32 pid, const std::string& runtimeDllPath)
{
    VIPER_LOG_BOOT("Injecting into process {} ...", pid);

    
    if (!platform::isProcess64Bit(pid)) {
        return Result<void>(err_tag,
            std::string("Target process is not x64. PID: ") + std::to_string(pid));
    }

    
    if (!fs::exists(runtimeDllPath)) {
        return Result<void>(err_tag,
            std::string("Runtime DLL not found: ") + runtimeDllPath);
    }

    if (!platform::isExecutable64Bit(runtimeDllPath)) {
        return Result<void>(err_tag,
            std::string("Runtime DLL is not x64: ") + runtimeDllPath);
    }

    
    std::string absPath = fs::absolute(runtimeDllPath).string();

    
    HANDLE hProcess = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, pid);

    if (!hProcess) {
        DWORD err = GetLastError();
        return Result<void>(err_tag,
            std::string("OpenProcess failed. Error: ") + std::to_string(err) +
            " (try running as administrator)");
    }

    
    size_t pathSize = absPath.size() + 1;
    LPVOID remoteMem = VirtualAllocEx(
        hProcess, nullptr, pathSize,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    if (!remoteMem) {
        DWORD err = GetLastError();
        CloseHandle(hProcess);
        return Result<void>(err_tag,
            std::string("VirtualAllocEx failed. Error: ") + std::to_string(err));
    }

    
    SIZE_T written = 0;
    BOOL writeOk = WriteProcessMemory(
        hProcess, remoteMem, absPath.c_str(), pathSize, &written);

    if (!writeOk || written != pathSize) {
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return Result<void>(err_tag, std::string("WriteProcessMemory failed"));
    }

    
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    FARPROC loadLibAddr = GetProcAddress(hKernel32, "LoadLibraryA");

    if (!loadLibAddr) {
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return Result<void>(err_tag, std::string("Could not resolve LoadLibraryA"));
    }

    
    HANDLE hThread = CreateRemoteThread(
        hProcess, nullptr, 0,
        reinterpret_cast<LPTHREAD_START_ROUTINE>(loadLibAddr),
        remoteMem, 0, nullptr);

    if (!hThread) {
        DWORD err = GetLastError();
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return Result<void>(err_tag,
            std::string("CreateRemoteThread failed. Error: ") + std::to_string(err));
    }

    
    WaitForSingleObject(hThread, 10000);

    
    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);

    
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    if (exitCode == 0) {
        return Result<void>(err_tag,
            std::string("LoadLibrary returned NULL — DLL failed to load in target"));
    }

    VIPER_LOG_BOOT("[OK] Runtime DLL injected into PID {}", pid);
    return Result<void>();
}





std::vector<MinecraftProcess> ProcessLauncher::findMinecraftProcesses() {
    std::vector<MinecraftProcess> results;

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return results;

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);

    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            HANDLE hProcess = OpenProcess(
                PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                FALSE, pe.th32ProcessID);

            if (!hProcess) continue;

            std::string jvmPath;
            if (hasJvmModule(hProcess, jvmPath)) {
                
                std::wstring cmdLine = getProcessCommandLine(hProcess);
                std::string cmdLineA(cmdLine.begin(), cmdLine.end());

                
                std::wstring wExe(pe.szExeFile);
                std::string exeName(wExe.begin(), wExe.end());

                
                std::string cmdLower = cmdLineA;
                std::transform(cmdLower.begin(), cmdLower.end(), cmdLower.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

                if (cmdLower.find("minecraft") != std::string::npos ||
                    cmdLower.find("net.minecraft") != std::string::npos ||
                    cmdLower.find("launchwrapper") != std::string::npos ||
                    cmdLower.find("lunar") != std::string::npos ||
                    cmdLower.find("fabric") != std::string::npos) {

                    MinecraftProcess proc;
                    proc.pid = pe.th32ProcessID;
                    proc.exeName = exeName;
                    proc.commandLine = cmdLineA;
                    proc.jvmDllPath = jvmPath;
                    proc.isX64 = platform::isProcess64Bit(pe.th32ProcessID);
                    results.push_back(proc);
                }
            }

            CloseHandle(hProcess);
        } while (Process32NextW(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);
    return results;
}





u32 ProcessLauncher::monitorProcess(HANDLE hProcess) {
    VIPER_LOG_BOOT("Monitoring process...");

    WaitForSingleObject(hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeThread(hProcess, &exitCode);

    VIPER_LOG_BOOT("Process exited with code: {}", exitCode);
    CloseHandle(hProcess);

    return exitCode;
}





bool ProcessLauncher::hasJvmModule(HANDLE hProcess, std::string& outPath) {
    HMODULE hMods[1024];
    DWORD cbNeeded = 0;

    if (!EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
        return false;
    }

    DWORD count = cbNeeded / sizeof(HMODULE);
    for (DWORD i = 0; i < count; ++i) {
        wchar_t modName[MAX_PATH];
        if (GetModuleFileNameExW(hProcess, hMods[i], modName,
                                  sizeof(modName) / sizeof(wchar_t))) {
            std::wstring name(modName);
            if (name.find(L"jvm.dll") != std::wstring::npos) {
                outPath = std::string(name.begin(), name.end());
                return true;
            }
        }
    }

    return false;
}

std::wstring ProcessLauncher::getProcessCommandLine(HANDLE hProcess) {
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (!hNtdll) return L"";

    auto NtQueryInformationProcess =
        reinterpret_cast<NtQueryInformationProcessFn>(
            GetProcAddress(hNtdll, "NtQueryInformationProcess"));
    if (!NtQueryInformationProcess) return L"";

    PROCESS_BASIC_INFORMATION pbi{};
    ULONG returnLen = 0;
    NTSTATUS status = NtQueryInformationProcess(
        hProcess, 0  , &pbi, sizeof(pbi), &returnLen);

    if (status != 0 || !pbi.PebBaseAddress) return L"";

    
    PEB peb{};
    SIZE_T bytesRead = 0;
    if (!ReadProcessMemory(hProcess, pbi.PebBaseAddress, &peb, sizeof(peb), &bytesRead)) {
        return L"";
    }

    
    RTL_USER_PROCESS_PARAMETERS params{};
    if (!ReadProcessMemory(hProcess, peb.ProcessParameters, &params, sizeof(params), &bytesRead)) {
        return L"";
    }

    if (params.CommandLine.Length == 0 || !params.CommandLine.Buffer) {
        return L"";
    }

    
    std::vector<wchar_t> cmdBuf(params.CommandLine.Length / sizeof(wchar_t) + 1, 0);
    if (!ReadProcessMemory(hProcess, params.CommandLine.Buffer,
                           cmdBuf.data(), params.CommandLine.Length, &bytesRead)) {
        return L"";
    }

    return std::wstring(cmdBuf.data());
}

} 
} 
