#pragma once
#include "Export.h"
#include <string>

namespace Prisma {

/// <summary>
/// 运行环境类型
/// </summary>
enum class EnvironmentType {
    Desktop,   // 桌面环境（有窗口系统）
    Headless,  // 无头环境（无窗口系统）
    Terminal,  // 纯终端环境
    Unknown    // 未知环境
};

/// <summary>
/// 环境检测工具类
/// 用于检测当前运行环境是否支持图形界面
/// </summary>
class EDITOR_API Environment {
public:
    /// <summary>
    /// 检测当前运行环境类型
    /// </summary>
    static EnvironmentType DetectEnvironment();

    /// <summary>
    /// 检测是否支持图形界面
    /// </summary>
    static bool HasDisplaySupport();

    /// <summary>
    /// 检测是否在终端中运行
    /// </summary>
    static bool IsRunningInTerminal();

    /// <summary>
    /// 获取环境描述字符串
    /// </summary>
    static std::string GetEnvironmentDescription();

private:
    // 平台特定检测函数
    static bool DetectDisplayLinux();
    static bool DetectDisplayWindows();
    static bool DetectDisplayAndroid();
    static bool IsRedirectedOutput();
};

} // namespace Prisma
