cmake_minimum_required(VERSION 3.20)

add_library(minhook STATIC
    ${minhook_SOURCE_DIR}/src/buffer.c
    ${minhook_SOURCE_DIR}/src/hook.c
    ${minhook_SOURCE_DIR}/src/trampoline.c
    ${minhook_SOURCE_DIR}/src/hde/hde64.c
)
target_include_directories(minhook PUBLIC ${minhook_SOURCE_DIR}/include)

add_library(viper-minecraft STATIC
    src/detector.cpp
    src/minecraft_core.cpp
    src/renderer.cpp
    src/event_system.cpp
    src/module_system.cpp
    src/notification_manager.cpp
)

target_include_directories(viper-minecraft PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${minhook_SOURCE_DIR}/include
)

target_link_libraries(viper-minecraft PUBLIC viper-shared viper-jni minhook opengl32.lib)
