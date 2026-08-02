#pragma once








#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>

namespace viper {





using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using f32 = float;
using f64 = double;

using usize = std::size_t;





enum class ErrorCode : u32 {
    Success                 = 0,

    
    PlatformNotSupported    = 100,
    ArchitectureMismatch    = 101,
    CompilerMismatch        = 102,

    
    JvmNotFound             = 200,
    JvmAttachFailed         = 201,
    JvmVersionMismatch      = 202,
    JvmArchMismatch         = 203,
    JniError                = 204,
    ClassNotFound           = 205,
    MethodNotFound          = 206,
    FieldNotFound           = 207,
    JniExceptionThrown      = 208,

    
    MinecraftNotFound       = 300,
    MinecraftVersionMismatch = 301,
    LauncherNotDetected     = 302,
    ClassLoaderNotFound     = 303,

    
    IpcPipeCreateFailed     = 400,
    IpcConnectFailed        = 401,
    IpcDisconnected         = 402,
    IpcSendFailed           = 403,
    IpcReceiveFailed        = 404,
    IpcHandshakeFailed      = 405,
    IpcTimeout              = 406,
    IpcProtocolError        = 407,

    
    RuntimeInitFailed       = 500,
    RuntimeAlreadyRunning   = 501,
    RuntimeShutdownFailed   = 502,
    ThreadPoolError         = 503,

    
    BootstrapJavaNotFound   = 600,
    BootstrapMinecraftNotFound = 601,
    BootstrapLaunchFailed   = 602,
    BootstrapInjectFailed   = 603,
    BootstrapValidationFailed = 604,

    
    Unknown                 = 9999,
};


const char* errorCodeToString(ErrorCode code) noexcept;





enum class MinecraftLauncher : u8 {
    Unknown     = 0,
    Vanilla     = 1,
    Forge       = 2,
    Fabric      = 3,
    LunarClient = 4,
};

const char* launcherToString(MinecraftLauncher launcher) noexcept;

enum class MappingStyle : u8 {
    Unknown       = 0,
    Notch         = 1,   
    Srg           = 2,   
    Mcp           = 3,   
    Intermediary  = 4,   
};

const char* mappingStyleToString(MappingStyle style) noexcept;





struct EnvironmentInfo {
    std::string minecraftVersion;
    MinecraftLauncher launcher     = MinecraftLauncher::Unknown;
    MappingStyle mappingStyle      = MappingStyle::Unknown;
    std::string javaVersion;
    std::string jvmPath;
    bool isX64                     = false;
    u32 processId                  = 0;
};

} 
