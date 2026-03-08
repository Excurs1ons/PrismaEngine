#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include "resource/Archive.h"
#include "Serializable.h"

namespace PrismaEngine {
    namespace Resource {

        struct ENGINE_API MetaData : public Serialization::Serializable {
            std::string name;
            std::string description;
            std::string author;
            std::string version;
            std::vector<std::string> tags;
            std::filesystem::path sourcePath;

            void Serialize(Serialization::OutputArchive& archive) const override {
                archive("name", name);
                archive("description", description);
                archive("author", author);
                archive("version", version);

                uint32_t tagCount = static_cast<uint32_t>(tags.size());
                archive.BeginArray("tags", tagCount);
                for (const auto& tag : tags) {
                    archive.WriteString(tag);
                }
                archive.EndArray();

                archive("sourcePath", sourcePath);
            }

            void Deserialize(Serialization::InputArchive& archive) override {
                archive("name", name);
                archive("description", description);
                archive("author", author);
                archive("version", version);

                uint32_t tagCount = 0;
                archive.BeginArray("tags", tagCount);
                tags.resize(tagCount);
                for (uint32_t i = 0; i < tagCount; ++i) {
                    tags[i] = archive.ReadString();
                }
                archive.EndArray();

                archive("sourcePath", sourcePath);
            }
        };

    } // namespace Resource
} // namespace PrismaEngine
