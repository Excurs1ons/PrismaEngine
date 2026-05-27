#pragma once

#include "Export.h"
#include <string>
#include <unordered_map>
#include <any>
#include <typeinfo>
#include <stdexcept>
#include <cassert>

namespace Prisma {
namespace AI {

/**
 * @brief 类型安全的黑板键值存储
 *
 * 用于行为树节点之间共享数据。底层使用 std::any 实现类型擦除，
 * Get<T>() 在类型不匹配时抛出异常。
 */
class ENGINE_API Blackboard {
public:
    Blackboard() = default;
    ~Blackboard() = default;

    Blackboard(const Blackboard&) = delete;
    Blackboard& operator=(const Blackboard&) = delete;

    Blackboard(Blackboard&&) = default;
    Blackboard& operator=(Blackboard&&) = default;

    // ========== 通用访问 ==========

    /** 检查键是否存在 */
    bool Has(const std::string& key) const {
        return m_data.find(key) != m_data.end();
    }

    /** 删除键值对 */
    bool Erase(const std::string& key) {
        return m_data.erase(key) > 0;
    }

    /** 清空黑板 */
    void Clear() {
        m_data.clear();
    }

    /** 获取存储的键值对数量 */
    size_t Size() const {
        return m_data.size();
    }

    /** 检查黑板是否为空 */
    bool IsEmpty() const {
        return m_data.empty();
    }

    // ========== 类型安全读写 ==========

    /**
     * @brief 设置值（左值引用重载）
     */
    template<typename T>
    void Set(const std::string& key, const T& value) {
        m_data[key] = std::any(value);
    }

    /**
     * @brief 设置值（右值引用重载）
     */
    template<typename T>
    void Set(const std::string& key, T&& value) {
        m_data[key] = std::any(std::forward<T>(value));
    }

    /**
     * @brief 获取值（引用）
     *
     * 返回指定键的值的引用。如果键不存在或类型不匹配，抛出异常。
     */
    template<typename T>
    T& Get(const std::string& key) {
        auto it = m_data.find(key);
        if (it == m_data.end()) {
            throw std::out_of_range("Blackboard: key '" + key + "' not found");
        }
        try {
            return std::any_cast<T&>(it->second);
        } catch (const std::bad_any_cast&) {
            throw std::runtime_error(
                "Blackboard: type mismatch for key '" + key + "'");
        }
    }

    /**
     * @brief 获取值（const 引用）
     */
    template<typename T>
    const T& Get(const std::string& key) const {
        auto it = m_data.find(key);
        if (it == m_data.end()) {
            throw std::out_of_range("Blackboard: key '" + key + "' not found");
        }
        try {
            return std::any_cast<const T&>(it->second);
        } catch (const std::bad_any_cast&) {
            throw std::runtime_error(
                "Blackboard: type mismatch for key '" + key + "'");
        }
    }

    /**
     * @brief 尝试获取值
     *
     * 如果键存在且类型匹配，返回值的指针；否则返回 nullptr。
     * 注意：对于存储为指针类型 T* 的值，此方法返回 T**（需要一次解引用）。
     * 推荐使用 TryGetValue<T>(key) 获取指针值。
     */
    template<typename T>
    T* TryGet(const std::string& key) {
        auto it = m_data.find(key);
        if (it == m_data.end()) return nullptr;
        try {
            return std::any_cast<T>(&it->second);
        } catch (const std::bad_any_cast&) {
            return nullptr;
        }
    }

    template<typename T>
    const T* TryGet(const std::string& key) const {
        auto it = m_data.find(key);
        if (it == m_data.end()) return nullptr;
        try {
            return std::any_cast<T>(&it->second);
        } catch (const std::bad_any_cast&) {
            return nullptr;
        }
    }

    /**
     * @brief 尝试获取值（按值返回）
     *
     * 对于指针类型的存储（如 NavAgentComponent*），直接返回正确的指针类型。
     * 如果键不存在或类型不匹配，返回 defaultValue。
     */
    template<typename T>
    T TryGetValue(const std::string& key, const T& defaultValue = T{}) const {
        auto it = m_data.find(key);
        if (it == m_data.end()) return defaultValue;
        try {
            return std::any_cast<T>(it->second);
        } catch (const std::bad_any_cast&) {
            return defaultValue;
        }
    }

    /**
     * @brief 获取值，不存在则返回默认值
     */
    template<typename T>
    T GetOrDefault(const std::string& key, const T& defaultValue) const {
        auto it = m_data.find(key);
        if (it == m_data.end()) return defaultValue;
        try {
            return std::any_cast<T>(it->second);
        } catch (const std::bad_any_cast&) {
            return defaultValue;
        }
    }

private:
    std::unordered_map<std::string, std::any> m_data;
};

} // namespace AI
} // namespace Prisma
