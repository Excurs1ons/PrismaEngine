#include <gtest/gtest.h>
#include "console/ConsoleCommand.h"

namespace Prisma {
namespace {

class TestCommand : public IConsoleCommand {
public:
    TestCommand() : IConsoleCommand("test", "A test command") {}
    std::vector<std::string> capturedArgs;

    void Execute(const std::vector<std::string>& args) override {
        capturedArgs = args;
    }
};

TEST(ConsoleCommandTest, CommandBasics) {
    TestCommand cmd;
    EXPECT_EQ(cmd.GetName(), "test");
    EXPECT_EQ(cmd.GetDescription(), "A test command");
}

TEST(ConsoleCommandTest, CommandExecution) {
    auto cmd = std::make_shared<TestCommand>();
    cmd->Execute({"hello", "world"});
    ASSERT_EQ(cmd->capturedArgs.size(), 2u);
    EXPECT_EQ(cmd->capturedArgs[0], "hello");
    EXPECT_EQ(cmd->capturedArgs[1], "world");
}

TEST(ConsoleCommandTest, CommandExecutionEmptyArgs) {
    auto cmd = std::make_shared<TestCommand>();
    cmd->Execute({});
    EXPECT_TRUE(cmd->capturedArgs.empty());
}

TEST(ConsoleCommandTest, RegistryRegisterAndFind) {
    CommandRegistry registry;
    auto cmd = std::make_shared<TestCommand>();
    registry.Register(cmd);

    auto found = registry.Find("test");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->GetName(), "test");

    EXPECT_EQ(registry.Find("nonexistent"), nullptr);
}

TEST(ConsoleCommandTest, RegistryExecute) {
    CommandRegistry registry;
    auto cmd = std::make_shared<TestCommand>();
    registry.Register(cmd);

    bool executed = registry.Execute("test", {"arg1"});
    EXPECT_TRUE(executed);
    ASSERT_EQ(cmd->capturedArgs.size(), 1u);
    EXPECT_EQ(cmd->capturedArgs[0], "arg1");
}

TEST(ConsoleCommandTest, RegistryExecuteUnknown) {
    CommandRegistry registry;
    EXPECT_FALSE(registry.Execute("unknown_cmd", {}));
}

TEST(ConsoleCommandTest, RegistryForEach) {
    CommandRegistry registry;
    registry.Register(std::make_shared<TestCommand>());

    int count = 0;
    registry.ForEach([&count](const std::shared_ptr<IConsoleCommand>&) { count++; });
    EXPECT_EQ(count, 1);
}

TEST(ConsoleCommandTest, RegistryGetAll) {
    CommandRegistry registry;
    registry.Register(std::make_shared<TestCommand>());

    auto all = registry.GetAll();
    ASSERT_EQ(all.size(), 1u);
    EXPECT_EQ(all[0]->GetName(), "test");
}

TEST(ConsoleCommandTest, RegistryGetCount) {
    CommandRegistry registry;
    EXPECT_EQ(registry.GetCount(), 0u);

    registry.Register(std::make_shared<TestCommand>());
    EXPECT_EQ(registry.GetCount(), 1u);
}

TEST(ConsoleCommandTest, RegistryMultipleCommands) {
    class CmdA : public IConsoleCommand {
    public: CmdA() : IConsoleCommand("cmd_a", "") {} void Execute(const std::vector<std::string>&) override {}
    };
    class CmdB : public IConsoleCommand {
    public: CmdB() : IConsoleCommand("cmd_b", "") {} void Execute(const std::vector<std::string>&) override {}
    };

    CommandRegistry registry;
    registry.Register(std::make_shared<CmdA>());
    registry.Register(std::make_shared<CmdB>());

    EXPECT_EQ(registry.GetCount(), 2u);
    EXPECT_NE(registry.Find("cmd_a"), nullptr);
    EXPECT_NE(registry.Find("cmd_b"), nullptr);
}

TEST(ConsoleCommandTest, RegistryRegisterNull) {
    CommandRegistry registry;
    registry.Register(nullptr);
    EXPECT_EQ(registry.GetCount(), 0u);
}

}
} // namespace Prisma
