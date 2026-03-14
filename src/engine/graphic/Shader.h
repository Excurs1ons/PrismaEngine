#pragma once

#include "Asset.h"
#include "interfaces/ShaderReflection.h"
#include <vector>
#include <memory>
#include <unordered_map>

namespace Prisma::Graphic {

/**
 * @brief 现代着色器资产 (Shader Asset)
 * 这是一个纯数据容器，包含 SPIR-V 字节码和反射信息。
 * 不再有复杂的 Implementation 切换，RHI 后端直接消费这些数据。
 */
class ENGINE_API Shader : public Prisma::Asset {
public:
    Shader() = default;
    ~Shader() override = default;

    // Asset 接口
    bool Load(const std::filesystem::path& path) override;
    void Unload() override;
    bool IsLoaded() const override { return !m_Bytecode.empty(); }
    Prisma::AssetType GetType() const override { return Prisma::AssetType::Shader; }

    // 访问器
    const std::vector<uint32_t>& GetBytecode() const { return m_Bytecode; }
    const ShaderReflection& GetReflection() const { return m_Reflection; }

    // 反射查找
    const ShaderResource* FindResource(const std::string& name) const;

private:
    std::vector<uint32_t> m_Bytecode;
    ShaderReflection m_Reflection;
    
    // 快速查找映射表
    std::unordered_map<std::string, uint32_t> m_ResourceMap;
};

/**
 * @brief 着色器管理器 (属于 Engine 的子系统)
 */
class ShaderLibrary : public ISubSystem {
public:
    int Initialize() override { return 0; }
    void Shutdown() override { m_Shaders.clear(); }
    void Update(Timestep ts) override {}

    std::shared_ptr<Shader> Load(const std::string& name, const std::filesystem::path& path);
    std::shared_ptr<Shader> Get(const std::string& name);

private:
    std::unordered_map<std::string, std::shared_ptr<Shader>> m_Shaders;
};

} // namespace Prisma::Graphic
