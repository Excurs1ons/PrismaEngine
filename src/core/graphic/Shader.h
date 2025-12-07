#pragma once
#include "ResourceManager.h"
#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl.h>
#include <memory>

using Microsoft::WRL::ComPtr;

namespace Engine {
class Shader : public ResourceBase {
public:
    Shader();
    ~Shader();

    // IResource implementation
    bool Load(const std::filesystem::path& path) override;
    void Unload() override;
    bool IsLoaded() const override;
    ResourceType GetType() const override;

    // Shader specific methods
    const ComPtr<ID3DBlob>& GetVertexShaderBlob() const { return m_vertexShader; }
    const ComPtr<ID3DBlob>& GetPixelShaderBlob() const { return m_pixelShader; }
    const std::string& GetEntryPoint() const { return m_entryPoint; }
    void SetEntryPoint(const std::string& entryPoint) { m_entryPoint = entryPoint; }
    const std::string& GetModel() const { return m_model; }
    void SetModel(const std::string& model) { m_model = model; }

private:
    ComPtr<ID3DBlob> m_vertexShader;
    ComPtr<ID3DBlob> m_pixelShader;
    std::string m_entryPoint = "main";
    std::string m_model      = "ps_5_0";
};

}  // namespace Engine