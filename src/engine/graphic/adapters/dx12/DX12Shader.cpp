#include "DX12Shader.h"
#include "DX12RenderDevice.h"
#include "../Logger.h"
#include <directx/d3d12shader.h>
// TODO: Use DXC for shader compilation when available
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
    return m_reflection.inputs.size() > 0 ||
           m_reflection.outputs.size() > 0 ||
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
    return static_cast<uint32_t>(m_reflection.inputs.size());
}

const ShaderReflection::InputParameter& DX12Shader::GetInputParameter(uint32_t index) const {
    return m_reflection.inputs[index];
}

uint32_t DX12Shader::GetOutputParameterCount() const {
    return static_cast<uint32_t>(m_reflection.outputs.size());
}

const ShaderReflection::OutputParameter& DX12Shader::GetOutputParameter(uint32_t index) const {
    return m_reflection.outputs[index];
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
        if (!errors.empty()) errors = "Device not available";
        return false;
    }

    // TODO: Implement proper shader compilation when DXC is available
    if (!errors.empty()) errors = "Shader compilation not implemented yet - please provide pre-compiled bytecode";
    return false;
}

bool DX12Shader::ReloadFromFile(std::string& errors) {
    if (m_desc.filename.empty()) {
        if (!errors.empty()) errors = "No filename available for reload";
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

    // TODO: Implement disassembly when DXC is available
    return "Disassembly not implemented yet";
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
    for (const auto& define : m_desc.compileOptions.additionalDefines) {
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
        file << "  Input Parameters: " << m_reflection.inputs.size() << "\n";
        file << "  Output Parameters: " << m_reflection.outputs.size() << "\n";
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
    return m_desc.compileOptions.additionalDefines;
}

// DirectX12特定方法
const void* DX12Shader::GetBytecodeData() const {
    return m_bytecode.data();
}

size_t DX12Shader::GetBytecodeSize() const {
    return m_bytecode.size();
}

} // namespace PrismaEngine::Graphic::DX12