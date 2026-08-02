# ============================================================================
# Viper Client — Dependency Management (FetchContent)
# ============================================================================

include(FetchContent)

# ============================================================================
# spdlog — Fast C++ logging library
# https://github.com/gabime/spdlog
# ============================================================================

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.15.0
    GIT_SHALLOW    TRUE
)

# Build spdlog as a static library (compiled, not header-only)
set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)

# ============================================================================
# nlohmann/json — JSON for Modern C++
# https://github.com/nlohmann/json
# ============================================================================

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)

set(JSON_BuildTests OFF CACHE BOOL "" FORCE)

# ============================================================================
# MinHook — The Minimalistic x86/x64 API Hooking Library
# https://github.com/TsudaKageyu/minhook
# ============================================================================

FetchContent_Declare(
    minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG        v1.3.3
    GIT_SHALLOW    TRUE
)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# ============================================================================
# Fetch All Dependencies
# ============================================================================

message(STATUS "[VIPER] Fetching dependencies...")
FetchContent_MakeAvailable(spdlog json minhook)
message(STATUS "[VIPER] Dependencies ready.")
