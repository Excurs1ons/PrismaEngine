# PrismaEngine RenderGraph 迁移计划

## 1. 架构概述

### 1.1 核心组件

```cpp
// 资源描述符
struct ResourceDesc {
    enum class Type {
        Texture,
        Buffer,
        RenderTarget,
        DepthStencil
    };

    Type type;
    uint32_t width, height, depth;
    uint32_t arraySize;
    uint32_t mipLevels;
    Format format;
    ResourceFlags flags;
};

// 资源句柄（强类型）
class ResourceHandle {
    uint32_t id;
    uint32_t version;  // 防止悬空引用

    bool isValid() const;
    ResourceDesc getDesc() const;
};

// 渲染Pass描述
struct RenderPassDesc {
    std::string name;
    std::vector<ResourceHandle> inputs;
    std::vector<ResourceHandle> outputs;
    std::vector<ResourceHandle> create;
    std::vector<ResourceHandle> destroy;
};

// Pass执行函数
using PassExecuteFunc = std::function<void(RenderGraphContext&)>;
```

### 1.2 RenderGraph类

```cpp
class RenderGraph {
public:
    // 构建阶段
    ResourceHandle createTexture(const ResourceDesc& desc, const std::string& name = "");
    ResourceHandle importTexture(void* nativeResource, const ResourceDesc& desc);

    template<typename PassData>
    RenderPassBuilder addPass(const std::string& name);

    void cullPasses();  // 剔除未使用的Pass
    void compile();     // 编译图，优化执行顺序

    // 执行阶段
    void execute(RenderBackend* backend);
    void present(ResourceHandle backbuffer);

    // 调试
    void visualizeGraph(const std::string& filename);
    ResourceDesc getResourceDesc(ResourceHandle handle) const;

private:
    friend class RenderPassBuilder;

    struct PassNode {
        std::string name;
        PassExecuteFunc execute;
        std::vector<ResourceHandle> inputs;
        std::vector<ResourceHandle> outputs;
        uint32_t refCount = 0;
        bool culled = false;
    };

    struct ResourceNode {
        ResourceDesc desc;
        std::string name;
        uint32_t producer = UINT32_MAX;
        uint32_t firstConsumer = UINT32_MAX;
        uint32_t lastConsumer = UINT32_MAX;
        void* importedResource = nullptr;
    };

    std::vector<PassNode> m_passes;
    std::vector<ResourceNode> m_resources;
    std::vector<uint32_t> m_executionOrder;

    // 编译优化
    void calculateResourceLifetimes();
    void optimizeResourceAliases();
    void generateBarriers();
};
```

### 1.3 Pass构建器

```cpp
class RenderPassBuilder {
public:
    template<typename T>
    RenderPassBuilder& read(ResourceHandle handle, T* data = nullptr);

    template<typename T>
    RenderPassBuilder& write(ResourceHandle handle, T* data = nullptr);

    RenderPassBuilder& createTexture(ResourceDesc& desc, ResourceHandle& outHandle);

    template<typename PassData, typename ExecuteFunc>
    RenderPassBuilder& setExecuteFunc(ExecuteFunc&& func);

private:
    RenderGraph* m_graph;
    uint32_t m_passIndex;
    friend class RenderGraph;
};
```

## 2. 迁移步骤

### Phase 1: 基础架构实现（2-3周）

1. **创建核心类型**
   - [ ] ResourceHandle/ResourceDesc
   - [ ] RenderGraph基础类
   - [ ] RenderPassBuilder
   - [ ] RenderGraphContext

2. **资源管理系统**
   - [ ] 资源池化
   - [ ] 生命周期跟踪
   - [ ] 自动别名优化

3. **依赖解析器**
   - [ ] DAG构建
   - [ ] 拓扑排序
   - [ ] 循环依赖检测

### Phase 2: 集成现有系统（2周）

1. **后端适配层**
   ```cpp
   // 在RenderBackend中添加支持
   class RenderBackend {
   public:
       virtual void executeRenderGraph(RenderGraph& graph) = 0;
       virtual void* allocateResource(const ResourceDesc& desc) = 0;
       virtual void deallocateResource(void* resource) = 0;
   };
   ```

2. **现有Pass的包装**
   ```cpp
   // 适配现有的RenderPass接口
   class LegacyPassWrapper {
   public:
       LegacyPassWrapper(std::shared_ptr<RenderPass> legacyPass);

       void recordCommands(RenderGraphContext& context) {
           // 转换为旧接口调用
           m_legacyPass->Execute(context.getCommandContext());
       }
   };
   ```

### Phase 3: 优化和新功能（3-4周）

1. **多线程支持**
   - CPU侧并行Pass准备
   - 异步计算队列支持

2. **高级特性**
   - GPU驱动的渲染
   - 渲染图热重载
   - 性能分析工具集成

3. **调试和可视化**
   - 图形化依赖查看器
   - 资源使用统计
   - Pass执行时间分析

## 3. 示例使用

### 3.1 创建前向渲染Pass

```cpp
void setupForwardRendering(RenderGraph& graph, Scene* scene) {
    // 创建GBuffer资源
    ResourceHandle gBufferAlbedo = graph.createTexture(
        ResourceDesc::Texture2D(1920, 1080, Format::RGBA8_UNorm),
        "GBuffer_Albedo"
    );

    ResourceHandle gBufferNormal = graph.createTexture(
        ResourceDesc::Texture2D(1920, 1080, Format::RG16_SNorm),
        "GBuffer_Normal"
    );

    ResourceHandle depthBuffer = graph.createTexture(
        ResourceDesc::DepthStencil(1920, 1080, Format::D32_Float),
        "Depth"
    );

    // 添加GBuffer生成Pass
    graph.addPass<GBufferData>("GBuffer")
        .write(gBufferAlbedo)
        .write(gBufferNormal)
        .write(depthBuffer)
        .setExecuteFunc([scene](RenderGraphContext& context, GBufferData& data) {
            // 渲染几何体到GBuffer
            renderGeometryToGBuffer(context, scene);
        });

    // 添加光照Pass
    graph.addPass<LightingData>("Lighting")
        .read(gBufferAlbedo)
        .read(gBufferNormal)
        .read(depthBuffer)
        .write(backbuffer)
        .setExecuteFunc([scene](RenderGraphContext& context, LightingData& data) {
            // 应用光照
            applyLighting(context, scene, data.gBufferAlbedo, data.gBufferNormal);
        });
}
```

### 3.2 后处理链

```cpp
void setupPostProcessing(RenderGraph& graph, ResourceHandle source) {
    auto toneMapHandle = graph.createTexture(
        ResourceDesc::Texture2D(1920, 1080, Format::RGBA16_Float),
        "ToneMapped"
    );

    // Tone mapping
    graph.addPass<ToneMapData>("ToneMap")
        .read(source)
        .write(toneMapHandle)
        .setExecuteFunc([](RenderGraphContext& context, ToneMapData& data) {
            applyToneMapping(context, data.source, data.target);
        });

    // FXAA
    graph.addPass<FXAAData>("FXAA")
        .read(toneMapHandle)
        .write(backbuffer)
        .setExecuteFunc([](RenderGraphContext& context, FXAAData& data) {
            applyFXAA(context, data.source, data.target);
        });
}
```

## 4. 性能优化策略

### 4.1 资源别名
```cpp
// 自动识别可以重用的资源
void RenderGraph::optimizeResourceAliases() {
    // 按生命周期排序资源
    auto sortedResources = sortResourcesByLifetime();

    // 寻找不重叠的生命周期进行别名
    for (size_t i = 0; i < sortedResources.size(); ++i) {
        for (size_t j = i + 1; j < sortedResources.size(); ++j) {
            if (canAlias(sortedResources[i], sortedResources[j])) {
                aliasResources(sortedResources[i], sortedResources[j]);
                break;
            }
        }
    }
}
```

### 4.2 并行执行
```cpp
// 识别可以并行执行的Pass
std::vector<std::vector<uint32_t>> RenderGraph::findParallelGroups() {
    std::vector<std::vector<uint32_t>> groups;

    for (auto& pass : m_executionOrder) {
        bool inserted = false;

        // 尝试插入到现有组
        for (auto& group : groups) {
            if (canExecuteInParallel(pass, group)) {
                group.push_back(pass);
                inserted = true;
                break;
            }
        }

        // 创建新组
        if (!inserted) {
            groups.push_back({pass});
        }
    }

    return groups;
}
```

## 5. 迁移风险评估

### 5.1 技术风险
- **复杂度增加**：新系统学习曲线陡峭
- **性能风险**：初期可能有性能回退
- **兼容性**：需要大量测试确保跨平台兼容

### 5.2 缓解措施
- 渐进式迁移，保持旧系统并行运行
- 完善的单元测试和基准测试
- 详细的文档和示例代码

## 6. 时间线

| 阶段 | 时间 | 里程碑 |
|------|------|--------|
| Phase 1 | 3周 | 基础架构完成 |
| Phase 2 | 2周 | 现有Pass迁移完成 |
| Phase 3 | 4周 | 性能优化完成 |
| 测试 | 2周 | 全面测试和调优 |
| 文档 | 1周 | API文档和示例 |

总计：12周

## 7. 后续优化方向

1. **GPU驱动的渲染**：使用Indirect Draw和GPU Culling
2. **实时光线追踪**：集成RTX/光线追踪扩展
3. **AI加速渲染**：使用DLSS/FSR等技术
4. **云渲染支持**：分布式渲染架构