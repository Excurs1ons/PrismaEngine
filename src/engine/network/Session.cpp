#include "network/Session.h"
#include "network/TCPTransport.h"
#include "network/UDPTransport.h"
#include "Logger.h"
#include "core/Timestep.h"

#include <cmath>

namespace Prisma::Network {

Session::Session() = default;

Session::~Session() {
    StopSession();
}

bool Session::StartSession(const SessionConfig& config) {
    if (m_running) {
        LOG_WARNING("Session", "Session already running. Stop it first.");
        return false;
    }

    m_config = config;
    m_mode = config.mode;
    m_serverToken = GenerateServerToken();
    m_sessionStartTime = 
        static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    m_stateSyncAccumulator = 0.0f;

    LOG_INFO("Session", "Starting session as {0} on {1}:{2}",
             config.mode == SessionMode::Server ? "Server" : "Client",
             config.host, config.port);

    // Create transports
    if (config.useTCP) {
        auto tcp = std::make_unique<TCPTransport>();
        TransportConfig tcpCfg;
        tcpCfg.host = config.host;
        tcpCfg.port = config.port;
        if (!tcp->Initialize(tcpCfg)) {
            LOG_ERROR("Session", "Failed to initialize TCP transport");
            return false;
        }
        m_tcpTransport = std::move(tcp);
    }

    if (config.useUDP) {
        auto udp = std::make_unique<UDPTransport>();
        TransportConfig udpCfg;
        udpCfg.host = config.host;
        udpCfg.port = config.port + 1; // UDP on port+1 by convention
        if (!udp->Initialize(udpCfg)) {
            LOG_ERROR("Session", "Failed to initialize UDP transport");
            return false;
        }
        m_udpTransport = std::move(udp);
    }

    // Start listening or connect
    if (config.mode == SessionMode::Server) {
        // Server: listen on TCP (reliable RPC) and optionally UDP (state sync)
        if (m_tcpTransport && !m_tcpTransport->Listen()) {
            LOG_ERROR("Session", "TCP listen failed on {0}:{1}", config.host, config.port);
            return false;
        }
        if (m_udpTransport && !m_udpTransport->Listen()) {
            LOG_ERROR("Session", "UDP listen failed on {0}:{1}", config.host, config.port + 1);
            return false;
        }
        m_localHandle = 0; // Server has no local handle
        LOG_INFO("Session", "Server listening on TCP {0}:{1}, UDP {0}:{2}",
                 config.host, config.port, config.port + 1);
    } else {
        // Client: connect to server
        if (m_tcpTransport && !m_tcpTransport->Connect()) {
            LOG_ERROR("Session", "TCP connect failed to {0}:{1}", config.host, config.port);
            return false;
        }
        if (m_udpTransport && !m_udpTransport->Connect()) {
            LOG_ERROR("Session", "UDP connect failed to {0}:{1}", config.host, config.port + 1);
            return false;
        }

        // Send connect packet over TCP
        if (m_tcpTransport) {
            BinarySerializer writer;
            writer.WriteUint32(12345); // Protocol version placeholder
            writer.WriteString("PrismaEngine");

            Packet connectPkt;
            connectPkt.header.sequence = 0;
            connectPkt.header.type = static_cast<uint8_t>(PacketType::Connect);
            connectPkt.header.payloadSize = static_cast<uint32_t>(writer.GetData().size());
            connectPkt.payload = writer.MoveData();
            m_tcpTransport->Send(0, connectPkt);
        }
        LOG_INFO("Session", "Client connecting to {0}:{1}", config.host, config.port);
    }

    m_running = true;
    return true;
}

void Session::StopSession() {
    if (!m_running) return;

    LOG_INFO("Session", "Stopping session...");

    // Send disconnect notifications
    if (m_tcpTransport) {
        if (m_mode == SessionMode::Client && m_tcpTransport->IsConnected()) {
            Packet disconnectPkt;
            disconnectPkt.header.type = static_cast<uint8_t>(PacketType::Disconnect);
            m_tcpTransport->Send(0, disconnectPkt);
        } else if (m_mode == SessionMode::Server) {
            // Broadcast disconnect to all peers
            Packet disconnectPkt;
            disconnectPkt.header.type = static_cast<uint8_t>(PacketType::Disconnect);
            m_tcpTransport->Broadcast(disconnectPkt);
        }
    }

    if (m_tcpTransport) m_tcpTransport->Disconnect();
    if (m_udpTransport) m_udpTransport->Disconnect();

    if (m_tcpTransport) m_tcpTransport->Shutdown();
    if (m_udpTransport) m_udpTransport->Shutdown();

    m_tcpTransport.reset();
    m_udpTransport.reset();
    m_connectedPeers.clear();
    m_fragmentBuffers.clear();
    m_running = false;
    m_mode = SessionMode::None;

    LOG_INFO("Session", "Session stopped");
}

void Session::DisconnectClient(ConnectionHandle handle) {
    if (m_mode != SessionMode::Server) return;

    if (m_onPeerDisconnected) {
        ConnectionInfo* info = nullptr;
        for (auto& peer : m_connectedPeers) {
            if (peer.handle == handle) {
                info = &peer;
                break;
            }
        }
        m_onPeerDisconnected(handle, info ? info->address : "unknown");
    }

    // Send disconnect notification
    if (m_tcpTransport) {
        Packet disconnectPkt;
        disconnectPkt.header.type = static_cast<uint8_t>(PacketType::Disconnect);
        m_tcpTransport->Send(handle, disconnectPkt);
    }

    std::erase_if(m_connectedPeers, [handle](const ConnectionInfo& peer) {
        return peer.handle == handle;
    });

    LOG_INFO("Session", "Client {0} disconnected", handle);
}

size_t Session::GetPeerCount() const {
    return m_connectedPeers.size();
}

bool Session::CallRPC(ConnectionHandle to, const std::string& name,
                      const std::vector<uint8_t>& args) {
    if (!m_tcpTransport || !m_running) return false;

    static uint32_t rpcSeq = 1;
    Packet pkt = m_rpcManager.CreateRPCPacket(rpcSeq++, name, args);
    return m_tcpTransport->Send(to, pkt);
}

bool Session::BroadcastRPC(const std::string& name,
                           const std::vector<uint8_t>& args) {
    if (!m_tcpTransport || !m_running || m_mode != SessionMode::Server) return false;

    static uint32_t rpcSeq = 1;
    Packet pkt = m_rpcManager.CreateRPCPacket(rpcSeq++, name, args);
    return m_tcpTransport->Broadcast(pkt);
}

ITransport* Session::GetActiveTransport() const {
    return m_tcpTransport ? m_tcpTransport.get() :
           m_udpTransport ? m_udpTransport.get() : nullptr;
}

void Session::PollTransports() {
    if (m_tcpTransport) m_tcpTransport->Poll();
    if (m_udpTransport) m_udpTransport->Poll();
}

void Session::ProcessIncoming() {
    if (!m_running) return;

    // Process TCP packets
    if (m_tcpTransport) {
        Packet pkt;
        ConnectionHandle from;
        while (m_tcpTransport->Receive(pkt, from)) {
            auto type = static_cast<PacketType>(pkt.header.type);
            switch (type) {
                case PacketType::Connect:
                    HandleConnectPacket(from, pkt);
                    break;
                case PacketType::Disconnect:
                    HandleDisconnectPacket(from, pkt);
                    break;
                case PacketType::ConnectAck:
                    // Client received ACK from server
                    m_localHandle = from;
                    LOG_INFO("Session", "Connection acknowledged by server, local handle={0}", from);
                    break;
                case PacketType::RPC:
                    m_rpcManager.ProcessRPCPacket(pkt);
                    break;
                case PacketType::StateSync:
                    HandleStateSyncPacket(from, pkt);
                    break;
                case PacketType::Fragment:
                    HandleFragmentPacket(from, pkt);
                    break;
                case PacketType::Ping:
                    // Reply with Pong
                    {
                        Packet pong;
                        pong.header.type = static_cast<uint8_t>(PacketType::Pong);
                        pong.header.sequence = pkt.header.sequence;
                        m_tcpTransport->Send(from, pong);
                    }
                    break;
                case PacketType::Pong:
                    break;
                default:
                    break;
            }
        }
    }

    // Process UDP packets
    if (m_udpTransport) {
        Packet pkt;
        ConnectionHandle from;
        while (m_udpTransport->Receive(pkt, from)) {
            auto type = static_cast<PacketType>(pkt.header.type);
            switch (type) {
                case PacketType::StateSync:
                    HandleStateSyncPacket(from, pkt);
                    break;
                case PacketType::Ack:
                    HandleAckPacket(from, pkt);
                    break;
                case PacketType::Ping:
                    {
                        Packet pong;
                        pong.header.type = static_cast<uint8_t>(PacketType::Pong);
                        pong.header.sequence = pkt.header.sequence;
                        m_udpTransport->Send(from, pong);
                    }
                    break;
                default:
                    break;
            }
        }
    }
}

void Session::SendStateSync() {
    if (!m_running) return;
    // State sync is driven externally via NetworkSystem
    // which calls this method at the configured tick rate.
}

void Session::HandleConnectPacket(ConnectionHandle from, const Packet& pkt) {
    if (m_mode != SessionMode::Server) return;

    auto info = m_tcpTransport ? m_tcpTransport->GetConnectionInfo(from) : ConnectionInfo{};
    m_connectedPeers.push_back(info);

    LOG_INFO("Session", "Client connected: {0}:{1} (handle={2})",
             info.address, info.port, from);

    // Send connection ACK
    SendConnectAck(from);

    // Notify handler
    if (m_onPeerConnected) {
        m_onPeerConnected(from, info.address);
    }
}

void Session::HandleDisconnectPacket(ConnectionHandle from, const Packet& pkt) {
    if (m_mode == SessionMode::Server) {
        if (m_onPeerDisconnected) {
            ConnectionInfo* info = nullptr;
            for (auto& peer : m_connectedPeers) {
                if (peer.handle == from) {
                    info = &peer;
                    break;
                }
            }
            m_onPeerDisconnected(from, info ? info->address : "unknown");
        }
        std::erase_if(m_connectedPeers, [from](const ConnectionInfo& peer) {
            return peer.handle == from;
        });
        LOG_INFO("Session", "Client {0} disconnected", from);
    } else {
        // Client received disconnect from server
        LOG_INFO("Session", "Disconnected from server");
        m_running = false;
        m_localHandle = 0;
    }
}

void Session::HandleAckPacket(ConnectionHandle from, const Packet& pkt) {
    // ACK tracking for UDP reliability - can be expanded later
}

void Session::HandleStateSyncPacket(ConnectionHandle from, const Packet& pkt) {
    // State sync processing is done by NetworkSystem
    // which handles the deserialized entity data
}

void Session::HandleFragmentPacket(ConnectionHandle from, const Packet& pkt) {
    if (pkt.payload.size() < 4) {
        LOG_WARNING("Session", "Invalid fragment packet (too small)");
        return;
    }

    // Parse fragment metadata
    uint16_t fragIndex = static_cast<uint16_t>(pkt.payload[0]) |
                         (static_cast<uint16_t>(pkt.payload[1]) << 8);
    uint16_t totalFrags = static_cast<uint16_t>(pkt.payload[2]) |
                          (static_cast<uint16_t>(pkt.payload[3]) << 8);

    uint32_t baseSeq = pkt.header.sequence - fragIndex;

    // Find or create fragment buffer
    auto it = m_fragmentBuffers.find(baseSeq);
    if (it == m_fragmentBuffers.end()) {
        FragmentBuffer fb;
        fb.baseSequence = baseSeq;
        fb.totalFragments = totalFrags;
        fb.received.resize(totalFrags, false);
        auto result = m_fragmentBuffers.emplace(baseSeq, std::move(fb));
        it = result.first;
    }

    auto& fb = it->second;
    if (fragIndex >= fb.totalFragments) {
        LOG_WARNING("Session", "Fragment index out of range");
        return;
    }

    // Store fragment data (skip 4-byte metadata)
    uint32_t dataSize = pkt.header.payloadSize > 4 ? pkt.header.payloadSize - 4 : 0;
    if (fb.received[fragIndex]) return; // Duplicate

    fb.received[fragIndex] = true;

    // Calculate position in reassembled buffer
    uint32_t fragmentDataSize = MAX_PAYLOAD_SIZE - 4;
    uint32_t offset = fragIndex * fragmentDataSize;
    if (offset + dataSize > fb.data.size()) {
        fb.data.resize(offset + dataSize);
    }
    std::memcpy(fb.data.data() + offset, pkt.payload.data() + 4, dataSize);

    // Check if all fragments received
    bool complete = true;
    for (size_t i = 0; i < fb.received.size(); ++i) {
        if (!fb.received[i]) { complete = false; break; }
    }

    if (complete) {
        // Reconstruct the original packet
        Packet reconstructed;
        reconstructed.header.sequence = baseSeq;
        reconstructed.header.type = static_cast<uint8_t>(PacketType::RPC); // Default, could be any type
        reconstructed.payload = std::move(fb.data);
        reconstructed.header.payloadSize = static_cast<uint32_t>(reconstructed.payload.size());

        // Process the reconstructed packet
        m_rpcManager.ProcessRPCPacket(reconstructed);

        // Cleanup
        m_fragmentBuffers.erase(it);
    }
}

void Session::SendConnectAck(ConnectionHandle to) {
    if (!m_tcpTransport) return;

    BinarySerializer writer;
    writer.WriteUint32(m_serverToken);

    Packet ackPkt;
    ackPkt.header.type = static_cast<uint8_t>(PacketType::ConnectAck);
    ackPkt.payload = writer.MoveData();
    ackPkt.header.payloadSize = static_cast<uint32_t>(ackPkt.payload.size());
    m_tcpTransport->Send(to, ackPkt);
}

void Session::SendPing() {
    if (!m_running) return;

    ITransport* transport = GetActiveTransport();
    if (!transport) return;

    Packet ping;
    static uint32_t pingSeq = 0;
    ping.header.sequence = ++pingSeq;
    ping.header.type = static_cast<uint8_t>(PacketType::Ping);

    if (m_mode == SessionMode::Client) {
        transport->Send(0, ping);
    } else {
        transport->Broadcast(ping);
    }
}

} // namespace Prisma::Network
