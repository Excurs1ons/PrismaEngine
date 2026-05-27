#pragma once

#include "Export.h"
#include "network/Packet.h"

#include <functional>
#include <unordered_map>
#include <string>
#include <vector>
#include <mutex>

namespace Prisma::Network {

// RPC callback: receives deserialized arguments as a BinarySerializer
using RPCCallback = std::function<void(BinarySerializer&)>;

// ═══════════════════════════════════════════════════════════════════
// RPCManager — register and invoke remote procedure calls
// ═══════════════════════════════════════════════════════════════════
class ENGINE_API RPCManager {
public:
    RPCManager() = default;
    ~RPCManager() = default;

    RPCManager(const RPCManager&) = delete;
    RPCManager& operator=(const RPCManager&) = delete;

    /// Register an RPC handler. name must be unique.
    /// If an RPC with the same name already exists, it will be overwritten.
    void RegisterRPC(const std::string& name, RPCCallback callback) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_rpcs[name] = std::move(callback);
    }

    /// Unregister an RPC handler.
    void UnregisterRPC(const std::string& name) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_rpcs.erase(name);
    }

    /// Check if an RPC is registered.
    bool HasRPC(const std::string& name) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_rpcs.find(name) != m_rpcs.end();
    }

    /// Call a registered RPC with the given BinarySerializer args.
    /// Returns true if the RPC was found and executed.
    bool CallRPC(const std::string& name, BinarySerializer& args) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_rpcs.find(name);
        if (it != m_rpcs.end()) {
            it->second(args);
            return true;
        }
        return false;
    }

    /// Create a packet containing an RPC call (serialized name + args).
    /// The returned packet has type PacketType::RPC.
    Packet CreateRPCPacket(uint32_t sequence, const std::string& name,
                           const std::vector<uint8_t>& args) {
        BinarySerializer writer;
        writer.WriteString(name);
        writer.WriteBytes(args.data(), static_cast<uint32_t>(args.size()));

        Packet pkt;
        pkt.header.sequence = sequence;
        pkt.header.type = static_cast<uint8_t>(PacketType::RPC);
        pkt.payload = writer.MoveData();
        pkt.header.payloadSize = static_cast<uint32_t>(pkt.payload.size());
        return pkt;
    }

    /// Process an incoming RPC packet. Extracts the RPC name and arguments,
    /// then dispatches to the registered handler.
    /// Returns true if the RPC was recognized and executed.
    bool ProcessRPCPacket(const Packet& pkt) {
        if (pkt.header.type != static_cast<uint8_t>(PacketType::RPC)) {
            return false;
        }
        BinarySerializer reader(pkt.payload);
        std::string name = reader.ReadString();
        // Remaining data is the original args
        return CallRPC(name, reader);
    }

    /// Remove all registered RPCs.
    void Clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_rpcs.clear();
    }

    /// Get the number of registered RPCs.
    size_t GetRPCCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_rpcs.size();
    }

private:
    std::unordered_map<std::string, RPCCallback> m_rpcs;
    mutable std::mutex m_mutex;
};

} // namespace Prisma::Network
