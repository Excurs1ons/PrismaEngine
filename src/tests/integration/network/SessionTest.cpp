#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <random>
#include <csignal>
#include "network/Session.h"

namespace Prisma::Network {
namespace {

using ::testing::_;
using ::testing::Return;
using ::testing::DoAll;
using ::testing::SetArgReferee;

// ============================================================================
// MockTransport — Pure mock of ITransport for Session testing
// ============================================================================
class MockTransport : public ITransport {
public:
    MOCK_METHOD(bool, Initialize, (const TransportConfig& config), (override));
    MOCK_METHOD(bool, Listen, (), (override));
    MOCK_METHOD(bool, Connect, (), (override));
    MOCK_METHOD(void, Disconnect, (), (override));
    MOCK_METHOD(void, Shutdown, (), (override));
    MOCK_METHOD(bool, Send, (ConnectionHandle to, const Packet& packet), (override));
    MOCK_METHOD(bool, Broadcast, (const Packet& packet), (override));
    MOCK_METHOD(bool, Receive, (Packet& outPacket, ConnectionHandle& outFrom), (override));
    MOCK_METHOD(bool, IsReady, (), (const, override));
    MOCK_METHOD(bool, IsServer, (), (const, override));
    MOCK_METHOD(bool, IsConnected, (), (const, override));
    MOCK_METHOD(ConnectionInfo, GetConnectionInfo, (ConnectionHandle handle), (const, override));
    MOCK_METHOD(const TransportConfig&, GetConfig, (), (const, override));
    MOCK_METHOD(void, Poll, (), (override));
};

// ============================================================================
// SessionTest — uses MockTransport
// ============================================================================
class SessionTest : public ::testing::Test {
protected:
    Session m_session;
    uint16_t m_testPort = 0;

    static void SetUpTestSuite() {
        // Ignore SIGPIPE — client connection attempts to non-listening ports
        // may trigger SIGPIPE on some platforms.
        std::signal(SIGPIPE, SIG_IGN);
    }

    void SetUp() override {
        m_testPort = static_cast<uint16_t>(23456 + (std::random_device{}() % 10000));
    }

    void TearDown() override {
        if (m_session.IsRunning()) {
            m_session.StopSession();
        }
    }
};

// ============================================================================
// Session — Default State
// ============================================================================
TEST_F(SessionTest, InitiallyNotRunning) {
    EXPECT_FALSE(m_session.IsRunning());
    EXPECT_EQ(m_session.GetMode(), SessionMode::None);
    EXPECT_EQ(m_session.GetLocalHandle(), 0u);
    EXPECT_EQ(m_session.GetPeerCount(), 0u);
}

// ============================================================================
// Session — Server Lifecycle
// ============================================================================
TEST_F(SessionTest, StartServerSession) {
    SessionConfig cfg;
    cfg.mode = SessionMode::Server;
    cfg.host = "127.0.0.1";
    cfg.port = m_testPort;

    bool started = m_session.StartSession(cfg);
    EXPECT_TRUE(started);
    EXPECT_TRUE(m_session.IsRunning());
    EXPECT_EQ(m_session.GetMode(), SessionMode::Server);
}

TEST_F(SessionTest, StopServerSession) {
    SessionConfig cfg;
    cfg.mode = SessionMode::Server;
    cfg.host = "127.0.0.1";
    cfg.port = m_testPort;

    ASSERT_TRUE(m_session.StartSession(cfg));
    EXPECT_TRUE(m_session.IsRunning());

    m_session.StopSession();
    EXPECT_FALSE(m_session.IsRunning());
    EXPECT_EQ(m_session.GetMode(), SessionMode::None);
}

// ============================================================================
// Session — Double Start Fails
// ============================================================================
TEST_F(SessionTest, DoubleStartSessionReturnsFalse) {
    SessionConfig cfg;
    cfg.mode = SessionMode::Server;
    cfg.host = "127.0.0.1";
    cfg.port = m_testPort;

    ASSERT_TRUE(m_session.StartSession(cfg));
    EXPECT_TRUE(m_session.IsRunning());
    EXPECT_FALSE(m_session.StartSession(cfg));
}

// ============================================================================
// Session — Stop On Non-Running Is No-Op
// ============================================================================
TEST_F(SessionTest, StopNonRunningSessionNoCrash) {
    EXPECT_FALSE(m_session.IsRunning());
    EXPECT_NO_THROW(m_session.StopSession());
}

// ============================================================================
// Session — Client Session
// ============================================================================
TEST_F(SessionTest, StartClientSession) {
    SessionConfig cfg;
    cfg.mode = SessionMode::Client;
    cfg.host = "127.0.0.1";
    cfg.port = m_testPort;

    // With non-blocking TCP, connect() returns immediately and
    // the OS handles the handshake asynchronously.
    bool started = m_session.StartSession(cfg);
    EXPECT_TRUE(started);
    EXPECT_TRUE(m_session.IsRunning());
    EXPECT_EQ(m_session.GetMode(), SessionMode::Client);

    // Stop cleanly — the transport will detect connection failure on Poll
    m_session.StopSession();
    EXPECT_FALSE(m_session.IsRunning());
}

// ============================================================================
// Session — Event Handlers
// ============================================================================
TEST_F(SessionTest, SetEventHandlers) {
    int connectCalls = 0;
    int disconnectCalls = 0;

    m_session.SetOnPeerConnected([&](ConnectionHandle, const std::string&) {
        ++connectCalls;
    });
    m_session.SetOnPeerDisconnected([&](ConnectionHandle, const std::string&) {
        ++disconnectCalls;
    });

    SessionConfig cfg;
    cfg.mode = SessionMode::Server;
    cfg.host = "127.0.0.1";
    cfg.port = m_testPort;
    ASSERT_TRUE(m_session.StartSession(cfg));

    EXPECT_EQ(connectCalls, 0);
    EXPECT_EQ(disconnectCalls, 0);

    m_session.StopSession();
}

// ============================================================================
// Session — Server With UDP
// ============================================================================
TEST_F(SessionTest, StartServerWithUDP) {
    SessionConfig cfg;
    cfg.mode = SessionMode::Server;
    cfg.host = "127.0.0.1";
    cfg.port = m_testPort;
    cfg.useTCP = true;
    cfg.useUDP = true;

    bool started = m_session.StartSession(cfg);
    EXPECT_TRUE(started);
    EXPECT_TRUE(m_session.IsRunning());

    EXPECT_NE(m_session.GetTCPTransport(), nullptr);
    EXPECT_NE(m_session.GetUDPTransport(), nullptr);

    m_session.StopSession();
}

// ============================================================================
// Session — Server Without UDP
// ============================================================================
TEST_F(SessionTest, StartServerTCPOnly) {
    SessionConfig cfg;
    cfg.mode = SessionMode::Server;
    cfg.host = "127.0.0.1";
    cfg.port = m_testPort;
    cfg.useTCP = true;
    cfg.useUDP = false;

    bool started = m_session.StartSession(cfg);
    EXPECT_TRUE(started);
    EXPECT_NE(m_session.GetTCPTransport(), nullptr);
    EXPECT_EQ(m_session.GetUDPTransport(), nullptr);

    m_session.StopSession();
}

// ============================================================================
// Session — Poll And Process Incoming On Idle Server
// ============================================================================
TEST_F(SessionTest, PollAndProcessOnIdleServer) {
    SessionConfig cfg;
    cfg.mode = SessionMode::Server;
    cfg.host = "127.0.0.1";
    cfg.port = m_testPort;
    ASSERT_TRUE(m_session.StartSession(cfg));

    EXPECT_NO_THROW(m_session.PollTransports());
    EXPECT_NO_THROW(m_session.ProcessIncoming());

    m_session.StopSession();
}

// ============================================================================
// Session — RPC Manager Access
// ============================================================================
TEST_F(SessionTest, RPCAccess) {
    EXPECT_NO_THROW(m_session.GetRPCManager());
    EXPECT_NO_THROW(const_cast<const Session&>(m_session).GetRPCManager());
}

// ============================================================================
// Session — Transport Config
// ============================================================================
TEST_F(SessionTest, ConfigAccess) {
    SessionConfig cfg;
    cfg.mode = SessionMode::Server;
    cfg.host = "127.0.0.1";
    cfg.port = m_testPort;
    cfg.useTCP = true;
    cfg.useUDP = false;

    ASSERT_TRUE(m_session.StartSession(cfg));

    const auto& retrieved = m_session.GetConfig();
    EXPECT_EQ(retrieved.mode, SessionMode::Server);
    EXPECT_EQ(retrieved.host, "127.0.0.1");
    EXPECT_EQ(retrieved.port, m_testPort);
    EXPECT_TRUE(retrieved.useTCP);
    EXPECT_FALSE(retrieved.useUDP);

    m_session.StopSession();
}

// ============================================================================
// Session — Multiple Start/Stop Cycles
// ============================================================================
TEST_F(SessionTest, MultipleStartStopCycles) {
    for (int i = 0; i < 3; ++i) {
        uint16_t port = static_cast<uint16_t>(m_testPort + i);

        SessionConfig cfg;
        cfg.mode = SessionMode::Server;
        cfg.host = "127.0.0.1";
        cfg.port = port;

        EXPECT_TRUE(m_session.StartSession(cfg));
        EXPECT_TRUE(m_session.IsRunning());
        m_session.StopSession();
        EXPECT_FALSE(m_session.IsRunning());
    }
}

} // namespace
} // namespace Prisma::Network
