#pragma once
#include "ISubSystem.h"
#include "Singleton.h"
#include <memory>

namespace PrismaEngine {
template <typename T> class ManagerBase : public ISubSystem {
public:
    ManagerBase(const ManagerBase&)            = delete;
    ManagerBase& operator=(const ManagerBase&) = delete;
    static std::shared_ptr<T> GetInstance() {
        static std::shared_ptr<T> instance = std::make_shared<T>();
        return instance;
    }
protected:
    ManagerBase() = default;
};
}  // namespace Engine