#pragma once

#include <functional>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#ifdef PRISMA_ENABLE_MONO
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/environment.h>
#include <mono/metadata/mono-config.h>
#endif

namespace Prisma {
namespace Scripting {

#ifdef PRISMA_ENABLE_MONO
// 使用实际的Mono类型
using MonoDomainPtr     = ::MonoDomain;
using MonoObjectPtr     = ::MonoObject;
using MonoClassPtr      = ::MonoClass;
using MonoAssemblyPtr   = ::MonoAssembly;
using MonoImagePtr      = ::MonoImage;
using MonoMethodDescPtr = ::MonoMethodDesc;
using MonoMethodPtr     = ::MonoMethod;
#else
// 占位符类型
struct MonoDomain {
    void* ptr;
};
struct MonoObject {
    void* ptr;
};
struct MonoClass {
    void* ptr;
};
struct MonoAssembly {
    void* ptr;
};
struct MonoImage {
    void* ptr;
};
struct MonoMethodDesc {
    void* ptr;
};
struct MonoMethod {
    void* ptr;
};

// 类型别名
using MonoDomainPtr     = MonoDomain;
using MonoObjectPtr     = MonoObject;
using MonoClassPtr      = MonoClass;
using MonoAssemblyPtr   = MonoAssembly;
using MonoImagePtr      = MonoImage;
using MonoMethodDescPtr = MonoMethodDesc;
using MonoMethodPtr     = MonoMethod;
#endif

// Mono对象包装类
class ManagedObject {
public:
    enum class ValueKind : uint8_t {
        None,
        Bool,
        Int,
        Float,
        String,
        Array,
        ScriptInstance,
        MethodResult
    };

    ManagedObject()          = default;
    virtual ~ManagedObject() = default;

    bool IsValid() const { return rawPtr != nullptr || m_kind != ValueKind::None; }

    // 通用方法调用
    template <typename... Args> ManagedObject InvokeMethod(const std::string& methodName, Args&&... args);

    static ManagedObject CreateBool(bool value);
    static ManagedObject CreateInt(int value);
    static ManagedObject CreateFloat(float value);
    static ManagedObject CreateString(const std::string& value);
    static ManagedObject CreateArray(int length);
    static ManagedObject CreateScriptInstance(const std::string& assemblyName,
                                              const std::string& className,
                                              const std::string& scriptPath);
    static ManagedObject CreateMethodResult(const std::string& methodName);

    ValueKind GetKind() const { return m_kind; }
    bool GetBool() const { return m_boolValue; }
    int GetInt() const { return m_intValue; }
    float GetFloat() const { return m_floatValue; }
    const std::string& GetString() const { return m_stringValue; }
    const std::string& GetAssemblyName() const { return m_assemblyName; }
    const std::string& GetTypeName() const { return m_typeName; }
    const std::string& GetScriptPath() const { return m_scriptPath; }
    const std::string& GetLastInvokedMethod() const { return m_lastInvokedMethod; }
    void SetLastInvokedMethod(const std::string& methodName) { m_lastInvokedMethod = methodName; }

    int GetArrayLength() const { return static_cast<int>(m_arrayElements.size()); }
    ManagedObject GetArrayElement(int index) const;
    void SetArrayElement(int index, const ManagedObject& value);

    void* rawPtr = nullptr;

private:
    ValueKind m_kind = ValueKind::None;
    bool m_boolValue = false;
    int m_intValue = 0;
    float m_floatValue = 0.0f;
    std::string m_stringValue;
    std::string m_assemblyName;
    std::string m_typeName;
    std::string m_scriptPath;
    std::string m_lastInvokedMethod;
    std::vector<std::shared_ptr<ManagedObject>> m_arrayElements;
    MonoDomainPtr* m_domain     = nullptr;
    MonoObjectPtr* m_monoObject = nullptr;
    MonoClassPtr* m_class       = nullptr;

    static ManagedObject CreateScalar(ValueKind kind);
    friend class MonoRuntime;
    friend class MonoAssemblyManager;
};

// Mono域管理
class MonoDomainManager {
public:
    MonoDomainManager()  = default;
    ~MonoDomainManager() = default;

    bool Initialize(const std::string& domainName) {
        m_domainName = domainName;
        if (!m_domain) {
            m_domain = std::make_unique<MonoDomain>();
            m_domain->ptr = m_domain.get();
        }
        m_initialized = true;
        return true;
    }
    void Shutdown() {
        m_initialized = false;
        m_domain.reset();
    }
    bool IsInitialized() const { return m_initialized; }
    void* GetNativeDomain() const { return m_domain.get(); }

private:
    bool m_initialized = false;
    std::string m_domainName;
    std::unique_ptr<MonoDomain> m_domain;
};

// 程序集管理
class MonoAssemblyManager {
public:
    MonoAssemblyManager()  = default;
    ~MonoAssemblyManager() = default;

    bool Load(const std::string& assemblyPath) {
        m_assemblyPath = assemblyPath;
        if (!m_assembly) {
            m_assembly = std::make_unique<MonoAssembly>();
            m_assembly->ptr = m_assembly.get();
        }
        if (!m_image) {
            m_image = std::make_unique<MonoImage>();
            m_image->ptr = m_image.get();
        }
        m_loaded = true;
        return true;
    }
    bool IsLoaded() const { return m_loaded; }
    ManagedObject CreateInstance(const std::string& className) {
        if (!m_loaded) {
            return ManagedObject();
        }
        return ManagedObject::CreateScriptInstance(m_assemblyPath, className, m_assemblyPath);
    }

private:
    bool m_loaded = false;
    std::string m_assemblyPath;
    std::unique_ptr<MonoAssembly> m_assembly;
    std::unique_ptr<MonoImage> m_image;
};

// Mono运行时管理器
class MonoRuntime {
public:
    MonoRuntime()  = default;
    ~MonoRuntime() = default;

    static MonoRuntime& Get();

    // 初始化和清理
    bool Initialize(const std::string& configPath = "");
    void Shutdown();
    bool IsInitialized() const;

    // 程序集管理
    bool LoadAssembly(const std::string& assemblyName, const std::string& path);
    ManagedObject CreateInstance(const std::string& assemblyName, const std::string& className);

    // 调用方法
    template <typename... Args>
    ManagedObject InvokeMethod(ManagedObject& instance, const std::string& methodName, Args&&... args);

    // 类型转换
    ManagedObject StringToMono(const std::string& str);
    std::string MonoToString(ManagedObject& obj);
    ManagedObject IntToMono(int value);
    int MonoToInt(ManagedObject& obj);
    ManagedObject FloatToMono(float value);
    float MonoToFloat(ManagedObject& obj);

    // 垃圾回收
    void CollectGarbage();

    // 创建脚本实例
    ManagedObject CreateScript(const std::string& scriptPath);
    ManagedObject BoolToMono(bool value);
    bool MonoToBool(ManagedObject& obj);

    // 数组操作
    ManagedObject CreateArray(int length);
    int GetArrayLength(ManagedObject& array);
    ManagedObject GetArrayElement(ManagedObject& array, int index);
    void SetArrayElement(ManagedObject& array, int index, ManagedObject& value);

    // 域管理
    MonoDomain* CreateDomain(const std::string& domainName);
    void UnloadDomain(MonoDomain* domain);
    MonoDomain* GetRootDomain() const;

    // 异常处理
    bool HasException();
    std::string GetExceptionMessage();
    void ClearException();

    // 配置
    void SetSearchPaths(const std::vector<std::string>& paths);
    void RegisterInternalCall(const std::string& signature, void* function);

private:
    ManagedObject InvokeMethodImpl(ManagedObject& instance, const std::string& methodName, const std::vector<ManagedObject>& args);
    static ManagedObject PackArgument(const ManagedObject& value);
    static ManagedObject PackArgument(const std::shared_ptr<ManagedObject>& value);
    static ManagedObject PackArgument(const std::string& value);
    static ManagedObject PackArgument(const char* value);
    static ManagedObject PackArgument(bool value);
    static ManagedObject PackArgument(int value);
    static ManagedObject PackArgument(float value);
    static ManagedObject PackArgument(const float* value);
    static ManagedObject PackArgument(float* value);
    static ManagedObject PackArgument(const int* value);
    static ManagedObject PackArgument(int* value);

    bool m_initialized = false;
    std::unordered_map<std::string, std::string> m_assemblies;
    std::vector<std::unique_ptr<MonoDomain>> m_domains;
    std::vector<std::string> m_searchPaths;
    std::vector<std::string> m_internalCalls;
    std::string m_lastExceptionMessage;
};

template <typename... Args>
ManagedObject ManagedObject::InvokeMethod(const std::string& methodName, Args&&... args) {
    return MonoRuntime::Get().InvokeMethod(*this, methodName, std::forward<Args>(args)...);
}

template <typename... Args>
ManagedObject MonoRuntime::InvokeMethod(ManagedObject& instance, const std::string& methodName, Args&&... args) {
    const std::vector<ManagedObject> packedArgs = {PackArgument(std::forward<Args>(args))...};
    return InvokeMethodImpl(instance, methodName, packedArgs);
}

}  // namespace Scripting
}  // namespace Prisma
