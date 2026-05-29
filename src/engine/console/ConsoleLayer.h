#pragma once

#include "../Export.h"
#include "../core/Layer.h"
#include "ConsoleSystem.h"
#include <string>
#include <vector>

namespace Prisma {

class ENGINE_API ConsoleLayer : public Layer {
public:
    explicit ConsoleLayer(ConsoleSystem* system);
    ~ConsoleLayer() override;

    void OnAttach() override;
    void OnDetach() override;
    void OnImGuiRender() override;
    void OnEvent(Event& e) override;

    void Toggle() { m_Open = !m_Open; }
    bool IsOpen() const { return m_Open; }

    void ScrollToBottom();
    void AddMessage(const ConsoleMessage& msg);

private:
    void ExecuteCommand();
    void UpdateAutocomplete();
    void HandleKeybinds();

    ConsoleSystem* m_ConsoleSystem;
    bool m_Open = false;
    bool m_JustToggled = false;  // 防止 ~ 键透传

    char m_InputBuffer[256] = {};

    std::vector<std::string> m_CommandHistory;
    int m_CommandHistoryIndex = -1;

    std::vector<std::string> m_AutocompleteMatches;
    int m_AutocompleteIndex = -1;
    bool m_ShowAutocomplete = false;

    bool m_ScrollToBottom = false;
    bool m_RequestFocus = false;
};

} // namespace Prisma
