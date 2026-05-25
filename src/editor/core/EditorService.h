#pragma once

#include "Export.h"
#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>
#include <string>
#include <functional>

namespace Prisma {

/**
 * 编辑器核心服务类
 * 
 * 提供与具体传输协议无关的编辑器操作接口，
 * 为 WebUI、Electron IPC 以及未来的命令行工具提供统一逻辑。
 */
class EDITOR_API EditorService {
public:
    /**
     * 处理通用的编辑器动作请求
     */
    static glz::json_t Dispatch(const std::string& action, const glz::json_t& params);

    // 获取视口原始二进制缓冲区 (RGBA)
    static void* GetViewportRawBuffer(size_t* outSize);
    static void* GetGameViewportRawBuffer(size_t* outSize);

private:
    // 具体业务逻辑分发
    static glz::json_t GetHierarchy();
    static glz::json_t GetEntity(uint32_t id);
    static glz::json_t UpdateEntity(const glz::json_t& params);
    static glz::json_t GetAssets();
    static glz::json_t GetStatus();
};

} // namespace Prisma
