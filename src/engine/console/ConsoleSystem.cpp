#include "ConsoleSystem.h"
#include "ConsoleUI.h"
#include "Engine.h"
#include "Logger.h"
#include <sstream>

namespace Prisma {

// 匿名命名空间中的命令实现类
namespace {

class HelpCommand : public IConsoleCommand {
public:
    HelpCommand() : IConsoleCommand("help", "显示所有可用命令及其描述") {}
    void Execute(const std::vector<std::string>& /*args*/) override {
        auto* console = Engine::Get().GetSystem<ConsoleSystem>();
        if (!console) return;
        console->LogInfo("--- 可用命令 ---");
        console->GetCommandRegistry().ForEach([](const std::shared_ptr<IConsoleCommand>& cmd) {
            Engine::Get().GetSystem<ConsoleSystem>()->LogInfo(
                "  " + cmd->GetName() + " - " + cmd->GetDescription());
        });
    }
};

class CVarListCommand : public IConsoleCommand {
public:
    CVarListCommand() : IConsoleCommand("cvarlist", "显示所有控制台变量及其当前值") {}
    void Execute(const std::vector<std::string>& /*args*/) override {
        auto* console = Engine::Get().GetSystem<ConsoleSystem>();
        if (!console) return;
        console->LogInfo("--- CVars (共 " + std::to_string(console->GetCVarRegistry().GetCount()) + " 个) ---");
        console->GetCVarRegistry().ForEach([](CVarBase* cvar) {
            std::string flags;
            if (cvar->HasFlag(CVarFlags::Cheat)) flags += " [作弊]";
            if (cvar->HasFlag(CVarFlags::ReadOnly)) flags += " [只读]";
            if (cvar->HasFlag(CVarFlags::RequireRestart)) flags += " [需重启]";
            if (cvar->HasFlag(CVarFlags::Archive)) flags += " [存档]";
            Engine::Get().GetSystem<ConsoleSystem>()->LogInfo(
                "  " + cvar->GetName() + " = " + cvar->GetString()
                + " (" + cvar->GetTypeName() + ")" + flags
                + " - " + cvar->GetDescription());
        });
    }
};

class ExecCommand : public IConsoleCommand {
public:
    ExecCommand() : IConsoleCommand("exec", "执行一条控制台命令") {}
    void Execute(const std::vector<std::string>& args) override {
        auto* console = Engine::Get().GetSystem<ConsoleSystem>();
        if (!console) return;
        if (args.empty()) {
            console->LogWarning("用法: exec <命令> [参数...]");
            return;
        }
        std::string cmdLine;
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) cmdLine += " ";
            cmdLine += args[i];
        }
        console->ExecuteCommand(cmdLine);
    }
};

class EchoCommand : public IConsoleCommand {
public:
    EchoCommand() : IConsoleCommand("echo", "将参数回显到控制台") {}
    void Execute(const std::vector<std::string>& args) override {
        auto* console = Engine::Get().GetSystem<ConsoleSystem>();
        if (!console) return;
        std::string message;
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) message += " ";
            message += args[i];
        }
        console->Log(message, ConsoleMessageLevel::Command);
    }
};

class ToggleCommand : public IConsoleCommand {
public:
    ToggleCommand() : IConsoleCommand("toggle", "切换布尔型 CVar 的值。用法: toggle <cvar_name>") {}
    void Execute(const std::vector<std::string>& args) override {
        auto* console = Engine::Get().GetSystem<ConsoleSystem>();
        if (!console) return;
        if (args.empty()) {
            console->LogWarning("用法: toggle <cvar_name>");
            return;
        }
        auto* cvar = console->GetCVarRegistry().Find(args[0]);
        if (!cvar) {
            console->LogWarning("未知 CVar: " + args[0]);
            return;
        }
        auto* boolCVar = dynamic_cast<CVar<bool>*>(cvar);
        if (!boolCVar) {
            console->LogWarning("'" + args[0] + "' 不是布尔型 CVar");
            return;
        }
        boolCVar->Set(!boolCVar->Get());
        console->LogInfo(args[0] + " = " + (boolCVar->Get() ? "true" : "false"));
    }
};

} // anonymous namespace

ConsoleSystem::ConsoleSystem() = default;
ConsoleSystem::~ConsoleSystem() = default;

int ConsoleSystem::Initialize() {
    LOG_INFO("Console", "初始化控制台系统...");

    RegisterDefaultCVars();
    RegisterDefaultCommands();

    m_ConsoleUI = std::make_unique<ConsoleUI>(this);

    LogInfo("控制台系统已初始化。输入 help 查看命令列表。");
    return 0;
}

void ConsoleSystem::Shutdown() {
    m_ConsoleUI.reset();
    LOG_INFO("Console", "控制台系统已关闭。");
}

void ConsoleSystem::Update([[maybe_unused]] Timestep ts) {
}

void ConsoleSystem::RegisterDefaultCVars() {
    m_CVarRegistry.Register(std::make_unique<CVar<bool>>("sv_cheats", false, "允许使用作弊指令", CVarFlags::Cheat));
    m_CVarRegistry.Register(std::make_unique<CVar<int>>("fps_max", 60, 0, 1000, "最大帧率限制 (0=不限)", CVarFlags::None));
    m_CVarRegistry.Register(std::make_unique<CVar<bool>>("r_vsync", true, "垂直同步", CVarFlags::None));
    m_CVarRegistry.Register(std::make_unique<CVar<int>>("r_resolution_x", 1920, 640, 7680, "水平分辨率", CVarFlags::RequireRestart));
    m_CVarRegistry.Register(std::make_unique<CVar<int>>("r_resolution_y", 1080, 480, 4320, "垂直分辨率", CVarFlags::RequireRestart));
    m_CVarRegistry.Register(std::make_unique<CVar<float>>("r_render_scale", 1.0f, 0.25f, 2.0f, "渲染分辨率缩放", CVarFlags::None));
    m_CVarRegistry.Register(std::make_unique<CVar<float>>("s_master_volume", 1.0f, 0.0f, 1.0f, "主音量", CVarFlags::None));
    m_CVarRegistry.Register(std::make_unique<CVar<float>>("s_music_volume", 0.8f, 0.0f, 1.0f, "音乐音量", CVarFlags::None));
    m_CVarRegistry.Register(std::make_unique<CVar<float>>("s_sfx_volume", 1.0f, 0.0f, 1.0f, "音效音量", CVarFlags::None));
    m_CVarRegistry.Register(std::make_unique<CVar<float>>("r_mouse_sensitivity", 0.5f, 0.01f, 2.0f, "鼠标灵敏度", CVarFlags::None));
    m_CVarRegistry.Register(std::make_unique<CVar<bool>>("r_invert_mouse", false, "反转鼠标 Y 轴", CVarFlags::None));
}

void ConsoleSystem::RegisterDefaultCommands() {
    m_CommandRegistry.Register(std::make_shared<HelpCommand>());
    m_CommandRegistry.Register(std::make_shared<CVarListCommand>());
    m_CommandRegistry.Register(std::make_shared<ExecCommand>());
    m_CommandRegistry.Register(std::make_shared<EchoCommand>());
    m_CommandRegistry.Register(std::make_shared<ToggleCommand>());
}

void ConsoleSystem::Log(const std::string& message, ConsoleMessageLevel level) {
    std::lock_guard<std::mutex> lock(m_MessageMutex);
    m_Messages.emplace_back(level, message);
    if (m_Messages.size() > m_MaxMessages) {
        m_Messages.erase(m_Messages.begin(),
            m_Messages.begin() + (m_Messages.size() - m_MaxMessages));
    }
}

void ConsoleSystem::ExecuteCommand(const std::string& commandLine) {
    if (commandLine.empty()) return;
    Log("] " + commandLine, ConsoleMessageLevel::Command);

    std::istringstream stream(commandLine);
    std::string cmdName;
    stream >> cmdName;

    std::vector<std::string> args;
    std::string arg;
    while (stream >> arg) {
        args.push_back(arg);
    }

    if (m_CommandRegistry.Execute(cmdName, args)) {
        return;
    }

    auto* cvar = m_CVarRegistry.Find(cmdName);
    if (cvar) {
        if (args.empty()) {
            LogInfo(cmdName + " = " + cvar->GetString() + " (" + cvar->GetTypeName() + ")");
            return;
        }
        if (cvar->HasFlag(CVarFlags::ReadOnly)) {
            LogWarning(cmdName + " 是只读 CVar");
            return;
        }
        bool isCheat = cvar->HasFlag(CVarFlags::Cheat);
        if (isCheat) {
            auto* svCheats = m_CVarRegistry.Find("sv_cheats");
            if (!svCheats || svCheats->GetString() != "true") {
                LogWarning("需要启用 sv_cheats 才能修改 " + cmdName);
                return;
            }
        }
        cvar->SetFromString(args[0]);
        LogInfo(cmdName + " = " + cvar->GetString());
        return;
    }

    LogWarning("未知命令: " + cmdName + " (输入 help 查看可用命令)");
}

} // namespace Prisma
