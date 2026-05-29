#include "ConsoleLayer.h"
#include "Logger.h"
#include "Engine.h"
#include "Application.h"
#include <imgui.h>
#include <algorithm>
#include <cctype>

namespace Prisma {

ConsoleLayer::ConsoleLayer(ConsoleSystem* system)
    : Layer("Console")
    , m_ConsoleSystem(system) {
}

ConsoleLayer::~ConsoleLayer() = default;

void ConsoleLayer::OnAttach() {
    if (m_ConsoleSystem) {
        m_ConsoleSystem->LogInfo("控制台已准备就绪 (按 ~ 切换)");
    }
}

void ConsoleLayer::OnDetach() {
}

void ConsoleLayer::OnEvent(Event& e) {
    if (!m_Open) return;
    if (e.GetEventType() == EventType::KeyPressed) {
        auto& keyEvent = static_cast<KeyPressedEvent&>(e);
        if (keyEvent.GetKeyCode() == static_cast<int>(Input::KeyCode::Escape)) {
            Toggle();
            e.Handled = true;
        }
    }
}

void ConsoleLayer::OnImGuiRender() {
    // 同步 ImGui 上下文（与 Editor 共享）
    if (auto* appCtx = Application::Get().GetImGuiContext()) {
        ImGui::SetCurrentContext(static_cast<ImGuiContext*>(appCtx));
    }
    if (!m_ConsoleSystem) return;

    HandleKeybinds();
    if (!m_Open) return;

    ImGuiIO& io = ImGui::GetIO();
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    // 默认位置：左下角，一半屏幕宽，300px 高
    float width = viewport->Size.x * 0.5f;
    float height = 300.0f;
    ImGui::SetNextWindowPos(ImVec2(0, viewport->Size.y - height), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_FirstUseEver);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (!ImGui::Begin("Console", &m_Open, flags)) {
        ImGui::End();
        return;
    }

    // 消息滚动区域
    float footerHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footerHeight), false,
        ImGuiWindowFlags_HorizontalScrollbar);

    const auto& messages = m_ConsoleSystem->GetMessages();
    for (const auto& msg : messages) {
        ImVec4 color;
        switch (msg.level) {
            case ConsoleMessageLevel::Info:    color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f); break;
            case ConsoleMessageLevel::Warning: color = ImVec4(1.0f, 0.9f, 0.3f, 1.0f); break;
            case ConsoleMessageLevel::Error:   color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f); break;
            case ConsoleMessageLevel::Command: color = ImVec4(0.3f, 0.8f, 1.0f, 1.0f); break;
            default:                           color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); break;
        }
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TextUnformatted(msg.text.c_str());
        ImGui::PopStyleColor();
    }

    if (m_ScrollToBottom) {
        ImGui::SetScrollHereY(1.0f);
        m_ScrollToBottom = false;
    }
    ImGui::EndChild();

    ImGui::Separator();

    // 自动补全下拉
    if (m_ShowAutocomplete && !m_AutocompleteMatches.empty()) {
        ImGui::BeginChild("Autocomplete", ImVec2(0, 80), false);
        for (int i = 0; i < static_cast<int>(m_AutocompleteMatches.size()); ++i) {
            bool isSelected = (i == m_AutocompleteIndex);
            if (ImGui::Selectable(m_AutocompleteMatches[i].c_str(), isSelected)) {
                strncpy(m_InputBuffer, m_AutocompleteMatches[i].c_str(), sizeof(m_InputBuffer) - 1);
                m_InputBuffer[sizeof(m_InputBuffer) - 1] = '\0';
                m_ShowAutocomplete = false;
                m_RequestFocus = true;
            }
        }
        ImGui::EndChild();
    }

    if (m_RequestFocus) {
        ImGui::SetKeyboardFocusHere();
        m_RequestFocus = false;
    }

    bool executeCommand = false;
    ImGui::PushItemWidth(-1.0f);
    executeCommand = ImGui::InputText("##console_input", m_InputBuffer, sizeof(m_InputBuffer),
        ImGuiInputTextFlags_EnterReturnsTrue);

    // Tab 自动补全
    if (ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Tab)) {
        UpdateAutocomplete();
        if (!m_AutocompleteMatches.empty()) {
            if (m_AutocompleteIndex >= 0 && m_AutocompleteIndex < static_cast<int>(m_AutocompleteMatches.size())) {
                strncpy(m_InputBuffer, m_AutocompleteMatches[m_AutocompleteIndex].c_str(), sizeof(m_InputBuffer) - 1);
            } else if (m_AutocompleteMatches.size() == 1) {
                strncpy(m_InputBuffer, m_AutocompleteMatches[0].c_str(), sizeof(m_InputBuffer) - 1);
            }
            m_InputBuffer[sizeof(m_InputBuffer) - 1] = '\0';
            m_ShowAutocomplete = true;
        }
    }

    // 上下键命令历史
    if (ImGui::IsItemActive()) {
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
            if (!m_CommandHistory.empty()) {
                m_CommandHistoryIndex = std::max(0, m_CommandHistoryIndex - 1);
                strncpy(m_InputBuffer, m_CommandHistory[m_CommandHistoryIndex].c_str(), sizeof(m_InputBuffer) - 1);
                m_InputBuffer[sizeof(m_InputBuffer) - 1] = '\0';
            }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
            if (!m_CommandHistory.empty()) {
                m_CommandHistoryIndex++;
                if (m_CommandHistoryIndex >= static_cast<int>(m_CommandHistory.size())) {
                    m_CommandHistoryIndex = static_cast<int>(m_CommandHistory.size());
                    m_InputBuffer[0] = '\0';
                } else {
                    strncpy(m_InputBuffer, m_CommandHistory[m_CommandHistoryIndex].c_str(), sizeof(m_InputBuffer) - 1);
                    m_InputBuffer[sizeof(m_InputBuffer) - 1] = '\0';
                }
            }
        }
    }

    ImGui::PopItemWidth();

    if (executeCommand) {
        ExecuteCommand();
    }

    ImGui::End();
}

void ConsoleLayer::HandleKeybinds() {
    ImGuiIO& io = ImGui::GetIO();

    if (ImGui::IsKeyPressed(ImGuiKey_GraveAccent)) {
        if (!m_JustToggled) {
            m_Open = !m_Open;
            m_JustToggled = true;
            m_RequestFocus = m_Open;
            if (m_Open) {
                m_CommandHistoryIndex = static_cast<int>(m_CommandHistory.size());
            }
        }
    } else {
        m_JustToggled = false;
    }

    if (m_Open) {
        io.WantCaptureKeyboard = true;
    }
}

void ConsoleLayer::ExecuteCommand() {
    std::string cmd(m_InputBuffer);
    m_InputBuffer[0] = '\0';

    if (!cmd.empty()) {
        m_CommandHistory.push_back(cmd);
        m_CommandHistoryIndex = static_cast<int>(m_CommandHistory.size());
        if (m_CommandHistory.size() > 100) {
            m_CommandHistory.erase(m_CommandHistory.begin());
        }
        m_ConsoleSystem->ExecuteCommand(cmd);
    }

    m_ShowAutocomplete = false;
    m_ScrollToBottom = true;
}

void ConsoleLayer::UpdateAutocomplete() {
    std::string input(m_InputBuffer);
    m_AutocompleteMatches.clear();
    m_AutocompleteIndex = -1;

    if (input.empty()) {
        m_ShowAutocomplete = false;
        return;
    }

    std::string lowerInput = input;
    std::transform(lowerInput.begin(), lowerInput.end(), lowerInput.begin(), ::tolower);

    m_ConsoleSystem->GetCVarRegistry().ForEach([&](CVarBase* cvar) {
        std::string name = cvar->GetName();
        std::string lowerName = name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        if (lowerName.find(lowerInput) == 0) {
            m_AutocompleteMatches.push_back(name);
        }
    });

    m_ConsoleSystem->GetCommandRegistry().ForEach([&](const std::shared_ptr<IConsoleCommand>& cmd) {
        std::string name = cmd->GetName();
        std::string lowerName = name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        if (lowerName.find(lowerInput) == 0) {
            if (std::find(m_AutocompleteMatches.begin(), m_AutocompleteMatches.end(), name)
                == m_AutocompleteMatches.end()) {
                m_AutocompleteMatches.push_back(name);
            }
        }
    });

    std::sort(m_AutocompleteMatches.begin(), m_AutocompleteMatches.end());
    m_ShowAutocomplete = !m_AutocompleteMatches.empty();
}

void ConsoleLayer::ScrollToBottom() {
    m_ScrollToBottom = true;
}

void ConsoleLayer::AddMessage(const ConsoleMessage& msg) {
    m_ScrollToBottom = true;
}

} // namespace Prisma
