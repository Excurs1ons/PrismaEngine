#include <gtest/gtest.h>
#include "network/Packet.h"

namespace Prisma::Network {
namespace {

// ============================================================================
// PacketHeader — Size & Layout
// ============================================================================
TEST(PacketHeaderTest, HeaderSizeIs12Bytes) {
    EXPECT_EQ(sizeof(PacketHeader), 12u);
}

TEST(PacketHeaderTest, DefaultValues) {
    PacketHeader h;
    EXPECT_EQ(h.sequence, 0u);
    EXPECT_EQ(h.type, 0u);
    EXPECT_EQ(h.payloadSize, 0u);
}

TEST(PacketHeaderTest, ConstantsAreSane) {
    EXPECT_EQ(PACKET_HEADER_SIZE, 12u);
    EXPECT_EQ(MAX_PACKET_SIZE, 1400u);
    EXPECT_EQ(MAX_PAYLOAD_SIZE, 1388u);
    EXPECT_EQ(PACKET_HEADER_SIZE + MAX_PAYLOAD_SIZE, MAX_PACKET_SIZE);
}

// ============================================================================
// Packet — Serialization Roundtrip
// ============================================================================
TEST(PacketTest, SerializeDeserializeRoundtrip) {
    Packet send;
    send.header.sequence = 42;
    send.header.type = static_cast<uint8_t>(PacketType::RPC);
    send.payload = {0xDE, 0xAD, 0xBE, 0xEF};
    send.header.payloadSize = static_cast<uint32_t>(send.payload.size());

    auto wire = send.Serialize();
    ASSERT_EQ(wire.size(), PACKET_HEADER_SIZE + send.payload.size());

    Packet received = Packet::Deserialize(wire.data(), static_cast<uint32_t>(wire.size()));
    EXPECT_EQ(received.header.sequence, send.header.sequence);
    EXPECT_EQ(received.header.type, send.header.type);
    EXPECT_EQ(received.header.payloadSize, send.header.payloadSize);
    ASSERT_EQ(received.payload.size(), send.payload.size());
    EXPECT_EQ(received.payload, send.payload);
}

TEST(PacketTest, SerializeEmptyPacket) {
    Packet pkt;
    pkt.header.sequence = 0;
    pkt.header.type = static_cast<uint8_t>(PacketType::Ping);

    auto wire = pkt.Serialize();
    EXPECT_EQ(wire.size(), PACKET_HEADER_SIZE);

    Packet decoded = Packet::Deserialize(wire.data(), static_cast<uint32_t>(wire.size()));
    EXPECT_EQ(decoded.header.type, static_cast<uint8_t>(PacketType::Ping));
    EXPECT_TRUE(decoded.payload.empty());
}

TEST(PacketTest, DeserializeTruncatedData) {
    // Less than header size should produce a default packet
    std::vector<uint8_t> trash = {0x01, 0x02, 0x03};
    Packet pkt = Packet::Deserialize(trash.data(), static_cast<uint32_t>(trash.size()));
    EXPECT_EQ(pkt.header.sequence, 0u);
    EXPECT_EQ(pkt.header.payloadSize, 0u);
    EXPECT_TRUE(pkt.payload.empty());
}

TEST(PacketTest, WouldFragmentOnLargePayload) {
    // Payload just under limit
    EXPECT_FALSE(Packet::WouldFragment(MAX_PAYLOAD_SIZE));
    // Payload exactly at limit
    EXPECT_FALSE(Packet::WouldFragment(MAX_PAYLOAD_SIZE));
    // Payload exceeding limit
    EXPECT_TRUE(Packet::WouldFragment(MAX_PAYLOAD_SIZE + 1));
}

TEST(PacketTest, TotalSizeCalculation) {
    Packet pkt;
    pkt.payload = {1, 2, 3, 4, 5};
    EXPECT_EQ(pkt.TotalSize(), PACKET_HEADER_SIZE + 5u);
}

// ============================================================================
// Packet — Fragmentation & Reassembly
// ============================================================================
TEST(PacketTest, FragmentSmallPayloadDoesNotFragment) {
    std::vector<uint8_t> smallPayload = {0x01, 0x02, 0x03};
    auto fragments = Packet::Fragment(0, PacketType::RPC,
                                       smallPayload.data(),
                                       static_cast<uint32_t>(smallPayload.size()));
    // Small payload should still produce at least one fragment
    ASSERT_GE(fragments.size(), 1u);
    // Each fragment should have type == Fragment
    EXPECT_EQ(static_cast<PacketType>(fragments[0].header.type), PacketType::Fragment);
}

TEST(PacketTest, FragmentLargePayload) {
    // Create a payload large enough to require multiple fragments
    std::vector<uint8_t> largePayload(MAX_PAYLOAD_SIZE * 2 + 100, 0xAB);
    for (size_t i = 0; i < largePayload.size(); ++i) {
        largePayload[i] = static_cast<uint8_t>(i & 0xFF);
    }

    auto fragments = Packet::Fragment(100, PacketType::RPC,
                                       largePayload.data(),
                                       static_cast<uint32_t>(largePayload.size()));
    ASSERT_GE(fragments.size(), 2u);

    // Check fragment metadata
    for (size_t i = 0; i < fragments.size(); ++i) {
        const auto& frag = fragments[i];
        EXPECT_EQ(static_cast<PacketType>(frag.header.type), PacketType::Fragment);

        // First 4 bytes are fragment metadata
        ASSERT_GE(frag.payload.size(), 4u);
        uint16_t fragIndex = static_cast<uint16_t>(frag.payload[0]) |
                             (static_cast<uint16_t>(frag.payload[1]) << 8);
        uint16_t totalFrags = static_cast<uint16_t>(frag.payload[2]) |
                              (static_cast<uint16_t>(frag.payload[3]) << 8);
        EXPECT_EQ(fragIndex, i);
        EXPECT_EQ(totalFrags, fragments.size());
    }

    // Reassemble and verify
    std::vector<uint8_t> reassembled;
    for (const auto& frag : fragments) {
        uint32_t dataSize = frag.header.payloadSize > 4 ? frag.header.payloadSize - 4 : 0;
        if (!frag.payload.empty()) {
            reassembled.insert(reassembled.end(),
                               frag.payload.begin() + 4,
                               frag.payload.begin() + 4 + dataSize);
        }
    }

    ASSERT_EQ(reassembled.size(), largePayload.size());
    for (size_t i = 0; i < largePayload.size(); ++i) {
        EXPECT_EQ(reassembled[i], largePayload[i]) << "Mismatch at byte " << i;
    }
}

// ============================================================================
// BinarySerializer — Write & Read
// ============================================================================
class BinarySerializerTest : public ::testing::Test {
protected:
    BinarySerializer writer;
};

TEST_F(BinarySerializerTest, WriteReadInt32) {
    writer.WriteInt32(-12345);
    auto data = writer.GetData();

    BinarySerializer reader(std::move(data));
    EXPECT_EQ(reader.ReadInt32(), -12345);
    EXPECT_FALSE(reader.HasMore());
}

TEST_F(BinarySerializerTest, WriteReadMultipleTypes) {
    writer.WriteInt8(-8);
    writer.WriteUint8(200);
    writer.WriteFloat(3.14159f);
    writer.WriteString("Hello");

    BinarySerializer reader(writer.GetData());
    EXPECT_EQ(reader.ReadInt8(), -8);
    EXPECT_EQ(reader.ReadUint8(), 200);
    EXPECT_FLOAT_EQ(reader.ReadFloat(), 3.14159f);
    EXPECT_EQ(reader.ReadString(), "Hello");
    EXPECT_FALSE(reader.HasMore());
}

TEST_F(BinarySerializerTest, WriteReadUint64) {
    uint64_t big = 0xDEADBEEFCAFE1234ULL;
    writer.WriteUint64(big);

    BinarySerializer reader(writer.GetData());
    EXPECT_EQ(reader.ReadUint64(), big);
}

TEST_F(BinarySerializerTest, WriteReadBytes) {
    std::vector<uint8_t> bytes = {10, 20, 30, 40, 50};
    writer.WriteBytes(bytes.data(), static_cast<uint32_t>(bytes.size()));

    BinarySerializer reader(writer.GetData());
    auto read = reader.ReadBytes();
    ASSERT_EQ(read.size(), bytes.size());
    for (size_t i = 0; i < bytes.size(); ++i) {
        EXPECT_EQ(read[i], bytes[i]);
    }
}

TEST_F(BinarySerializerTest, Clear) {
    writer.WriteInt32(42);
    EXPECT_GT(writer.GetSize(), 0u);
    writer.Clear();
    EXPECT_EQ(writer.GetSize(), 0u);
    EXPECT_EQ(writer.GetReadPos(), 0u);
}

TEST_F(BinarySerializerTest, ReadFromEmptyReturnsZero) {
    BinarySerializer reader(std::vector<uint8_t>{});
    EXPECT_EQ(reader.ReadInt32(), 0);
    EXPECT_FALSE(reader.HasMore());
}

// ============================================================================
// PacketQueue — Thread-Safe Queue
// ============================================================================
TEST(PacketQueueTest, InitiallyEmpty) {
    PacketQueue q;
    EXPECT_TRUE(q.IsEmpty());
    EXPECT_EQ(q.Size(), 0u);
}

TEST(PacketQueueTest, PushAndPop) {
    PacketQueue q;
    Packet pkt;
    pkt.header.sequence = 1;

    q.Push(std::move(pkt));
    EXPECT_FALSE(q.IsEmpty());
    EXPECT_EQ(q.Size(), 1u);

    Packet out;
    EXPECT_TRUE(q.Pop(out));
    EXPECT_EQ(out.header.sequence, 1u);
    EXPECT_TRUE(q.IsEmpty());
}

TEST(PacketQueueTest, PopFromEmptyReturnsFalse) {
    PacketQueue q;
    Packet out;
    EXPECT_FALSE(q.Pop(out));
}

TEST(PacketQueueTest, MultiplePushPopOrder) {
    PacketQueue q;
    for (uint32_t i = 0; i < 10; ++i) {
        Packet pkt;
        pkt.header.sequence = i;
        q.Push(std::move(pkt));
    }
    EXPECT_EQ(q.Size(), 10u);

    for (uint32_t i = 0; i < 10; ++i) {
        Packet out;
        ASSERT_TRUE(q.Pop(out));
        EXPECT_EQ(out.header.sequence, i);
    }
    EXPECT_TRUE(q.IsEmpty());
}

TEST(PacketQueueTest, ClearEmptiesQueue) {
    PacketQueue q;
    for (uint32_t i = 0; i < 5; ++i) {
        Packet pkt;
        pkt.header.sequence = i;
        q.Push(std::move(pkt));
    }
    EXPECT_EQ(q.Size(), 5u);
    q.Clear();
    EXPECT_TRUE(q.IsEmpty());
}

// ============================================================================
// PacketType — Enum Values
// ============================================================================
TEST(PacketTypeTest, KnownTypes) {
    EXPECT_EQ(static_cast<int>(PacketType::Unknown), 0);
    EXPECT_EQ(static_cast<int>(PacketType::Connect), 1);
    EXPECT_EQ(static_cast<int>(PacketType::Disconnect), 2);
    EXPECT_EQ(static_cast<int>(PacketType::ConnectAck), 3);
    EXPECT_EQ(static_cast<int>(PacketType::RPC), 4);
    EXPECT_EQ(static_cast<int>(PacketType::StateSync), 5);
    EXPECT_EQ(static_cast<int>(PacketType::Ping), 6);
    EXPECT_EQ(static_cast<int>(PacketType::Pong), 7);
    EXPECT_EQ(static_cast<int>(PacketType::Ack), 8);
    EXPECT_EQ(static_cast<int>(PacketType::Fragment), 255);
}

} // namespace
} // namespace Prisma::Network
