#pragma once








#include <string>






#ifndef _WIN64
    static_assert(false, "Viper Client requires Windows x64. _WIN64 is not defined.");
#endif


#ifndef _MSC_VER
    static_assert(false, "Viper Client requires MSVC. _MSC_VER is not defined.");
#endif


#if __cplusplus < 201703L && !defined(_MSVC_LANG)
    static_assert(false, "Viper Client requires C++17 or later.");
#elif defined(_MSVC_LANG) && _MSVC_LANG < 201703L
    static_assert(false, "Viper Client requires C++17 or later. Set /std:c++17.");
#endif


static_assert(sizeof(void*) == 8, "Viper Client requires 64-bit (x64) pointer size.");

namespace viper {
namespace platform {






std::string getWindowsVersion();


std::string getCompilerVersion();


bool isProcess64Bit();


bool isProcess64Bit(unsigned long pid);


bool isExecutable64Bit(const std::string& path);


unsigned long getCurrentProcessId();


unsigned long getCurrentThreadId();


void logPlatformInfo();

} 
} 
