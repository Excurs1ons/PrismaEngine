#include "Shader.h"
#include <d3dcompiler.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include "StringUtils.h"
using namespace Engine;
using namespace StringUtils;
Shader::Shader() {
}

Shader::~Shader() {
    Unload();
}
const std::string EXT_CompiledShaderObject = "cso";
const std::string EXT_HLSL = "hlsl";
const std::string EXT_SPIRV = "spv";
bool Shader::Load(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        LOG_ERROR("Shader", "着色器文件不存在: {0}", path.string());
        return false;
    }
    // 检查后缀名，是否是预编译的文件
    auto ext = ToLower(path.extension().string());
    if (Equals(ext,EXT_HLSL)){

    }
    else if (Equals(ext, EXT_CompiledShaderObject)) {

    }
    else if (Equals(ext, EXT_SPIRV)) {

    }
    else {

    }
    // For now, we'll compile both vertex and pixel shaders from the same file
    // In a more advanced implementation, you might have separate files or metadata
    
    UINT compileFlags = 0;
#if defined(_DEBUG) || defined(DBG)
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    // Compile vertex shader
    ComPtr<ID3DBlob> errors;
    HRESULT hr = D3DCompileFromFile(
        path.c_str(), 
        nullptr, 
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "VSMain", 
        "vs_5_0", 
        compileFlags, 
        0, 
        m_vertexShader.GetAddressOf(), 
        errors.GetAddressOf()
    );

    if (errors != nullptr) {
        LOG_ERROR("Shader", "顶点着色器编译错误: {0}", static_cast<char*>(errors->GetBufferPointer()));
    }

    if (FAILED(hr)) {
        LOG_ERROR("Shader", "顶点着色器编译失败: {0}", path.string());
        return false;
    }

    // Compile pixel shader
    hr = D3DCompileFromFile(
        path.c_str(), 
        nullptr, 
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "PSMain", 
        "ps_5_0", 
        compileFlags, 
        0, 
        m_pixelShader.GetAddressOf(), 
        errors.GetAddressOf()
    );

    if (errors != nullptr) {
        LOG_ERROR("Shader", "像素着色器编译错误: {0}", static_cast<char*>(errors->GetBufferPointer()));
    }

    if (FAILED(hr)) {
        LOG_ERROR("Shader", "像素着色器编译失败: {0}", path.string());
        return false;
    }

    m_path = path;
    m_name = path.filename().string();
    m_isLoaded = true;
    return true;
}

void Shader::Unload() {
    m_vertexShader.Reset();
    m_pixelShader.Reset();
    m_isLoaded = false;
}

bool Shader::IsLoaded() const {
    return m_isLoaded;
}

ResourceType Shader::GetType() const {
    return ResourceType::Shader;
}

bool Shader::CompileFromString(const char* vsSource, const char* psSource)
{
    if (!vsSource || !psSource) {
        LOG_ERROR("Shader", "着色器源码为空");
        return false;
    }

    UINT compileFlags = 0;
#if defined(_DEBUG) || defined(DBG)
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    // 编译顶点着色器
    ComPtr<ID3DBlob> errors;
    HRESULT hr = D3DCompile(
        vsSource,
        strlen(vsSource),
        "DefaultVertexShader",
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "VSMain",
        "vs_5_0",
        compileFlags,
        0,
        &m_vertexShader,
        &errors);

    if (errors != nullptr) {
        LOG_ERROR("Shader", "顶点着色器编译错误: {0}", static_cast<char*>(errors->GetBufferPointer()));
    }

    if (FAILED(hr)) {
        LOG_ERROR("Shader", "顶点着色器编译失败");
        return false;
    }

    // 编译像素着色器
    errors.Reset();
    hr = D3DCompile(
        psSource,
        strlen(psSource),
        "DefaultPixelShader",
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "PSMain",
        "ps_5_0",
        compileFlags,
        0,
        &m_pixelShader,
        &errors);

    if (errors != nullptr) {
        LOG_ERROR("Shader", "像素着色器编译错误: {0}", static_cast<char*>(errors->GetBufferPointer()));
    }

    if (FAILED(hr)) {
        LOG_ERROR("Shader", "像素着色器编译失败");
        return false;
    }

    m_name = "DefaultShader";
    m_isLoaded = true;
    LOG_INFO("Shader", "默认着色器编译成功");
    return true;
}