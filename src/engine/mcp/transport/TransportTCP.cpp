#include "TransportTCP.h"
#include "Logger.h"
#include <sstream>
#include <cstring>

// Platform-specific socket includes
#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
    #include <ws2tcpip.h>
    using SOCKADDR_IN = struct sockaddr_in;
    using SOCKET_HANDLE = SOCKET;
    constexpr SOCKET_HANDLE INVALID_SOCKET_HANDLE = INVALID_SOCKET;
    constexpr int SOCKET_ERROR_RET = SOCKET_ERROR;
    #define CLOSE_SOCKET(s) closesocket(s)
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <arpa/inet.h>
    using SOCKET_HANDLE = int;
    constexpr SOCKET_HANDLE INVALID_SOCKET_HANDLE = -1;
    constexpr int SOCKET_ERROR_RET = -1;
    #define CLOSE_SOCKET(s) close(s)
#endif

namespace Prisma {
namespace MCP {

TransportTCP::TransportTCP(uint16_t port)
    : m_Port(port)
    , m_ServerSocket(INVALID_SOCKET_HANDLE)
    , m_ClientSocket(INVALID_SOCKET_HANDLE) {}

TransportTCP::~TransportTCP() { Stop(); }

bool TransportTCP::Start(MCPMessageHandler handler) {
    if (m_Running) return false;
    m_Handler = std::move(handler);
    m_Running = true;

#if defined(_WIN32)
    // Initialize Winsock2
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        LOG_ERROR("MCP", "WSAStartup failed: {}", result);
        m_Running = false;
        return false;
    }
#endif

    // Create server socket
    m_ServerSocket = static_cast<int>(socket(AF_INET, SOCK_STREAM, 0));
    if (m_ServerSocket == INVALID_SOCKET_HANDLE) {
        LOG_ERROR("MCP", "Failed to create TCP socket");
        m_Running = false;
        return false;
    }

    // Allow address reuse
    int opt = 1;
    setsockopt(m_ServerSocket, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&opt), sizeof(opt));

    // Bind
    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(m_Port);

    if (bind(m_ServerSocket, reinterpret_cast<struct sockaddr*>(&serverAddr),
             sizeof(serverAddr)) == SOCKET_ERROR_RET) {
        LOG_ERROR("MCP", "Failed to bind TCP socket to port {}", m_Port);
        CLOSE_SOCKET(m_ServerSocket);
        m_ServerSocket = INVALID_SOCKET_HANDLE;
        m_Running = false;
        return false;
    }

    // Listen
    if (listen(m_ServerSocket, 1) == SOCKET_ERROR_RET) {
        LOG_ERROR("MCP", "Failed to listen on TCP socket");
        CLOSE_SOCKET(m_ServerSocket);
        m_ServerSocket = INVALID_SOCKET_HANDLE;
        m_Running = false;
        return false;
    }

    LOG_INFO("MCP", "TCP server listening on port {} (waiting for AI agent connection...)",
             m_Port);

    // Accept client in background thread
    m_AcceptThread = std::thread(&TransportTCP::acceptLoop, this);

    return true;
}

void TransportTCP::Stop() {
    m_Running = false;

    // Close sockets to unblock accept/recv
    if (m_ClientSocket != INVALID_SOCKET_HANDLE) {
        CLOSE_SOCKET(m_ClientSocket);
        m_ClientSocket = INVALID_SOCKET_HANDLE;
    }
    if (m_ServerSocket != INVALID_SOCKET_HANDLE) {
        CLOSE_SOCKET(m_ServerSocket);
        m_ServerSocket = INVALID_SOCKET_HANDLE;
    }

    if (m_AcceptThread.joinable()) {
        m_AcceptThread.join();
    }
    if (m_ReadThread.joinable()) {
        m_ReadThread.join();
    }

#if defined(_WIN32)
    WSACleanup();
#endif
}

bool TransportTCP::Send(const nlohmann::json& message) {
    if (!m_Running || m_ClientSocket == INVALID_SOCKET_HANDLE)
        return false;

    try {
        std::string output = message.dump() + "\n";
        int sent = static_cast<int>(::send(m_ClientSocket, output.c_str(),
                                            static_cast<int>(output.size()), 0));
        return sent > 0;
    } catch (...) {
        return false;
    }
}

bool TransportTCP::IsConnected() const {
    return m_Running && m_ClientSocket != INVALID_SOCKET_HANDLE;
}

void TransportTCP::acceptLoop() {
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    SOCKET_HANDLE client = static_cast<SOCKET_HANDLE>(
        accept(m_ServerSocket, reinterpret_cast<struct sockaddr*>(&clientAddr),
               &clientLen));

    if (client == INVALID_SOCKET_HANDLE || !m_Running) {
        return;
    }

    m_ClientSocket = client;

    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
    LOG_INFO("MCP", "TCP client connected from {}:{}", clientIP, ntohs(clientAddr.sin_port));

    // Start read loop for this client
    m_ReadThread = std::thread([this]() {
        std::string buffer;
        char temp[4096];

        while (m_Running && m_ClientSocket != INVALID_SOCKET_HANDLE) {
            int bytes = static_cast<int>(
                ::recv(m_ClientSocket, temp, sizeof(temp) - 1, 0));

            if (bytes <= 0) {
                // Client disconnected
                LOG_INFO("MCP", "TCP client disconnected");
                break;
            }

            temp[bytes] = '\0';
            buffer += temp;

            // Process complete lines
            size_t pos;
            while ((pos = buffer.find('\n')) != std::string::npos) {
                std::string line = buffer.substr(0, pos);
                buffer.erase(0, pos + 1);

                if (!line.empty()) {
                    try {
                        auto json = nlohmann::json::parse(line);
                        if (m_Handler) {
                            m_Handler(json);
                        }
                    } catch (const nlohmann::json::parse_error& e) {
                        nlohmann::json err = {
                            {"jsonrpc", "2.0"},
                            {"id", nullptr},
                            {"error", {{"code", -32700},
                                       {"message", std::string("Parse error: ") + e.what()}}}
                        };
                        Send(err);
                    }
                }
            }
        }

        CLOSE_SOCKET(m_ClientSocket);
        m_ClientSocket = INVALID_SOCKET_HANDLE;
    });
}

} // namespace MCP
} // namespace Prisma
