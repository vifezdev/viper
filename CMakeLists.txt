# ============================================================================
# Viper Client — Modern Minecraft 1.8.9 Framework
# Root CMake Build Configuration
# ============================================================================
# Target: Windows x64 | MSVC 2022 | C++17 | Java 8 x64 | CMake + Ninja
# ============================================================================

cmake_minimum_required(VERSION 3.20)

project(viper-client
    VERSION 1.0.0
    DESCRIPTION "Modern Minecraft 1.8.9 Client Framework"
    LANGUAGES C CXX
)

# ============================================================================
# Global Standards
# ============================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MSVC Dynamic Runtime (/MD for Release, /MDd for Debug)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# ============================================================================
# Architecture Enforcement
# ============================================================================

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR
        "[VIPER] FATAL: 64-bit (x64) architecture required!\n"
        "  CMAKE_SIZEOF_VOID_P = ${CMAKE_SIZEOF_VOID_P} (expected 8)\n"
        "  Ensure you are using the x64 Native Tools Command Prompt for VS 2022."
    )
endif()

if(NOT MSVC)
    message(FATAL_ERROR
        "[VIPER] FATAL: MSVC compiler required!\n"
        "  CMAKE_CXX_COMPILER_ID = ${CMAKE_CXX_COMPILER_ID}\n"
        "  Use Visual Studio 2022 x64 toolchain."
    )
endif()

# ============================================================================
# Output Directories
# ============================================================================

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# Per-configuration output directories
foreach(CONFIG_TYPE Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER ${CONFIG_TYPE} CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CMAKE_BINARY_DIR}/bin/${CONFIG_TYPE}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CMAKE_BINARY_DIR}/bin/${CONFIG_TYPE}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CMAKE_BINARY_DIR}/lib/${CONFIG_TYPE}")
endforeach()

# ============================================================================
# Include Build Modules
# ============================================================================

include(cmake/CompilerConfig.cmake)
include(cmake/Dependencies.cmake)

# ============================================================================
# Java / JNI Detection
# ============================================================================

find_package(JNI REQUIRED)

if(JNI_FOUND)
    message(STATUS "[VIPER] JNI Found:")
    message(STATUS "  JNI Include:  ${JNI_INCLUDE_DIRS}")
    message(STATUS "  JNI Libraries: ${JNI_LIBRARIES}")
    message(STATUS "  JAVA_HOME:    $ENV{JAVA_HOME}")
else()
    message(FATAL_ERROR
        "[VIPER] FATAL: Java JNI not found!\n"
        "  Set JAVA_HOME to your Java 8 x64 JDK installation.\n"
        "  Example: set JAVA_HOME=C:\\Program Files\\Java\\jdk1.8.0_401"
    )
endif()

# ============================================================================
# Component Subdirectories
# ============================================================================

# Shared library (static) — common types, logging, IPC protocol
add_subdirectory(shared)

# JNI integration layer (static) — JVM bridge, class loading
add_subdirectory(jni)

# IPC layer (static) — named pipe server/client, message handling
add_subdirectory(ipc)

# Minecraft integration (static) — environment detection, abstraction
add_subdirectory(minecraft)

# Runtime DLL (shared) — loaded into Minecraft JVM process
add_subdirectory(runtime)

# Bootstrap executable — environment discovery, process launching
add_subdirectory(bootstrap)

# ============================================================================
# Build Summary
# ============================================================================

message(STATUS "")
message(STATUS "============================================================")
message(STATUS " Viper Client v${PROJECT_VERSION} — Build Configuration")
message(STATUS "============================================================")
message(STATUS "  Platform:      Windows x64")
message(STATUS "  Compiler:      MSVC ${MSVC_VERSION}")
message(STATUS "  C++ Standard:  C++17")
message(STATUS "  Runtime:       /MD (Dynamic)")
message(STATUS "  Build Type:    ${CMAKE_BUILD_TYPE}")
message(STATUS "  Generator:     ${CMAKE_GENERATOR}")
message(STATUS "  Output:        ${CMAKE_BINARY_DIR}/bin")
message(STATUS "============================================================")
message(STATUS "")
