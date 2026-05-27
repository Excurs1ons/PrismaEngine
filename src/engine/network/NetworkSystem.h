#pragma once

#ifdef _MSC_VER
#include <intrin.h>
#pragma intrinsic(__faststorefence)
#endif

#include "ISubSystem.h"
#include "Export.h"
#include "network/Session.h"
#include "network/NetworkEntity.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <atomic>
#include <thread>
#include <functional>

namespace Prisma {

class Scene;

namespace Network {

// ═══════════════════════════════════════════════════════════════════
// NetworkSystem — ISubSystem implementation for multiplayer networking.
// Initializes transport, manages the network session, processes
// incoming/outgoing packets, and synchronizes entity state.
// ═══════════════════════════════════════════════════════════════════
class ENGINE_API NetworkSystem : public ISubSystem {
public:
    NetworkSystem() = default;
    ~NetworkSystem() override;

    NetworkSystem(const NetworkSystem&) = delete;
    NetworkSystem& operator=(const NetworkSystem&) = delete;

    // ISubSystem interface
    int Initialize() override;
    void Shutdown() override;
    void Update(Timestep ts) override;
    const char* GetName() const override { return "NetworkSystem"; }

    // ── Session Management ──

    /// Start a server session.
    bool StartServer(const SessionConfig& config);

    /// Start a client session.
    bool StartClient(const SessionConfig& config);

    /// Stop the current session.
    void StopSession();

    /// Check if a session is active.
    bool HasActiveSession() const { return m_session && m_session->IsRunning(); }

    /// Get the current session.
    Session* GetSession() { return m_session.get(); }
    const Session* GetSession() const { return m_session.get(); }

    // ── Entity State Sync ──

    /// Register a scene for network entity sync.
    void RegisterScene(Scene* scene);

    /// Unregister a scene.
    void UnregisterScene();

    /// Get the entity-to-NetworkId mapping.
    const std::unordered_map<uint32_t, NetworkId>& GetEntityMap() const { return m_entityMap; }

    /// Get the NetworkId-to-entity mapping.
    const std::unordered_map<NetworkId, uint32_t>& GetIdEntityMap() const { return m_idEntityMap; }

    /// Assign a NetworkId to a scene entity.
    NetworkId AssignNetworkId(uint32_t entityHandle);

    /// Remove a NetworkId from an entity.
    void RemoveNetworkId(uint32_t entityHandle);

    /// Find entity by NetworkId.
    uint32_t FindEntity(NetworkId netId) const;

    // ── RPC ──

    RPCManager& GetRPCManager();
    void RegisterRPC(const std::string& name, RPCCallback callback);
    bool CallRPC(ConnectionHandle to, const std::string& name,
                 const std::vector<uint8_t>& args = {});

    // ── Configuration ──

    void SetTickRate(float rate) { m_tickRate = rate; }
    float GetTickRate() const { return m_tickRate; }

private:
    // Internal update
    void ProcessNetwork(Timestep ts);
    void SendEntityStates();
    void ReceiveEntityStates();
    void TickNetwork(Timestep ts);

    // Entity state serialization
    std::vector<uint8_t> SerializeEntityState(uint32_t entityHandle, NetworkComponent& netComp);
    void DeserializeEntityState(uint32_t entityHandle, const std::vector<uint8_t>& data);

    // Network ID generation
    std::atomic<uint32_t> m_nextEntityIndex{1};

    // Session
    std::unique_ptr<Session> m_session;
    bool m_sessionOwned = false;

    // Scene reference
    Scene* m_scene = nullptr;

    // Entity ↔ NetworkId mappings
    std::unordered_map<uint32_t, NetworkId> m_entityMap;
    std::unordered_map<NetworkId, uint32_t> m_idEntityMap;
    mutable std::mutex m_entityMapMutex;

    // State sync
    float m_stateAccumulator = 0.0f;
    float m_tickRate = 20.0f; // 20 updates per second
};

} // namespace Network
} // namespace Prisma
