#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include "resource/Archive.h"
#include "Serializable.h"

namespace Prisma {
namespace Resource {

struct ENGINE_API MetaData : public Serialization::ISerializable {
    std::string name;
    std::string description;
    std::string author;
    std::string version;
    std::vector<std::string> tags;
    std::filesystem::path sourcePath;

    void Serialize(Serialization::OutputArchive& archive) const override {
        archive.Write("name", name);
        archive.Write("description", description);
        archive.Write("author", author);
        archive.Write("version", version);
        archive.Write("tagCount", static_cast<uint32_t>(tags.size()));
        archive.Write("sourcePath", sourcePath.string());
    }

    void Deserialize(Serialization::InputArchive& archive) override {
        archive.Read("name", name);
        archive.Read("description", description);
        archive.Read("author", author);
        archive.Read("version", version);
        uint32_t tagCount = 0;
        archive.Read("tagCount", tagCount);
        tags.resize(tagCount);
        std::string path;
        if (archive.Read("sourcePath", path)) sourcePath = path;
    }
};

} // namespace Resource
} // namespace Prisma
