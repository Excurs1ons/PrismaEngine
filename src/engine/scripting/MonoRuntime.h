#pragma once

#ifdef PRISMA_ENABLE_MONO
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/environment.h>
#include <mono/metadata/mono-config.h>
#endif
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <functional>

namespace Engine {
namespace Scripting {

#ifdef PRISMA_ENABLE_MONO
// 前向声明Mono类型
struct MonoDomain;
struct MonoObject;
struct MonoClass;
struct MonoAssembly;
struct MonoImage;
struct MonoMethodDesc;
struct MonoMethod;
#else
// 占位符类型
using MonoDomain = void;
using MonoObject = void;
using MonoClass = void;
using MonoAssembly = void;
using MonoImage = void;
using MonoMethodDesc = void;
using MonoMethod = void;
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
    MonoDomain* m_domain = nullptr;
    ::MonoObject* m_monoObject = nullptr;
    MonoClass* m_class = nullptr;
};

// Mono域管理
class MonoDomain {
public:
    MonoDomain() = default;
    ~MonoDomain() = default;

    bool Initialize(const std::string& domainName) { return false; }
    void Shutdown() {}
    bool IsInitialized() const { return false; }
    void* GetNativeDomain() const { return nullptr; }
};

// 程序集管理
class MonoAssembly {
public:
    MonoAssembly() = default;
    ~MonoAssembly() = default;

    bool Load(const std::string& assemblyPath) { return false; }
    bool IsLoaded() const { return false; }
    ManagedObject CreateInstance(const std::string& className) { return ManagedObject(); }
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