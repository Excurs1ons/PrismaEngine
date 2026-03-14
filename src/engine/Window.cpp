#include "Window.h"

#ifdef PRISMA_PLATFORM_WINDOWS
    // 以后可以实现原生的 WindowsWindow
    // #include "platform/Windows/WindowsWindow.h"
#endif

// 目前默认使用 SDL 窗口
#include "PlatformSDLWindow.h"

namespace Prisma {

std::unique_ptr<Window> Window::Create(const WindowProps& props) {
    return std::make_unique<PlatformSDLWindow>(props);
}

} // namespace Prisma
