#pragma once
#include <cstdint>
#include <string>
#include <stdexcept>
#include <sstream>
namespace Engine {
    namespace Serialization {
        // 序列化格式枚举
        enum class SerializationFormat {
            Binary,
            JSON
        };

        // 序列化版本信息
        struct SerializationVersion {
            uint32_t major = 1;
            uint32_t minor = 0;
            uint32_t patch = 0;

            std::string ToString() const {
                return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
            }

            static SerializationVersion FromString(const std::string& versionStr) {
                SerializationVersion version;
                std::istringstream iss(versionStr);
                char dot;
                iss >> version.major >> dot >> version.minor >> dot >> version.patch;
                return version;
            }
        };

        // 序列化异常类
        class SerializationException : public std::runtime_error {
        public:
            explicit SerializationException(const std::string& message)
                : std::runtime_error("Serialization Error: " + message) {}
        };
    }
}