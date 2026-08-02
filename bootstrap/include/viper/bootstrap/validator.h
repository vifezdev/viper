#pragma once





#include <viper/common/types.h>
#include <string>
#include <vector>

namespace viper {
namespace bootstrap {

enum class ValidationStatus : u8 {
    OK      = 0,
    Warning = 1,
    Error   = 2,
};

struct ValidationResult {
    std::string checkName;
    ValidationStatus status;
    std::string detail;
};

class SystemValidator {
public:
    
    std::vector<ValidationResult> validateAll(
        const std::string& javaPath = "",
        const std::string& runtimeDllPath = "");

    
    ValidationResult checkBootstrapArch();
    ValidationResult checkJvmArch(const std::string& javaPath);
    ValidationResult checkRuntimeDll(const std::string& dllPath);
    ValidationResult checkWindowsVersion();
    ValidationResult checkDiskSpace(const std::string& path);

    
    static void logResults(const std::vector<ValidationResult>& results);
};

} 
} 
