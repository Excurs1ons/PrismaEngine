#pragma once

#include "RenderTypes.h"
#include <string>
#include <string_view>
#include <atomic>

namespace PrismaEngine::Graphic {

/// @brief 资源基础抽象类
/// 所有渲染资源的基类
class IResource {
public:
    virtual ~IResource() = default;

    /// @brief 获取资源类型
    /// @return 资源类型
    virtual ResourceType GetType() const = 0;

    /// @brief 获取资源ID
    /// @return 资源ID
    virtual ResourceId GetId() const = 0;

    /// @brief 获取资源名称
    /// @return 资源名称
    virtual const std::string& GetName() const = 0;

    /// @brief 设置资源名称
    /// @param name 资源名称
    virtual void SetName(const std::string& name) = 0;

    /// @brief 获取资源大小（字节）
    /// @return 资源大小
    virtual uint64_t GetSize() const = 0;

    /// @brief 检查资源是否已加载
    /// @return 是否已加载
    virtual bool IsLoaded() const = 0;

    /// @brief 检查资源是否有效
    /// @return 是否有效
    virtual bool IsValid() const = 0;

    /// @brief 增加引用计数
    virtual void AddRef() = 0;

    /// @brief 减少引用计数
    /// @return 剩余引用计数
    virtual uint32_t Release() = 0;

    /// @brief 获取引用计数
    /// @return 当前引用计数
    virtual uint32_t GetRefCount() const = 0;

    /// @brief 获取调试标记
    /// @return 调试标记
    virtual const std::string& GetDebugTag() const = 0;

    /// @brief 设置调试标记
    /// @param tag 调试标记
    virtual void SetDebugTag(const std::string& tag) = 0;

    /// @brief 获取创建时间戳
    /// @return 创建时间戳
    virtual uint64_t GetCreationTimestamp() const = 0;

    /// @brief 获取最后访问时间戳
    /// @return 最后访问时间戳
    virtual uint64_t GetLastAccessTimestamp() const = 0;

    /// @brief 标记资源为脏（需要更新）
    virtual void MarkDirty() = 0;

    /// @brief 检查资源是否为脏
    /// @return 是否为脏
    virtual bool IsDirty() const = 0;

    /// @brief 清除脏标记
    virtual void ClearDirty() = 0;

protected:
    mutable std::atomic<uint32_t> m_refCount{1};
    ResourceId m_id;
    std::string m_name;
    std::string m_debugTag;
    uint64_t m_size = 0;
    std::atomic<bool> m_isLoaded{false};
    std::atomic<bool> m_isDirty{false};
    uint64_t m_creationTimestamp = 0;
    mutable std::atomic<uint64_t> m_lastAccessTimestamp{0};
};

} // namespace PrismaEngine::Graphic