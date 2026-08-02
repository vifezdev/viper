# ============================================================================
# Viper Runtime DLL — Loaded into Minecraft JVM process
# ============================================================================

add_library(viper-runtime SHARED
    src/dllmain.cpp
    src/runtime.cpp
    src/thread_pool.cpp
)

target_include_directories(viper-runtime
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(viper-runtime
    PRIVATE
        viper-shared
        viper-jni
        viper-ipc
        viper-minecraft
        Psapi.lib
)

# Export Agent_OnLoad for JVMTI agent mode
target_compile_definitions(viper-runtime PRIVATE
    VIPER_RUNTIME_EXPORTS
)

# Set output name
set_target_properties(viper-runtime PROPERTIES
    OUTPUT_NAME "viper-runtime"
    PREFIX ""
)
