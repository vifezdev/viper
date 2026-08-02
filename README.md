# Viper | Minecraft 1.8.9 Ghost Client Runtime Framework
> A modern, modular C++ injection and runtime framework for Minecraft 1.8.9.

<img width="1272" height="744" alt="Group 88" src="https://github.com/user-attachments/assets/e62d7c50-ecf1-402f-a122-2449eec751ff" />

## Overview

Viper is a clean, headless injector engine and foundation designed for Minecraft 1.8.9. 

**Note:** This is *not* a finished, usable gameplay client. It is a "blank slate" developer framework. It provides all the necessary low level scaffolding, including JNI reflection, lock free OpenGL rendering hooks, and binary IPC communication... **so you can focus on building your own custom modules and features without reinventing the wheel.**

## Key Features

- **Multi Environment Support:** Safely resolves Java classes across Vanilla, Forge, and Lunar Client environments.
- **Thread Safe Architecture:** Built with modern C++17, utilizing RAII wrappers, atomic hazard pointers, and a custom thread pool to guarantee a 100% crash free DLL injection and ejection lifecycle.
- **Safe OpenGL Hooks:** Integrates MinHook for `wglSwapBuffers` with strict OpenGL state preservation, enabling you to safely render 2D overlays in game without visual corruption.
- **High Performance IPC:** Implements an asynchronous Named Pipe server for binary framed JSON communication with external controllers.

## Architecture

```mermaid
flowchart LR
    A[viper-bootstrap.exe<br/><br/>• Process scanning<br/>• JVM validation<br/>• DLL injection]
    B[viper-runtime.dll<br/><br/>• JNI Integration<br/>• Event System<br/>• Module Framework<br/>• OpenGL Renderer]

    A -->|Inject DLL| B

    style A fill:#1f2937,stroke:#60a5fa,color:#fff
    style B fill:#111827,stroke:#22c55e,color:#fff
```

## Components
```mermaid
flowchart TB

    S["viper-shared<br/><br/>Static Library<br/>• Types<br/>• Logging (spdlog)<br/>• Platform<br/>• IPC Protocol"]

    J["viper-jni<br/><br/>Static Library<br/>• JVM Attachment<br/>• Class Loading<br/>• JNI Guards"]

    I["viper-ipc<br/><br/>Static Library<br/>• Named Pipes<br/>• Message Handler<br/>• IPC Protocol"]

    M["viper-minecraft<br/><br/>Static Library<br/>• MC Detection<br/>• Core Abstraction<br/>• Events<br/>• Modules"]

    R["viper-runtime.dll<br/><br/>Runtime DLL<br/>• Runtime Coordinator<br/>• Loaded Into Minecraft"]

    B["viper-bootstrap.exe<br/><br/>Executable<br/>• Environment Discovery<br/>• JVM Validation<br/>• DLL Injection"]

    B --> R
    R --> J
    R --> I
    R --> M
    J --> S
    I --> S
    M --> S

    style B fill:#1f2937,stroke:#60a5fa,color:#fff
    style R fill:#111827,stroke:#22c55e,color:#fff
    style M fill:#312e81,stroke:#818cf8,color:#fff
    style J fill:#374151,stroke:#a78bfa,color:#fff
    style I fill:#374151,stroke:#34d399,color:#fff
    style S fill:#374151,stroke:#fbbf24,color:#fff
```

## Supported Environments

| Environment | Detection | Class Loading |
|-------------|-----------|---------------|
| Vanilla 1.8.9 | JVM args, main class | `FindClass` |
| Forge 1.8.9 | LaunchWrapper classes | `LaunchClassLoader.findClass` |
| Lunar Client | Lunar bootstrap | Context classloader |

*Note: Lunar Client is supported and compatible. Other custom or unlisted launchers are not guaranteed to work.*

## Build Requirements

- **Windows x64** (Windows 10+)
- **Visual Studio 2022** with C++ Desktop Development workload
- **CMake 3.20+** with Ninja generator
- **Java 8 JDK x64** (Ensure `JAVA_HOME` is set)

## Quick Start

### Build C++ Backend
```powershell
cmake --preset x64-Release
cmake --build --preset x64-Release
```

## Usage Modes

### Auto Mode (Recommended)
```powershell
build\x64-Release\bin\Release\viper-bootstrap.exe
```
Automatically detects a running Minecraft instance and injects the runtime DLL. If none is found, it will log an error and exit.

### Inject Mode
```powershell
viper-bootstrap.exe --inject
viper-bootstrap.exe --inject <PID>
```
Uses standard remote thread injection into an already running Minecraft process.

## IPC Protocol

The framework exposes a Named Pipe (`\\.\pipe\viper-ipc-<PID>`) using binary framed messages for external tool integration:

```text
┌──────────────────────────────────────────┐
│ Header (18 bytes, packed)                │
│  u32 magic     = 0x52504956 ("VIPR")     │
│  u16 version   = 1                       │
│  u16 type      = MessageType enum        │
│  u32 payloadSize                         │
│  u32 sequenceId                          │
│  u16 reserved  = 0                       │
├──────────────────────────────────────────┤
│ Payload (0..65536 bytes)                 │
│  JSON string                             │
└──────────────────────────────────────────┘
```

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
