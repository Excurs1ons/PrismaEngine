#pragma once

#include "Serializable.h"
#include "../core/Asset.h"
#include <fstream>
#include <memory>
#include <glaze/glaze.hpp>
#include <glaze/json/json_t.hpp>
#include <sstream>
#include <vector>

#include "ArchiveBinary.h"
#include "ArchiveJson.h"
#include "SerializationVersion.h"
#include <filesystem>

using json = glz::json_t;

namespace Prisma {
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
                return false;
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
                std::string buffer;
                auto ec = glz::write<glz::opts{.indent = 4}>(archive.GetJson(), buffer);
                if (!ec) {
                    file << buffer;
                }
            }

            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

    // 从文件反序列化Asset
    template<typename T>
    static std::shared_ptr<T> DeserializeFromFile(const std::filesystem::path& filePath,
                                                    SerializationFormat format = SerializationFormat::JSON) {
        try {
            std::ifstream file(filePath, std::ios::binary);
            if (!file) {
                return nullptr;
            }

            // 读取版本信息
            auto version = ReadVersionHeader(file, format);

            auto asset = std::make_shared<T>();
            
            if (format == SerializationFormat::Binary) {
                BinaryInputArchive archive(file);
                asset->Deserialize(archive);
            } else {
                // 读取整个JSON文件
                std::string jsonStr((std::istreambuf_iterator<char>(file)),
                                    std::istreambuf_iterator<char>());
                json jsonData;
                auto ec = glz::read_json(jsonData, jsonStr);
                if (ec) return nullptr;
                
                JsonInputArchive archive(jsonData);
                asset->Deserialize(archive);
            }

            return asset;
        } catch (const std::exception&) {
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
                std::string buffer;
                glz::write_json(archive.GetJson(), buffer);
                stream << buffer;
            }

            std::string str = stream.str();
            result.assign(str.begin(), str.end());
        } catch (const std::exception&) {
            result.clear();
        }
        
        return result;
    }

    // 从内存反序列化Asset
    template<typename T>
    static std::shared_ptr<T> DeserializeFromMemory(const std::vector<uint8_t>& data,
                                                    SerializationFormat format = SerializationFormat::JSON) {
        try {
            std::istringstream stream(std::string(data.begin(), data.end()));
            
            // 读取版本信息
            auto version = ReadVersionHeader(stream, format);

            auto asset = std::make_shared<T>();
            
            if (format == SerializationFormat::Binary) {
                BinaryInputArchive archive(stream);
                asset->Deserialize(archive);
            } else {
                // 读取整个JSON字符串
                std::string jsonStr((std::istreambuf_iterator<char>(stream)),
                                    std::istreambuf_iterator<char>());
                json jsonData;
                auto ec = glz::read_json(jsonData, jsonStr);
                if (ec) return nullptr;
                
                JsonInputArchive archive(jsonData);
                asset->Deserialize(archive);
            }

            return asset;
        } catch (const std::exception&) {
            return nullptr;
        }
    }

private:
    // 写入版本头
    static void WriteVersionHeader(std::ostream& stream, 
                                    const SerializationVersion& version,
                                    SerializationFormat format) {
        if (format == SerializationFormat::Binary) {
            stream.write("PRISMA", 6);  // Modified magic
            uint8_t formatByte = static_cast<uint8_t>(format);
            stream.write(reinterpret_cast<const char*>(&formatByte), 1);
            stream.write(reinterpret_cast<const char*>(&version.major), sizeof(version.major));
            stream.write(reinterpret_cast<const char*>(&version.minor), sizeof(version.minor));
            stream.write(reinterpret_cast<const char*>(&version.patch), sizeof(version.patch));
        } else {
            json header = glz::json_t::object_t{
                {"format", "json"},
                {"version", glz::json_t::object_t{
                    {"major", static_cast<double>(version.major)},
                    {"minor", static_cast<double>(version.minor)},
                    {"patch", static_cast<double>(version.patch)}
                }}
            };
            std::string buffer;
            glz::write_json(header, buffer);
            stream << buffer << "\n";
        }
    }

    // 读取版本头
    static SerializationVersion ReadVersionHeader(std::istream& stream, SerializationFormat format) {
        SerializationVersion version;
        
        if (format == SerializationFormat::Binary) {
            char magic[6];
            stream.read(magic, 6);
            if (std::string(magic, 6) != "PRISMA") {
                throw std::runtime_error("Invalid file format");
            }
            
            uint8_t formatByte;
            stream.read(reinterpret_cast<char*>(&formatByte), 1);
            if (static_cast<SerializationFormat>(formatByte) != format) {
                throw std::runtime_error("Format mismatch");
            }
            
            stream.read(reinterpret_cast<char*>(&version.major), sizeof(version.major));
            stream.read(reinterpret_cast<char*>(&version.minor), sizeof(version.minor));
            stream.read(reinterpret_cast<char*>(&version.patch), sizeof(version.patch));
        } else {
            std::string headerLine;
            std::getline(stream, headerLine);
            json header;
            auto ec = glz::read_json(header, headerLine);
            if (ec) throw std::runtime_error("Failed to parse header");
            
            if (header.get_object()["format"].get_string() != "json") {
                throw std::runtime_error("Format mismatch");
            }
            
            auto& ver = header.get_object()["version"].get_object();
            version.major = static_cast<uint32_t>(ver["major"].get_double());
            version.minor = static_cast<uint32_t>(ver["minor"].get_double());
            version.patch = static_cast<uint32_t>(ver["patch"].get_double());
        }
        
        return version;
    }
};

} // namespace Serialization
} // namespace Prisma
