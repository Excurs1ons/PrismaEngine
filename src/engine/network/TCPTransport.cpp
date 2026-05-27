// Platform-specific networking headers (must precede TCPTransport.h for type definitions)
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <arpa/inet.h>
    using SOCKET = int;
    #define closesocket(fd) close(fd)
#endif

#include "network/TCPTransport.h"

namespace Prisma::Network {

TCPTransport::TCPTransport() = default;

TCPTransport::~TCPTransport() {
    Shutdown();
}

bool TCPTransport::Initialize(const TransportConfig& config) {
    if (m_initialized) return true;

    m_config = config;

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        LOG_ERROR("TCPTransport", "WSAStartup failed");
        return false;
    }
    m_wsaStarted = true;
#endif

    m_initialized = true;
    LOG_INFO("TCPTransport", "Initialized (host={0}, port={1})", config.host, config.port);
    return true;
}

bool TCPTransport::Listen() {
    if (!m_initialized) return false;

    m_listenSocket = CreateSocket();
    if (m_listenSocket == INVALID_SOCKET_VALUE) {
        LOG_ERROR("TCPTransport", "Failed to create listen socket");
        return false;
    }

    if (!BindSocket(m_listenSocket)) {
        LOG_ERROR("TCPTransport", "Failed to bind listen socket");
        CloseSocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET_VALUE;
        return false;
    }

#ifdef _WIN32
    if (::listen(m_listenSocket, SOMAXCONN) == SOCKET_ERROR) {
#else
    if (::listen(m_listenSocket, SOMAXCONN) < 0) {
#endif
        LOG_ERROR("TCPTransport", "listen() failed");
        CloseSocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET_VALUE;
        return false;
    }

    SetNonBlocking(m_listenSocket);
    m_isServer = true;

    // Start receive thread
    m_running = true;
    m_recvThread = std::make_unique<std::thread>(&TCPTransport::ReceiveThreadFunc, this);

    LOG_INFO("TCPTransport", "Listening on {0}:{1}", m_config.host, m_config.port);
    return true;
}

bool TCPTransport::Connect() {
    if (!m_initialized) return false;

    m_clientSocket = CreateSocket();
    if (m_clientSocket == INVALID_SOCKET_VALUE) {
        LOG_ERROR("TCPTransport", "Failed to create client socket");
        return false;
    }

    SetNonBlocking(m_clientSocket);

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(m_config.port);
#ifdef _WIN32
    serverAddr.sin_addr.s_addr = inet_addr(m_config.host.c_str());
    if (serverAddr.sin_addr.s_addr == INADDR_NONE) {
        // Try DNS resolution
        struct hostent* host = gethostbyname(m_config.host.c_str());
        if (!host) {
            LOG_ERROR("TCPTransport", "Failed to resolve hostname: {0}", m_config.host);
            CloseSocket(m_clientSocket);
            m_clientSocket = INVALID_SOCKET_VALUE;
            return false;
        }
        std::memcpy(&serverAddr.sin_addr, host->h_addr, host->h_length);
    }
#else
    if (inet_pton(AF_INET, m_config.host.c_str(), &serverAddr.sin_addr) <= 0) {
        // Try DNS resolution
        struct addrinfo hints{}, *res;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        if (getaddrinfo(m_config.host.c_str(), nullptr, &hints, &res) != 0) {
            LOG_ERROR("TCPTransport", "Failed to resolve hostname: {0}", m_config.host);
            CloseSocket(m_clientSocket);
            m_clientSocket = INVALID_SOCKET_VALUE;
            return false;
        }
        std::memcpy(&serverAddr.sin_addr,
                     &reinterpret_cast<sockaddr_in*>(res->ai_addr)->sin_addr,
                     sizeof(in_addr));
        freeaddrinfo(res);
    }
#endif

    int result = ::connect(m_clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr));
    if (result != 0) {
#ifdef _WIN32
        if (WSAGetLastError() != WSAEWOULDBLOCK) {
#else
        if (errno != EINPROGRESS) {
#endif
            LOG_ERROR("TCPTransport", "connect() failed");
            CloseSocket(m_clientSocket);
            m_clientSocket = INVALID_SOCKET_VALUE;
            return false;
        }
        // Wait for connection to complete
        pollfd fd;
        fd.fd = m_clientSocket;
        fd.events = POLLOUT;
#ifdef _WIN32
        if (WSAPoll(&fd, 1, 5000) <= 0) {
#else
        if (poll(&fd, 1, 5000) <= 0) {
#endif
            LOG_ERROR("TCPTransport", "connect() timed out");
            CloseSocket(m_clientSocket);
            m_clientSocket = INVALID_SOCKET_VALUE;
            return false;
        }
    }

    m_isConnected = true;
    m_running = true;

    // Start receive thread
    m_recvThread = std::make_unique<std::thread>(&TCPTransport::ReceiveThreadFunc, this);

    LOG_INFO("TCPTransport", "Connected to {0}:{1}", m_config.host, m_config.port);
    return true;
}

void TCPTransport::Disconnect() {
    m_running = false;

    if (m_recvThread && m_recvThread->joinable()) {
        m_recvThread->join();
        m_recvThread.reset();
    }

    // Close all peer sockets (server mode)
    {
        std::lock_guard<std::mutex> lock(m_peersMutex);
        for (auto& [handle, peer] : m_peers) {
            CloseSocket(peer.socket);
        }
        m_peers.clear();
    }

    if (m_clientSocket != INVALID_SOCKET_VALUE) {
        CloseSocket(m_clientSocket);
        m_clientSocket = INVALID_SOCKET_VALUE;
    }

    if (m_listenSocket != INVALID_SOCKET_VALUE) {
        CloseSocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET_VALUE;
    }

    m_isConnected = false;
    m_isServer = false;
}

void TCPTransport::Shutdown() {
    Disconnect();
    m_initialized = false;

#ifdef _WIN32
    if (m_wsaStarted) {
        WSACleanup();
        m_wsaStarted = false;
    }
#endif

    LOG_INFO("TCPTransport", "Shut down");
}

bool TCPTransport::Send(ConnectionHandle to, const Packet& packet) {
    if (!m_initialized) return false;

    std::vector<uint8_t> wireData = packet.Serialize();

    if (m_isServer) {
        std::lock_guard<std::mutex> lock(m_peersMutex);
        auto it = m_peers.find(to);
        if (it == m_peers.end()) return false;
#ifdef _WIN32
        int sent = ::send(it->second.socket,
                          reinterpret_cast<const char*>(wireData.data()),
                          static_cast<int>(wireData.size()), 0);
        return sent > 0;
#else
        ssize_t sent = ::send(it->second.socket,
                              wireData.data(), wireData.size(), 0);
        return sent > 0;
#endif
    } else {
        if (m_clientSocket == INVALID_SOCKET_VALUE) return false;
#ifdef _WIN32
        int sent = ::send(m_clientSocket,
                          reinterpret_cast<const char*>(wireData.data()),
                          static_cast<int>(wireData.size()), 0);
        return sent > 0;
#else
        ssize_t sent = ::send(m_clientSocket,
                              wireData.data(), wireData.size(), 0);
        return sent > 0;
#endif
    }
}

bool TCPTransport::Broadcast(const Packet& packet) {
    if (!m_initialized || !m_isServer) return false;

    std::vector<uint8_t> wireData = packet.Serialize();
    bool allSent = true;

    std::lock_guard<std::mutex> lock(m_peersMutex);
    for (auto& [handle, peer] : m_peers) {
#ifdef _WIN32
        int sent = ::send(peer.socket,
                          reinterpret_cast<const char*>(wireData.data()),
                          static_cast<int>(wireData.size()), 0);
#else
        ssize_t sent = ::send(peer.socket,
                              wireData.data(), wireData.size(), 0);
#endif
        if (sent <= 0) allSent = false;
    }
    return allSent;
}

bool TCPTransport::Receive(Packet& outPacket, ConnectionHandle& outFrom) {
    std::lock_guard<std::mutex> lock(m_incomingMutex);
    if (m_incoming.empty()) return false;
    auto [from, pkt] = std::move(m_incoming.front());
    m_incoming.pop_front();
    outPacket = std::move(pkt);
    outFrom = from;
    return true;
}

ConnectionInfo TCPTransport::GetConnectionInfo(ConnectionHandle handle) const {
    ConnectionInfo info;
    info.handle = handle;
    std::lock_guard<std::mutex> lock(m_peersMutex);
    auto it = m_peers.find(handle);
    if (it != m_peers.end()) {
        info.address = it->second.address;
        info.port = it->second.port;
        info.connectTime = it->second.connectTime;
    }
    return info;
}

void TCPTransport::Poll() {
    AcceptNewConnections();
}

// ── Private Helpers ──

bool TCPTransport::SetNonBlocking(SOCKET_HANDLE sock) {
#ifdef _WIN32
    u_long mode = 1;
    return ioctlsocket(sock, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(sock, F_GETFL, 0);
    return fcntl(sock, F_SETFL, flags | O_NONBLOCK) == 0;
#endif
}

SOCKET_HANDLE TCPTransport::CreateSocket() {
    SOCKET_HANDLE sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET_VALUE) {
        LOG_ERROR("TCPTransport", "socket() failed");
        return INVALID_SOCKET_VALUE;
    }

    // Enable TCP_NODELAY (disable Nagle's algorithm)
    int flag = 1;
#ifdef _WIN32
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
               reinterpret_cast<const char*>(&flag), sizeof(flag));
#else
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
#endif

    // Allow address reuse
#ifdef _WIN32
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&flag), sizeof(flag));
#else
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
#endif

    return sock;
}

bool TCPTransport::BindSocket(SOCKET_HANDLE sock) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_config.port);

    if (m_config.host == "0.0.0.0" || m_config.host == "*") {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
#ifdef _WIN32
        addr.sin_addr.s_addr = inet_addr(m_config.host.c_str());
        if (addr.sin_addr.s_addr == INADDR_NONE) return false;
#else
        if (inet_pton(AF_INET, m_config.host.c_str(), &addr.sin_addr) <= 0)
            return false;
#endif
    }

#ifdef _WIN32
    return ::bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0;
#else
    return ::bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) >= 0;
#endif
}

void TCPTransport::AcceptNewConnections() {
    if (m_listenSocket == INVALID_SOCKET_VALUE) return;

    sockaddr_in clientAddr{};
#ifdef _WIN32
    int addrLen = sizeof(clientAddr);
    SOCKET_HANDLE clientSock = ::accept(m_listenSocket,
                                        reinterpret_cast<sockaddr*>(&clientAddr),
                                        &addrLen);
    if (clientSock == INVALID_SOCKET && WSAGetLastError() != WSAEWOULDBLOCK) {
        return;
    }
#else
    socklen_t addrLen = sizeof(clientAddr);
    SOCKET_HANDLE clientSock = ::accept(m_listenSocket,
                                        reinterpret_cast<sockaddr*>(&clientAddr),
                                        &addrLen);
    if (clientSock < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            LOG_ERROR("TCPTransport", "accept() failed");
        }
        return;
    }
#endif

    SetNonBlocking(clientSock);

    ConnectionHandle handle = m_nextHandle++;
    PeerConnection peer;
    peer.socket = clientSock;
#ifdef _WIN32
    peer.address = inet_ntoa(clientAddr.sin_addr);
#else
    char addrStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, addrStr, sizeof(addrStr));
    peer.address = addrStr;
#endif
    peer.port = ntohs(clientAddr.sin_port);
    peer.connectTime = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());

    {
        std::lock_guard<std::mutex> lock(m_peersMutex);
        m_peers[handle] = std::move(peer);
    }

    LOG_INFO("TCPTransport", "New connection from {0}:{1} (handle={2})",
             peer.address, peer.port, handle);
}

void TCPTransport::ReceiveThreadFunc() {
    // Buffer for raw received data
    std::vector<uint8_t> recvBuf(m_config.bufferSize);

    while (m_running) {
        // Use poll to check for data availability
        if (m_isServer) {
            // Check all peer sockets
            std::vector<pollfd> pollFds;
            std::vector<ConnectionHandle> handles;

            {
                std::lock_guard<std::mutex> lock(m_peersMutex);
                for (auto& [handle, peer] : m_peers) {
                    pollfd fd;
                    fd.fd = peer.socket;
                    fd.events = POLLIN;
                    fd.revents = 0;
                    pollFds.push_back(fd);
                    handles.push_back(handle);
                }
            }

            if (pollFds.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

#ifdef _WIN32
            int result = WSAPoll(pollFds.data(), static_cast<ULONG>(pollFds.size()), 10);
#else
            int result = poll(pollFds.data(), pollFds.size(), 10);
#endif
            if (result <= 0) continue;

            for (size_t i = 0; i < pollFds.size(); ++i) {
                if (!(pollFds[i].revents & POLLIN)) continue;

                ConnectionHandle handle = handles[i];
                SOCKET_HANDLE sock;

                {
                    std::lock_guard<std::mutex> lock(m_peersMutex);
                    auto it = m_peers.find(handle);
                    if (it == m_peers.end()) continue;
                    sock = it->second.socket;
                }

#ifdef _WIN32
                int bytesRead = ::recv(sock,
                                       reinterpret_cast<char*>(recvBuf.data()),
                                       static_cast<int>(recvBuf.size()), 0);
#else
                ssize_t bytesRead = ::recv(sock, recvBuf.data(), recvBuf.size(), 0);
#endif
                if (bytesRead <= 0) {
                    if (bytesRead == 0) {
                        LOG_INFO("TCPTransport", "Client {0} disconnected", handle);
                    }
                    {
                        std::lock_guard<std::mutex> lock(m_peersMutex);
                        CloseSocket(m_peers[handle].socket);
                        m_peers.erase(handle);
                    }
                    continue;
                }

                // Process received data into packets
                size_t offset = 0;
                while (offset < static_cast<size_t>(bytesRead)) {
                    size_t remaining = static_cast<size_t>(bytesRead) - offset;
                    if (remaining < PACKET_HEADER_SIZE) break;

                    PacketHeader header;
                    std::memcpy(&header, recvBuf.data() + offset, PACKET_HEADER_SIZE);
                    uint32_t totalSize = PACKET_HEADER_SIZE + header.payloadSize;

                    if (remaining < totalSize) break;

                    Packet pkt = Packet::Deserialize(recvBuf.data() + offset, totalSize);
                    offset += totalSize;

                    {
                        std::lock_guard<std::mutex> lock(m_incomingMutex);
                        m_incoming.emplace_back(handle, std::move(pkt));
                    }
                }
            }
        } else {
            // Client mode: single connection
            pollfd fd;
            fd.fd = m_clientSocket;
            fd.events = POLLIN;
            fd.revents = 0;

#ifdef _WIN32
            int result = WSAPoll(&fd, 1, 10);
#else
            int result = poll(&fd, 1, 10);
#endif
            if (result <= 0) continue;
            if (!(fd.revents & POLLIN)) continue;

#ifdef _WIN32
            int bytesRead = ::recv(m_clientSocket,
                                   reinterpret_cast<char*>(recvBuf.data()),
                                   static_cast<int>(recvBuf.size()), 0);
#else
            ssize_t bytesRead = ::recv(m_clientSocket, recvBuf.data(), recvBuf.size(), 0);
#endif
            if (bytesRead <= 0) {
                if (bytesRead == 0) {
                    LOG_INFO("TCPTransport", "Server disconnected");
                }
                m_isConnected = false;
                continue;
            }

            // Process received data into packets
            size_t offset = 0;
            while (offset < static_cast<size_t>(bytesRead)) {
                size_t remaining = static_cast<size_t>(bytesRead) - offset;
                if (remaining < PACKET_HEADER_SIZE) break;

                PacketHeader header;
                std::memcpy(&header, recvBuf.data() + offset, PACKET_HEADER_SIZE);
                uint32_t totalSize = PACKET_HEADER_SIZE + header.payloadSize;

                if (remaining < totalSize) break;

                Packet pkt = Packet::Deserialize(recvBuf.data() + offset, totalSize);
                offset += totalSize;

                {
                    std::lock_guard<std::mutex> lock(m_incomingMutex);
                    // Client mode: from handle = 0 (server)
                    m_incoming.emplace_back(0, std::move(pkt));
                }
            }
        }
    }
}

void TCPTransport::CloseSocket(SOCKET_HANDLE sock) {
    if (sock == INVALID_SOCKET_VALUE) return;
#ifdef _WIN32
    closesocket(sock);
#else
    ::close(sock);
#endif
}

} // namespace Prisma::Network
