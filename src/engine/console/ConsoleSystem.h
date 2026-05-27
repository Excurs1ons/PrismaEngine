#pragma once

#include "../Export.h"
#include "../core/ISubSystem.h"
#include "CVar.h"
#include "ConsoleCommand.h"
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace Prisma {

class ConsoleUI;

enum class ConsoleMessageLevel : uint8_t {
    Info,
    Warning,
    Error,
    Command
};

struct ConsoleMessage {
    ConsoleMessageLevel level;
    std::string text;
    std::chrono::system_clock::time_point timestamp;

    ConsoleMessage(ConsoleMessageLevel lvl, std::string msg)
        : level(lvl)
        , text(std::move(msg))
        , timestamp(std::chrono::system_clock::now()) {}
};

class ENGINE_API ConsoleSystem : public ISubSystem {
public:
    ConsoleSystem();
    ~ConsoleSystem() override;

    int Initialize() override;
    void Shutdown() override;
    void Update(Timestep ts) override;
    const char* GetName() const override { return "ConsoleSystem"; }

    CVarRegistry& GetCVarRegistry() { return m_CVarRegistry; }
    CommandRegistry& GetCommandRegistry() { return m_CommandRegistry; }

    ConsoleUI* GetConsoleUI() { return m_ConsoleUI.get(); }

    void Log(const std::string& message, ConsoleMessageLevel level = ConsoleMessageLevel::Info);
    void LogInfo(const std::string& msg) { Log(msg, ConsoleMessageLevel::Info); }
    void LogWarning(const std::string& msg) { Log(msg, ConsoleMessageLevel::Warning); }
    void LogError(const std::string& msg) { Log(msg, ConsoleMessageLevel::Error); }

    const std::vector<ConsoleMessage>& GetMessages() const { return m_Messages; }
    size_t GetMessageCount() const { return m_Messages.size(); }
    void SetMaxMessages(size_t max) { m_MaxMessages = max; }

    void ExecuteCommand(const std::string& commandLine);

private:
    void RegisterDefaultCVars();
    void RegisterDefaultCommands();

    CVarRegistry m_CVarRegistry;
    CommandRegistry m_CommandRegistry;
    std::unique_ptr<ConsoleUI> m_ConsoleUI;

    std::vector<ConsoleMessage> m_Messages;
    size_t m_MaxMessages = 1000;
    mutable std::mutex m_MessageMutex;
};

} // namespace Prisma
