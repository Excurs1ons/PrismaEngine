#include "MonoRuntime.h"
#include "Logger.h"
#include "core/ECS.h"
#include <iostream>

namespace Prisma {
namespace Scripting {

MonoRuntime& MonoRuntime::Get() {
    static MonoRuntime instance;
    return instance;
}

bool MonoRuntime::Initialize(const std::string& configPath) {
#ifdef PRISMA_ENABLE_MONO
    LOG_INFO("MonoRuntime", "Initializing Mono with config: {0}", configPath);
    m_initialized = true;
    return true;
#else
    (void)configPath;
    LOG_INFO("MonoRuntime", "Mono support is disabled. Scripting system will not be available.");
    return false;
#endif
}

void MonoRuntime::Shutdown() {
    m_initialized = false;
    m_assemblies.clear();
    m_domains.clear();
    LOG_INFO("MonoRuntime", "Mono runtime shutdown completed");
}

bool MonoRuntime::IsInitialized() const {
    return m_initialized;
}

bool MonoRuntime::LoadAssembly(const std::string& assemblyName, const std::string& path) {
    if (!m_initialized) {
        LOG_ERROR("MonoRuntime", "MonoRuntime not initialized, cannot load: {0}", assemblyName);
        return false;
    }

#ifdef PRISMA_ENABLE_MONO
    LOG_INFO("MonoRuntime", "Loading assembly: {0} from {1}", assemblyName, path);
    m_assemblies[assemblyName] = path;
    return true;
#else
    (void)path;
    LOG_WARNING("MonoRuntime", "Mono support is disabled, cannot load assembly: {0}", assemblyName);
    return false;
#endif
}

ManagedObject MonoRuntime::CreateInstance(const std::string& assemblyName, const std::string& className) {
    LOG_DEBUG("MonoRuntime", "Creating instance of {0} from assembly {1}", className, assemblyName);
    return ManagedObject();
}

ManagedObject MonoRuntime::StringToMono(const std::string& str) {
    if (str.empty()) return ManagedObject();
    return ManagedObject();
}

std::string MonoRuntime::MonoToString(ManagedObject& obj) {
    if (!obj.rawPtr) return "";
    return "MonoObject";
}

ManagedObject MonoRuntime::IntToMono(int value) {
    (void)value;
    return ManagedObject();
}

int MonoRuntime::MonoToInt(ManagedObject& obj) {
    (void)obj;
    return 0;
}

ManagedObject MonoRuntime::FloatToMono(float value) {
    (void)value;
    return ManagedObject();
}

float MonoRuntime::MonoToFloat(ManagedObject& obj) {
    (void)obj;
    return 0.0f;
}

ManagedObject MonoRuntime::BoolToMono(bool value) {
    (void)value;
    return ManagedObject();
}

bool MonoRuntime::MonoToBool(ManagedObject& obj) {
    (void)obj;
    return false;
}

ManagedObject MonoRuntime::CreateArray(int length) {
    LOG_DEBUG("MonoRuntime", "Creating array of length {0}", length);
    return ManagedObject();
}

int MonoRuntime::GetArrayLength(ManagedObject& array) {
    (void)array;
    return 0;
}

ManagedObject MonoRuntime::GetArrayElement(ManagedObject& array, int index) {
    (void)array;
    (void)index;
    return ManagedObject();
}

void MonoRuntime::SetArrayElement(ManagedObject& array, int index, ManagedObject& value) {
    (void)array;
    (void)index;
    (void)value;
}

MonoDomain* MonoRuntime::CreateDomain(const std::string& domainName) {
    LOG_INFO("MonoRuntime", "Creating app domain: {0}", domainName);
    return nullptr;
}

void MonoRuntime::UnloadDomain(MonoDomain* domain) {
    if (domain) {
        LOG_INFO("MonoRuntime", "Unloading domain");
    }
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
}

void MonoRuntime::SetSearchPaths(const std::vector<std::string>& paths) {
    m_searchPaths = paths;
}

void MonoRuntime::RegisterInternalCall(const std::string& signature, void* function) {
    LOG_DEBUG("MonoRuntime", "Registering internal call: {0}", signature);
    (void)function;
}

void MonoRuntime::CollectGarbage() {
    LOG_DEBUG("MonoRuntime", "Garbage collection triggered");
#ifdef PRISMA_ENABLE_MONO
    if (m_initialized) {
        mono_gc_collect(mono_gc_max_generation());
    }
#endif
}

ManagedObject MonoRuntime::CreateScript(const std::string& scriptPath) {
    LOG_INFO("MonoRuntime", "Creating script object for: {0}", scriptPath);
    return ManagedObject();
}

} // namespace Scripting
} // namespace Prisma