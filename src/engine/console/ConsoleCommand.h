#pragma once

#include "../Export.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Prisma {

class ENGINE_API IConsoleCommand {
public:
    IConsoleCommand(std::string name, std::string description)
        : m_Name(std::move(name))
        , m_Description(std::move(description)) {}
    virtual ~IConsoleCommand() = default;

    virtual void Execute(const std::vector<std::string>& args) = 0;

    const std::string& GetName() const { return m_Name; }
    const std::string& GetDescription() const { return m_Description; }

private:
    std::string m_Name;
    std::string m_Description;
};

class ENGINE_API CommandRegistry {
public:
    CommandRegistry() = default;
    ~CommandRegistry() = default;

    void Register(std::shared_ptr<IConsoleCommand> cmd) {
        if (cmd) m_Commands[cmd->GetName()] = std::move(cmd);
    }

    std::shared_ptr<IConsoleCommand> Find(const std::string& name) {
        auto it = m_Commands.find(name);
        return it != m_Commands.end() ? it->second : nullptr;
    }

    bool Execute(const std::string& name, const std::vector<std::string>& args) {
        auto cmd = Find(name);
        if (!cmd) return false;
        cmd->Execute(args);
        return true;
    }

    void ForEach(std::function<void(const std::shared_ptr<IConsoleCommand>&)> func) {
        for (auto& [name, cmd] : m_Commands) {
            func(cmd);
        }
    }

    std::vector<std::shared_ptr<IConsoleCommand>> GetAll() const {
        std::vector<std::shared_ptr<IConsoleCommand>> result;
        result.reserve(m_Commands.size());
        for (auto& [name, cmd] : m_Commands) {
            result.push_back(cmd);
        }
        return result;
    }

    size_t GetCount() const { return m_Commands.size(); }

private:
    std::unordered_map<std::string, std::shared_ptr<IConsoleCommand>> m_Commands;
};

} // namespace Prisma
