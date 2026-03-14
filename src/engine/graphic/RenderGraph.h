#pragma once

#include "../Export.h"
#include "interfaces/IRenderDevice.h"
#include "RenderCommandContext.h"
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>

namespace Prisma::Graphic {

/**
 * @brief 渲染图资源描述
 */
struct RGResourceDesc {
    enum class Type { Texture2D, Buffer, RenderTarget };
    enum class Format { RGBA8, RGBA16F, D32F };

    Type type;
    uint32_t width;
    uint32_t height;
    Format format;

    static RGResourceDesc Texture2D(uint32_t w, uint32_t h, Format f = Format::RGBA8) {
        return { Type::Texture2D, w, h, f };
    }
};

/**
 * @brief 渲染图资源句柄 (强类型)
 */
struct RGResourceHandle {
    uint32_t id = 0xFFFFFFFF;
    bool IsValid() const { return id != 0xFFFFFFFF; }
};

/**
 * @brief 渲染图执行上下文
 */
class RGContext {
public:
    RGContext(RenderCommandContext* cmd) : m_Cmd(cmd) {}
    RenderCommandContext* GetCmd() const { return m_Cmd; }
private:
    RenderCommandContext* m_Cmd;
};

/**
 * @brief 渲染图构建器 (由 Pass 使用)
 */
class RenderGraph;
class RGBuilder {
public:
    RGBuilder(RenderGraph* graph, uint32_t passIndex) : m_Graph(graph), m_PassIndex(passIndex) {}

    RGResourceHandle CreateTexture(const RGResourceDesc& desc, const std::string& name);
    void Read(RGResourceHandle handle);
    void Write(RGResourceHandle handle);

private:
    RenderGraph* m_Graph;
    uint32_t m_PassIndex;
};

/**
 * @brief 现代渲染图 (Render Graph)
 */
class ENGINE_API RenderGraph {
public:
    RenderGraph(IRenderDevice* device);
    ~RenderGraph();

    // 添加一个 Pass
    template<typename PassData, typename SetupFunc, typename ExecuteFunc>
    void AddPass(const std::string& name, SetupFunc&& setup, ExecuteFunc&& execute) {
        uint32_t passIndex = (uint32_t)m_Passes.size();
        auto& pass = m_Passes.emplace_back();
        pass.Name = name;

        // 设置阶段
        RGBuilder builder(this, passIndex);
        PassData data;
        setup(builder, data);

        // 执行阶段 (保存 Lambda)
        pass.Execute = [data, execute](RGContext& ctx) {
            execute(data, ctx);
        };
    }

    // 编译并执行
    void Compile();
    void Execute(RenderCommandContext* cmd);

private:
    friend class RGBuilder;

    struct PassNode {
        std::string Name;
        std::function<void(RGContext&)> Execute;
        std::vector<uint32_t> Inputs;
        std::vector<uint32_t> Outputs;
    };

    struct ResourceNode {
        RGResourceDesc Desc;
        std::string Name;
        void* NativeResource = nullptr; // 映射到 Vulkan Image/Buffer
    };

    IRenderDevice* m_Device;
    std::vector<PassNode> m_Passes;
    std::vector<ResourceNode> m_Resources;
};

} // namespace Prisma::Graphic
