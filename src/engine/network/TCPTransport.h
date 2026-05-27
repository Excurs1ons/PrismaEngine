#pragma once

#include "Export.h"
#include "network/NetworkTransport.h"
#include "Logger.h"

#include <thread>
#include <atomic>
#include <unordered_map>
#include <deque>
#include <mutex>
#include <vector>
#include <cstring>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
    #include <ws2tcpip.h>
    using SOCKET_HANDLE = SOCKET;
    static constexpr SOCKET_HANDLE INVALID_SOCKET_VALUE = INVALID_SOCKET;
    static constexpr int SOCKET_ERROR_RET = SOCKET_ERROR;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <poll.h>
    #include <cerrno>
    using SOCKET_HANDLE = int;
    static constexpr SOCKET_HANDLE INVALID_SOCKET_VALUE = -1;
    static constexpr int SOCKET_ERROR_RET = -1;
    static constexpr int INVALID_SOCKET = -1;
#endif

namespace Prisma::Network {

// ═══════════════════════════════════════════════════════════════════
// TCPTransport — reliable, ordered transport via BSD sockets
// ═══════════════════════════════════════════════════════════════════
class ENGINE_API TCPTransport : public ITransport {
public:
    TCPTransport();
    ~TCPTransport() override;

    TCPTransport(const TCPTransport&) = delete;
    TCPTransport& operator=(const TCPTransport&) = delete;

    // ITransport interface
    bool Initialize(const TransportConfig& config) override;
    bool Listen() override;
    bool Connect() override;
    void Disconnect() override;
    void Shutdown() override;
    bool Send(ConnectionHandle to, const Packet& packet) override;
    bool Broadcast(const Packet& packet) override;
    bool Receive(Packet& outPacket, ConnectionHandle& outFrom) override;
    bool IsReady() const override { return m_initialized; }
    bool IsServer() const override { return m_isServer; }
    bool IsConnected() const override { return m_isConnected; }
    ConnectionInfo GetConnectionInfo(ConnectionHandle handle) const override;
    const TransportConfig& GetConfig() const override { return m_config; }
    void Poll() override;

private:
    struct PeerConnection {
        SOCKET_HANDLE socket = INVALID_SOCKET_VALUE;
        std::string address;
        uint16_t port = 0;
        uint64_t connectTime = 0;
        std::vector<uint8_t> recvBuffer; // Partial packet buffer
    };

    bool SetNonBlocking(SOCKET_HANDLE sock);
    SOCKET_HANDLE CreateSocket();
    bool BindSocket(SOCKET_HANDLE sock);
    void AcceptNewConnections();
    void ReceiveThreadFunc();
    void CloseSocket(SOCKET_HANDLE sock);

    TransportConfig m_config;
    SOCKET_HANDLE m_listenSocket = INVALID_SOCKET_VALUE;
    SOCKET_HANDLE m_clientSocket = INVALID_SOCKET_VALUE;
    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_isServer{false};
    std::atomic<bool> m_isConnected{false};

    // Connected peers (server mode)
    std::unordered_map<ConnectionHandle, PeerConnection> m_peers;
    mutable std::mutex m_peersMutex;
    ConnectionHandle m_nextHandle = 1;

    // Receive thread
    std::unique_ptr<std::thread> m_recvThread;

    // Incoming packet queue (thread-safe)
    mutable std::mutex m_incomingMutex;
    std::deque<std::pair<ConnectionHandle, Packet>> m_incoming;

    // Send buffer per peer
    mutable std::mutex m_sendMutex;
    std::unordered_map<ConnectionHandle, std::deque<Packet>> m_sendQueues;

#ifdef _WIN32
    bool m_wsaStarted = false;
#endif
};

} // namespace Prisma::Network
