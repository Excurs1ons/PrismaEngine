#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <functional>

#ifdef PRISMA_ENABLE_MONO
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/environment.h>
#include <mono/metadata/mono-config.h>
#endif

namespace Engine {
namespace Scripting {

#ifdef PRISMA_ENABLE_MONO
// 使用实际的Mono类型
using MonoDomainPtr = ::MonoDomain;
using MonoObjectPtr = ::MonoObject;
using MonoClassPtr = ::MonoClass;
using MonoAssemblyPtr = ::MonoAssembly;
using MonoImagePtr = ::MonoImage;
using MonoMethodDescPtr = ::MonoMethodDesc;
using MonoMethodPtr = ::MonoMethod;
#else
// 占位符类型
struct MonoDomain { void* ptr; };
struct MonoObject { void* ptr; };
struct MonoClass { void* ptr; };
struct MonoAssembly { void* ptr; };
struct MonoImage { void* ptr; };
struct MonoMethodDesc { void* ptr; };
struct MonoMethod { void* ptr; };

// 类型别名
using MonoDomainPtr = MonoDomain;
using MonoObjectPtr = MonoObject;
using MonoClassPtr = MonoClass;
using MonoAssemblyPtr = MonoAssembly;
using MonoImagePtr = MonoImage;
using MonoMethodDescPtr = MonoMethodDesc;
using MonoMethodPtr = MonoMethod;
#endif

// Mono对象包装类
class ManagedObject {
public:
    ManagedObject() = default;
    virtual ~ManagedObject() = default;

    bool IsValid() const { return m_monoObject != nullptr; }

    // 通用方法调用
    template<typename... Args>
    ManagedObject InvokeMethod(const std::string& methodName, Args&&... args) {
        return ManagedObject(); // 空实现
    }

private:
    MonoDomainPtr* m_domain = nullptr;
    MonoObjectPtr* m_monoObject = nullptr;
    MonoClassPtr* m_class = nullptr;
};

// Mono域管理
class MonoDomainManager {
public:
    MonoDomainManager() = default;
    ~MonoDomainManager() = default;

    bool Initialize(const std::string& domainName) { return false; }
    void Shutdown() {}
    bool IsInitialized() const { return false; }
    void* GetNativeDomain() const { return nullptr; }

private:
    MonoDomainPtr* m_domain = nullptr;
};

// 程序集管理
class MonoAssemblyManager {
public:
    MonoAssemblyManager() = default;
    ~MonoAssemblyManager() = default;

    bool Load(const std::string& assemblyPath) { return false; }
    bool IsLoaded() const { return false; }
    ManagedObject CreateInstance(const std::string& className) { return ManagedObject(); }

private:
    MonoAssemblyPtr* m_assembly = nullptr;
    MonoImagePtr* m_image = nullptr;
};

// Mono运行时管理器
class MonoRuntime {
public:
    MonoRuntime() = default;
    ~MonoRuntime() = default;

    static MonoRuntime& GetInstance();

    // 初始化和清理
    bool Initialize(const std::string& configPath = "");
    void Shutdown();
    bool IsInitialized() const;

    // 程序集管理
    bool LoadAssembly(const std::string& assemblyName, const std::string& path);
    ManagedObject CreateInstance(const std::string& assemblyName, const std::string& className);

    // 调用方法
    template<typename... Args>
    ManagedObject InvokeMethod(ManagedObject& instance, const std::string& methodName, Args&&... args) {
        return ManagedObject();
    }

    // 类型转换
    ManagedObject StringToMono(const std::string& str);
    std::string MonoToString(ManagedObject& obj);
    ManagedObject IntToMono(int value);
    int MonoToInt(ManagedObject& obj);
    ManagedObject FloatToMono(float value);
    float MonoToFloat(ManagedObject& obj);
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
    bool m_initialized = false;
    std::unordered_map<std::string, std::unique_ptr<MonoAssembly>> m_assemblies;
    std::vector<std::unique_ptr<MonoDomain>> m_domains;
    std::vector<std::string> m_searchPaths;
};

} // namespace Scripting
} // namespace Engine