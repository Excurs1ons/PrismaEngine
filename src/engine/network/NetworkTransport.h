#pragma once

#include "Export.h"
#include "network/Packet.h"
#include <string>
#include <cstdint>

namespace Prisma::Network {

// ═══════════════════════════════════════════════════════════════════
// TransportConfig — common configuration for all transports
// ═══════════════════════════════════════════════════════════════════
struct TransportConfig {
    std::string host     = "127.0.0.1";
    uint16_t    port     = 25565;  // Default Minecraft-inspired port
    uint32_t    bufferSize = 65536; // 64KB receive buffer
    bool        nonBlocking = true;
};

// ═══════════════════════════════════════════════════════════════════
// ConnectionHandle — opaque identifier for a connected peer
// ═══════════════════════════════════════════════════════════════════
using ConnectionHandle = uint32_t;
static constexpr ConnectionHandle INVALID_CONNECTION = 0;

// ═══════════════════════════════════════════════════════════════════
// ConnectionInfo — information about a connected peer
// ═══════════════════════════════════════════════════════════════════
struct ConnectionInfo {
    ConnectionHandle handle = INVALID_CONNECTION;
    std::string address;
    uint16_t port = 0;
    uint64_t connectTime = 0;
    double   rttMs = 0.0;  // Round-trip time estimate
};

// ═══════════════════════════════════════════════════════════════════
// ITransport — abstract network transport interface
// ═══════════════════════════════════════════════════════════════════
//
// Provides a unified interface for reliable (TCP) and unreliable (UDP)
// transport layers. Each transport implementation manages its own
// socket lifecycle.
//
class ENGINE_API ITransport {
public:
    virtual ~ITransport() = default;

    // ── Lifecycle ──

    /// Initialize the transport with given configuration
    virtual bool Initialize(const TransportConfig& config) = 0;

    /// Start listening (server mode). Must call Initialize() first.
    virtual bool Listen() = 0;

    /// Connect to a remote host (client mode). Must call Initialize() first.
    virtual bool Connect() = 0;

    /// Disconnect from remote host or stop listening
    virtual void Disconnect() = 0;

    /// Shutdown and release all resources
    virtual void Shutdown() = 0;

    // ── Data I/O ──

    /// Send a packet to a specific connection. For client mode, connection may be ignored.
    /// Returns true if the send was queued successfully.
    virtual bool Send(ConnectionHandle to, const Packet& packet) = 0;

    /// Send a packet to all connected peers (server broadcast).
    virtual bool Broadcast(const Packet& packet) = 0;

    /// Receive a packet from any connection. Returns false if no data available.
    virtual bool Receive(Packet& outPacket, ConnectionHandle& outFrom) = 0;

    // ── Status ──

    /// Returns true if transport is initialized and ready
    virtual bool IsReady() const = 0;

    /// Returns true if this transport is in server mode (listening)
    virtual bool IsServer() const = 0;

    /// Returns true if this transport is connected (client mode)
    virtual bool IsConnected() const = 0;

    /// Get connection info for a specific handle
    virtual ConnectionInfo GetConnectionInfo(ConnectionHandle handle) const = 0;

    /// Get the transport configuration
    virtual const TransportConfig& GetConfig() const = 0;

    /// Poll the transport for any pending work (non-blocking)
    virtual void Poll() = 0;
};

} // namespace Prisma::Network
