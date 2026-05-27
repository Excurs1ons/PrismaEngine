#pragma once

#include "Export.h"
#include "core/Component.h"
#include "network/NetworkTransport.h"

#include <cstdint>
#include <atomic>
#include <chrono>

namespace Prisma::Network {

// ═══════════════════════════════════════════════════════════════════
// NetworkId — unique identifier for network-replicated entities.
// High 32 bits: server start time (seconds since epoch, low 32 bits)
// Low 32 bits:  entity index (auto-increment)
// ═══════════════════════════════════════════════════════════════════
using NetworkId = uint64_t;

inline NetworkId MakeNetworkId(uint32_t serverToken, uint32_t entityIndex) {
    return (static_cast<NetworkId>(serverToken) << 32) | static_cast<NetworkId>(entityIndex);
}

inline uint32_t GetNetworkIdToken(NetworkId id) {
    return static_cast<uint32_t>(id >> 32);
}

inline uint32_t GetNetworkIdIndex(NetworkId id) {
    return static_cast<uint32_t>(id & 0xFFFFFFFFULL);
}

/// Generate a server token from the current time
inline uint32_t GenerateServerToken() {
    auto now = std::chrono::system_clock::now();
    auto secs = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
    return static_cast<uint32_t>(secs & 0xFFFFFFFFULL);
}

// ═══════════════════════════════════════════════════════════════════
// SyncFlags — bitmask controlling which components are synchronized
// ═══════════════════════════════════════════════════════════════════
enum class SyncFlags : uint8_t {
    None        = 0,
    Transform   = 1 << 0,
    RigidBody   = 1 << 1,
    Animation   = 1 << 2,
    Custom      = 1 << 3,
    All         = Transform | RigidBody | Animation | Custom,
};

inline SyncFlags operator|(SyncFlags a, SyncFlags b) {
    return static_cast<SyncFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline SyncFlags operator&(SyncFlags a, SyncFlags b) {
    return static_cast<SyncFlags>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

inline bool HasFlag(SyncFlags value, SyncFlags flag) {
    return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0;
}

// ═══════════════════════════════════════════════════════════════════
// NetworkComponent — component attached to entities that should
// be synchronized over the network.
// ═══════════════════════════════════════════════════════════════════
class ENGINE_API NetworkComponent : public Component {
public:
    NetworkComponent() = default;
    explicit NetworkComponent(NetworkId netId, SyncFlags flags = SyncFlags::Transform)
        : m_networkId(netId), m_syncFlags(flags) {}

    // Component interface
    void Initialize() override {}
    void Shutdown() override {}
    const char* GetComponentTypeName() const override { return "NetworkComponent"; }
    ComponentId GetComponentId() const override {
        return GetComponentTypeId<NetworkComponent>();
    }

    // Network ID
    void SetNetworkId(NetworkId id) { m_networkId = id; }
    NetworkId GetNetworkId() const { return m_networkId; }

    // Sync flags
    void SetSyncFlags(SyncFlags flags) { m_syncFlags = flags; }
    SyncFlags GetSyncFlags() const { return m_syncFlags; }

    // Authority (server owns truth)
    bool IsAuthoritative() const { return m_authoritative; }
    void SetAuthoritative(bool auth) { m_authoritative = auth; }

    // Dirty flag: set when any synced property changes
    void MarkDirty() { m_dirty = true; }
    void ClearDirty() { m_dirty = false; }
    bool IsDirty() const { return m_dirty; }

    // Owner connection (server-side, which client owns this entity)
    ConnectionHandle GetOwnerConnection() const { return m_ownerConnection; }
    void SetOwnerConnection(ConnectionHandle handle) { m_ownerConnection = handle; }

private:
    NetworkId m_networkId = 0;
    SyncFlags m_syncFlags = SyncFlags::Transform;
    bool m_authoritative = false;
    bool m_dirty = true;
    ConnectionHandle m_ownerConnection = INVALID_CONNECTION;
};

} // namespace Prisma::Network
