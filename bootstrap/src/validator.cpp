



#include <viper/bootstrap/validator.h>
#include <viper/common/logger.h>
#include <viper/common/platform.h>

#include <Windows.h>
#include <filesystem>

namespace fs = std::filesystem;

namespace viper {
namespace bootstrap {

std::vector<ValidationResult> SystemValidator::validateAll(
    const std::string& javaPath,
    const std::string& runtimeDllPath)
{
    std::vector<ValidationResult> results;

    results.push_back(checkBootstrapArch());
    results.push_back(checkWindowsVersion());

    if (!javaPath.empty()) {
        results.push_back(checkJvmArch(javaPath));
    }

    if (!runtimeDllPath.empty()) {
        results.push_back(checkRuntimeDll(runtimeDllPath));
    }

    return results;
}

ValidationResult SystemValidator::checkBootstrapArch() {
    ValidationResult r;
    r.checkName = "Bootstrap Architecture";

    if constexpr (sizeof(void*) == 8) {
        r.status = ValidationStatus::OK;
        r.detail = "x64";
    } else {
        r.status = ValidationStatus::Error;
        r.detail = "NOT x64 — Viper requires 64-bit";
    }

    return r;
}

ValidationResult SystemValidator::checkJvmArch(const std::string& javaPath) {
    ValidationResult r;
    r.checkName = "JVM Architecture";

    if (!fs::exists(javaPath)) {
        r.status = ValidationStatus::Error;
        r.detail = "java.exe not found: " + javaPath;
        return r;
    }

    if (platform::isExecutable64Bit(javaPath)) {
        r.status = ValidationStatus::OK;
        r.detail = "x64 — " + javaPath;
    } else {
        r.status = ValidationStatus::Error;
        r.detail = "NOT x64 — " + javaPath + " (Java must be 64-bit)";
    }

    return r;
}

ValidationResult SystemValidator::checkRuntimeDll(const std::string& dllPath) {
    ValidationResult r;
    r.checkName = "Runtime DLL";

    if (!fs::exists(dllPath)) {
        r.status = ValidationStatus::Error;
        r.detail = "viper-runtime.dll not found: " + dllPath;
        return r;
    }

    if (platform::isExecutable64Bit(dllPath)) {
        r.status = ValidationStatus::OK;
        r.detail = "x64 — " + dllPath;
    } else {
        r.status = ValidationStatus::Error;
        r.detail = "NOT x64 — " + dllPath;
    }

    return r;
}

ValidationResult SystemValidator::checkWindowsVersion() {
    ValidationResult r;
    r.checkName = "Windows Version";

    std::string ver = platform::getWindowsVersion();
    r.status = ValidationStatus::OK;
    r.detail = ver;

    return r;
}

ValidationResult SystemValidator::checkDiskSpace(const std::string& path) {
    ValidationResult r;
    r.checkName = "Disk Space";

    try {
        auto spaceInfo = fs::space(path);
        double freeGB = static_cast<double>(spaceInfo.available) / (1024.0 * 1024.0 * 1024.0);

        if (freeGB >= 1.0) {
            r.status = ValidationStatus::OK;
            char buf[64];
            snprintf(buf, sizeof(buf), "%.1f GB free", freeGB);
            r.detail = buf;
        } else {
            r.status = ValidationStatus::Warning;
            r.detail = "Low disk space: less than 1 GB free";
        }
    } catch (...) {
        r.status = ValidationStatus::Warning;
        r.detail = "Could not check disk space";
    }

    return r;
}

void SystemValidator::logResults(const std::vector<ValidationResult>& results) {
    auto logger = log::boot();
    logger->info("============================================================");
    logger->info(" System Validation");
    logger->info("============================================================");

    bool allOk = true;
    for (const auto& r : results) {
        const char* statusStr = "???";
        switch (r.status) {
            case ValidationStatus::OK:      statusStr = "[OK]  "; break;
            case ValidationStatus::Warning: statusStr = "[WARN]"; break;
            case ValidationStatus::Error:   statusStr = "[FAIL]"; break;
        }

        logger->info("  [CHECK] {:30s} {} {}", r.checkName, statusStr, r.detail);

        if (r.status == ValidationStatus::Error) allOk = false;
    }

    logger->info("============================================================");
    if (allOk) {
        logger->info("  RESULT: All checks passed — READY");
    } else {
        logger->error("  RESULT: Validation FAILED — see errors above");
    }
    logger->info("============================================================");
}

} 
} 
