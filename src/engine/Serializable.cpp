#include "Serializable.h"
#include <filesystem>

namespace PrismaEngine {
    namespace Serialization {
        // 实现非模板方法
        void OutputArchive::SetCurrent(const std::string& key) {
            // 默认实现为空，子类可以重写
        }

        void InputArchive::EnterField(const std::string& field) {
            SetCurrent(field);
        }

        void InputArchive::SetCurrent(const std::string& key) {
            // 默认实现为空，子类可以重写
        }
    }
}