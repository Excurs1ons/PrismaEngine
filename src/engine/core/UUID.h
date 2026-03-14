#pragma once

#include <cstdint>
#include <xhash>

namespace Prisma {

class UUID {
public:
    UUID();
    UUID(uint64_t uuid);
    UUID(const UUID&) = default;

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
