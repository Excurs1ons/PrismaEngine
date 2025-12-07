#pragma once
#include "../resource/Archive.h"
#include <filesystem>
#include <string>
#include <vector>

namespace Engine {
    namespace Serialization {

        // 资产元数据
        struct Metadata {
            std::string name;
            std::string description;
            std::string author;
            std::string version;
            std::vector<std::string> tags;
            std::filesystem::path sourcePath;

            void Serialize(Serialization::OutputArchive& archive) const {
                archive.BeginObject(6);
                archive("name", name);
                archive("description", description);
                archive("author", author);
                archive("version", version);

                archive.BeginArray(tags.size());
                for (const auto& tag : tags) {
                    archive("", tag);
                }
                archive.EndArray();

                archive("sourcePath", sourcePath);
                archive.EndObject();
            }

            void Deserialize(Serialization::InputArchive& archive) {
                size_t fieldCount = archive.BeginObject();

                for (size_t i = 0; i < fieldCount; ++i) {
                    if (archive.HasNextField("name")) {
                        name = archive.ReadString();
                    }
                    else if (archive.HasNextField("description")) {
                        description = archive.ReadString();
                    }
                    else if (archive.HasNextField("author")) {
                        author = archive.ReadString();
                    }
                    else if (archive.HasNextField("version")) {
                        version = archive.ReadString();
                    }
                    else if (archive.HasNextField("tags")) {
                        size_t tagCount = archive.BeginArray();
                        tags.resize(tagCount);
                        for (size_t j = 0; j < tagCount; ++j) {
                            tags[j] = archive.ReadString();
                        }
                        archive.EndArray();
                    }
                    else if (archive.HasNextField("sourcePath")) {
                        std::string pathStr = archive.ReadString();
                        sourcePath = pathStr;
                    }
                }

                archive.EndObject();
            }
        };


    }
}