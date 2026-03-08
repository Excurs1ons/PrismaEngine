#pragma once
#include "Export.h"
#include "IApplication.h"
#include "CommandLineParser.h"
#include "ManagerBase.h"
#include <string>
#include <unordered_map>
#include <functional>

namespace PrismaEngine {

class EDITOR_API CommandLineEditor : public IApplication<CommandLineEditor>, public ManagerBase<CommandLineEditor> {
public:
    CommandLineEditor();
    ~CommandLineEditor() override;

    static std::shared_ptr<CommandLineEditor> GetInstance();

    int Initialize() override;
    int Run() override;
    void Shutdown() override;

    void SetArguments(const CommandLineParser::Arguments& args);

    using CommandHandler = std::function<int(const std::vector<std::string>&)>;
    void RegisterCommand(const std::string& name, const std::string& description, CommandHandler handler);

private:
    CommandLineParser::Arguments m_args;

    struct Impl;
    std::unique_ptr<Impl> m_impl;

    int CommandBuild(const std::vector<std::string>& args);
    int CommandClean(const std::vector<std::string>& args);
    int CommandExport(const std::vector<std::string>& args);
    int CommandImport(const std::vector<std::string>& args);
    int CommandPackage(const std::vector<std::string>& args);
    int CommandShowInfo(const std::vector<std::string>& args);
    int CommandValidate(const std::vector<std::string>& args);
    int CommandRun(const std::vector<std::string>& args);

    void RegisterBuiltinCommands();
    void ShowHelp();
    int ExecuteCommand();
};

} // namespace PrismaEngine
