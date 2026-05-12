#pragma once

/**
 * @brief IEngine — 纯虚接口
 *
 * 这是 Engine DLL 对外暴露的唯一接口。
 * PrismaLauncher 通过 LoadLibrary + CreateInterface 获取 IEngine*
 * 然后通过虚函数表（vtable）调用所有方法。
 *
 * 此头文件无任何外部依赖（不含 Export.h，不含任何 Engine 内部类型），
 * 运行时加载 DLL 的项目可直接引用，无需链接 Engine 导入库。
 */

#include <stdint.h>

// ============================================================
// Engine 创建信息（纯 C 结构，可跨 DLL 边界传递）
// ============================================================
struct EngineCreateInfo {
    const char* Name;
    int         Headless;                           // bool
    int         MinLogLevel;                        // 0=Trace … 5=Fatal
    int         PresentMode;                        // 0=VSync, 1=Immediate, 2=Mailbox
    uint32_t    MaxFPS;                             // 0 = unlimited
    int         RefreshAssetDatabaseOnStartup;      // bool
};

// ============================================================
// IEngine 纯虚接口
// ============================================================
class IEngine {
public:
    virtual ~IEngine() = default;

    /**
     * @brief 初始化引擎所有子系统
     * @return 0 = 成功，非 0 = 失败
     */
    virtual int Initialize() = 0;

    /**
     * @brief 运行引擎主循环
     * @param pluginPath Application 插件 DLL 路径（如 "Template2D.dll"）
     *                   引擎内部 LoadLibrary + CreateApplication 加载
     * @return 0 = 成功，非 0 = 失败
     */
    virtual int Run(const char* pluginPath) = 0;

    /**
     * @brief 关闭引擎
     */
    virtual void Shutdown() = 0;
};
