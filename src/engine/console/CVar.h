#pragma once

#include "../Export.h"
#include <algorithm>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Prisma {

// CVar 标志位
enum class CVarFlags : uint32_t {
    None           = 0,
    Cheat          = 1 << 0,
    ReadOnly       = 1 << 1,
    RequireRestart = 1 << 2,
    Archive        = 1 << 3,
};

inline CVarFlags operator|(CVarFlags a, CVarFlags b) {
    return static_cast<CVarFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
inline CVarFlags operator&(CVarFlags a, CVarFlags b) {
    return static_cast<CVarFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}
inline bool HasFlag(CVarFlags val, CVarFlags flag) {
    return (static_cast<uint32_t>(val) & static_cast<uint32_t>(flag)) != 0;
}

// CVar 基类
class ENGINE_API CVarBase {
public:
    CVarBase(std::string name, std::string description, CVarFlags flags)
        : m_Name(std::move(name))
        , m_Description(std::move(description))
        , m_Flags(flags) {}
    virtual ~CVarBase() = default;

    const std::string& GetName() const { return m_Name; }
    const std::string& GetDescription() const { return m_Description; }
    CVarFlags GetFlags() const { return m_Flags; }
    bool HasFlag(CVarFlags flag) const { return Prisma::HasFlag(m_Flags, flag); }

    virtual void SetFromString(const std::string& value) = 0;
    virtual std::string GetString() const = 0;
    virtual std::string GetTypeName() const = 0;
    virtual void ResetToDefault() = 0;

private:
    std::string m_Name;
    std::string m_Description;
    CVarFlags m_Flags;
};

// 类型化 CVar
template<typename T>
class CVar : public CVarBase {
public:
    CVar(std::string name, T defaultValue, std::string description = "", CVarFlags flags = CVarFlags::None)
        : CVarBase(std::move(name), std::move(description), flags)
        , m_Value(std::move(defaultValue))
        , m_DefaultValue(m_Value) {}

    CVar(std::string name, T defaultValue, T min, T max,
         std::string description = "", CVarFlags flags = CVarFlags::None)
        : CVarBase(std::move(name), std::move(description), flags)
        , m_Value(std::move(defaultValue))
        , m_DefaultValue(m_Value)
        , m_Min(std::move(min))
        , m_Max(std::move(max)) {
        Clamp();
    }

    const T& Get() const { return m_Value; }
    operator const T&() const { return m_Value; }

    void Set(const T& value) {
        if (HasFlag(CVarFlags::ReadOnly)) return;
        m_Value = value;
        Clamp();
    }

    void SetFromString(const std::string& value) override {
        if (HasFlag(CVarFlags::ReadOnly)) return;
        m_Value = ParseString(value);
        Clamp();
    }

    std::string GetString() const override { return ToString(m_Value); }
    std::string GetTypeName() const override { return TypeName(); }
    void ResetToDefault() override {
        m_Value = m_DefaultValue;
        Clamp();
    }

    const std::optional<T>& GetMin() const { return m_Min; }
    const std::optional<T>& GetMax() const { return m_Max; }

private:
    void Clamp() {
        if (m_Min.has_value()) m_Value = std::max(m_Value, m_Min.value());
        if (m_Max.has_value()) m_Value = std::min(m_Value, m_Max.value());
    }

    static std::string ToString(const T& val);
    static T ParseString(const std::string& str);
    static std::string TypeName();

    T m_Value;
    T m_DefaultValue;
    std::optional<T> m_Min;
    std::optional<T> m_Max;
};

// --- 特化声明 ---
// int
template<>
inline std::string CVar<int>::ToString(const int& val) { return std::to_string(val); }
template<>
inline int CVar<int>::ParseString(const std::string& str) { return std::stoi(str); }
template<>
inline std::string CVar<int>::TypeName() { return "int"; }

// float
template<>
inline std::string CVar<float>::ToString(const float& val) { return std::to_string(val); }
template<>
inline float CVar<float>::ParseString(const std::string& str) { return std::stof(str); }
template<>
inline std::string CVar<float>::TypeName() { return "float"; }

// bool
template<>
inline std::string CVar<bool>::ToString(const bool& val) { return val ? "true" : "false"; }
template<>
inline bool CVar<bool>::ParseString(const std::string& str) {
    auto lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower == "true" || lower == "1" || lower == "on") return true;
    return false;
}
template<>
inline std::string CVar<bool>::TypeName() { return "bool"; }

// string
template<>
inline std::string CVar<std::string>::ToString(const std::string& val) { return val; }
template<>
inline std::string CVar<std::string>::ParseString(const std::string& str) { return str; }
template<>
inline std::string CVar<std::string>::TypeName() { return "string"; }

// --- CVarRegistry ---
class ENGINE_API CVarRegistry {
public:
    CVarRegistry() = default;
    CVarRegistry(const CVarRegistry&) = delete;
    CVarRegistry& operator=(const CVarRegistry&) = delete;

    void Register(std::unique_ptr<CVarBase> cvar) {
        if (cvar) m_CVars[cvar->GetName()] = std::move(cvar);
    }

    CVarBase* Find(const std::string& name) {
        auto it = m_CVars.find(name);
        return it != m_CVars.end() ? it->second.get() : nullptr;
    }

    void ForEach(std::function<void(CVarBase*)> func) {
        for (auto& [name, cvar] : m_CVars) {
            func(cvar.get());
        }
    }

    std::vector<CVarBase*> GetAll() const {
        std::vector<CVarBase*> result;
        result.reserve(m_CVars.size());
        for (auto& [name, cvar] : m_CVars) {
            result.push_back(cvar.get());
        }
        return result;
    }

    size_t GetCount() const { return m_CVars.size(); }

private:
    std::unordered_map<std::string, std::unique_ptr<CVarBase>> m_CVars;
};

} // namespace Prisma
