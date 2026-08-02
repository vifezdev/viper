# ============================================================================
# Viper Shared Library — Common types, logging, IPC protocol
# ============================================================================

add_library(viper-shared STATIC
    src/logger.cpp
    src/platform.cpp
)

target_include_directories(viper-shared
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(viper-shared
    PUBLIC
        spdlog::spdlog
        nlohmann_json::nlohmann_json
)

# Windows system libraries
target_link_libraries(viper-shared
    PUBLIC
        Advapi32.lib
)

target_compile_definitions(viper-shared
    PUBLIC
        VIPER_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        VIPER_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        VIPER_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)
