#include "Application.h"
#include "Platform.h"
#include "Logger.h"
#include "Engine.h"
#include "input/InputManager.h"

namespace Prisma {

Application* Application::s_Instance = nullptr;

Application::Application(const ApplicationSpecification& spec)
    : m_Spec(spec) {
    s_Instance = this;
    m_Running = true;
}

Application::~Application() {
    s_Instance = nullptr;
}

bool Application::Initialize() {
    InitWindow();
    return true;
}

void Application::InitWindow() {
    WindowProps props;
    props.Title = m_Spec.Name;
    props.Width = m_Spec.Width;
    props.Height = m_Spec.Height;
    
    m_Window = Window::Create(props);
    if (!m_Window) {
        LOG_FATAL("Application", "Failed to create window!");
        return;
    }

    m_Window->SetEventCallback([this](Event& e) { OnEvent(e); });
}

void Application::Run() {
    while (m_Running) {
        float time = (float)Platform::GetProcessId(); // Placeholder
        Timestep ts = time - m_LastFrameTime;
        m_LastFrameTime = time;

        if (!m_Minimized) {
            OnUpdate(ts);
            OnRender();
        }

        m_Window->OnUpdate();
    }
}

void Application::Close() {
    m_Running = false;
}

void Application::OnEvent(Event& e) {
    if (e.GetEventType() == EventType::WindowClose) {
        Close();
        e.Handled = true;
    }

    if (auto* inputManager = Engine::Get().GetInputManager()) {
        inputManager->OnEvent(e);
    }

    for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it) {
        if (e.Handled) break;
        (*it)->OnEvent(e);
    }
}

void Application::OnUpdate(Timestep ts) {
    for (Layer* layer : m_LayerStack)
        layer->OnUpdate(ts);
}

void Application::OnRender() {
    for (Layer* layer : m_LayerStack)
        layer->OnImGuiRender();
}

void Application::OnImGuiRender() {
}

void Application::PushLayer(Layer* layer) {
    m_LayerStack.PushLayer(layer);
}

void Application::PushOverlay(Layer* overlay) {
    m_LayerStack.PushOverlay(overlay);
}

} // namespace Prisma
