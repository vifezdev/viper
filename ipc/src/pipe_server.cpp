







#include <viper/ipc/pipe_server.h>
#include <viper/common/logger.h>

#include <sddl.h>
#include <cstring>

namespace viper {
namespace ipc {





static constexpr DWORD PIPE_BUFFER_SIZE = 65536;


static constexpr const wchar_t* PIPE_SDDL =
    L"D:(A;;GA;;;SY)(A;;GA;;;BA)(A;;GRGW;;;IU)";





PipeServer::PipeServer(u32 processId)
    : m_processId(processId)
{
    m_pipeName = std::wstring(PIPE_NAME_PREFIX) + std::to_wstring(processId);
    m_hShutdownEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    m_hConnectEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
}

PipeServer::~PipeServer() {
    stop();
    if (m_hShutdownEvent) CloseHandle(m_hShutdownEvent);
    if (m_hConnectEvent) CloseHandle(m_hConnectEvent);
}





Result<void> PipeServer::start() {
    if (m_running.load()) {
        return Result<void>(err_tag, std::string("Pipe server already running"));
    }

    auto result = createPipe();
    if (result.isErr()) {
        return result;
    }

    m_running.store(true);
    ResetEvent(m_hShutdownEvent);

    m_listenerThread = std::thread(&PipeServer::listenerLoop, this);

    VIPER_LOG_IPC("Pipe server started on {}", 
        std::string(m_pipeName.begin(), m_pipeName.end()));
    return Result<void>();
}

void PipeServer::stop() {
    if (!m_running.load()) return;

    VIPER_LOG_IPC("Stopping pipe server...");
    m_running.store(false);
    SetEvent(m_hShutdownEvent);

    
    if (m_hPipe != INVALID_HANDLE_VALUE) {
        CancelIoEx(m_hPipe, nullptr);
    }

    if (m_listenerThread.joinable()) {
        if (m_listenerThread.get_id() != std::this_thread::get_id()) {
            m_listenerThread.join();
        } else {
            VIPER_LOG_IPC_WARN("PipeServer::stop() called from listener thread! Detaching to prevent deadlock.");
            m_listenerThread.detach();
        }
    }

    closePipe();
    m_clientConnected.store(false);
    VIPER_LOG_IPC("Pipe server stopped");
}

Result<void> PipeServer::send(const Message& msg) {
    if (!m_clientConnected.load()) {
        return Result<void>(err_tag, std::string("No client connected"));
    }

    auto data = msg.serialize();
    return writeBytes(data.data(), static_cast<u32>(data.size()));
}

void PipeServer::setMessageHandler(MessageCallback handler) {
    m_messageHandler = std::move(handler);
}

bool PipeServer::isConnected() const noexcept {
    return m_clientConnected.load();
}

const std::wstring& PipeServer::getPipeName() const noexcept {
    return m_pipeName;
}





Result<void> PipeServer::createPipe() {
    
    PSECURITY_DESCRIPTOR pSD = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
            PIPE_SDDL, SDDL_REVISION_1, &pSD, nullptr)) {
        DWORD err = GetLastError();
        return Result<void>(err_tag,
            std::string("Failed to create security descriptor. Error: ") +
            std::to_string(err));
    }

    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = pSD;
    sa.bInheritHandle = FALSE;

    m_hPipe = CreateNamedPipeW(
        m_pipeName.c_str(),
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,                  
        PIPE_BUFFER_SIZE,   
        PIPE_BUFFER_SIZE,   
        0,                  
        &sa
    );

    LocalFree(pSD);

    if (m_hPipe == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        return Result<void>(err_tag,
            std::string("CreateNamedPipe failed. Error: ") + std::to_string(err));
    }

    return Result<void>();
}

void PipeServer::closePipe() {
    if (m_hPipe != INVALID_HANDLE_VALUE) {
        if (m_clientConnected.load()) {
            FlushFileBuffers(m_hPipe);
            DisconnectNamedPipe(m_hPipe);
        }
        CloseHandle(m_hPipe);
        m_hPipe = INVALID_HANDLE_VALUE;
    }
}





void PipeServer::listenerLoop() {
    VIPER_LOG_IPC("Listener thread started");

    while (m_running.load()) {
        
        if (!waitForConnection()) {
            if (!m_running.load()) break;  

            
            closePipe();
            auto result = createPipe();
            if (result.isErr()) {
                VIPER_LOG_IPC_ERR("Failed to recreate pipe: {}", result.error());
                break;
            }
            continue;
        }

        m_clientConnected.store(true);
        VIPER_LOG_IPC("Client connected");

        
        while (m_running.load() && m_clientConnected.load()) {
            auto msgResult = readMessage();
            if (msgResult.isErr()) {
                VIPER_LOG_IPC_WARN("Read error: {}", msgResult.error());
                m_clientConnected.store(false);
                break;
            }

            const auto& msg = msgResult.value();
            if (!msg.isValid()) {
                VIPER_LOG_IPC_WARN("Received invalid message (bad magic/version)");
                continue;
            }

            VIPER_LOG_IPC_DEBUG("Received: {} (seq={})",
                messageTypeToString(static_cast<MessageType>(msg.header.type)),
                msg.header.sequenceId);

            
            if (static_cast<MessageType>(msg.header.type) == MessageType::Disconnect) {
                VIPER_LOG_IPC("Client sent DISCONNECT");
                m_clientConnected.store(false);
                break;
            }

            
            if (m_messageHandler) {
                m_messageHandler(msg);
            }
        }

        
        if (m_running.load()) {
            VIPER_LOG_IPC("Client disconnected, waiting for new connection...");
            DisconnectNamedPipe(m_hPipe);
        }
    }

    VIPER_LOG_IPC("Listener thread exiting");
}





bool PipeServer::waitForConnection() {
    OVERLAPPED overlapped{};
    overlapped.hEvent = m_hConnectEvent;
    ResetEvent(m_hConnectEvent);

    BOOL connected = ConnectNamedPipe(m_hPipe, &overlapped);
    if (connected) {
        
        return true;
    }

    DWORD err = GetLastError();
    if (err == ERROR_PIPE_CONNECTED) {
        
        return true;
    }

    if (err != ERROR_IO_PENDING) {
        VIPER_LOG_IPC_ERR("ConnectNamedPipe failed. Error: {}", err);
        return false;
    }

    
    HANDLE waitHandles[2] = { m_hConnectEvent, m_hShutdownEvent };
    DWORD waitResult = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);

    if (waitResult == WAIT_OBJECT_0) {
        
        DWORD bytesTransferred = 0;
        return GetOverlappedResult(m_hPipe, &overlapped, &bytesTransferred, FALSE) != FALSE;
    }

    
    CancelIoEx(m_hPipe, &overlapped);
    return false;
}





Result<Message> PipeServer::readMessage() {
    
    MessageHeader header{};
    OVERLAPPED readOverlapped{};
    readOverlapped.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

    if (!readOverlapped.hEvent) {
        return Result<Message>(err_tag, std::string("Failed to create read event"));
    }

    DWORD bytesRead = 0;
    BOOL readOk = ReadFile(m_hPipe, &header, sizeof(MessageHeader), nullptr, &readOverlapped);

    if (!readOk) {
        DWORD err = GetLastError();
        if (err == ERROR_IO_PENDING) {
            HANDLE waitHandles[2] = { readOverlapped.hEvent, m_hShutdownEvent };
            DWORD waitResult = WaitForMultipleObjects(2, waitHandles, FALSE, 10000);

            if (waitResult == WAIT_OBJECT_0) {
                GetOverlappedResult(m_hPipe, &readOverlapped, &bytesRead, FALSE);
            } else {
                CancelIoEx(m_hPipe, &readOverlapped);
                CloseHandle(readOverlapped.hEvent);
                if (waitResult == WAIT_OBJECT_0 + 1) {
                    return Result<Message>(err_tag, std::string("Shutdown during read"));
                }
                return Result<Message>(err_tag, std::string("Read timeout"));
            }
        } else if (err == ERROR_BROKEN_PIPE || err == ERROR_PIPE_NOT_CONNECTED) {
            CloseHandle(readOverlapped.hEvent);
            return Result<Message>(err_tag, std::string("Pipe disconnected"));
        } else {
            CloseHandle(readOverlapped.hEvent);
            return Result<Message>(err_tag,
                std::string("ReadFile header failed. Error: ") + std::to_string(err));
        }
    } else {
        GetOverlappedResult(m_hPipe, &readOverlapped, &bytesRead, FALSE);
    }

    CloseHandle(readOverlapped.hEvent);

    if (bytesRead < sizeof(MessageHeader)) {
        return Result<Message>(err_tag, std::string("Incomplete header"));
    }

    
    if (header.magic != PROTOCOL_MAGIC) {
        return Result<Message>(err_tag, std::string("Invalid magic bytes"));
    }

    if (header.payloadSize > MAX_PAYLOAD_SIZE) {
        return Result<Message>(err_tag, std::string("Payload too large: ") +
                               std::to_string(header.payloadSize));
    }

    
    std::vector<u8> fullBuffer(sizeof(MessageHeader) + header.payloadSize);
    std::memcpy(fullBuffer.data(), &header, sizeof(MessageHeader));

    if (header.payloadSize > 0) {
        OVERLAPPED payloadOverlapped{};
        payloadOverlapped.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

        DWORD payloadRead = 0;
        BOOL pOk = ReadFile(m_hPipe,
                            fullBuffer.data() + sizeof(MessageHeader),
                            header.payloadSize,
                            nullptr,
                            &payloadOverlapped);

        if (!pOk && GetLastError() == ERROR_IO_PENDING) {
            HANDLE wh[2] = { payloadOverlapped.hEvent, m_hShutdownEvent };
            DWORD wr = WaitForMultipleObjects(2, wh, FALSE, 5000);
            if (wr == WAIT_OBJECT_0) {
                GetOverlappedResult(m_hPipe, &payloadOverlapped, &payloadRead, FALSE);
            } else {
                CancelIoEx(m_hPipe, &payloadOverlapped);
                CloseHandle(payloadOverlapped.hEvent);
                return Result<Message>(err_tag, std::string("Payload read timeout"));
            }
        } else if (pOk) {
            GetOverlappedResult(m_hPipe, &payloadOverlapped, &payloadRead, FALSE);
        } else {
            DWORD err = GetLastError();
            CloseHandle(payloadOverlapped.hEvent);
            return Result<Message>(err_tag,
                std::string("Payload read failed. Error: ") + std::to_string(err));
        }

        CloseHandle(payloadOverlapped.hEvent);
    }

    return Result<Message>(Message::deserialize(fullBuffer));
}





Result<void> PipeServer::writeBytes(const void* data, u32 size) {
    std::lock_guard<std::mutex> lock(m_sendMutex);

    OVERLAPPED writeOverlapped{};
    writeOverlapped.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!writeOverlapped.hEvent) {
        return Result<void>(err_tag, std::string("Failed to create write event"));
    }

    DWORD bytesWritten = 0;
    BOOL ok = WriteFile(m_hPipe, data, size, nullptr, &writeOverlapped);

    if (!ok) {
        DWORD err = GetLastError();
        if (err == ERROR_IO_PENDING) {
            DWORD waitResult = WaitForSingleObject(writeOverlapped.hEvent, 5000);
            if (waitResult == WAIT_OBJECT_0) {
                GetOverlappedResult(m_hPipe, &writeOverlapped, &bytesWritten, FALSE);
            } else {
                CancelIoEx(m_hPipe, &writeOverlapped);
                CloseHandle(writeOverlapped.hEvent);
                return Result<void>(err_tag, std::string("Write timeout"));
            }
        } else {
            CloseHandle(writeOverlapped.hEvent);
            return Result<void>(err_tag,
                std::string("WriteFile failed. Error: ") + std::to_string(err));
        }
    }

    CloseHandle(writeOverlapped.hEvent);
    return Result<void>();
}

} 
} 
