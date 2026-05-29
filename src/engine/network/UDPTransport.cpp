#include "network/UDPTransport.h"

namespace Prisma::Network {

UDPTransport::UDPTransport() = default;

UDPTransport::~UDPTransport() {
    Shutdown();
}

bool UDPTransport::Initialize(const TransportConfig& config) {
    if (m_initialized) return true;

    m_config = config;

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        LOG_ERROR("UDPTransport", "WSAStartup failed");
        return false;
    }
    m_wsaStarted = true;
#endif

    m_initialized = true;
    LOG_INFO("UDPTransport", "Initialized (host={0}, port={1})", config.host, config.port);
    return true;
}

bool UDPTransport::Listen() {
    if (!m_initialized) return false;

    m_socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_socket == INVALID_SOCKET_VALUE) {
        LOG_ERROR("UDPTransport", "Failed to create UDP socket");
        return false;
    }

    // Allow address reuse
    int flag = 1;
#ifdef _WIN32
    setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&flag), sizeof(flag));
#else
    setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
#endif

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_config.port);

    if (m_config.host == "0.0.0.0" || m_config.host == "*") {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
#ifdef _WIN32
        addr.sin_addr.s_addr = inet_addr(m_config.host.c_str());
        if (addr.sin_addr.s_addr == INADDR_NONE) {
            closesocket(m_socket);
            m_socket = INVALID_SOCKET_VALUE;
            return false;
        }
#else
        if (inet_pton(AF_INET, m_config.host.c_str(), &addr.sin_addr) <= 0) {
            closesocket(m_socket);
            m_socket = INVALID_SOCKET_VALUE;
            return false;
        }
#endif
    }

#ifdef _WIN32
    if (::bind(m_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
#else
    if (::bind(m_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
#endif
        LOG_ERROR("UDPTransport", "bind() failed on {0}:{1}", m_config.host, m_config.port);
        closesocket(m_socket);
        m_socket = INVALID_SOCKET_VALUE;
        return false;
    }

    // Set non-blocking
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(m_socket, FIONBIO, &mode);
#else
    int flags = fcntl(m_socket, F_GETFL, 0);
    fcntl(m_socket, F_SETFL, flags | O_NONBLOCK);
#endif

    m_isServer = true;
    m_running = true;

    // Start receive thread
    m_recvThread = std::make_unique<std::thread>(&UDPTransport::ReceiveThreadFunc, this);

    LOG_INFO("UDPTransport", "Listening on UDP {0}:{1}", m_config.host, m_config.port);
    return true;
}

bool UDPTransport::Connect() {
    if (!m_initialized) return false;

    m_socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_socket == INVALID_SOCKET_VALUE) {
        LOG_ERROR("UDPTransport", "Failed to create UDP socket");
        return false;
    }

    // Set remote address
    m_remoteAddr.sin_family = AF_INET;
    m_remoteAddr.sin_port = htons(m_config.port);

#ifdef _WIN32
    m_remoteAddr.sin_addr.s_addr = inet_addr(m_config.host.c_str());
    if (m_remoteAddr.sin_addr.s_addr == INADDR_NONE) {
        LOG_ERROR("UDPTransport", "Failed to resolve hostname: {0}", m_config.host);
        closesocket(m_socket);
        m_socket = INVALID_SOCKET_VALUE;
        return false;
    }
#else
    if (inet_pton(AF_INET, m_config.host.c_str(), &m_remoteAddr.sin_addr) <= 0) {
        LOG_ERROR("UDPTransport", "Failed to resolve hostname: {0}", m_config.host);
        closesocket(m_socket);
        m_socket = INVALID_SOCKET_VALUE;
        return false;
    }
#endif

    // Set non-blocking
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(m_socket, FIONBIO, &mode);
#else
    int flags = fcntl(m_socket, F_GETFL, 0);
    fcntl(m_socket, F_SETFL, flags | O_NONBLOCK);
#endif

    m_hasRemoteAddr = true;
    m_isConnected = true;
    m_running = true;

    // Register remote as peer
    ConnectionHandle handle = GetOrCreatePeer(m_remoteAddr);

    // Start receive thread
    m_recvThread = std::make_unique<std::thread>(&UDPTransport::ReceiveThreadFunc, this);

    LOG_INFO("UDPTransport", "UDP connected to {0}:{1}", m_config.host, m_config.port);
    return true;
}

void UDPTransport::Disconnect() {
    m_running = false;

    if (m_recvThread && m_recvThread->joinable()) {
        m_recvThread->join();
        m_recvThread.reset();
    }

    if (m_socket != INVALID_SOCKET_VALUE) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET_VALUE;
    }

    m_isConnected = false;
    m_isServer = false;
    m_hasRemoteAddr = false;
    m_peers.clear();
    m_addrToHandle.clear();
}

void UDPTransport::Shutdown() {
    Disconnect();
    m_initialized = false;

#ifdef _WIN32
    if (m_wsaStarted) {
        WSACleanup();
        m_wsaStarted = false;
    }
#endif

    LOG_INFO("UDPTransport", "Shut down");
}

bool UDPTransport::Send(ConnectionHandle to, const Packet& packet) {
    if (!m_initialized || m_socket == INVALID_SOCKET_VALUE) return false;

    std::vector<uint8_t> wireData = packet.Serialize();

    sockaddr_in destAddr{};
    if (m_isServer) {
        std::lock_guard<std::mutex> lock(m_peersMutex);
        auto it = m_peers.find(to);
        if (it == m_peers.end()) return false;
        destAddr = it->second.addr;
    } else {
        if (!m_hasRemoteAddr) return false;
        destAddr = m_remoteAddr;
    }

#ifdef _WIN32
    int sent = ::sendto(m_socket,
                        reinterpret_cast<const char*>(wireData.data()),
                        static_cast<int>(wireData.size()), 0,
                        reinterpret_cast<sockaddr*>(&destAddr), sizeof(destAddr));
#else
    ssize_t sent = ::sendto(m_socket, wireData.data(), wireData.size(), 0,
                            reinterpret_cast<sockaddr*>(&destAddr), sizeof(destAddr));
#endif
    if (sent > 0) {
        // Track for ACK
        if (packet.header.type != static_cast<uint8_t>(PacketType::Ack)) {
            std::lock_guard<std::mutex> ackLock(m_ackMutex);
            PendingAck pending;
            pending.sequence = packet.header.sequence;
            pending.sendTime = 
                std::chrono::duration<double>(
                    std::chrono::steady_clock::now().time_since_epoch()).count();
            m_pendingAcks[packet.header.sequence] = pending;
        }
    }

    return sent > 0;
}

bool UDPTransport::Broadcast(const Packet& packet) {
    if (!m_initialized || !m_isServer) return false;

    std::vector<uint8_t> wireData = packet.Serialize();
    bool allSent = true;

    std::lock_guard<std::mutex> lock(m_peersMutex);
    for (auto& [handle, peer] : m_peers) {
#ifdef _WIN32
        int sent = ::sendto(m_socket,
                            reinterpret_cast<const char*>(wireData.data()),
                            static_cast<int>(wireData.size()), 0,
                            reinterpret_cast<sockaddr*>(&peer.addr), sizeof(peer.addr));
#else
        ssize_t sent = ::sendto(m_socket, wireData.data(), wireData.size(), 0,
                                reinterpret_cast<sockaddr*>(&peer.addr), sizeof(peer.addr));
#endif
        if (sent <= 0) allSent = false;
    }
    return allSent;
}

bool UDPTransport::Receive(Packet& outPacket, ConnectionHandle& outFrom) {
    std::lock_guard<std::mutex> lock(m_incomingMutex);
    if (m_incoming.empty()) return false;
    auto [from, pkt] = std::move(m_incoming.front());
    m_incoming.pop_front();
    outPacket = std::move(pkt);
    outFrom = from;
    return true;
}

ConnectionInfo UDPTransport::GetConnectionInfo(ConnectionHandle handle) const {
    ConnectionInfo info;
    info.handle = handle;
    std::lock_guard<std::mutex> lock(m_peersMutex);
    auto it = m_peers.find(handle);
    if (it != m_peers.end()) {
        info.address = it->second.address;
        info.port = it->second.port;
        info.connectTime = it->second.connectTime;
        info.rttMs = it->second.rttMs;
    }
    return info;
}

void UDPTransport::Poll() {
    // Nothing extra to poll; receive thread handles everything
}

// ── Private Helpers ──

ConnectionHandle UDPTransport::GetOrCreatePeer(const sockaddr_in& addr) {
#ifdef _WIN32
    const char* result = inet_ntoa(addr.sin_addr);
    std::string key = std::string(result) + ":" + std::to_string(ntohs(addr.sin_port));
#else
    char addrStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, addrStr, sizeof(addrStr));
    std::string key = std::string(addrStr) + ":" + std::to_string(ntohs(addr.sin_port));
#endif

    std::lock_guard<std::mutex> lock(m_peersMutex);

    auto it = m_addrToHandle.find(key);
    if (it != m_addrToHandle.end()) {
        return it->second;
    }

    ConnectionHandle handle = m_nextHandle++;
    PeerEndpoint peer;
    peer.addr = addr;
#ifdef _WIN32
    peer.address = inet_ntoa(addr.sin_addr);
#else
    inet_ntop(AF_INET, &addr.sin_addr, addrStr, sizeof(addrStr));
    peer.address = addrStr;
#endif
    peer.port = ntohs(addr.sin_port);
    peer.connectTime = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());

    m_peers[handle] = peer;
    m_addrToHandle[key] = handle;

    LOG_INFO("UDPTransport", "New peer {0}:{1} (handle={2})", peer.address, peer.port, handle);
    return handle;
}

void UDPTransport::SendAck(ConnectionHandle to, uint32_t sequence) {
    BinarySerializer writer;
    writer.WriteUint32(sequence);

    Packet ackPkt;
    ackPkt.header.sequence = sequence;
    ackPkt.header.type = static_cast<uint8_t>(PacketType::Ack);
    ackPkt.payload = writer.MoveData();
    ackPkt.header.payloadSize = static_cast<uint32_t>(ackPkt.payload.size());

    Send(to, ackPkt);
}

void UDPTransport::ReceiveThreadFunc() {
    std::vector<uint8_t> recvBuf(m_config.bufferSize);

    while (m_running) {
        sockaddr_in fromAddr{};
#ifdef _WIN32
        int addrLen = sizeof(fromAddr);
        int bytesRead = ::recvfrom(m_socket,
                                   reinterpret_cast<char*>(recvBuf.data()),
                                   static_cast<int>(recvBuf.size()), 0,
                                   reinterpret_cast<sockaddr*>(&fromAddr), &addrLen);
#else
        socklen_t addrLen = sizeof(fromAddr);
        ssize_t bytesRead = ::recvfrom(m_socket, recvBuf.data(), recvBuf.size(), 0,
                                       reinterpret_cast<sockaddr*>(&fromAddr), &addrLen);
#endif

        if (bytesRead <= 0) {
#ifdef _WIN32
            if (WSAGetLastError() == WSAEWOULDBLOCK) {
#else
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
#endif
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            break; // Fatal error
        }

        // Parse packet
        Packet pkt = Packet::Deserialize(recvBuf.data(), static_cast<uint32_t>(bytesRead));
        ConnectionHandle handle = GetOrCreatePeer(fromAddr);

        // Handle ACKs
        if (pkt.header.type == static_cast<uint8_t>(PacketType::Ack)) {
            if (pkt.payload.size() >= 4) {
                BinarySerializer reader(pkt.payload);
                uint32_t ackedSeq = reader.ReadUint32();
                std::lock_guard<std::mutex> ackLock(m_ackMutex);
                auto it = m_pendingAcks.find(ackedSeq);
                if (it != m_pendingAcks.end()) {
                    double now = std::chrono::duration<double>(
                        std::chrono::steady_clock::now().time_since_epoch()).count();
                    double rtt = (now - it->second.sendTime) * 1000.0;

                    std::lock_guard<std::mutex> peerLock(m_peersMutex);
                    auto peerIt = m_peers.find(handle);
                    if (peerIt != m_peers.end()) {
                        // Exponential moving average for RTT
                        peerIt->second.rttMs = peerIt->second.rttMs * 0.75 + rtt * 0.25;
                    }
                    m_pendingAcks.erase(it);
                }
            }
            continue;
        }

        // Send ACK for received packets (only for non-ACK packets)
        if (pkt.header.sequence > 0) {
            SendAck(handle, pkt.header.sequence);

            std::lock_guard<std::mutex> peerLock(m_peersMutex);
            auto peerIt = m_peers.find(handle);
            if (peerIt != m_peers.end()) {
                if (pkt.header.sequence > peerIt->second.nextExpectedSeq) {
                    peerIt->second.nextExpectedSeq = pkt.header.sequence;
                }
            }
        }

        // Queue incoming packet
        {
            std::lock_guard<std::mutex> lock(m_incomingMutex);
            m_incoming.emplace_back(handle, std::move(pkt));
        }
    }
}

} // namespace Prisma::Network
