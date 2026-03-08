#include "Serializable.h"
#include "Export.h"
#include <filesystem>

namespace PrismaEngine {
    namespace Serialization {
        // 实现非模板方法
        void OutputArchive::SetCurrent([[maybe_unused]] const std::string& key) {
            // 默认实现为空，子类可以重写
        }

        void InputArchive::EnterField(const std::string& field) {
            SetCurrent(field);
        }

        void InputArchive::SetCurrent([[maybe_unused]] const std::string& key) {
            // 默认实现为空，子类可以重写
        }
    }
}