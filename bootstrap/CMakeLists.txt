# ============================================================================
# Viper Bootstrap — Environment discovery and process launching
# ============================================================================

add_executable(viper-bootstrap
    src/main.cpp
    src/environment.cpp
    src/launcher.cpp
    src/validator.cpp
)

target_include_directories(viper-bootstrap
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(viper-bootstrap
    PRIVATE
        viper-shared
        Psapi.lib
        Shlwapi.lib
        Advapi32.lib
)
