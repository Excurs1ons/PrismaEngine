#pragma once
#include "Singleton.h"
#include "ISubSystem.h"
#include <memory>

namespace Engine {
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
    virtual ~ManagerBase() = default;
};
}  // namespace Engine