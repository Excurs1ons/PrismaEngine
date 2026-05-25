#pragma once

#include "Export.h"
#include "UUID.h"
#include "../math/MurmurHash3.h"
#include <string>
#include <unordered_map>
#include <filesystem>
#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>

namespace Prisma {

struct ENGINE_API AssetMetadata {
    UUID guid;
    std::string path;
    std::string type;
    std::string hash;      // MurmurHash3 128-bit hex string
    uint64_t lastSize;     // Fast fingerprint
    int64_t lastModified;  // Fast fingerprint
    glz::json_t customData;

    // 序列化逻辑
    void ToJson(glz::json_t& j) const;
    void FromJson(const glz::json_t& j);
};

/* 全局资源数据库，基于 mmh3 128位哈希和文件指纹策略。 */
class ENGINE_API AssetDatabase {
public:
    static AssetDatabase& Get();

    bool Load(const std::string& dbPath = "assets/metadata.json");
    void Save();

    // 扫描指定目录（递归），快速同步文件状态
    void Refresh(const std::string& rootPath = "assets");

    // 查询接口
    bool HasAsset(const std::string& path) const;
    AssetMetadata* GetMetadata(const std::string& path);
    AssetMetadata* GetMetadata(const UUID& guid);

    // 获取所有资源的元数据 (只读)
    const std::unordered_map<std::string, AssetMetadata>& GetAllMetadata() const { return m_pathMap; }

private:
    AssetDatabase() = default;
    
    std::string m_dbPath;
    // 双向索引
    std::unordered_map<std::string, AssetMetadata> m_pathMap;
    std::unordered_map<UUID, std::string> m_guidToPath;

    // 是否有未保存的更改
    bool m_isDirty = false;
};

} // namespace Prisma
