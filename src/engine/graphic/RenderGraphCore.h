#pragma once

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include "RenderAPI.h"
#include "RenderCommandContext.h"
#include "Logger.h"

namespace Engine {
namespace Graphic {

// 前向声明
class RenderGraph;
class RenderGraphContext;

// 资源描述符
struct ResourceDesc {
    enum class Type {
        Texture2D,
        TextureCube,
        Buffer,
        RenderTarget,
        DepthStencil
    };

    enum class Format {
        Unknown,
        RGBA8_UNorm,
        RGBA16_Float,
        RG16_SNorm,
        R32_Float,
        D32_Float
    };

    enum class Flags : uint32_t {
        None = 0,
        ShaderResource = 1 << 0,
        RenderTarget = 1 << 1,
        UnorderedAccess = 1 << 2,
        ShaderResource = 1 << 3
    };

    Type type;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 1;
    uint32_t arraySize = 1;
    uint32_t mipLevels = 1;
    Format format = Format::Unknown;
    uint32_t flags = 0;

    static ResourceDesc Texture2D(uint32_t width, uint32_t height, Format format) {
        ResourceDesc desc;
        desc.type = Type::Texture2D;
        desc.width = width;
        desc.height = height;
        desc.depth = 1;
        desc.format = format;
        return desc;
    }

    static ResourceDesc DepthStencil(uint32_t width, uint32_t height, Format format) {
        ResourceDesc desc;
        desc.type = Type::DepthStencil;
        desc.width = width;
        desc.height = height;
        desc.depth = 1;
        desc.format = format;
        return desc;
    }
};

// 强类型资源句柄
class ResourceHandle {
public:
    static const uint32_t InvalidIndex = UINT32_MAX;

    ResourceHandle() : m_id(InvalidIndex), m_version(0) {}
    ResourceHandle(uint32_t id, uint32_t version) : m_id(id), m_version(version) {}

    bool isValid() const { return m_id != InvalidIndex; }
    uint32_t getId() const { return m_id; }
    uint32_t getVersion() const { return m_version; }

    bool operator==(const ResourceHandle& other) const {
        return m_id == other.m_id && m_version == other.m_version;
    }

    bool operator!=(const ResourceHandle& other) const {
        return !(*this == other);
    }

private:
    uint32_t m_id;
    uint32_t m_version;
};

// Pass执行上下文
class RenderGraphContext {
public:
    RenderGraphContext(RenderCommandContext* cmdContext) : m_cmdContext(cmdContext) {}

    RenderCommandContext* getCommandContext() const { return m_cmdContext; }

    // 获取资源描述
    ResourceDesc getResourceDesc(ResourceHandle handle) const;

    // 获取原生资源（用于后端API调用）
    void* getNativeResource(ResourceHandle handle) const;

private:
    RenderCommandContext* m_cmdContext;
    const RenderGraph* m_graph;
};

// Pass构建器
class RenderPassBuilder {
public:
    template<typename T>
    RenderPassBuilder& read(ResourceHandle handle, T** outData = nullptr);

    template<typename T>
    RenderPassBuilder& write(ResourceHandle handle, T** outData = nullptr);

    RenderPassBuilder& createTexture(const ResourceDesc& desc, ResourceHandle& outHandle, const std::string& name = "");

    template<typename PassData, typename ExecuteFunc>
    RenderPassBuilder& setExecuteFunc(ExecuteFunc&& func);

private:
    RenderGraph* m_graph;
    uint32_t m_passIndex;

    RenderPassBuilder(RenderGraph* graph, uint32_t passIndex)
        : m_graph(graph), m_passIndex(passIndex) {}

    friend class RenderGraph;
};

// RenderGraph主类
class RenderGraph {
public:
    RenderGraph();
    ~RenderGraph();

    // 资源管理
    ResourceHandle createTexture(const ResourceDesc& desc, const std::string& name = "");
    ResourceHandle importTexture(void* nativeResource, const ResourceDesc& desc, const std::string& name = "");
    ResourceHandle getBackbuffer() const { return m_backbufferHandle; }

    // Pass构建
    template<typename PassData = void>
    RenderPassBuilder addPass(const std::string& name);

    // 图编译和执行
    void compile();
    void execute(RenderAPI* backend);
    void present(ResourceHandle backbuffer);

    // 调试和分析
    void visualizeGraph(const std::string& filename) const;
    void dumpExecutionOrder() const;
    ResourceDesc getResourceDesc(ResourceHandle handle) const;
    const std::string& getResourceName(ResourceHandle handle) const;

private:
    friend class RenderPassBuilder;
    friend class RenderGraphContext;

    struct ResourceNode {
        ResourceDesc desc;
        std::string name;
        uint32_t producer = InvalidIndex;
        uint32_t firstConsumer = InvalidIndex;
        uint32_t lastConsumer = InvalidIndex;
        void* importedResource = nullptr;
        void* allocatedResource = nullptr;
        bool isImported = false;
        bool isCulled = false;
    };

    struct PassNode {
        std::string name;
        std::function<void(RenderGraphContext&)> execute;
        std::vector<uint32_t> inputs;
        std::vector<uint32_t> outputs;
        std::vector<uint32_t> creates;
        std::vector<uint32_t> destroys;
        bool isCulled = false;
        uint32_t refCount = 0;
    };

    static const uint32_t InvalidIndex = UINT32_MAX;

    std::vector<PassNode> m_passes;
    std::vector<ResourceNode> m_resources;
    std::vector<uint32_t> m_executionOrder;
    std::vector<ResourceHandle> m_resourceHandles;
    std::unordered_map<std::string, uint32_t> m_passNameToIndex;
    std::unordered_map<std::string, uint32_t> m_resourceNameToIndex;

    ResourceHandle m_backbufferHandle;
    uint32_t m_nextResourceId = 0;
    uint32_t m_nextResourceVersion = 1;

    // 编译阶段
    void calculateRefCounts();
    void cullUnusedResources();
    void cullUnusedPasses();
    void calculateExecutionOrder();
    void optimizeResourceAliases();
    void generateBarriers();

    // 资源生命周期管理
    void allocateResources(RenderAPI* backend);
    void deallocateResources(RenderAPI* backend);

    // 调试辅助
    void validateGraph() const;
    bool hasCycles() const;
};

// RenderPassBuilder模板实现
template<typename PassData>
RenderPassBuilder RenderGraph::addPass(const std::string& name) {
    uint32_t passIndex = static_cast<uint32_t>(m_passes.size());

    PassNode pass;
    pass.name = name;
    pass.refCount = 0;

    m_passes.push_back(pass);
    m_passNameToIndex[name] = passIndex;

    LOG_DEBUG("RenderGraph", "Added pass: {0} (index: {1})", name, passIndex);

    return RenderPassBuilder(this, passIndex);
}

// 全局访问点
using RenderGraphPtr = std::shared_ptr<RenderGraph>;

} // namespace Graphic
} // namespace Engine