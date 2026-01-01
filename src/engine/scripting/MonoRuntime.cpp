#include "MonoRuntime.h"
#include "Logger.h"
#include "core/ECS.h"
#include <iostream>

namespace PrismaEngine {
namespace Scripting {

MonoRuntime& MonoRuntime::GetInstance() {
    static MonoRuntime instance;
    return instance;
}

bool MonoRuntime::Initialize(const std::string& configPath) {
#ifdef PRISMA_ENABLE_MONO
    // 实际的Mono初始化代码
    return true;
#else
    LOG_INFO("MonoRuntime", "Mono support is disabled. Scripting system will not be available.");
    return false;
#endif
}

void MonoRuntime::Shutdown() {
    m_initialized = false;
    m_assemblies.clear();
    m_domains.clear();
}

bool MonoRuntime::IsInitialized() const {
    return m_initialized;
}

bool MonoRuntime::LoadAssembly(const std::string& assemblyName, const std::string& path) {
    if (!m_initialized) {
        LOG_ERROR("MonoRuntime", "MonoRuntime not initialized");
        return false;
    }

#ifdef PRISMA_ENABLE_MONO
    // 实际的加载代码
    return true;
#else
    LOG_WARNING("MonoRuntime", "Mono support is disabled, cannot load assembly: {0}", assemblyName);
    return false;
#endif
}

ManagedObject MonoRuntime::CreateInstance(const std::string& assemblyName, const std::string& className) {
    return ManagedObject();
}

ManagedObject MonoRuntime::StringToMono(const std::string& str) {
    return ManagedObject();
}

std::string MonoRuntime::MonoToString(ManagedObject& obj) {
    return "";
}

ManagedObject MonoRuntime::IntToMono(int value) {
    return ManagedObject();
}

int MonoRuntime::MonoToInt(ManagedObject& obj) {
    return 0;
}

ManagedObject MonoRuntime::FloatToMono(float value) {
    return ManagedObject();
}

float MonoRuntime::MonoToFloat(ManagedObject& obj) {
    return 0.0f;
}

ManagedObject MonoRuntime::BoolToMono(bool value) {
    return ManagedObject();
}

bool MonoRuntime::MonoToBool(ManagedObject& obj) {
    return false;
}

ManagedObject MonoRuntime::CreateArray(int length) {
    return ManagedObject();
}

int MonoRuntime::GetArrayLength(ManagedObject& array) {
    return 0;
}

ManagedObject MonoRuntime::GetArrayElement(ManagedObject& array, int index) {
    return ManagedObject();
}

void MonoRuntime::SetArrayElement(ManagedObject& array, int index, ManagedObject& value) {
    // 空实现
}

MonoDomain* MonoRuntime::CreateDomain(const std::string& domainName) {
    return nullptr;
}

void MonoRuntime::UnloadDomain(MonoDomain* domain) {
    // 空实现
}

MonoDomain* MonoRuntime::GetRootDomain() const {
    return nullptr;
}

bool MonoRuntime::HasException() {
    return false;
}

std::string MonoRuntime::GetExceptionMessage() {
    return "";
}

void MonoRuntime::ClearException() {
    // 空实现
}

void MonoRuntime::SetSearchPaths(const std::vector<std::string>& paths) {
    m_searchPaths = paths;
}

void MonoRuntime::RegisterInternalCall(const std::string& signature, void* function) {
    // 空实现
}

void MonoRuntime::CollectGarbage() {
    // 空实现
#ifdef PRISMA_ENABLE_MONO
    // TODO: 实现实际的垃圾回收
#endif
}

ManagedObject MonoRuntime::CreateScript(const std::string& scriptPath) {
    // 创建空的脚本对象
    return ManagedObject();
}

} // namespace Scripting
} // namespace Engine