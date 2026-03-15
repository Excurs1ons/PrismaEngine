#include "ArchiveJson.h"

namespace Prisma {
namespace Serialization {

// JsonOutputArchive implementation
JsonOutputArchive::JsonOutputArchive() {
    m_stack.push(&m_root);
}

JsonOutputArchive::~JsonOutputArchive() = default;

// JsonInputArchive implementation
JsonInputArchive::JsonInputArchive(const json& data) : m_root(data) {
    m_stack.push(const_cast<json*>(&m_root));
}

JsonInputArchive::~JsonInputArchive() = default;

} // namespace Serialization
} // namespace Prisma