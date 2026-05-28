#include <gtest/gtest.h>
#include "console/ConsoleSystem.h"

namespace Prisma {
namespace {

class ConsoleSystemTest : public ::testing::Test {
protected:
    ConsoleSystem m_Console;
};

TEST_F(ConsoleSystemTest, GetName) {
    EXPECT_STREQ(m_Console.GetName(), "ConsoleSystem");
}

TEST_F(ConsoleSystemTest, LogAndRetrieveMessages) {
    EXPECT_EQ(m_Console.GetMessageCount(), 0u);

    m_Console.LogInfo("test info message");
    EXPECT_EQ(m_Console.GetMessageCount(), 1u);

    m_Console.LogWarning("test warning message");
    EXPECT_EQ(m_Console.GetMessageCount(), 2u);

    m_Console.LogError("test error message");
    EXPECT_EQ(m_Console.GetMessageCount(), 3u);
}

TEST_F(ConsoleSystemTest, LogWithCustomLevel) {
    m_Console.Log("custom level message", ConsoleMessageLevel::Command);
    ASSERT_EQ(m_Console.GetMessageCount(), 1u);

    auto& msgs = m_Console.GetMessages();
    EXPECT_EQ(msgs.back().level, ConsoleMessageLevel::Command);
    EXPECT_EQ(msgs.back().text, "custom level message");
}

TEST_F(ConsoleSystemTest, SetMaxMessages) {
    m_Console.SetMaxMessages(2);

    m_Console.LogInfo("msg1");
    m_Console.LogInfo("msg2");
    EXPECT_EQ(m_Console.GetMessageCount(), 2u);

    m_Console.LogInfo("msg3");
    EXPECT_EQ(m_Console.GetMessageCount(), 2u);
}

TEST_F(ConsoleSystemTest, MaxMessagesDefault) {
    m_Console.SetMaxMessages(5);
    for (int i = 0; i < 10; ++i) {
        m_Console.LogInfo("msg" + std::to_string(i));
    }
    EXPECT_EQ(m_Console.GetMessageCount(), 5u);
}

TEST_F(ConsoleSystemTest, CVarRegistryAccess) {
    auto& registry = m_Console.GetCVarRegistry();
    EXPECT_EQ(registry.GetCount(), 0u);

    registry.Register(std::make_unique<CVar<int>>("custom_cvar", 42));
    EXPECT_EQ(registry.GetCount(), 1u);

    auto* cvar = registry.Find("custom_cvar");
    ASSERT_NE(cvar, nullptr);
    EXPECT_EQ(cvar->GetString(), "42");
}

TEST_F(ConsoleSystemTest, CommandRegistryAccess) {
    auto& registry = m_Console.GetCommandRegistry();
    EXPECT_EQ(registry.GetCount(), 0u);
}

TEST_F(ConsoleSystemTest, ExecuteCommandEmptyLine) {
    m_Console.ExecuteCommand("");
    EXPECT_EQ(m_Console.GetMessageCount(), 0u);
}

TEST_F(ConsoleSystemTest, ExecuteCommandUnknown) {
    m_Console.ExecuteCommand("nonexistent_cmd");
    ASSERT_GT(m_Console.GetMessageCount(), 0u);

    auto& msgs = m_Console.GetMessages();
    auto& last = msgs.back();
    EXPECT_EQ(last.level, ConsoleMessageLevel::Warning);
    EXPECT_NE(last.text.find("未知命令"), std::string::npos);
}

TEST_F(ConsoleSystemTest, ExecuteCommandEcho) {
    auto& registry = m_Console.GetCommandRegistry();

    class EchoCmd : public IConsoleCommand {
    public:
        EchoCmd(ConsoleSystem* console) : IConsoleCommand("echo_test", ""), m_Console(console) {}
        void Execute(const std::vector<std::string>& args) override {
            std::string msg;
            for (size_t i = 0; i < args.size(); ++i) {
                if (i > 0) msg += " ";
                msg += args[i];
            }
            m_Console->Log(msg, ConsoleMessageLevel::Command);
        }
    private:
        ConsoleSystem* m_Console;
    };

    registry.Register(std::make_shared<EchoCmd>(&m_Console));
    m_Console.ExecuteCommand("echo_test hello world");

    bool foundHello = false;
    for (auto& msg : m_Console.GetMessages()) {
        if (msg.text.find("hello world") != std::string::npos) {
            foundHello = true;
            break;
        }
    }
    EXPECT_TRUE(foundHello);
}

TEST_F(ConsoleSystemTest, ExecuteCommandSetsCVar) {
    auto& registry = m_Console.GetCVarRegistry();
    registry.Register(std::make_unique<CVar<int>>("test_var", 0));
    m_Console.ExecuteCommand("test_var 42");

    auto* cvar = registry.Find("test_var");
    ASSERT_NE(cvar, nullptr);
    EXPECT_EQ(cvar->GetString(), "42");
}

TEST_F(ConsoleSystemTest, ExecuteCommandReadsCVar) {
    auto& registry = m_Console.GetCVarRegistry();
    registry.Register(std::make_unique<CVar<int>>("display_var", 77));
    m_Console.ExecuteCommand("display_var");

    auto* cvar = registry.Find("display_var");
    ASSERT_NE(cvar, nullptr);
    EXPECT_EQ(cvar->GetString(), "77");
}

TEST_F(ConsoleSystemTest, ExecuteCommandLogsCommandLine) {
    m_Console.ExecuteCommand("some_command");
    ASSERT_GT(m_Console.GetMessageCount(), 0u);

    auto& first = m_Console.GetMessages()[0];
    EXPECT_EQ(first.level, ConsoleMessageLevel::Command);
    EXPECT_NE(first.text.find("] some_command"), std::string::npos);
}

}
} // namespace Prisma
