#pragma once

// DebugOverlay - 调试覆盖层
// 使用 ImGui 在屏幕左上角显示调试信息
// 仅在 Debug 构建中可用

#include "Build.h"

#if PRISMA_ENABLE_IMGUI_DEBUG && defined(_DEBUG)
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace PrismaEngine {

// 调试消息类型
enum class DebugMessageType {
    Info,       // 白色
    Warning,    // 黄色
    Error,      // 红色
    Success,    // 绿色
};

// 调试消息结构
struct DebugMessage {
    std::string text;
    DebugMessageType type;
    float timeLeft;    // 剩余显示时间（秒）

    DebugMessage(const std::string& t, DebugMessageType ty, float duration = 5.0f)
        : text(t), type(ty), timeLeft(duration) {}
};

/**
 * DebugOverlay - 屏幕调试覆盖层
 *
 * 使用示例：
 *   DebugOverlay::AddMessage("Player spawned");
 *   DebugOverlay::AddMessage("Low health!", DebugMessageType::Warning);
 *   DebugOverlay::AddVar("FPS", &fpsCounter);
 *   DebugOverlay::AddVar("Position", &position.x);
 */
class DebugOverlay {
public:
    // 单例访问
    static DebugOverlay& GetInstance();

    // ========== 消息输出 ==========

    // 添加消息（默认 5 秒后消失）
    static void AddMessage(const std::string& text, DebugMessageType type = DebugMessageType::Info, float duration = 5.0f);

    // 添加 Info 消息（快捷方式）
    static void Log(const std::string& text);

    // 添加 Warning 消息（快捷方式）
    static void Warning(const std::string& text);

    // 添加 Error 消息（快捷方式）
    static void Error(const std::string& text);

    // 添加 Success 消息（快捷方式）
    static void Success(const std::string& text);

    // ========== 变量监视 ==========

    // 添加变量监视（显示变量名和值）
    static void WatchVar(const std::string& name, const float* value);
    static void WatchVar(const std::string& name, const int* value);
    static void WatchVar(const std::string& name, const bool* value);
    static void WatchVar(const std::string& name, const std::string* value);

    // 移除变量监视
    static void UnwatchVar(const std::string& name);

    // ========== 统计面板 ==========

    // 添加统计值（用于显示 FPS、内存等）
    static void AddStat(const std::string& name, const std::function<std::string()>& getter);
    static void SetStat(const std::string& name, const std::string& value);
    static void RemoveStat(const std::string& name);

    // ========== 控制 ==========

    // 显示/隐藏调试覆盖层
    void SetVisible(bool visible) { m_visible = visible; }
    bool IsVisible() const { return m_visible; }

    // 切换显示状态
    void Toggle() { m_visible = !m_visible; }

    // 设置是否显示消息区域
    void SetShowMessages(bool show) { m_showMessages = show; }

    // 设置是否显示统计面板
    void SetShowStats(bool show) { m_showStats = show; }

    // 设置是否显示变量监视
    void SetShowWatchVars(bool show) { m_showWatchVars = show; }

    // ========== 渲染 ==========

    // 每帧更新（更新消息时间）
    void Update(float deltaTime);

    // 渲染调试覆盖层（在渲染管线中调用）
    void Render();

    // 初始化/关闭
    void Initialize();
    void Shutdown();

private:
    DebugOverlay();
    ~DebugOverlay();

    // 防止复制
    DebugOverlay(const DebugOverlay&) = delete;
    DebugOverlay& operator=(const DebugOverlay&) = delete;

    // 获取消息类型的颜色
    const char* GetTypeColor(DebugMessageType type) const;

private:
    bool m_visible = true;
    bool m_showMessages = true;
    bool m_showStats = true;
    bool m_showWatchVars = true;

    std::vector<DebugMessage> m_messages;
    int m_maxMessages = 20;  // 最多显示的消息数

    // 变量监视
    struct WatchedVar {
        std::string name;
        enum Type { Float, Int, Bool, String } type;
        union {
            const float* f;
            const int* i;
            const bool* b;
        };
        const std::string* s;

        WatchedVar() : f(nullptr) {}  // 联合体初始化
    };
    std::vector<WatchedVar> m_watchedVars;

    // 统计面板
    struct StatEntry {
        std::string name;
        std::string value;
        std::function<std::string()> dynamicGetter;  // 动态获取值的函数
    };
    std::vector<StatEntry> m_stats;

    bool m_initialized = false;
};

// 便捷宏
#define DEBUG_LOG(text)           PrismaEngine::DebugOverlay::Log(text)
#define DEBUG_WARNING(text)        PrismaEngine::DebugOverlay::Warning(text)
#define DEBUG_ERROR(text)          PrismaEngine::DebugOverlay::Error(text)
#define DEBUG_SUCCESS(text)        PrismaEngine::DebugOverlay::Success(text)
#define DEBUG_WATCH_FLOAT(name, v) PrismaEngine::DebugOverlay::WatchVar(name, &(v))
#define DEBUG_WATCH_INT(name, v)   PrismaEngine::DebugOverlay::WatchVar(name, &(v))
#define DEBUG_WATCH_BOOL(name, v)  PrismaEngine::DebugOverlay::WatchVar(name, &(v))

} // namespace PrismaEngine

#else
// 非Debug构建时，定义为空操作
namespace PrismaEngine {
struct DebugOverlay {
    static void AddMessage(const std::string&, int = 0, float = 5.0f) {}
    static void Log(const std::string&) {}
    static void Warning(const std::string&) {}
    static void Error(const std::string&) {}
    static void Success(const std::string&) {}
    static void WatchVar(const std::string&, const float*) {}
    static void WatchVar(const std::string&, const int*) {}
    static void WatchVar(const std::string&, const bool*) {}
    static void WatchVar(const std::string&, const std::string*) {}
    static void UnwatchVar(const std::string&) {}
    static void AddStat(const std::string&, const std::function<std::string()>&) {}
    static void SetStat(const std::string&, const std::string&) {}
    static void RemoveStat(const std::string&) {}
};
}

#define DEBUG_LOG(text)
#define DEBUG_WARNING(text)
#define DEBUG_ERROR(text)
#define DEBUG_SUCCESS(text)
#define DEBUG_WATCH_FLOAT(name, v)
#define DEBUG_WATCH_INT(name, v)
#define DEBUG_WATCH_BOOL(name, v)

#endif // PRISMA_ENABLE_IMGUI_DEBUG && _DEBUG
