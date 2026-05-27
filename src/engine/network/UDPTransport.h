#pragma once

#include "Export.h"
#include "network/NetworkTransport.h"
#include "Logger.h"

#include <thread>
#include <atomic>
#include <unordered_map>
#include <deque>
#include <mutex>
#include <set>
#include <cstdint>
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
// UDPTransport — unreliable transport via BSD sockets.
// Supports optional delivery tracking via sequence numbers and ACKs.
// ═══════════════════════════════════════════════════════════════════
class ENGINE_API UDPTransport : public ITransport {
public:
    UDPTransport();
    ~UDPTransport() override;

    UDPTransport(const UDPTransport&) = delete;
    UDPTransport& operator=(const UDPTransport&) = delete;

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
    struct PeerEndpoint {
        sockaddr_in addr;
        std::string address;
        uint16_t port = 0;
        uint64_t connectTime = 0;
        double rttMs = 0.0;
        uint32_t nextExpectedSeq = 0;
    };

    struct PendingAck {
        uint32_t sequence;
        double sendTime;
    };

    ConnectionHandle GetOrCreatePeer(const sockaddr_in& addr);
    void ReceiveThreadFunc();
    void SendAck(ConnectionHandle to, uint32_t sequence);
    static bool IsPrivateAddress(const sockaddr_in& addr);

    TransportConfig m_config;
    SOCKET_HANDLE m_socket = INVALID_SOCKET_VALUE;
    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_isServer{false};
    std::atomic<bool> m_isConnected{false};

    // Sequence tracking
    std::atomic<uint32_t> m_nextSequence{1};

    // Peers (identified by address)
    std::unordered_map<ConnectionHandle, PeerEndpoint> m_peers;
    std::unordered_map<std::string, ConnectionHandle> m_addrToHandle;
    mutable std::mutex m_peersMutex;
    ConnectionHandle m_nextHandle = 1;

    // Client mode: remote endpoint
    sockaddr_in m_remoteAddr{};
    bool m_hasRemoteAddr = false;

    // Incoming packet queue
    mutable std::mutex m_incomingMutex;
    std::deque<std::pair<ConnectionHandle, Packet>> m_incoming;

    // Send buffer
    mutable std::mutex m_sendMutex;
    std::deque<std::pair<ConnectionHandle, Packet>> m_sendQueue;

    // Pending ACKs for RTT estimation
    std::unordered_map<uint32_t, PendingAck> m_pendingAcks;
    mutable std::mutex m_ackMutex;

    // Receive thread
    std::unique_ptr<std::thread> m_recvThread;

#ifdef _WIN32
    bool m_wsaStarted = false;
#endif
};

} // namespace Prisma::Network
