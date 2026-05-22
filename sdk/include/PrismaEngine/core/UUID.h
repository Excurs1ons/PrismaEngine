#pragma once

#include "Export.h"
#include <cstdint>
#include <functional>
#include <string>

namespace Prisma {

class ENGINE_API UUID {
public:
    UUID();
    UUID(uint64_t uuid);
    UUID(const UUID&) = default;

    std::string ToString() const;
    static UUID FromString(const std::string& str);

    operator uint64_t() const { return m_UUID; }

private:
    uint64_t m_UUID;
};

} // namespace Prisma

namespace std {
    template <typename T> struct hash;
    template<>
    struct hash<Prisma::UUID> {
        std::size_t operator()(const Prisma::UUID& uuid) const {
            return std::hash<uint64_t>{}((uint64_t)uuid);
        }
    };
}
