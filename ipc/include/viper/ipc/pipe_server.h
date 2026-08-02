#pragma once









#include <viper/common/types.h>
#include <viper/common/result.h>
#include <viper/ipc/protocol.h>

#include <Windows.h>
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>

namespace viper {
namespace ipc {


using MessageCallback = std::function<void(const Message&)>;

class PipeServer {
public:
    
    explicit PipeServer(u32 processId);
    ~PipeServer();

    
    PipeServer(const PipeServer&) = delete;
    PipeServer& operator=(const PipeServer&) = delete;

    
    Result<void> start();

    
    void stop();

    
    Result<void> send(const Message& msg);

    
    void setMessageHandler(MessageCallback handler);

    
    bool isConnected() const noexcept;

    
    const std::wstring& getPipeName() const noexcept;

private:
    
    void listenerLoop();

    
    bool waitForConnection();

    
    Result<Message> readMessage();

    
    Result<void> writeBytes(const void* data, u32 size);

    
    Result<void> createPipe();

    
    void closePipe();

    u32 m_processId;
    std::wstring m_pipeName;

    HANDLE m_hPipe          = INVALID_HANDLE_VALUE;
    HANDLE m_hConnectEvent  = nullptr;   
    HANDLE m_hShutdownEvent = nullptr;   

    std::thread m_listenerThread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_clientConnected{false};

    MessageCallback m_messageHandler;
    std::mutex m_sendMutex;              
};

} 
} 
