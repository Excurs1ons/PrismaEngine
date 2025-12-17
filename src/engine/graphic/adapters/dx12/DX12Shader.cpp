#include "DX12Shader.h"
#include "DX12RenderDevice.h"
#include "../Logger.h"
#include <directx/d3d12shader.h>
#include <dxcapi.h>
#include <wrl/client.h>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <chrono>

using Microsoft::WRL::ComPtr;

namespace PrismaEngine::Graphic::DX12 {

DX12Shader::DX12Shader(DX12RenderDevice* device,
                       const ShaderDesc& desc,
                       const std::vector<uint8_t>& bytecode,
                       const ShaderReflection& reflection)
    : m_device(device)
    , m_desc(desc)
    , m_bytecode(bytecode)
    , m_reflection(reflection) {

    // 获取文件修改时间
    if (!desc.filename.empty()) {
        std::filesystem::path filePath(desc.filename);
        if (std::filesystem::exists(filePath)) {
            auto ftime = std::filesystem::last_write_time(filePath);
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - std::filesystem::file_time_type::clock::now()
                + std::chrono::system_clock::now());
            m_fileModificationTime = std::chrono::duration_cast<std::chrono::seconds>(
                sctp.time_since_epoch()).count();
        }
    }
}

DX12Shader::~DX12Shader() {
    // 资源由智能指针自动管理
}

// IShader接口实现
ShaderType DX12Shader::GetShaderType() const {
    return m_desc.type;
}

ShaderLanguage DX12Shader::GetLanguage() const {
    return m_desc.language;
}

const std::string& DX12Shader::GetEntryPoint() const {
    return m_desc.entryPoint;
}

const std::string& DX12Shader::GetTarget() const {
    return m_desc.target;
}

const std::string& DX12Shader::GetSource() const {
    return m_desc.source;
}

const std::vector<uint8_t>& DX12Shader::GetBytecode() const {
    return m_bytecode;
}

const std::string& DX12Shader::GetFilename() const {
    return m_desc.filename;
}

uint64_t DX12Shader::GetCompileTimestamp() const {
    return m_desc.compileTimestamp;
}

uint64_t DX12Shader::GetCompileHash() const {
    return m_desc.compileHash;
}

const ShaderCompileOptions& DX12Shader::GetCompileOptions() const {
    return m_desc.compileOptions;
}

const ShaderReflection& DX12Shader::GetReflection() const {
    return m_reflection;
}

bool DX12Shader::HasReflection() const {
    return m_reflection.inputParameters.size() > 0 ||
           m_reflection.outputParameters.size() > 0 ||
           m_reflection.resources.size() > 0 ||
           m_reflection.constantBuffers.size() > 0;
}

const ShaderReflection::Resource* DX12Shader::FindResource(const std::string& name) const {
    for (const auto& resource : m_reflection.resources) {
        if (resource.name == name) {
            return &resource;
        }
    }
    return nullptr;
}

const ShaderReflection::Resource* DX12Shader::FindResourceByBindPoint(uint32_t bindPoint, uint32_t space) const {
    for (const auto& resource : m_reflection.resources) {
        if (resource.bindPoint == bindPoint && resource.space == space) {
            return &resource;
        }
    }
    return nullptr;
}

const ShaderReflection::ConstantBuffer* DX12Shader::FindConstantBuffer(const std::string& name) const {
    for (const auto& cb : m_reflection.constantBuffers) {
        if (cb.name == name) {
            return &cb;
        }
    }
    return nullptr;
}

uint32_t DX12Shader::GetInputParameterCount() const {
    return static_cast<uint32_t>(m_reflection.inputParameters.size());
}

const ShaderReflection::InputParameter& DX12Shader::GetInputParameter(uint32_t index) const {
    return m_reflection.inputParameters[index];
}

uint32_t DX12Shader::GetOutputParameterCount() const {
    return static_cast<uint32_t>(m_reflection.outputParameters.size());
}

const ShaderReflection::OutputParameter& DX12Shader::GetOutputParameter(uint32_t index) const {
    return m_reflection.outputParameters[index];
}

bool DX12Shader::Recompile(const ShaderCompileOptions* options, std::string& errors) {
    // 如果没有文件名，无法重新编译
    if (m_desc.filename.empty()) {
        errors = "Cannot recompile shader: no filename available";
        return false;
    }

    // 从文件重新加载源码
    std::ifstream file(m_desc.filename);
    if (!file) {
        errors = "Cannot open shader file: " + m_desc.filename;
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    return RecompileFromSource(source, options, errors);
}

bool DX12Shader::RecompileFromSource(const std::string& source,
                                     const ShaderCompileOptions* options,
                                     std::string& errors) {
    if (!m_device) {
        errors = "Device not available";
        return false;
    }

    // 更新源码
    m_desc.source = source;

    // 更新编译选项
    if (options) {
        m_desc.compileOptions = *options;
    }

    // 使用DXC编译着色器
    ComPtr<IDxcCompiler> compiler;
    ComPtr<IDxcLibrary> library;
    ComPtr<IDxcBlobEncoding> sourceBlob;
    ComPtr<IDxcOperationResult> result;

    // 创建DXC实例
    HRESULT hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
    if (FAILED(hr)) {
        errors = "Failed to create DXC compiler";
        return false;
    }

    hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library));
    if (FAILED(hr)) {
        errors = "Failed to create DXC library";
        return false;
    }

    // 创建源码blob
    hr = library->CreateBlobWithEncodingFromPinned(
        source.c_str(),
        static_cast<uint32_t>(source.size()),
        CP_UTF8,
        &sourceBlob
    );
    if (FAILED(hr)) {
        errors = "Failed to create source blob";
        return false;
    }

    // 准备编译参数
    std::vector<std::wstring> wArgs;
    std::vector<LPCWSTR> args;

    // 添加标准参数
    wArgs.push_back(L"-T");
    wArgs.push_back(std::wstring(m_desc.target.begin(), m_desc.target.end()));

    wArgs.push_back(L"-E");
    wArgs.push_back(std::wstring(m_desc.entryPoint.begin(), m_desc.entryPoint.end()));

    // 添加优化等级
    switch (m_desc.compileOptions.optimizationLevel) {
        case OptimizationLevel::None:
            wArgs.push_back(L"-O0");
            break;
        case OptimizationLevel::Size:
            wArgs.push_back(L"-Os");
            break;
        case OptimizationLevel::Speed:
            wArgs.push_back(L"-O2");
            break;
        case OptimizationLevel::Full:
            wArgs.push_back(L"-O3");
            break;
    }

    // 添加调试信息
    if (HasFlag(m_desc.compileOptions.flags, ShaderCompileFlag::Debug)) {
        wArgs.push_back(L"-Zi");
        wArgs.push_back(L"-Qembed_debug");
    }

    // 跳过优化
    if (HasFlag(m_desc.compileOptions.flags, ShaderCompileFlag::SkipOptimization)) {
        wArgs.push_back(L"-Od");
    }

    // 严格模式
    if (HasFlag(m_desc.compileOptions.flags, ShaderCompileFlag::Strict)) {
        wArgs.push_back(L"-Ges");
    }

    // 禁用警告
    if (!HasFlag(m_desc.compileOptions.flags, ShaderCompileFlag::WarningsAsErrors)) {
        wArgs.push_back(L"-no-warnings-as-errors");
    }

    // 添加自定义定义
    for (const auto& define : m_desc.compileOptions.defines) {
        wArgs.push_back(L"-D");
        wArgs.push_back(std::wstring(define.begin(), define.end()));
    }

    // 添加包含目录
    for (const auto& include : m_desc.compileOptions.includeDirectories) {
        wArgs.push_back(L"-I");
        wArgs.push_back(std::wstring(include.begin(), include.end()));
    }

    // 转换为LPCWSTR数组
    for (const auto& arg : wArgs) {
        args.push_back(arg.c_str());
    }

    // 编译着色器
    hr = compiler->Compile(
        sourceBlob.Get(),
        nullptr,  // 可以指定文件名
        m_desc.entryPoint.c_str(),
        m_desc.target.c_str(),
        args.data(),
        static_cast<uint32_t>(args.size()),
        nullptr,  // Defines
        0,
        nullptr,  // Include handler
        &result
    );

    if (FAILED(hr)) {
        errors = "Failed to compile shader: D3DCompile failed";
        return false;
    }

    // 检查编译结果
    HRESULT compilationStatus = result->GetStatus();
    if (FAILED(compilationStatus)) {
        ComPtr<IDxcBlobEncoding> errorBlob;
        result->GetErrorBuffer(&errorBlob);
        if (errorBlob) {
            errors = static_cast<char*>(errorBlob->GetBufferPointer());
        } else {
            errors = "Shader compilation failed with unknown error";
        }
        return false;
    }

    // 获取编译后的字节码
    ComPtr<IDxcBlob> codeBlob;
    result->GetResult(&codeBlob);
    if (!codeBlob) {
        errors = "Compilation succeeded but failed to get bytecode";
        return false;
    }

    // 更新字节码
    m_bytecode.clear();
    m_bytecode.resize(codeBlob->GetBufferSize());
    memcpy(m_bytecode.data(), codeBlob->GetBufferPointer(), codeBlob->GetBufferSize());

    // 更新编译时间戳和哈希
    m_desc.compileTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    // 简单哈希计算
    m_desc.compileHash = 0;
    for (uint8_t byte : m_bytecode) {
        m_desc.compileHash = m_desc.compileHash * 31 + byte;
    }

    // 清除旧的编译日志
    m_compileLog.clear();

    LOG_INFO("DX12Shader", "Shader recompiled successfully: {0}", m_desc.filename);
    return true;
}

bool DX12Shader::ReloadFromFile(std::string& errors) {
    if (m_desc.filename.empty()) {
        errors = "No filename available for reload";
        return false;
    }

    return Recompile(nullptr, errors);
}

void DX12Shader::EnableHotReload(bool enable) {
    m_hotReloadEnabled = enable;
}

bool DX12Shader::IsFileModified() const {
    if (m_desc.filename.empty()) {
        return false;
    }

    std::filesystem::path filePath(m_desc.filename);
    if (!std::filesystem::exists(filePath)) {
        return false;
    }

    auto ftime = std::filesystem::last_write_time(filePath);
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - std::filesystem::file_time_type::clock::now()
        + std::chrono::system_clock::now());
    uint64_t currentModificationTime = std::chrono::duration_cast<std::chrono::seconds>(
        sctp.time_since_epoch()).count();

    return currentModificationTime > m_fileModificationTime;
}

bool DX12Shader::NeedsReload() const {
    return m_hotReloadEnabled && IsFileModified();
}

uint64_t DX12Shader::GetFileModificationTime() const {
    return m_fileModificationTime;
}

const std::string& DX12Shader::GetCompileLog() const {
    return m_compileLog;
}

bool DX12Shader::HasWarnings() const {
    return m_compileLog.find("warning") != std::string::npos;
}

bool DX12Shader::HasErrors() const {
    return m_compileLog.find("error") != std::string::npos;
}

bool DX12Shader::Validate() {
    return !m_bytecode.empty() && m_desc.type != ShaderType::Unknown;
}

std::string DX12Shader::Disassemble() const {
    if (m_bytecode.empty()) {
        return "";
    }

    // 使用DXC反汇编
    ComPtr<IDxcCompiler> compiler;
    ComPtr<IDxcLibrary> library;
    ComPtr<IDxcBlobEncoding> disassembly;
    ComPtr<IDxcBlob> codeBlob;

    HRESULT hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
    if (FAILED(hr)) {
        return "Failed to create DXC compiler for disassembly";
    }

    hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library));
    if (FAILED(hr)) {
        return "Failed to create DXC library for disassembly";
    }

    // 创建字节码blob
    hr = library->CreateBlobWithEncodingFromPinned(
        m_bytecode.data(),
        static_cast<uint32_t>(m_bytecode.size()),
        CP_ACP,
        reinterpret_cast<IDxcBlobEncoding**>(&codeBlob)
    );
    if (FAILED(hr)) {
        return "Failed to create bytecode blob for disassembly";
    }

    // 反汇编
    hr = compiler->Disassemble(codeBlob.Get(), &disassembly);
    if (FAILED(hr)) {
        return "Failed to disassemble shader";
    }

    if (disassembly) {
        return std::string(
            static_cast<char*>(disassembly->GetBufferPointer()),
            disassembly->GetBufferSize()
        );
    }

    return "";
}

bool DX12Shader::DebugSaveToFile(const std::string& filename,
                                bool includeDisassembly, bool includeReflection) const {
    std::ofstream file(filename);
    if (!file) {
        return false;
    }

    // 写入基本信息
    file << "Shader Debug Information\n";
    file << "=======================\n\n";
    file << "Type: " << static_cast<uint32_t>(m_desc.type) << "\n";
    file << "Language: " << static_cast<uint32_t>(m_desc.language) << "\n";
    file << "Entry Point: " << m_desc.entryPoint << "\n";
    file << "Target: " << m_desc.target << "\n";
    file << "Filename: " << m_desc.filename << "\n";
    file << "Bytecode Size: " << m_bytecode.size() << " bytes\n";
    file << "Compile Timestamp: " << m_desc.compileTimestamp << "\n";
    file << "Compile Hash: " << m_desc.compileHash << "\n\n";

    // 写入编译选项
    file << "Compile Options:\n";
    file << "  Optimization Level: " << static_cast<uint32_t>(m_desc.compileOptions.optimizationLevel) << "\n";
    file << "  Flags: " << static_cast<uint32_t>(m_desc.compileOptions.flags) << "\n";
    file << "  Defines:\n";
    for (const auto& define : m_desc.compileOptions.defines) {
        file << "    " << define << "\n";
    }
    file << "\n";

    // 写入编译日志
    if (!m_compileLog.empty()) {
        file << "Compile Log:\n" << m_compileLog << "\n\n";
    }

    // 写入反汇编
    if (includeDisassembly) {
        std::string disassembly = Disassemble();
        file << "Disassembly:\n" << disassembly << "\n\n";
    }

    // 写入反射信息
    if (includeReflection) {
        file << "Reflection Information:\n";
        file << "  Input Parameters: " << m_reflection.inputParameters.size() << "\n";
        file << "  Output Parameters: " << m_reflection.outputParameters.size() << "\n";
        file << "  Resources: " << m_reflection.resources.size() << "\n";
        file << "  Constant Buffers: " << m_reflection.constantBuffers.size() << "\n";
        file << "\n";

        // 详细信息...
    }

    return true;
}

const std::vector<std::string>& DX12Shader::GetDependencies() const {
    return m_desc.compileOptions.dependencies;
}

const std::vector<std::string>& DX12Shader::GetIncludes() const {
    return m_desc.compileOptions.includeDirectories;
}

const std::vector<std::string>& DX12Shader::GetDefines() const {
    return m_desc.compileOptions.defines;
}

// DirectX12特定方法
const void* DX12Shader::GetBytecodeData() const {
    return m_bytecode.data();
}

size_t DX12Shader::GetBytecodeSize() const {
    return m_bytecode.size();
}

} // namespace PrismaEngine::Graphic::DX12