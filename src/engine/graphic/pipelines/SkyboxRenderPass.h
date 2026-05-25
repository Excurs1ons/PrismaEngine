#pragma once

#include "forward/ForwardRenderPassBase.h"
#include "graphic/LogicalPass.h"
#include "graphic/Material.h"
#include "graphic/Mesh.h"
#include "graphic/Shader.h"
#include "graphic/interfaces/IDeviceContext.h"
#include "graphic/interfaces/IPass.h"
#include "graphic/interfaces/IRenderTarget.h"
#include "graphic/interfaces/ITexture.h"
#include "math/MathTypes.h"
#include <memory>
#include <vector>

namespace Prisma::Graphic {

// 天空盒逻辑 Pass
/// 负责渲染天空盒，不包含具体图形 API
class SkyboxPass : public ForwardRenderPass {
public:
    SkyboxPass();
    ~SkyboxPass() override = default;

    // === IPass 接口实现 ===

    // 执行 Pass
    void Execute(const PassExecutionContext& context) override;

    // 更新 Pass 数据
    void Update(Prisma::Timestep ts) override;

    // === 天空盒特有功能 ===

    // 设置立方体纹理
    void SetCubeMapTexture(ITexture* cubeTexture) { m_cubeMapTexture = cubeTexture; }

    // 获取立方体纹理
    ITexture* GetCubeMapTexture() const { return m_cubeMapTexture; }

private:
    // 初始化天空盒网格
    void InitializeSkyboxMesh();

    // 检查资源是否已正确初始化
    bool IsInitialized() const { return m_initialized; }

private:
    // 资源
    ITexture* m_cubeMapTexture;  // 立方体贴图纹理

    // 网格数据
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;

    // 标志
    bool m_initialized;
    bool m_meshInitialized;
};

} // namespace Prisma::Graphic
