# ============================================================================
# Viper Client — MSVC x64 Compiler Configuration
# ============================================================================

if(NOT MSVC)
    message(FATAL_ERROR "[VIPER] Only MSVC is supported. Current: ${CMAKE_CXX_COMPILER_ID}")
endif()

# ============================================================================
# Global Compile Options
# ============================================================================

add_compile_options(
    /W4             # Warning level 4
    /permissive-    # Strict C++ conformance
    /utf-8          # Source and execution character set UTF-8
    /EHsc           # Standard C++ exception handling
    /MP             # Multi-processor compilation
    /Zc:__cplusplus # Report correct __cplusplus value
    /Zc:inline      # Remove unreferenced COMDAT
    /Zc:preprocessor # Use conforming preprocessor
)

# ============================================================================
# Global Preprocessor Definitions
# ============================================================================

add_compile_definitions(
    WIN32_LEAN_AND_MEAN     # Exclude rarely-used Windows headers
    NOMINMAX                # Prevent min/max macro conflicts
    _CRT_SECURE_NO_WARNINGS # Suppress CRT security warnings
    UNICODE                 # Use Unicode APIs
    _UNICODE                # Use Unicode CRT functions
    _WIN64                  # Explicitly mark 64-bit
)

# ============================================================================
# Configuration-Specific Settings
# ============================================================================

# Debug: Full debug info, no optimization
set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi /RTC1 /DVIPER_DEBUG=1" CACHE STRING "" FORCE)

# Release: Full optimization, generate PDBs for crash analysis
set(CMAKE_CXX_FLAGS_RELEASE "/O2 /Oi /GL /DNDEBUG /DVIPER_RELEASE=1" CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/LTCG /OPT:REF /OPT:ICF /DEBUG:FULL" CACHE STRING "" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "/LTCG /OPT:REF /OPT:ICF /DEBUG:FULL" CACHE STRING "" FORCE)

# ============================================================================
# Runtime Library Enforcement
# ============================================================================

# Verify /MD is used (not /MT)
if(CMAKE_MSVC_RUNTIME_LIBRARY MATCHES "MultiThreadedDLL" OR
   CMAKE_MSVC_RUNTIME_LIBRARY MATCHES "MultiThreaded\\$")
    message(STATUS "[VIPER] Runtime: Dynamic MSVC Runtime (/MD)")
else()
    message(WARNING "[VIPER] Expected dynamic runtime (/MD). Got: ${CMAKE_MSVC_RUNTIME_LIBRARY}")
endif()
