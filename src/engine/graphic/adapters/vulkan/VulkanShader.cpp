#include "VulkanShader.h"
#include "RenderDeviceVulkan.h"
#include <fstream>

namespace PrismaEngine {
    namespace Graphic {
        namespace Vulkan {


            VulkanShader::VulkanShader(RenderDeviceVulkan *device,
                                       const ShaderDesc &desc,
                                       const std::vector <uint32_t> &spirv,
                                       const ShaderReflection &reflection)
                    : m_device(device), m_desc(desc), m_spirv(spirv), m_reflection(reflection) {

                // 转换SPIR-V到字节码向量以兼容IShader接口
                m_bytecode.resize(m_spirv.size() * sizeof(uint32_t));
                std::memcpy(m_bytecode.data(), m_spirv.data(), m_bytecode.size());

                CreateShaderModule();
            }

            VulkanShader::~VulkanShader() {
                DestroyShaderModule();
            }

            ShaderType VulkanShader::GetShaderType() const {
                return m_desc.type;
            }

            ShaderLanguage VulkanShader::GetLanguage() const {
                return ShaderLanguage::SPIRV;
            }

            const std::string &VulkanShader::GetEntryPoint() const {
                return m_desc.entryPoint;
            }

            const std::string &VulkanShader::GetTarget() const {
                return m_desc.target;
            }

            const std::string &VulkanShader::GetSource() const {
                return m_desc.source;
            }

            const std::vector <uint8_t> &VulkanShader::GetBytecode() const {
                return m_bytecode;
            }

            const std::string &VulkanShader::GetFilename() const {
                return m_desc.filename;
            }

            uint64_t VulkanShader::GetCompileTimestamp() const {
                return m_desc.compileTimestamp;
            }

            uint64_t VulkanShader::GetCompileHash() const {
                return m_desc.compileHash;
            }

            const ShaderCompileOptions &VulkanShader::GetCompileOptions() const {
                return m_desc.compileOptions;
            }

            const ShaderReflection &VulkanShader::GetReflection() const {
                return m_reflection;
            }

            bool VulkanShader::HasReflection() const {
                return !m_reflection.resources.empty() || !m_reflection.constantBuffers.empty();
            }

            const ShaderReflection::Resource *
            VulkanShader::FindResource(const std::string &name) const {
                for (const auto &resource: m_reflection.resources) {
                    if (resource.name == name) {
                        return &resource;
                    }
                }
                return nullptr;
            }

            const ShaderReflection::Resource *
            VulkanShader::FindResourceByBindPoint(uint32_t bindPoint, uint32_t space) const {
                for (const auto &resource: m_reflection.resources) {
                    if (resource.bindPoint == bindPoint && resource.space == space) {
                        return &resource;
                    }
                }
                return nullptr;
            }

            const ShaderReflection::ConstantBuffer *
            VulkanShader::FindConstantBuffer(const std::string &name) const {
                for (const auto &cb: m_reflection.constantBuffers) {
                    if (cb.name == name) {
                        return &cb;
                    }
                }
                return nullptr;
            }

            uint32_t VulkanShader::GetInputParameterCount() const {
                return static_cast<uint32_t>(m_reflection.inputs.size());
            }

            const ShaderReflection::InputParameter &
            VulkanShader::GetInputParameter(uint32_t index) const {
                return m_reflection.inputs[index];
            }

            uint32_t VulkanShader::GetOutputParameterCount() const {
                return static_cast<uint32_t>(m_reflection.outputs.size());
            }

            const ShaderReflection::OutputParameter &
            VulkanShader::GetOutputParameter(uint32_t index) const {
                return m_reflection.outputs[index];
            }

            bool VulkanShader::Recompile(const ShaderCompileOptions *options, std::string &errors) {
                // TODO: 实现重新编译逻辑
                errors = "Vulkan shader recompilation not yet implemented";
                return false;
            }

            bool VulkanShader::RecompileFromSource(const std::string &source,
                                                   const ShaderCompileOptions *options,
                                                   std::string &errors) {
                // TODO: 实现从源码重新编译
                errors = "Vulkan shader recompilation from source not yet implemented";
                return false;
            }

            bool VulkanShader::ReloadFromFile(std::string &errors) {
                // TODO: 实现从文件重新加载
                errors = "Vulkan shader reload not yet implemented";
                return false;
            }

            void VulkanShader::EnableHotReload(bool enable) {
                m_hotReloadEnabled = enable;
            }

            bool VulkanShader::IsFileModified() const {
                // TODO: 实现文件修改检测
                return false;
            }

            bool VulkanShader::NeedsReload() const {
                // TODO: 实现重新加载需求检测
                return false;
            }

            uint64_t VulkanShader::GetFileModificationTime() const {
                return m_fileModificationTime;
            }

            const std::string &VulkanShader::GetCompileLog() const {
                return m_compileLog;
            }

            bool VulkanShader::HasWarnings() const {
                return m_compileLog.find("warning") != std::string::npos;
            }

            bool VulkanShader::HasErrors() const {
                return m_compileLog.find("error") != std::string::npos;
            }

            bool VulkanShader::Validate() {
                // TODO: 实现着色器验证
                return m_shaderModule != VK_NULL_HANDLE;
            }

            std::string VulkanShader::Disassemble() const {
                // TODO: 使用SPIRV-Tools反汇编
                return "Disassembly not yet implemented";
            }

            bool VulkanShader::DebugSaveToFile(const std::string &filename,
                                               bool includeDisassembly,
                                               bool includeReflection) const {
                std::ofstream file(filename, std::ios::binary);
                if (!file) {
                    return false;
                }

                // 保存SPIR-V字节码
                file.write(reinterpret_cast<const char *>(m_spirv.data()),
                           m_spirv.size() * sizeof(uint32_t));

                return true;
            }

            const std::vector <std::string> &VulkanShader::GetDependencies() const {
                return m_desc.dependencies;
            }

            const std::vector <std::string> &VulkanShader::GetIncludes() const {
                return m_desc.includes;
            }

            const std::vector <std::string> &VulkanShader::GetDefines() const {
                return m_desc.defines;
            }

            VkShaderStageFlagBits VulkanShader::GetVkShaderStage() const {
                switch (m_desc.type) {
                    case ShaderType::Vertex:
                        return VK_SHADER_STAGE_VERTEX_BIT;
                    case ShaderType::Pixel:
                        return VK_SHADER_STAGE_FRAGMENT_BIT;
                    case ShaderType::Geometry:
                        return VK_SHADER_STAGE_GEOMETRY_BIT;
                    case ShaderType::Hull:
                        return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
                    case ShaderType::Domain:
                        return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
                    case ShaderType::Compute:
                        return VK_SHADER_STAGE_COMPUTE_BIT;
                    default:
                        return VK_SHADER_STAGE_ALL;
                }
            }

            bool VulkanShader::CreateShaderModule() {
                if (!m_device || m_spirv.empty()) {
                    return false;
                }

                VkShaderModuleCreateInfo createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
                createInfo.codeSize = m_spirv.size() * sizeof(uint32_t);
                createInfo.pCode = m_spirv.data();

//                VkResult result = vkCreateShaderModule(
//                        m_device->GetDevice(),
//                        &createInfo,
//                        nullptr,
//                        &m_shaderModule
//                );
//
//                if (result != VK_SUCCESS) {
//                    m_compileLog =
//                            "Failed to create Vulkan shader module: " + std::to_string(result);
//                    return false;
//                }

                return true;
            }

            void VulkanShader::DestroyShaderModule() {
//                if (m_shaderModule != VK_NULL_HANDLE && m_device) {
//                    vkDestroyShaderModule(
//                            m_device->GetDevice(),
//                            m_shaderModule,
//                            nullptr
//                    );
//                    m_shaderModule = VK_NULL_HANDLE;
//                }
            }

            VkDevice VulkanShader::GetNativeDevice() const {
                return
                //m_device ?
                //m_device->GetDevice() :
                VK_NULL_HANDLE;
            }

        }
    }
}