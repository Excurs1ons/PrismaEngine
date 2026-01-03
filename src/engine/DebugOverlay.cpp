#include "DebugOverlay.h"

#if PRISMA_ENABLE_IMGUI_DEBUG && defined(_DEBUG)

#include "Logger.h"
#include "imgui.h"
#include <algorithm>
#include <cctype>

namespace PrismaEngine {

DebugOverlay& DebugOverlay::GetInstance() {
    static DebugOverlay instance;
    return instance;
}

void DebugOverlay::AddMessage(const std::string& text, DebugMessageType type, float duration) {
    GetInstance().m_messages.emplace_back(text, type, duration);
}

void DebugOverlay::Log(const std::string& text) {
    AddMessage(text, DebugMessageType::Info);
}

void DebugOverlay::Warning(const std::string& text) {
    AddMessage(text, DebugMessageType::Warning);
}

void DebugOverlay::Error(const std::string& text) {
    AddMessage(text, DebugMessageType::Error);
}

void DebugOverlay::Success(const std::string& text) {
    AddMessage(text, DebugMessageType::Success);
}

void DebugOverlay::WatchVar(const std::string& name, const float* value) {
    auto& inst = GetInstance();
    // 移除同名变量
    inst.UnwatchVar(name);
    WatchedVar var;
    var.name = name;
    var.type = WatchedVar::Float;
    var.f = value;
    inst.m_watchedVars.push_back(var);
}

void DebugOverlay::WatchVar(const std::string& name, const int* value) {
    auto& inst = GetInstance();
    inst.UnwatchVar(name);
    WatchedVar var;
    var.name = name;
    var.type = WatchedVar::Int;
    var.i = value;
    inst.m_watchedVars.push_back(var);
}

void DebugOverlay::WatchVar(const std::string& name, const bool* value) {
    auto& inst = GetInstance();
    inst.UnwatchVar(name);
    WatchedVar var;
    var.name = name;
    var.type = WatchedVar::Bool;
    var.b = value;
    inst.m_watchedVars.push_back(var);
}

void DebugOverlay::WatchVar(const std::string& name, const std::string* value) {
    auto& inst = GetInstance();
    inst.UnwatchVar(name);
    WatchedVar var;
    var.name = name;
    var.type = WatchedVar::String;
    var.s = value;
    inst.m_watchedVars.push_back(var);
}

void DebugOverlay::UnwatchVar(const std::string& name) {
    auto& inst = GetInstance();
    auto it = std::remove_if(inst.m_watchedVars.begin(), inst.m_watchedVars.end(),
        [&name](const WatchedVar& v) { return v.name == name; });
    inst.m_watchedVars.erase(it, inst.m_watchedVars.end());
}

void DebugOverlay::AddStat(const std::string& name, const std::function<std::string()>& getter) {
    auto& inst = GetInstance();
    // 移除同名统计
    inst.RemoveStat(name);
    StatEntry entry;
    entry.name = name;
    entry.dynamicGetter = getter;
    inst.m_stats.push_back(entry);
}

void DebugOverlay::SetStat(const std::string& name, const std::string& value) {
    auto& inst = GetInstance();
    // 查找现有统计
    auto it = std::find_if(inst.m_stats.begin(), inst.m_stats.end(),
        [&name](const StatEntry& e) { return e.name == name; });
    if (it != inst.m_stats.end()) {
        it->value = value;
        it->dynamicGetter = nullptr;
    } else {
        StatEntry entry;
        entry.name = name;
        entry.value = value;
        inst.m_stats.push_back(entry);
    }
}

void DebugOverlay::RemoveStat(const std::string& name) {
    auto& inst = GetInstance();
    auto it = std::remove_if(inst.m_stats.begin(), inst.m_stats.end(),
        [&name](const StatEntry& e) { return e.name == name; });
    inst.m_stats.erase(it, inst.m_stats.end());
}

const char* DebugOverlay::GetTypeColor(DebugMessageType type) const {
    switch (type) {
        case DebugMessageType::Info:    return "[1m";      // 白色
        case DebugMessageType::Warning: return "[33m";     // 黄色
        case DebugMessageType::Error:   return "[31m";     // 红色
        case DebugMessageType::Success: return "[32m";     // 绿色
        default: return "[0m";
    }
}

void DebugOverlay::Update(float deltaTime) {
    // 更新消息时间
    for (auto& msg : m_messages) {
        msg.timeLeft -= deltaTime;
    }
    // 移除过期消息
    m_messages.erase(
        std::remove_if(m_messages.begin(), m_messages.end(),
            [](const DebugMessage& m) { return m.timeLeft <= 0.0f; }),
        m_messages.end()
    );

    // 限制消息数量
    if (m_messages.size() > static_cast<size_t>(m_maxMessages)) {
        m_messages.erase(m_messages.begin(), m_messages.end() - m_maxMessages);
    }
}

void DebugOverlay::Render() {
    if (!m_visible || !m_initialized) {
        return;
    }

    // 设置 ImGui 窗口在左上角
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(350, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(250, 100), ImVec2(500, 1000));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.7f));

    if (ImGui::Begin("Debug Overlay", &m_visible,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_AlwaysAutoResize))
    {
        // ========== 统计面板 ==========
        if (m_showStats && !m_stats.empty()) {
            ImGui::Separator();
            ImGui::Text("Statistics:");
            for (const auto& stat : m_stats) {
                if (stat.dynamicGetter) {
                    ImGui::Text("%s: %s", stat.name.c_str(), stat.dynamicGetter().c_str());
                } else {
                    ImGui::Text("%s: %s", stat.name.c_str(), stat.value.c_str());
                }
            }
            ImGui::Separator();
        }

        // ========== 变量监视 ==========
        if (m_showWatchVars && !m_watchedVars.empty()) {
            ImGui::Text("Watched Variables:");
            for (const auto& var : m_watchedVars) {
                switch (var.type) {
                    case WatchedVar::Float:
                        ImGui::Text("%s: %.4f", var.name.c_str(), *var.f);
                        break;
                    case WatchedVar::Int:
                        ImGui::Text("%s: %d", var.name.c_str(), *var.i);
                        break;
                    case WatchedVar::Bool:
                        ImGui::Text("%s: %s", var.name.c_str(), *var.b ? "true" : "false");
                        break;
                    case WatchedVar::String:
                        ImGui::Text("%s: %s", var.name.c_str(), var.s->c_str());
                        break;
                }
            }
            ImGui::Separator();
        }

        // ========== 消息输出 ==========
        if (m_showMessages && !m_messages.empty()) {
            ImGui::Text("Messages:");
            for (const auto& msg : m_messages) {
                ImVec4 color;
                switch (msg.type) {
                    case DebugMessageType::Info:    color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f); break;
                    case DebugMessageType::Warning: color = ImVec4(1.0f, 0.8f, 0.0f, 1.0f); break;
                    case DebugMessageType::Error:   color = ImVec4(1.0f, 0.2f, 0.2f, 1.0f); break;
                    case DebugMessageType::Success: color = ImVec4(0.2f, 1.0f, 0.2f, 1.0f); break;
                }
                ImGui::TextColored(color, "%s", msg.text.c_str());
            }
        }

        // 右键菜单
        if (ImGui::BeginPopupContextWindow()) {
            ImGui::Checkbox("Show Messages", &m_showMessages);
            ImGui::Checkbox("Show Stats", &m_showStats);
            ImGui::Checkbox("Show Watched Vars", &m_showWatchVars);
            ImGui::Separator();
            if (ImGui::MenuItem("Clear Messages")) {
                m_messages.clear();
            }
            if (ImGui::MenuItem("Clear Stats")) {
                m_stats.clear();
            }
            if (ImGui::MenuItem("Clear Watched Vars")) {
                m_watchedVars.clear();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Hide")) {
                m_visible = false;
            }
            ImGui::EndPopup();
        }
    }
    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

void DebugOverlay::Initialize() {
    if (m_initialized) {
        return;
    }

    // ImGui 上下文应该已经在外部创建
    // 这里只需设置一些 ImGui 样式
    ImGui::StyleColorsDark();

    m_initialized = true;
    Log("DebugOverlay initialized");
}

void DebugOverlay::Shutdown() {
    m_messages.clear();
    m_watchedVars.clear();
    m_stats.clear();
    m_initialized = false;
}

DebugOverlay::DebugOverlay() = default;
DebugOverlay::~DebugOverlay() = default;

} // namespace PrismaEngine

#endif // PRISMA_ENABLE_IMGUI_DEBUG && PRISMA_DEBUG
