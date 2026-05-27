#include "network/NetworkSystem.h"
#include "network/TCPTransport.h"
#include "network/UDPTransport.h"
#include "scene/Scene.h"
#include "core/Node.h"
#include "Logger.h"

#include <cmath>

namespace Prisma::Network {

NetworkSystem::~NetworkSystem() {
    Shutdown();
}

int NetworkSystem::Initialize() {
    LOG_INFO("NetworkSystem", "Network system initializing");
    m_nextEntityIndex.store(1);
    m_stateAccumulator = 0.0f;
    return 0;
}

void NetworkSystem::Shutdown() {
    LOG_INFO("NetworkSystem", "Network system shutting down");
    StopSession();
    m_entityMap.clear();
    m_idEntityMap.clear();
}

void NetworkSystem::Update(Timestep ts) {
    ProcessNetwork(ts);
}

void NetworkSystem::ProcessNetwork(Timestep ts) {
    if (!m_session || !m_session->IsRunning()) return;

    // Poll transports for new data
    m_session->PollTransports();

    // Process incoming packets
    m_session->ProcessIncoming();

    // Throttled state sync
    m_stateAccumulator += ts.GetSeconds();
    float tickInterval = 1.0f / m_tickRate;

    while (m_stateAccumulator >= tickInterval) {
        m_stateAccumulator -= tickInterval;
        TickNetwork(Timestep(tickInterval));
    }
}

bool NetworkSystem::StartServer(const SessionConfig& config) {
    if (m_session && m_session->IsRunning()) {
        LOG_WARNING("NetworkSystem", "Session already running, stopping first");
        StopSession();
    }

    SessionConfig cfg = config;
    cfg.mode = SessionMode::Server;

    m_session = std::make_unique<Session>();
    m_sessionOwned = true;

    if (!m_session->StartSession(cfg)) {
        m_session.reset();
        m_sessionOwned = false;
        LOG_ERROR("NetworkSystem", "Failed to start server session");
        return false;
    }

    LOG_INFO("NetworkSystem", "Server started on {0}:{1}", cfg.host, cfg.port);
    return true;
}

bool NetworkSystem::StartClient(const SessionConfig& config) {
    if (m_session && m_session->IsRunning()) {
        LOG_WARNING("NetworkSystem", "Session already running, stopping first");
        StopSession();
    }

    SessionConfig cfg = config;
    cfg.mode = SessionMode::Client;

    m_session = std::make_unique<Session>();
    m_sessionOwned = true;

    if (!m_session->StartSession(cfg)) {
        m_session.reset();
        m_sessionOwned = false;
        LOG_ERROR("NetworkSystem", "Failed to start client session");
        return false;
    }

    LOG_INFO("NetworkSystem", "Client connecting to {0}:{1}", cfg.host, cfg.port);
    return true;
}

void NetworkSystem::StopSession() {
    if (m_session && m_sessionOwned) {
        m_session->StopSession();
        m_session.reset();
        m_sessionOwned = false;
        LOG_INFO("NetworkSystem", "Session stopped");
    }
    m_entityMap.clear();
    m_idEntityMap.clear();
}

void NetworkSystem::RegisterScene(Scene* scene) {
    m_scene = scene;
    LOG_INFO("NetworkSystem", "Scene registered for network sync");
}

void NetworkSystem::UnregisterScene() {
    m_scene = nullptr;
}

NetworkId NetworkSystem::AssignNetworkId(uint32_t entityHandle) {
    uint32_t serverToken = m_session ? m_session->GetServerToken() : GenerateServerToken();
    uint32_t index = m_nextEntityIndex.fetch_add(1);
    NetworkId netId = MakeNetworkId(serverToken, index);

    {
        std::lock_guard<std::mutex> lock(m_entityMapMutex);
        m_entityMap[entityHandle] = netId;
        m_idEntityMap[netId] = entityHandle;
    }

    return netId;
}

void NetworkSystem::RemoveNetworkId(uint32_t entityHandle) {
    std::lock_guard<std::mutex> lock(m_entityMapMutex);
    auto it = m_entityMap.find(entityHandle);
    if (it != m_entityMap.end()) {
        m_idEntityMap.erase(it->second);
        m_entityMap.erase(it);
    }
}

uint32_t NetworkSystem::FindEntity(NetworkId netId) const {
    std::lock_guard<std::mutex> lock(m_entityMapMutex);
    auto it = m_idEntityMap.find(netId);
    return it != m_idEntityMap.end() ? it->second : Scene::INVALID_ENTITY;
}

RPCManager& NetworkSystem::GetRPCManager() {
    if (m_session) return m_session->GetRPCManager();
    static RPCManager fallback;
    return fallback;
}

void NetworkSystem::RegisterRPC(const std::string& name, RPCCallback callback) {
    GetRPCManager().RegisterRPC(name, std::move(callback));
}

bool NetworkSystem::CallRPC(ConnectionHandle to, const std::string& name,
                            const std::vector<uint8_t>& args) {
    if (!m_session) return false;
    return m_session->CallRPC(to, name, args);
}

void NetworkSystem::TickNetwork(Timestep ts) {
    if (!m_session || !m_session->IsRunning()) return;
    if (!m_scene) return;

    if (m_session->GetMode() == SessionMode::Server) {
        SendEntityStates();
    }
}

void NetworkSystem::SendEntityStates() {
    if (!m_scene || !m_session) return;
    if (m_session->GetMode() != SessionMode::Server) return;

    ITransport* udp = m_session->GetUDPTransport();
    if (!udp) return;

    // Iterate entities with NetworkComponent and serialize dirty ones
    std::lock_guard<std::mutex> lock(m_entityMapMutex);

    for (auto& [entityHandle, netId] : m_entityMap) {
        Node node(entityHandle);
        auto netComp = m_scene->GetComponent<NetworkComponent>(node);
        if (!netComp || !netComp->IsEnabled()) continue;
        if (!netComp->IsDirty()) continue;

        // Serialize the entity state
        std::vector<uint8_t> stateData = SerializeEntityState(entityHandle, *netComp);

        // Create state sync packet
        Packet statePkt;
        statePkt.header.sequence = 0;
        statePkt.header.type = static_cast<uint8_t>(PacketType::StateSync);
        statePkt.payload = std::move(stateData);
        statePkt.header.payloadSize = static_cast<uint32_t>(statePkt.payload.size());

        udp->Broadcast(statePkt);
        netComp->ClearDirty();
    }
}

void NetworkSystem::ReceiveEntityStates() {
    // State sync reception is handled in Session::ProcessIncoming()
    // and dispatched to HandleStateSyncPacket. The actual deserialization
    // and application to scene entities happens here.
    if (!m_scene || !m_session) return;
    if (m_session->GetMode() != SessionMode::Client) return;
}

std::vector<uint8_t> NetworkSystem::SerializeEntityState(uint32_t entityHandle,
                                                          NetworkComponent& netComp) {
    BinarySerializer writer;
    writer.WriteUint64(netComp.GetNetworkId());
    writer.WriteUint8(static_cast<uint8_t>(netComp.GetSyncFlags()));

    if (!m_scene) return writer.MoveData();

    Node node(entityHandle);

    // Serialize transform
    if (HasFlag(netComp.GetSyncFlags(), SyncFlags::Transform)) {
        auto transform = m_scene->GetComponent<Transform>(node);
        if (transform) {
            auto pos = transform->GetLocalPosition();
            auto rot = transform->GetLocalRotation();
            auto scl = transform->GetLocalScale();
            writer.WriteFloat(pos.x);
            writer.WriteFloat(pos.y);
            writer.WriteFloat(pos.z);
            writer.WriteFloat(rot.x);
            writer.WriteFloat(rot.y);
            writer.WriteFloat(rot.z);
            writer.WriteFloat(rot.w);
            writer.WriteFloat(scl.x);
            writer.WriteFloat(scl.y);
            writer.WriteFloat(scl.z);
        }
    }

    return writer.MoveData();
}

void NetworkSystem::DeserializeEntityState(uint32_t entityHandle,
                                           const std::vector<uint8_t>& data) {
    if (!m_scene) return;

    BinarySerializer reader(data);
    NetworkId netId = reader.ReadUint64();
    SyncFlags flags = static_cast<SyncFlags>(reader.ReadUint8());

    Node node(entityHandle);
    auto netComp = m_scene->GetComponent<NetworkComponent>(node);

    // Deserialize transform
    if (HasFlag(flags, SyncFlags::Transform)) {
        auto transform = m_scene->GetComponent<Transform>(node);
        if (transform) {
            glm::vec3 pos, scl;
            glm::quat rot;
            pos.x = reader.ReadFloat();
            pos.y = reader.ReadFloat();
            pos.z = reader.ReadFloat();
            rot.x = reader.ReadFloat();
            rot.y = reader.ReadFloat();
            rot.z = reader.ReadFloat();
            rot.w = reader.ReadFloat();
            scl.x = reader.ReadFloat();
            scl.y = reader.ReadFloat();
            scl.z = reader.ReadFloat();
            transform->SetLocalPosition(pos);
            transform->SetLocalRotation(rot);
            transform->SetLocalScale(scl);
        }
    }

    if (netComp) {
        netComp->ClearDirty();
    }
}

} // namespace Prisma::Network
