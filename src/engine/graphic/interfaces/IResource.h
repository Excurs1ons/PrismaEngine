#pragma once

#include "RenderTypes.h"
#include <string>
#include <string_view>
#include <atomic>

namespace Prisma::Graphic {

// 资源基础抽象类
/// 所有渲染资源的基类
class IResource {
public:
    IResource() = default;
    virtual ~IResource() = default;

    // 获取资源类型
    virtual ResourceType GetResourceType() const = 0;

    // 获取资源ID
    virtual ResourceId GetId() const { return m_id; }

    // 获取资源名称
    virtual const std::string& GetName() const { return m_name; }

    // 设置资源名称
    virtual void SetName(const std::string& name) { m_name = name; }

    // 获取资源大小（字节）
    virtual uint64_t GetSize() const { return m_size; }

    // 检查资源是否已加载
    virtual bool IsLoaded() const { return m_isLoaded; }

    // 检查资源是否有效
    virtual bool IsValid() const { return m_isLoaded; }

    // 增加引用计数
    virtual void AddRef() { m_refCount++; }

    // 减少引用计数
    virtual uint32_t Release() { 
        uint32_t count = --m_refCount;
        if (count == 0) {
            // 注意：这里通常由外部管理器处理物理删除
        }
        return count;
    }

    // 获取引用计数
    virtual uint32_t GetRefCount() const { return m_refCount; }

    // 获取调试标记
    virtual const std::string& GetDebugTag() const { return m_debugTag; }

    // 设置调试标记
    virtual void SetDebugTag(const std::string& tag) { m_debugTag = tag; }

    // 获取创建时间戳
    virtual uint64_t GetCreationTimestamp() const { return m_creationTimestamp; }

    // 获取最后访问时间戳
    virtual uint64_t GetLastAccessTimestamp() const { return m_lastAccessTimestamp; }

    // 标记资源为脏（需要更新）
    virtual void MarkDirty() { m_isDirty = true; }

    // 检查资源是否为脏
    virtual bool IsDirty() const { return m_isDirty; }

    // 清除脏标记
    virtual void ClearDirty() { m_isDirty = false; }

protected:
    mutable std::atomic<uint32_t> m_refCount{1};
    ResourceId m_id = 0;
    std::string m_name;
    std::string m_debugTag;
    uint64_t m_size = 0;
    std::atomic<bool> m_isLoaded{false};
    std::atomic<bool> m_isDirty{false};
    uint64_t m_creationTimestamp = 0;
    mutable std::atomic<uint64_t> m_lastAccessTimestamp{0};
};

} // namespace Prisma::Graphic