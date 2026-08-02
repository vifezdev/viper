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
#include <chrono>

namespace viper {
namespace ipc {

class PipeClient {
public:
    PipeClient();
    ~PipeClient();

    PipeClient(const PipeClient&) = delete;
    PipeClient& operator=(const PipeClient&) = delete;

    
    
    
    Result<void> connect(const std::wstring& pipeName, u32 timeoutMs = 5000);

    
    void disconnect();

    
    Result<void> send(const Message& msg);

    
    
    Result<Message> receive(u32 timeoutMs = 5000);

    
    bool isConnected() const noexcept;

    
    void setMessageHandler(MessageCallback handler);

    
    void startReceiving();

    
    void stopReceiving();

    
    
    
    Result<void> reconnect(u32 maxAttempts = 5, u32 initialDelayMs = 500);

    
    const std::wstring& getPipeName() const noexcept;

private:
    
    void receiveLoop();

    
    Result<Message> readMessage();

    
    Result<void> writeBytes(const void* data, u32 size);

    std::wstring m_pipeName;
    HANDLE m_hPipe = INVALID_HANDLE_VALUE;

    std::thread m_receiveThread;
    std::atomic<bool> m_receiving{false};
    std::atomic<bool> m_connected{false};

    MessageCallback m_messageHandler;
    std::mutex m_sendMutex;
    std::mutex m_connectMutex;
};

} 
} 
