#pragma once
#include "ISubSystem.h"
#include "Export.h"
#include <memory>

namespace Prisma {

/// @brief 子系统基类接口
template <typename T> 
class ManagerBase : public ISubSystem {
public:
    virtual ~ManagerBase() = default;

    /// @brief 获取子系统名称 (默认使用 typeid)
    const char* GetName() const override { return typeid(T).name(); }

protected:
    ManagerBase() = default;
};

// 辅助宏：快速在派生类中覆盖名称
#define PRISMA_SUBSET_NAME(name_str) const char* GetName() const override { return name_str; }

}  // namespace Prisma
