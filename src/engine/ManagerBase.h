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

protected:
    ManagerBase() = default;
};

}  // namespace Prisma
