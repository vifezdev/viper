






#include <viper/ipc/pipe_client.h>
#include <viper/common/logger.h>

#include <cstring>
#include <algorithm>

namespace viper {
namespace ipc {

PipeClient::PipeClient() = default;

PipeClient::~PipeClient() {
    stopReceiving();
    disconnect();
}





Result<void> PipeClient::connect(const std::wstring& pipeName, u32 timeoutMs) {
    std::lock_guard<std::mutex> lock(m_connectMutex);

    if (m_connected.load()) {
        disconnect();
    }

    m_pipeName = pipeName;

    
    if (!WaitNamedPipeW(pipeName.c_str(), timeoutMs)) {
        DWORD err = GetLastError();
        return Result<void>(err_tag,
            std::string("WaitNamedPipe failed. Error: ") + std::to_string(err));
    }

    m_hPipe = CreateFileW(
        pipeName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,              
        nullptr,        
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        nullptr
    );

    if (m_hPipe == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        return Result<void>(err_tag,
            std::string("CreateFile failed. Error: ") + std::to_string(err));
    }

    
    DWORD dwMode = PIPE_READMODE_MESSAGE;
    if (!SetNamedPipeHandleState(m_hPipe, &dwMode, nullptr, nullptr)) {
        DWORD err = GetLastError();
        CloseHandle(m_hPipe);
        m_hPipe = INVALID_HANDLE_VALUE;
        return Result<void>(err_tag,
            std::string("SetNamedPipeHandleState failed. Error: ") + std::to_string(err));
    }

    m_connected.store(true);
    VIPER_LOG_IPC("Connected to pipe: {}",
        std::string(pipeName.begin(), pipeName.end()));

    return Result<void>();
}

void PipeClient::disconnect() {
    stopReceiving();

    if (m_hPipe != INVALID_HANDLE_VALUE) {
        
        if (m_connected.load()) {
            auto msg = Message::create(MessageType::Disconnect);
            auto data = msg.serialize();
            DWORD written = 0;
            WriteFile(m_hPipe, data.data(), static_cast<DWORD>(data.size()), &written, nullptr);
        }
        CloseHandle(m_hPipe);
        m_hPipe = INVALID_HANDLE_VALUE;
    }
    m_connected.store(false);
}





Result<void> PipeClient::send(const Message& msg) {
    if (!m_connected.load()) {
        return Result<void>(err_tag, std::string("Not connected"));
    }

    auto data = msg.serialize();
    return writeBytes(data.data(), static_cast<u32>(data.size()));
}

Result<Message> PipeClient::receive(u32 timeoutMs) {
    if (!m_connected.load()) {
        return Result<Message>(err_tag, std::string("Not connected"));
    }
    return readMessage();
}

bool PipeClient::isConnected() const noexcept {
    return m_connected.load();
}

void PipeClient::setMessageHandler(MessageCallback handler) {
    m_messageHandler = std::move(handler);
}

const std::wstring& PipeClient::getPipeName() const noexcept {
    return m_pipeName;
}





void PipeClient::startReceiving() {
    if (m_receiving.load()) return;
    m_receiving.store(true);
    m_receiveThread = std::thread(&PipeClient::receiveLoop, this);
}

void PipeClient::stopReceiving() {
    if (!m_receiving.load()) return;
    m_receiving.store(false);

    if (m_hPipe != INVALID_HANDLE_VALUE) {
        CancelIoEx(m_hPipe, nullptr);
    }

    if (m_receiveThread.joinable()) {
        m_receiveThread.join();
    }
}

void PipeClient::receiveLoop() {
    VIPER_LOG_IPC("Receive thread started");

    while (m_receiving.load() && m_connected.load()) {
        auto result = readMessage();
        if (result.isErr()) {
            if (m_receiving.load()) {
                VIPER_LOG_IPC_WARN("Receive error: {}", result.error());
                m_connected.store(false);
            }
            break;
        }

        const auto& msg = result.value();
        if (msg.isValid() && m_messageHandler) {
            m_messageHandler(msg);
        }
    }

    VIPER_LOG_IPC("Receive thread exiting");
}





Result<void> PipeClient::reconnect(u32 maxAttempts, u32 initialDelayMs) {
    u32 delay = initialDelayMs;
    u32 attempt = 0;

    while (maxAttempts == 0 || attempt < maxAttempts) {
        attempt++;
        VIPER_LOG_IPC("Reconnect attempt {}/{} (delay: {}ms)",
                       attempt, maxAttempts == 0 ? 0 : maxAttempts, delay);

        disconnect();
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));

        auto result = connect(m_pipeName);
        if (result.isOk()) {
            VIPER_LOG_IPC("Reconnected successfully on attempt {}", attempt);
            return Result<void>();
        }

        
        delay = std::min(delay * 2, u32(30000));
    }

    return Result<void>(err_tag, std::string("Reconnect failed after ") +
                         std::to_string(maxAttempts) + " attempts");
}





Result<Message> PipeClient::readMessage() {
    MessageHeader header{};
    OVERLAPPED readOv{};
    readOv.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!readOv.hEvent) {
        return Result<Message>(err_tag, std::string("Failed to create read event"));
    }

    DWORD bytesRead = 0;
    BOOL ok = ReadFile(m_hPipe, &header, sizeof(MessageHeader), nullptr, &readOv);

    if (!ok) {
        DWORD err = GetLastError();
        if (err == ERROR_IO_PENDING) {
            DWORD waitResult = WaitForSingleObject(readOv.hEvent, 10000);
            if (waitResult == WAIT_OBJECT_0) {
                GetOverlappedResult(m_hPipe, &readOv, &bytesRead, FALSE);
            } else {
                CancelIoEx(m_hPipe, &readOv);
                CloseHandle(readOv.hEvent);
                return Result<Message>(err_tag, std::string("Read timeout"));
            }
        } else if (err == ERROR_BROKEN_PIPE || err == ERROR_PIPE_NOT_CONNECTED) {
            CloseHandle(readOv.hEvent);
            m_connected.store(false);
            return Result<Message>(err_tag, std::string("Pipe disconnected"));
        } else {
            CloseHandle(readOv.hEvent);
            return Result<Message>(err_tag,
                std::string("ReadFile failed. Error: ") + std::to_string(err));
        }
    } else {
        GetOverlappedResult(m_hPipe, &readOv, &bytesRead, FALSE);
    }

    CloseHandle(readOv.hEvent);

    if (bytesRead < sizeof(MessageHeader)) {
        return Result<Message>(err_tag, std::string("Incomplete header"));
    }

    if (header.magic != PROTOCOL_MAGIC || header.payloadSize > MAX_PAYLOAD_SIZE) {
        return Result<Message>(err_tag, std::string("Invalid message header"));
    }

    
    std::vector<u8> buffer(sizeof(MessageHeader) + header.payloadSize);
    std::memcpy(buffer.data(), &header, sizeof(MessageHeader));

    if (header.payloadSize > 0) {
        OVERLAPPED payOv{};
        payOv.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        DWORD payRead = 0;

        BOOL pOk = ReadFile(m_hPipe,
                            buffer.data() + sizeof(MessageHeader),
                            header.payloadSize,
                            nullptr, &payOv);

        if (!pOk && GetLastError() == ERROR_IO_PENDING) {
            WaitForSingleObject(payOv.hEvent, 5000);
            GetOverlappedResult(m_hPipe, &payOv, &payRead, FALSE);
        } else if (pOk) {
            GetOverlappedResult(m_hPipe, &payOv, &payRead, FALSE);
        }

        CloseHandle(payOv.hEvent);
    }

    return Result<Message>(Message::deserialize(buffer));
}





Result<void> PipeClient::writeBytes(const void* data, u32 size) {
    std::lock_guard<std::mutex> lock(m_sendMutex);

    OVERLAPPED writeOv{};
    writeOv.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!writeOv.hEvent) {
        return Result<void>(err_tag, std::string("Failed to create write event"));
    }

    DWORD bytesWritten = 0;
    BOOL ok = WriteFile(m_hPipe, data, size, nullptr, &writeOv);

    if (!ok) {
        DWORD err = GetLastError();
        if (err == ERROR_IO_PENDING) {
            WaitForSingleObject(writeOv.hEvent, 5000);
            GetOverlappedResult(m_hPipe, &writeOv, &bytesWritten, FALSE);
        } else {
            CloseHandle(writeOv.hEvent);
            return Result<void>(err_tag,
                std::string("WriteFile failed. Error: ") + std::to_string(err));
        }
    }

    CloseHandle(writeOv.hEvent);
    return Result<void>();
}

} 
} 
