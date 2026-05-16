#include "AssetDatabase.h"
#include "Logger.h"
#include <fstream>
#include <chrono>

namespace Prisma {

namespace fs = std::filesystem;

void AssetMetadata::ToJson(glz::json_t& j) const {
    j = glz::json_t::object_t{
        {"guid", guid.ToString()},
        {"path", path},
        {"type", type},
        {"hash", hash},
        {"lastSize", static_cast<double>(lastSize)},
        {"lastModified", static_cast<double>(lastModified)},
        {"customData", customData}
    };
}

void AssetMetadata::FromJson(const glz::json_t& j) {
    auto& obj = j.get_object();
    guid = UUID::FromString(obj.at("guid").get_string());
    path = obj.at("path").get_string();
    type = obj.at("type").get_string();
    hash = obj.at("hash").get_string();
    lastSize = static_cast<uint64_t>(obj.at("lastSize").get_double());
    lastModified = static_cast<int64_t>(obj.at("lastModified").get_double());
    if (obj.contains("customData")) {
        customData = obj.at("customData");
    }
}

AssetDatabase& AssetDatabase::Get() {
    static AssetDatabase instance;
    return instance;
}

bool AssetDatabase::Load(const std::string& dbPath) {
    m_dbPath = dbPath;
    m_pathMap.clear();
    m_guidToPath.clear();

    if (!fs::exists(m_dbPath)) {
        LOG_DEBUG("AssetDB", "未找到元数据库文件，将创建新库。");
        return true;
    }

    try {
        std::ifstream file(m_dbPath);
        std::string jsonStr((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        glz::json_t j;
        auto ec = glz::read_json(j, jsonStr);
        if (ec) throw std::runtime_error("Parse error");

        for (auto& [key, value] : j.get_object()) {
            AssetMetadata meta;
            meta.FromJson(value);
            m_pathMap[meta.path] = meta;
            m_guidToPath[meta.guid] = meta.path;
        }
        LOG_INFO("AssetDB", "成功加载元数据库，包含 {0} 个资源信息。", m_pathMap.size());
    } catch (const std::exception& e) {
        LOG_ERROR("AssetDB", "加载元数据库失败: {0}", e.what());
        return false;
    }

    return true;
}

void AssetDatabase::Save() {
    if (!m_isDirty) return;

    try {
        glz::json_t j = glz::json_t::object_t{};
        for (auto& [path, meta] : m_pathMap) {
            glz::json_t metaJson;
            meta.ToJson(metaJson);
            j.get_object()[path] = metaJson;
        }

        std::string buffer;
        glz::write<glz::opts{.indent = 4}>(j, buffer);
        std::ofstream file(m_dbPath);
        file << buffer;
        m_isDirty = false;
        LOG_DEBUG("AssetDB", "元数据库已保存。");
    } catch (const std::exception& e) {
        LOG_ERROR("AssetDB", "保存元数据库失败: {0}", e.what());
    }
}

void AssetDatabase::Refresh(const std::string& rootPath) {
    if (!fs::exists(rootPath)) return;

    LOG_DEBUG("AssetDB", "正在快速扫描资源目录: {0}...", rootPath);
    auto startTime = std::chrono::high_resolution_clock::now();

    int updatedCount = 0;
    int newCount = 0;

    for (const auto& entry : fs::recursive_directory_iterator(rootPath)) {
        if (!entry.is_regular_file()) continue;

        // 排除数据库本身
        if (entry.path() == fs::path(m_dbPath)) continue;

        std::string path = entry.path().generic_string();
        uint64_t size = fs::file_size(entry.path());
        auto mtime = fs::last_write_time(entry.path()).time_since_epoch().count();

        bool needsUpdate = false;
        
        auto it = m_pathMap.find(path);
        if (it != m_pathMap.end()) {
            // --- 核心优化：快速指纹对比 ---
            if (it->second.lastSize != size || it->second.lastModified != mtime) {
                needsUpdate = true;
                updatedCount++;
            }
        } else {
            // 新资源
            needsUpdate = true;
            newCount++;
        }

        if (needsUpdate) {
            AssetMetadata meta;
            if (it != m_pathMap.end()) {
                meta = it->second;
            } else {
                meta.guid = UUID(); // 生成新 GUID
                meta.path = path;
                // 简单的类型推断逻辑，后续可扩展
                meta.type = entry.path().extension().string();
            }

            // 只有指纹变了才读内容计算 MurmurHash3
            auto contentHash = Math::MurmurHash3::HashFile(path);
            meta.hash = contentHash.ToString();
            meta.lastSize = size;
            meta.lastModified = mtime;

            m_pathMap[path] = meta;
            m_guidToPath[meta.guid] = path;
            m_isDirty = true;
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    double duration = std::chrono::duration<double>(endTime - startTime).count();

    if (updatedCount > 0 || newCount > 0) {
        LOG_INFO("AssetDB", "扫描完成。耗时: {0}s。新增: {1}, 更新: {2}", duration, newCount, updatedCount);
        Save();
    } else {
        LOG_INFO("AssetDB", "扫描完成。耗时: {0}s。所有资源均为最新。", duration);
    }
}

bool AssetDatabase::HasAsset(const std::string& path) const {
    return m_pathMap.find(path) != m_pathMap.end();
}

AssetMetadata* AssetDatabase::GetMetadata(const std::string& path) {
    auto it = m_pathMap.find(path);
    return (it != m_pathMap.end()) ? &it->second : nullptr;
}

AssetMetadata* AssetDatabase::GetMetadata(const UUID& guid) {
    auto it = m_guidToPath.find(guid);
    if (it != m_guidToPath.end()) {
        return GetMetadata(it->second);
    }
    return nullptr;
}

} // namespace Prisma
