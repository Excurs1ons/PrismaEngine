#include "MonoRuntime.h"
#include "Logger.h"
#include "core/ECS.h"
#include <algorithm>
#include <filesystem>
#include <iterator>

namespace Prisma {
namespace Scripting {

ManagedObject ManagedObject::CreateScalar(ValueKind kind) {
    ManagedObject object;
    object.m_kind = kind;
    object.rawPtr = &object.m_kind;
    return object;
}

ManagedObject ManagedObject::CreateBool(bool value) {
    ManagedObject object = CreateScalar(ValueKind::Bool);
    object.m_boolValue = value;
    return object;
}

ManagedObject ManagedObject::CreateInt(int value) {
    ManagedObject object = CreateScalar(ValueKind::Int);
    object.m_intValue = value;
    object.m_floatValue = static_cast<float>(value);
    return object;
}

ManagedObject ManagedObject::CreateFloat(float value) {
    ManagedObject object = CreateScalar(ValueKind::Float);
    object.m_floatValue = value;
    object.m_intValue = static_cast<int>(value);
    return object;
}

ManagedObject ManagedObject::CreateString(const std::string& value) {
    ManagedObject object = CreateScalar(ValueKind::String);
    object.m_stringValue = value;
    return object;
}

ManagedObject ManagedObject::CreateArray(int length) {
    ManagedObject object = CreateScalar(ValueKind::Array);
    object.m_arrayElements.resize(std::max(length, 0));
    return object;
}

ManagedObject ManagedObject::CreateScriptInstance(const std::string& assemblyName,
                                                  const std::string& className,
                                                  const std::string& scriptPath) {
    ManagedObject object = CreateScalar(ValueKind::ScriptInstance);
    object.m_assemblyName = assemblyName;
    object.m_typeName = className;
    object.m_scriptPath = scriptPath;
    object.m_stringValue = className;
    return object;
}

ManagedObject ManagedObject::CreateMethodResult(const std::string& methodName) {
    ManagedObject object = CreateScalar(ValueKind::MethodResult);
    object.m_stringValue = methodName;
    object.m_lastInvokedMethod = methodName;
    return object;
}

ManagedObject ManagedObject::GetArrayElement(int index) const {
    if (m_kind != ValueKind::Array || index < 0 || index >= static_cast<int>(m_arrayElements.size())) {
        return ManagedObject();
    }
    const auto& element = m_arrayElements[static_cast<size_t>(index)];
    return element ? *element : ManagedObject();
}

void ManagedObject::SetArrayElement(int index, const ManagedObject& value) {
    if (m_kind != ValueKind::Array || index < 0 || index >= static_cast<int>(m_arrayElements.size())) {
        return;
    }
    m_arrayElements[static_cast<size_t>(index)] = std::make_shared<ManagedObject>(value);
}

MonoRuntime& MonoRuntime::Get() {
    static MonoRuntime instance;
    return instance;
}

bool MonoRuntime::Initialize(const std::string& configPath) {
#ifdef PRISMA_ENABLE_MONO
    LOG_INFO("MonoRuntime", "正在使用配置初始化 Mono: {0}", configPath);
    m_initialized = true;
    return true;
#else
    if (configPath.empty()) {
        LOG_INFO("MonoRuntime", "Mono support is disabled. Using native fallback scripting objects.");
    } else {
        LOG_INFO("MonoRuntime", "Mono support is disabled. Using native fallback scripting objects with config hint: {0}", configPath);
    }

    if (m_domains.empty()) {
        auto rootDomain = std::make_unique<MonoDomain>();
        rootDomain->ptr = rootDomain.get();
        m_domains.push_back(std::move(rootDomain));
    }

    m_lastExceptionMessage.clear();
    m_initialized = true;
    return true;
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
    m_assemblies[assemblyName] = path;
    if (std::filesystem::exists(path)) {
        LOG_INFO("MonoRuntime", "Registered fallback assembly {0} from {1}", assemblyName, path);
    } else {
        LOG_WARNING("MonoRuntime", "Assembly file not found for {0}: {1}. Registered logical fallback assembly only.", assemblyName, path);
    }
    return true;
#endif
}

ManagedObject MonoRuntime::CreateInstance(const std::string& assemblyName, const std::string& className) {
    if (!m_initialized) {
        m_lastExceptionMessage = "MonoRuntime not initialized";
        return ManagedObject();
    }

    auto assemblyIt = m_assemblies.find(assemblyName);
    if (assemblyIt == m_assemblies.end()) {
        m_lastExceptionMessage = "Assembly not loaded: " + assemblyName;
        LOG_WARNING("MonoRuntime", "Cannot create instance of {0} from missing assembly {1}", className, assemblyName);
        return ManagedObject();
    }

    LOG_DEBUG("MonoRuntime", "Creating instance of {0} from assembly {1}", className, assemblyName);
    return ManagedObject::CreateScriptInstance(assemblyName, className, assemblyIt->second);
}

ManagedObject MonoRuntime::StringToMono(const std::string& str) {
    return ManagedObject::CreateString(str);
}

std::string MonoRuntime::MonoToString(ManagedObject& obj) {
    switch (obj.GetKind()) {
        case ManagedObject::ValueKind::String:
        case ManagedObject::ValueKind::MethodResult:
            return obj.GetString();
        case ManagedObject::ValueKind::ScriptInstance:
            return obj.GetScriptPath().empty() ? obj.GetTypeName() : obj.GetScriptPath();
        case ManagedObject::ValueKind::Bool:
            return obj.GetBool() ? "true" : "false";
        case ManagedObject::ValueKind::Int:
            return std::to_string(obj.GetInt());
        case ManagedObject::ValueKind::Float:
            return std::to_string(obj.GetFloat());
        default:
            return {};
    }
}

ManagedObject MonoRuntime::IntToMono(int value) {
    return ManagedObject::CreateInt(value);
}

int MonoRuntime::MonoToInt(ManagedObject& obj) {
    switch (obj.GetKind()) {
        case ManagedObject::ValueKind::Bool:
            return obj.GetBool() ? 1 : 0;
        case ManagedObject::ValueKind::Float:
            return static_cast<int>(obj.GetFloat());
        case ManagedObject::ValueKind::Int:
            return obj.GetInt();
        default:
            return 0;
    }
}

ManagedObject MonoRuntime::FloatToMono(float value) {
    return ManagedObject::CreateFloat(value);
}

float MonoRuntime::MonoToFloat(ManagedObject& obj) {
    switch (obj.GetKind()) {
        case ManagedObject::ValueKind::Bool:
            return obj.GetBool() ? 1.0f : 0.0f;
        case ManagedObject::ValueKind::Int:
            return static_cast<float>(obj.GetInt());
        case ManagedObject::ValueKind::Float:
            return obj.GetFloat();
        default:
            return 0.0f;
    }
}

ManagedObject MonoRuntime::BoolToMono(bool value) {
    return ManagedObject::CreateBool(value);
}

bool MonoRuntime::MonoToBool(ManagedObject& obj) {
    switch (obj.GetKind()) {
        case ManagedObject::ValueKind::Bool:
            return obj.GetBool();
        case ManagedObject::ValueKind::Int:
            return obj.GetInt() != 0;
        case ManagedObject::ValueKind::Float:
            return obj.GetFloat() != 0.0f;
        case ManagedObject::ValueKind::String:
            return !obj.GetString().empty();
        default:
            return obj.IsValid();
    }
}

ManagedObject MonoRuntime::CreateArray(int length) {
    LOG_DEBUG("MonoRuntime", "Creating array of length {0}", length);
    return ManagedObject::CreateArray(length);
}

int MonoRuntime::GetArrayLength(ManagedObject& array) {
    return array.GetArrayLength();
}

ManagedObject MonoRuntime::GetArrayElement(ManagedObject& array, int index) {
    return array.GetArrayElement(index);
}

void MonoRuntime::SetArrayElement(ManagedObject& array, int index, ManagedObject& value) {
    array.SetArrayElement(index, value);
}

MonoDomain* MonoRuntime::CreateDomain(const std::string& domainName) {
    LOG_INFO("MonoRuntime", "Creating app domain: {0}", domainName);
    auto domain = std::make_unique<MonoDomain>();
    domain->ptr = domain.get();
    MonoDomain* rawDomain = domain.get();
    m_domains.push_back(std::move(domain));
    return rawDomain;
}

void MonoRuntime::UnloadDomain(MonoDomain* domain) {
    if (domain) {
        const auto oldSize = m_domains.size();
        m_domains.erase(std::remove_if(m_domains.begin(),
                                       m_domains.end(),
                                       [domain](const auto& ownedDomain) { return ownedDomain.get() == domain; }),
                        m_domains.end());
        if (m_domains.size() != oldSize) {
            LOG_INFO("MonoRuntime", "正在卸载域");
        }
    }
}

MonoDomain* MonoRuntime::GetRootDomain() const {
    return m_domains.empty() ? nullptr : m_domains.front().get();
}

bool MonoRuntime::HasException() {
    return !m_lastExceptionMessage.empty();
}

std::string MonoRuntime::GetExceptionMessage() {
    return m_lastExceptionMessage;
}

void MonoRuntime::ClearException() {
    m_lastExceptionMessage.clear();
}

void MonoRuntime::SetSearchPaths(const std::vector<std::string>& paths) {
    m_searchPaths = paths;
}

void MonoRuntime::RegisterInternalCall(const std::string& signature, void* function) {
    LOG_DEBUG("MonoRuntime", "正在注册内部调用: {0}", signature);
    if (!function) {
        LOG_WARNING("MonoRuntime", "内部调用 {0} 已注册，但函数指针为空", signature);
    }
    m_internalCalls.push_back(signature);
}

void MonoRuntime::CollectGarbage() {
    LOG_DEBUG("MonoRuntime", "触发垃圾回收");
#ifdef PRISMA_ENABLE_MONO
    if (m_initialized) {
        mono_gc_collect(mono_gc_max_generation());
    }
#endif
}

ManagedObject MonoRuntime::CreateScript(const std::string& scriptPath) {
    LOG_INFO("MonoRuntime", "正在为以下路径创建脚本对象: {0}", scriptPath);
    const auto scriptFile = std::filesystem::path(scriptPath);
    const std::string className = scriptFile.stem().empty() ? scriptPath : scriptFile.stem().string();

    std::string assemblyName = "assembly";
    if (!m_assemblies.empty()) {
        assemblyName = m_assemblies.begin()->first;
    }

    auto instance = CreateInstance(assemblyName, className);
    if (!instance.IsValid()) {
        instance = ManagedObject::CreateScriptInstance(assemblyName, className, scriptPath);
    }
    instance.m_scriptPath = scriptPath;
    return instance;
}

ManagedObject MonoRuntime::InvokeMethodImpl(ManagedObject& instance,
                                            const std::string& methodName,
                                            const std::vector<ManagedObject>& args) {
    if (!instance.IsValid()) {
        m_lastExceptionMessage = "Attempted to invoke " + methodName + " on an invalid managed object";
        return ManagedObject();
    }

    instance.SetLastInvokedMethod(methodName);
    ClearException();

    ManagedObject result = ManagedObject::CreateMethodResult(methodName);
    result.m_assemblyName = instance.m_assemblyName;
    result.m_typeName = instance.m_typeName;
    result.m_scriptPath = instance.m_scriptPath;

    if (!args.empty()) {
        ManagedObject argArray = ManagedObject::CreateArray(static_cast<int>(args.size()));
        for (int index = 0; index < static_cast<int>(args.size()); ++index) {
            argArray.SetArrayElement(index, args[static_cast<size_t>(index)]);
        }
        result.m_arrayElements.push_back(std::make_shared<ManagedObject>(argArray));
    }

    return result;
}

ManagedObject MonoRuntime::PackArgument(const ManagedObject& value) {
    return value;
}

ManagedObject MonoRuntime::PackArgument(const std::shared_ptr<ManagedObject>& value) {
    return value ? *value : ManagedObject();
}

ManagedObject MonoRuntime::PackArgument(const std::string& value) {
    return ManagedObject::CreateString(value);
}

ManagedObject MonoRuntime::PackArgument(const char* value) {
    return value ? ManagedObject::CreateString(value) : ManagedObject();
}

ManagedObject MonoRuntime::PackArgument(bool value) {
    return ManagedObject::CreateBool(value);
}

ManagedObject MonoRuntime::PackArgument(int value) {
    return ManagedObject::CreateInt(value);
}

ManagedObject MonoRuntime::PackArgument(float value) {
    return ManagedObject::CreateFloat(value);
}

ManagedObject MonoRuntime::PackArgument(const float* value) {
    return value ? ManagedObject::CreateFloat(*value) : ManagedObject();
}

ManagedObject MonoRuntime::PackArgument(float* value) {
    return PackArgument(static_cast<const float*>(value));
}

ManagedObject MonoRuntime::PackArgument(const int* value) {
    return value ? ManagedObject::CreateInt(*value) : ManagedObject();
}

ManagedObject MonoRuntime::PackArgument(int* value) {
    return PackArgument(static_cast<const int*>(value));
}

} // namespace Scripting
} // namespace Prisma
