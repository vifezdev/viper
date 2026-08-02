add_library(viper-jni STATIC
    src/jvm_manager.cpp
    src/class_loader.cpp
    src/jni_guard.cpp
)

target_link_libraries(viper-jni PUBLIC viper-shared JNI::JNI)
target_include_directories(viper-jni PUBLIC include ${JNI_INCLUDE_DIRS})
