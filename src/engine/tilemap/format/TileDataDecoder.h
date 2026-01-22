#pragma once

#include "../core/Types.h"
#include <vector>
#include <string>
#include <cstdint>

namespace PrismaEngine {

// ============================================================================
// 瓦片数据解码器
// ============================================================================

class TileDataDecoder {
public:
    // 解码瓦片数据
    static std::vector<uint32_t> Decode(
        const std::string& data,
        TileDataEncoding encoding,
        int expectedSize = -1  // 期望的瓦片数量，-1 表示不检查
    );

    // 编码瓦片数据
    static std::string Encode(
        const std::vector<uint32_t>& data,
        TileDataEncoding encoding,
        bool compression = false
    );

    // 解析 CSV 格式
    static std::vector<uint32_t> ParseCSV(
        const std::string& csvData,
        int expectedSize = -1
    );

    // 解析 Base64 格式
    static std::vector<uint32_t> ParseBase64(
        const std::string& base64Data,
        int expectedSize = -1
    );

    // 解析 Base64 + zlib 压缩
    static std::vector<uint32_t> ParseBase64Zlib(
        const std::string& base64Data,
        int expectedSize = -1
    );

    // 解析 Base64 + zstd 压缩
    static std::vector<uint32_t> ParseBase64Zstd(
        const std::string& base64Data,
        int expectedSize = -1
    );

    // 解析 Base64 + gzip 压缩
    static std::vector<uint32_t> ParseBase64Gzip(
        const std::string& base64Data,
        int expectedSize = -1
    );

    // Base64 解码
    static std::vector<uint8_t> Base64Decode(const std::string& encoded);

    // Base64 编码
    static std::string Base64Encode(const std::vector<uint8_t>& data);

private:
    // zlib 解压
    static std::vector<uint8_t> DecompressZlib(
        const uint8_t* compressedData,
        size_t compressedSize,
        size_t expectedSize = 0
    );

    // zstd 解压
    static std::vector<uint8_t> DecompressZstd(
        const uint8_t* compressedData,
        size_t compressedSize,
        size_t expectedSize = 0
    );

    // gzip 解压
    static std::vector<uint8_t> DecompressGzip(
        const uint8_t* compressedData,
        size_t compressedSize,
        size_t expectedSize = 0
    );

    // zlib 压缩
    static std::vector<uint8_t> CompressZlib(
        const uint8_t* data,
        size_t dataSize
    );

    // zstd 压缩
    static std::vector<uint8_t> CompressZstd(
        const uint8_t* data,
        size_t dataSize
    );

    // gzip 压缩
    static std::vector<uint8_t> CompressGzip(
        const uint8_t* data,
        size_t dataSize
    );
};

} // namespace PrismaEngine
