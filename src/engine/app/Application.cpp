#include "Application.h"
#include "Platform.h"
#include "Logger.h"
#include "Engine.h"
#include "input/InputManager.h"

namespace Prisma {

Application* Application::s_Instance = nullptr;

Application& Application::Get() {
    return *s_Instance;
}

Application::Application(const ApplicationSpecification& spec)
    : m_Spec(spec) {
    s_Instance = this;
    m_Running = true;
}

Application::~Application() {
    s_Instance = nullptr;
}

void Application::OnShutdown() {
}

void Application::OnEvent(Event& e) {
    // 1. Dispatch to Engine-wide systems first (Input, etc.)
    if (auto* inputManager = Engine::Get().GetInputManager()) {
        inputManager->OnEvent(e);
    }

    // 2. Dispatch to Layers (from top to bottom)
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
        layer->OnRender();
}

void Application::OnImGuiRender() {
    for (Layer* layer : m_LayerStack)
        layer->OnImGuiRender();
}

void Application::PushLayer(Layer* layer) {
    m_LayerStack.PushLayer(layer);
    layer->OnAttach();
}

void Application::PushOverlay(Layer* overlay) {
    m_LayerStack.PushOverlay(overlay);
    overlay->OnAttach();
}

const ApplicationSpecification& Application::GetSpecification() const {
    return m_Spec;
}

ApplicationSpecification& Application::GetSpecification() {
    return m_Spec;
}

} // namespace Prisma
