#pragma once

#include "Export.h"
#include "network/NetworkTransport.h"
#include "network/RPCManager.h"
#include "network/NetworkEntity.h"

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <atomic>

namespace Prisma::Network {

// ═══════════════════════════════════════════════════════════════════
// SessionMode — server or client
// ═══════════════════════════════════════════════════════════════════
enum class SessionMode {
    None,
    Server,
    Client,
};

// ═══════════════════════════════════════════════════════════════════
// SessionConfig — configuration for starting a session
// ═══════════════════════════════════════════════════════════════════
struct SessionConfig {
    std::string host     = "127.0.0.1";
    uint16_t    port     = 25565;
    SessionMode mode     = SessionMode::None;
    uint32_t    maxClients = 16;      // Server: max connected clients
    bool        useTCP   = true;      // TCP for reliable RPC
    bool        useUDP   = true;      // UDP for fast state sync
    float       tickRate = 20.0f;     // State sync updates per second
};

// ═══════════════════════════════════════════════════════════════════
// Session — manages a network session (server or client)
// ═══════════════════════════════════════════════════════════════════
class ENGINE_API Session {
public:
    Session();
    ~Session();

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    // ── Lifecycle ──

    /// Start a session with the given configuration.
    /// Returns true on success.
    bool StartSession(const SessionConfig& config);

    /// Stop the session and disconnect all peers.
    void StopSession();

    /// Disconnect a specific client (server only).
    void DisconnectClient(ConnectionHandle handle);

    // ── Status ──

    bool IsRunning() const { return m_running; }
    SessionMode GetMode() const { return m_mode; }
    const SessionConfig& GetConfig() const { return m_config; }

    /// Get this session's connection handle (0 for server, assigned by server for client).
    ConnectionHandle GetLocalHandle() const { return m_localHandle; }

    /// Get the number of connected peers.
    size_t GetPeerCount() const;

    // ── RPC ──

    RPCManager& GetRPCManager() { return m_rpcManager; }
    const RPCManager& GetRPCManager() const { return m_rpcManager; }

    /// Call an RPC on a remote peer.
    bool CallRPC(ConnectionHandle to, const std::string& name,
                 const std::vector<uint8_t>& args = {});

    /// Broadcast an RPC to all connected peers (server only).
    bool BroadcastRPC(const std::string& name,
                      const std::vector<uint8_t>& args = {});

    /// Register a handler for session events
    using SessionEventHandler = std::function<void(ConnectionHandle, const std::string&)>;
    void SetOnPeerConnected(SessionEventHandler handler) { m_onPeerConnected = std::move(handler); }
    void SetOnPeerDisconnected(SessionEventHandler handler) { m_onPeerDisconnected = std::move(handler); }

    // ── Transport Access ──

    ITransport* GetTCPTransport() const { return m_tcpTransport.get(); }
    ITransport* GetUDPTransport() const { return m_udpTransport.get(); }
    ITransport* GetActiveTransport() const;

    // ── Internals (called by NetworkSystem) ──

    void PollTransports();
    void ProcessIncoming();
    void SendStateSync();

    /// Get the server token for NetworkId generation.
    uint32_t GetServerToken() const { return m_serverToken; }

private:
    void HandleConnectPacket(ConnectionHandle from, const Packet& pkt);
    void HandleDisconnectPacket(ConnectionHandle from, const Packet& pkt);
    void HandleAckPacket(ConnectionHandle from, const Packet& pkt);
    void HandleStateSyncPacket(ConnectionHandle from, const Packet& pkt);
    void HandleFragmentPacket(ConnectionHandle from, const Packet& pkt);
    void SendConnectAck(ConnectionHandle to);
    void SendPing();

    SessionConfig m_config;
    SessionMode m_mode = SessionMode::None;
    std::atomic<bool> m_running{false};
    ConnectionHandle m_localHandle = 0; // 0 = not connected / server

    // Transports
    std::unique_ptr<ITransport> m_tcpTransport;
    std::unique_ptr<ITransport> m_udpTransport;

    // RPC
    RPCManager m_rpcManager;

    // State tracking
    uint32_t m_serverToken = 0;
    uint64_t m_sessionStartTime = 0;
    float m_stateSyncAccumulator = 0.0f;

    // Connection list (server tracks here, client just has remote)
    std::vector<ConnectionInfo> m_connectedPeers;

    // Fragment reassembly
    struct FragmentBuffer {
        uint32_t baseSequence;
        std::vector<uint8_t> data;
        uint16_t totalFragments;
        std::vector<bool> received;
    };
    std::unordered_map<uint32_t, FragmentBuffer> m_fragmentBuffers;

    // Event handlers
    SessionEventHandler m_onPeerConnected;
    SessionEventHandler m_onPeerDisconnected;
};

} // namespace Prisma::Network
