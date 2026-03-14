#include "Application.h"
#include "Platform.h"
#include "Logger.h"
#include "input/InputManager.h"

namespace Prisma {

Application* Application::s_Instance = nullptr;

Application::Application(const ApplicationSpecification& spec)
    : m_Spec(spec) {
    s_Instance = this;
}

Application::~Application() {
    if (s_Instance == this) {
        s_Instance = nullptr;
    }
}

void Application::InitWindow() {
    // 显式创建窗口
    m_Window = Window::Create(m_Spec.WindowProperties);
    m_Window->SetEventCallback([this](Event& e) {
        this->OnEvent(e);
    });
}

void Application::OnEvent(Event& e) {
    EventDispatcher dispatcher(e);
    
    // 1. 基础窗口事件处理
    dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& event) {
        this->Close();
        return true;
    });

    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& event) {
        if (event.GetWidth() == 0 || event.GetHeight() == 0) {
            m_Minimized = true;
        } else {
            m_Minimized = false;
        }
        return false;
    });

    // 2. 将事件分发给 InputManager 更新状态 (Polling 支持)
    if (auto* inputManager = Input::InputManager::Get().get()) {
        inputManager->OnEvent(e);
    }

    // 3. 分发到各层 (逆序)
    for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it) {
        if (e.Handled) break;
        (*it)->OnEvent(e);
    }
}

void Application::OnUpdate(Timestep ts) {
    for (Layer* layer : m_LayerStack) {
        layer->OnUpdate(ts);
    }
}

void Application::OnRender() {
}

void Application::OnImGuiRender() {
    for (Layer* layer : m_LayerStack) {
        layer->OnImGuiRender();
    }
}

void Application::PushLayer(Layer* layer) {
    m_LayerStack.PushLayer(layer);
}

void Application::PushOverlay(Layer* overlay) {
    m_LayerStack.PushOverlay(overlay);
}

} // namespace Prisma
