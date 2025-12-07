#pragma once

#include "../include/Resources.h"
#include "../include/Serializable.h"
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include <sstream>
#include <vector>

#include "../include/ArchiveBinary.h"
#include "../include/ArchiveJson.h"
#include <filesystem>

using json = nlohmann::json;

namespace Engine {
    namespace Serialization {

        // Asset序列化器
        class AssetSerializer {
        public:
            // 序列化Asset到文件
            template<typename T>
            static bool SerializeToFile(const T& asset, const std::filesystem::path& filePath, 
                                      SerializationFormat format = SerializationFormat::JSON,
                                      const SerializationVersion& version = SerializationVersion()) {
                try {
                    std::ofstream file(filePath, std::ios::binary);
                    if (!file) {
                        throw SerializationException("Failed to open file for writing: " + filePath.string());
                    }

                    // 写入版本信息
                    WriteVersionHeader(file, version, format);

                    if (format == SerializationFormat::Binary) {
                        BinaryOutputArchive archive(file);
                        asset.Serialize(archive);
                    } else {
                        JsonOutputArchive archive;
                        asset.Serialize(archive);
                        
                        // 将JSON写入文件
                        file << archive.GetJson().dump(4);
                    }

                    return true;
                } catch (const std::exception&) {
                    // 记录错误
                    return false;
                }
            }

            // 从文件反序列化Asset
            template<typename T>
            static std::unique_ptr<T> DeserializeFromFile(const std::filesystem::path& filePath,
                                                         SerializationFormat format = SerializationFormat::JSON) {
                try {
                    std::ifstream file(filePath, std::ios::binary);
                    if (!file) {
                        throw SerializationException("Failed to open file for reading: " + filePath.string());
                    }

                    // 读取版本信息
                    auto version = ReadVersionHeader(file, format);

                    auto asset = std::make_unique<T>();
                    
                    if (format == SerializationFormat::Binary) {
                        BinaryInputArchive archive(file);
                        asset->Deserialize(archive);
                    } else {
                        // 读取整个JSON文件
                        std::string jsonStr((std::istreambuf_iterator<char>(file)),
                                           std::istreambuf_iterator<char>());
                        json jsonData = json::parse(jsonStr);
                        
                        JsonInputArchive archive(jsonData);
                        asset->Deserialize(archive);
                    }

                    return asset;
                } catch (const std::exception&) {
                    // 记录错误
                    return nullptr;
                }
            }

            // 序列化Asset到内存
            template<typename T>
            static std::vector<uint8_t> SerializeToMemory(const T& asset, 
                                                         SerializationFormat format = SerializationFormat::JSON,
                                                         const SerializationVersion& version = SerializationVersion()) {
                std::vector<uint8_t> result;
                
                try {
                    std::ostringstream stream;
                    
                    // 写入版本信息
                    WriteVersionHeader(stream, version, format);

                    if (format == SerializationFormat::Binary) {
                        BinaryOutputArchive archive(stream);
                        asset.Serialize(archive);
                    } else {
                        JsonOutputArchive archive;
                        asset.Serialize(archive);
                        
                        // 将JSON写入流
                        stream << archive.GetJson().dump();
                    }

                    std::string str = stream.str();
                    result.assign(str.begin(), str.end());
                } catch (const std::exception& e) {
                    // 记录错误
                    result.clear();
                }
                
                return result;
            }

            // 从内存反序列化Asset
            template<typename T>
            static std::unique_ptr<T> DeserializeFromMemory(const std::vector<uint8_t>& data,
                                                          SerializationFormat format = SerializationFormat::JSON) {
                try {
                    std::istringstream stream(std::string(data.begin(), data.end()));
                    
                    // 读取版本信息
                    auto version = ReadVersionHeader(stream, format);

                    auto asset = std::make_unique<T>();
                    
                    if (format == SerializationFormat::Binary) {
                        BinaryInputArchive archive(stream);
                        asset->Deserialize(archive);
                    } else {
                        // 读取整个JSON字符串
                        std::string jsonStr((std::istreambuf_iterator<char>(stream)),
                                           std::istreambuf_iterator<char>());
                        json jsonData = json::parse(jsonStr);
                        
                        JsonInputArchive archive(jsonData);
                        asset->Deserialize(archive);
                    }

                    return asset;
                } catch (const std::exception&) {
                    // 记录错误
                    return nullptr;
                }
            }

        private:
            // 写入版本头
            static void WriteVersionHeader(std::ostream& stream, 
                                         const SerializationVersion& version,
                                         SerializationFormat format) {
                if (format == SerializationFormat::Binary) {
                    // 二进制格式的版本头
                    stream.write("YAGE", 4);  // 魔数
                    uint8_t formatByte = static_cast<uint8_t>(format);
                    stream.write(reinterpret_cast<const char*>(&formatByte), 1);
                    stream.write(reinterpret_cast<const char*>(&version.major), sizeof(version.major));
                    stream.write(reinterpret_cast<const char*>(&version.minor), sizeof(version.minor));
                    stream.write(reinterpret_cast<const char*>(&version.patch), sizeof(version.patch));
                } else {
                    // JSON格式的版本信息作为元数据
                    json header = {
                        {"format", "json"},
                        {"version", {
                            {"major", version.major},
                            {"minor", version.minor},
                            {"patch", version.patch}
                        }}
                    };
                    stream << header.dump() << "\n";
                }
            }

            // 读取版本头
            static SerializationVersion ReadVersionHeader(std::istream& stream, SerializationFormat format) {
                SerializationVersion version;
                
                if (format == SerializationFormat::Binary) {
                    // 读取二进制格式的版本头
                    char magic[4];
                    stream.read(magic, 4);
                    if (std::string(magic, 4) != "YAGE") {
                        throw SerializationException("Invalid file format");
                    }
                    
                    uint8_t formatByte;
                    stream.read(reinterpret_cast<char*>(&formatByte), 1);
                    if (static_cast<SerializationFormat>(formatByte) != format) {
                        throw SerializationException("Format mismatch");
                    }
                    
                    stream.read(reinterpret_cast<char*>(&version.major), sizeof(version.major));
                    stream.read(reinterpret_cast<char*>(&version.minor), sizeof(version.minor));
                    stream.read(reinterpret_cast<char*>(&version.patch), sizeof(version.patch));
                } else {
                    // 读取JSON格式的版本信息
                    std::string headerLine;
                    std::getline(stream, headerLine);
                    json header = json::parse(headerLine);
                    
                    if (header["format"] != "json") {
                        throw SerializationException("Format mismatch");
                    }
                    
                    version.major = header["version"]["major"];
                    version.minor = header["version"]["minor"];
                    version.patch = header["version"]["patch"];
                }
                
                return version;
            }
        };

    } // namespace Serialization
} // namespace Engine