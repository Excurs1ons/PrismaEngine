#pragma once

#include "Export.h"
#include "core/Event.h"
#include "Platform.h"
#include <string>
#include <functional>
#include <memory>

namespace Prisma {

/**
 * @brief 窗口抽象接口
 */
class ENGINE_API Window {
public:
    using EventCallbackFn = std::function<void(Event&)>;

    virtual ~Window() = default;

    virtual void OnUpdate() = 0;

    virtual uint32_t GetWidth() const = 0;
    virtual uint32_t GetHeight() const = 0;

    // 窗口属性
    virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
    virtual void SetVSync(bool enabled) = 0;
    virtual bool IsVSync() const = 0;

    virtual void* GetNativeWindow() const = 0;

    static std::unique_ptr<Window> Create(const WindowProps& props = WindowProps());
};

} // namespace Prisma
