# ============================================================================
# Viper IPC Layer — Named pipe server/client + message handling
# ============================================================================

add_library(viper-ipc STATIC
    src/pipe_server.cpp
    src/pipe_client.cpp
    src/message_handler.cpp
    src/protocol.cpp
)

target_include_directories(viper-ipc
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(viper-ipc
    PUBLIC
        viper-shared
    PRIVATE
        Advapi32.lib
)
