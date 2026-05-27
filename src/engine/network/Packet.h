#pragma once

#include "Export.h"
#include <cstdint>
#include <vector>
#include <cstring>
#include <algorithm>
#include <deque>
#include <mutex>
#include <string>
#include <bit>

namespace Prisma::Network {

// ═══════════════════════════════════════════════════════════════════
// PacketType — all packet types for the network protocol
// ═══════════════════════════════════════════════════════════════════
enum class PacketType : uint8_t {
    Unknown     = 0,
    Connect     = 1,   // Client → Server: handshake
    Disconnect  = 2,   // Both sides: graceful disconnect
    ConnectAck  = 3,   // Server → Client: connection accepted
    RPC         = 4,   // Remote Procedure Call payload
    StateSync   = 5,   // Entity state synchronization
    Ping        = 6,   // Keep-alive / latency measurement
    Pong        = 7,   // Latency response
    Ack         = 8,   // UDP delivery confirmation
    Fragment    = 255, // Fragmented packet continuation
};

// ═══════════════════════════════════════════════════════════════════
// PacketHeader — wire format header (little-endian on wire)
// ═══════════════════════════════════════════════════════════════════
//
// Layout (12 bytes):
//   [0..3]   uint32    sequence number
//   [4]      uint8     packet type (PacketType)
//   [5..7]   uint8[3]  reserved (padding)
//   [8..11]  uint32    payload size in bytes
//   [12..]   uint8[]   payload data
//
// Maximum total packet size: 1400 bytes (under typical MTU of 1500)
// Maximum payload size:      1388 bytes
//
#pragma pack(push, 1)
struct PacketHeader {
    uint32_t sequence    = 0;
    uint8_t  type        = 0;
    uint8_t  reserved[3] = {0, 0, 0};
    uint32_t payloadSize = 0;
};
#pragma pack(pop)

static constexpr uint32_t PACKET_HEADER_SIZE = sizeof(PacketHeader); // 12
static constexpr uint32_t MAX_PACKET_SIZE    = 1400;
static constexpr uint32_t MAX_PAYLOAD_SIZE   = MAX_PACKET_SIZE - PACKET_HEADER_SIZE; // 1388

// ═══════════════════════════════════════════════════════════════════
// Packet — complete packet with header + payload
// ═══════════════════════════════════════════════════════════════════
struct ENGINE_API Packet {
    PacketHeader header;
    std::vector<uint8_t> payload;

    // Total wire size
    uint32_t TotalSize() const {
        return PACKET_HEADER_SIZE + static_cast<uint32_t>(payload.size());
    }

    // Serialize to wire bytes
    std::vector<uint8_t> Serialize() const {
        std::vector<uint8_t> data(TotalSize());
        std::memcpy(data.data(), &header, PACKET_HEADER_SIZE);
        if (!payload.empty()) {
            std::memcpy(data.data() + PACKET_HEADER_SIZE, payload.data(), payload.size());
        }
        return data;
    }

    // Deserialize from wire bytes
    static Packet Deserialize(const uint8_t* data, uint32_t size) {
        Packet pkt;
        if (size < PACKET_HEADER_SIZE) return pkt;
        std::memcpy(&pkt.header, data, PACKET_HEADER_SIZE);
        uint32_t paySize = std::min(pkt.header.payloadSize, size - PACKET_HEADER_SIZE);
        pkt.payload.assign(data + PACKET_HEADER_SIZE, data + PACKET_HEADER_SIZE + paySize);
        return pkt;
    }

    // Check if payload would exceed MTU
    static bool WouldFragment(uint32_t payloadSize) {
        return (PACKET_HEADER_SIZE + payloadSize) > MAX_PACKET_SIZE;
    }

    // Fragment a large payload into multiple packets
    static std::vector<Packet> Fragment(uint32_t baseSequence, PacketType type,
                                        const uint8_t* payload, uint32_t payloadSize) {
        std::vector<Packet> fragments;
        uint32_t offset = 0;
        uint32_t fragIndex = 0;
        while (offset < payloadSize) {
            uint32_t chunkSize = std::min(MAX_PAYLOAD_SIZE - 4, payloadSize - offset); // -4 for frag header
            Packet frag;
            frag.header.sequence = baseSequence + fragIndex;
            frag.header.type = static_cast<uint8_t>(PacketType::Fragment);
            frag.header.payloadSize = chunkSize + 4; // 4 bytes fragment metadata

            // Fragment metadata: [0..1] uint16 fragIndex, [2..3] uint16 totalFragments
            frag.payload.resize(chunkSize + 4);
            frag.payload[0] = static_cast<uint8_t>(fragIndex & 0xFF);
            frag.payload[1] = static_cast<uint8_t>((fragIndex >> 8) & 0xFF);
            uint16_t totalFrags = static_cast<uint16_t>((payloadSize + MAX_PAYLOAD_SIZE - 4 - 1) / (MAX_PAYLOAD_SIZE - 4));
            frag.payload[2] = static_cast<uint8_t>(totalFrags & 0xFF);
            frag.payload[3] = static_cast<uint8_t>((totalFrags >> 8) & 0xFF);

            std::memcpy(frag.payload.data() + 4, payload + offset, chunkSize);
            fragments.push_back(std::move(frag));
            offset += chunkSize;
            ++fragIndex;
        }
        return fragments;
    }
};

// ═══════════════════════════════════════════════════════════════════
// PacketQueue — thread-safe queue for incoming/outgoing packets
// ═══════════════════════════════════════════════════════════════════
class ENGINE_API PacketQueue {
public:
    void Push(Packet pkt) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push_back(std::move(pkt));
    }

    bool Pop(Packet& out) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty()) return false;
        out = std::move(m_queue.front());
        m_queue.pop_front();
        return true;
    }

    bool IsEmpty() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    size_t Size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

    void Clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.clear();
    }

private:
    std::deque<Packet> m_queue;
    mutable std::mutex m_mutex;
};

// ═══════════════════════════════════════════════════════════════════
// BinarySerializer — manual binary serialization for RPC args
// ═══════════════════════════════════════════════════════════════════
class ENGINE_API BinarySerializer {
public:
    BinarySerializer() = default;
    explicit BinarySerializer(std::vector<uint8_t> data) : m_data(std::move(data)), m_readPos(0) {}

    // Write methods
    void WriteInt8(int8_t v)   { WriteRaw(&v, sizeof(v)); }
    void WriteUint8(uint8_t v) { WriteRaw(&v, sizeof(v)); }
    void WriteInt16(int16_t v) { v = ToNetwork(v); WriteRaw(&v, sizeof(v)); }
    void WriteUint16(uint16_t v) { v = ToNetwork(v); WriteRaw(&v, sizeof(v)); }
    void WriteInt32(int32_t v) { v = ToNetwork(v); WriteRaw(&v, sizeof(v)); }
    void WriteUint32(uint32_t v) { v = ToNetwork(v); WriteRaw(&v, sizeof(v)); }
    void WriteUint64(uint64_t v) { v = ToNetwork(v); WriteRaw(&v, sizeof(v)); }
    void WriteFloat(float v)   { WriteRaw(&v, sizeof(v)); }
    void WriteDouble(double v) { WriteRaw(&v, sizeof(v)); }
    void WriteString(const std::string& v) {
        uint32_t len = static_cast<uint32_t>(v.size());
        WriteUint32(len);
        WriteRaw(v.data(), len);
    }
    void WriteBytes(const uint8_t* data, uint32_t size) {
        WriteUint32(size);
        WriteRaw(data, size);
    }

    // Read methods
    int8_t     ReadInt8()    { int8_t v; ReadRaw(&v, sizeof(v)); return v; }
    uint8_t    ReadUint8()   { uint8_t v; ReadRaw(&v, sizeof(v)); return v; }
    int16_t    ReadInt16()   { int16_t v; ReadRaw(&v, sizeof(v)); return FromNetwork(v); }
    uint16_t   ReadUint16()  { uint16_t v; ReadRaw(&v, sizeof(v)); return FromNetwork(v); }
    int32_t    ReadInt32()   { int32_t v; ReadRaw(&v, sizeof(v)); return FromNetwork(v); }
    uint32_t   ReadUint32()  { uint32_t v; ReadRaw(&v, sizeof(v)); return FromNetwork(v); }
    uint64_t   ReadUint64()  { uint64_t v; ReadRaw(&v, sizeof(v)); return FromNetwork(v); }
    float      ReadFloat()   { float v; ReadRaw(&v, sizeof(v)); return v; }
    double     ReadDouble()  { double v; ReadRaw(&v, sizeof(v)); return v; }
    std::string ReadString() {
        uint32_t len = ReadUint32();
        if (len > m_data.size() - m_readPos) len = static_cast<uint32_t>(m_data.size() - m_readPos);
        std::string s(reinterpret_cast<const char*>(m_data.data() + m_readPos), len);
        m_readPos += len;
        return s;
    }
    std::vector<uint8_t> ReadBytes() {
        uint32_t len = ReadUint32();
        if (len > m_data.size() - m_readPos) len = static_cast<uint32_t>(m_data.size() - m_readPos);
        std::vector<uint8_t> b(m_data.begin() + m_readPos, m_data.begin() + m_readPos + len);
        m_readPos += len;
        return b;
    }

    // Accessors
    const std::vector<uint8_t>& GetData() const { return m_data; }
    std::vector<uint8_t>&& MoveData() { return std::move(m_data); }
    size_t GetSize() const { return m_data.size(); }
    size_t GetReadPos() const { return m_readPos; }
    bool HasMore() const { return m_readPos < m_data.size(); }
    void Clear() { m_data.clear(); m_readPos = 0; }

private:
    void WriteRaw(const void* data, size_t size) {
        const uint8_t* bytes = static_cast<const uint8_t*>(data);
        m_data.insert(m_data.end(), bytes, bytes + size);
    }

    void ReadRaw(void* data, size_t size) {
        if (m_readPos + size > m_data.size()) {
            std::memset(data, 0, size);
            m_readPos = m_data.size();
            return;
        }
        std::memcpy(data, m_data.data() + m_readPos, size);
        m_readPos += size;
    }

    // Host to network byte order (big endian)
    template<typename T>
    static T ToNetwork(T v) {
        if constexpr (std::endian::native == std::endian::little) {
            T result;
            uint8_t* src = reinterpret_cast<uint8_t*>(&v);
            uint8_t* dst = reinterpret_cast<uint8_t*>(&result);
            for (size_t i = 0; i < sizeof(T); ++i)
                dst[i] = src[sizeof(T) - 1 - i];
            return result;
        } else {
            return v;
        }
    }

    template<typename T>
    static T FromNetwork(T v) {
        return ToNetwork(v); // symmetric
    }

    std::vector<uint8_t> m_data;
    size_t m_readPos = 0;
};

} // namespace Prisma::Network
