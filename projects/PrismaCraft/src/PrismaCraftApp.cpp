#include "PrismaCraftApp.h"
#include "app/Engine.h"

namespace Prisma {

PrismaCraftApp::PrismaCraftApp()
    : Application({"PrismaCraft", "", 1280, 720, true, true, Graphic::PresentMode::Mailbox, 0})
{
}

int PrismaCraftApp::OnInitialize() {
    LOG_INFO("PrismaCraft", "Initializing...");
    LOG_INFO("PrismaCraft", "Game logic runs in C#, rendering via SRP pipeline");
    return 0;
}

void PrismaCraftApp::OnUpdate(Timestep ts) {
    (void)ts;
}

void PrismaCraftApp::OnRender() {
    // 由 C# SRP 管线控制渲染，传递 delta time
#if PRISMA_ENABLE_SCRIPTING
    auto& se = Engine::Get().GetScriptEngine();
    se.Render(0.016f);
#endif
}

void PrismaCraftApp::OnEvent(Event& e) {
    (void)e;
}

} // namespace Prisma
