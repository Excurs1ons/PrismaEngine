#include "TileDataDecoder.h"
#include <sstream>
#include <algorithm>
#include <cstring>

// zstd header
#ifdef PRISMA_USE_ZSTD
#include <zstd.h>
#endif

// zlib/miniz header
#ifdef PRISMA_USE_MINIZ
#define MINIZ_HEADER_FILE_ONLY
#include <miniz.c>
#else
#include <zlib.h>
#endif

namespace PrismaEngine {

// ============================================================================
// Base64 编码/解码
// ============================================================================

static const char* BASE64_CHARS =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

static inline bool IsBase64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

std::vector<uint8_t> TileDataDecoder::Base64Decode(const std::string& encoded) {
    std::string cleaned;
    cleaned.reserve(encoded.size());

    // 移除空白字符
    for (char c : encoded) {
        if (!std::isspace(static_cast<unsigned char>(c))) {
            cleaned += c;
        }
    }

    size_t len = cleaned.size();
    if (len == 0) return {};

    size_t outLen = len * 3 / 4;
    if (cleaned[len - 1] == '=') outLen--;
    if (cleaned[len - 2] == '=') outLen--;

    std::vector<uint8_t> result;
    result.reserve(outLen);

    std::array<int, 4> sextets = {0};

    for (size_t i = 0, j = 0; i < len; i += 4, j += 3) {
        for (size_t k = 0; k < 4; ++k) {
            char c = cleaned[i + k];
            if (c == '=') {
                sextets[k] = 0;
            } else {
                const char* pos = std::strchr(BASE64_CHARS, c);
                if (pos) {
                    sextets[k] = static_cast<int>(pos - BASE64_CHARS);
                } else {
                    // 无效字符，跳过
                    sextets[k] = 0;
                }
            }
        }

        uint32_t triple = (static_cast<uint32_t>(sextets[0]) << 18) |
                         (static_cast<uint32_t>(sextets[1]) << 12) |
                         (static_cast<uint32_t>(sextets[2]) << 6) |
                         static_cast<uint32_t>(sextets[3]);

        if (j < outLen) result.push_back(static_cast<uint8_t>((triple >> 16) & 0xFF));
        if (j + 1 < outLen) result.push_back(static_cast<uint8_t>((triple >> 8) & 0xFF));
        if (j + 2 < outLen) result.push_back(static_cast<uint8_t>(triple & 0xFF));
    }

    return result;
}

std::string TileDataDecoder::Base64Encode(const std::vector<uint8_t>& data) {
    std::string result;
    result.reserve(((data.size() + 2) / 3) * 4);

    for (size_t i = 0; i < data.size(); i += 3) {
        uint32_t triple = (static_cast<uint32_t>(data[i]) << 16);

        if (i + 1 < data.size()) {
            triple |= static_cast<uint32_t>(data[i + 1]) << 8;
        }
        if (i + 2 < data.size()) {
            triple |= data[i + 2];
        }

        result.push_back(BASE64_CHARS[(triple >> 18) & 0x3F]);
        result.push_back(BASE64_CHARS[(triple >> 12) & 0x3F]);
        result.push_back((i + 1 < data.size()) ? BASE64_CHARS[(triple >> 6) & 0x3F] : '=');
        result.push_back((i + 2 < data.size()) ? BASE64_CHARS[triple & 0x3F] : '=');
    }

    return result;
}

// ============================================================================
// CSV 解析
// ============================================================================

std::vector<uint32_t> TileDataDecoder::ParseCSV(const std::string& csvData, int expectedSize) {
    std::vector<uint32_t> result;
    std::istringstream stream(csvData);
    std::string token;

    result.reserve(expectedSize > 0 ? static_cast<size_t>(expectedSize) : 1024);

    while (std::getline(stream, token, ',')) {
        // 移除空白
        token.erase(0, token.find_first_not_of(" \t\r\n"));
        token.erase(token.find_last_not_of(" \t\r\n") + 1);

        if (!token.empty()) {
            try {
                uint32_t value = static_cast<uint32_t>(std::stoul(token));
                result.push_back(value);
            } catch (const std::exception&) {
                // 忽略无效值
            }
        }
    }

    return result;
}

// ============================================================================
// Base64 解析 (无压缩)
// ============================================================================

std::vector<uint32_t> TileDataDecoder::ParseBase64(const std::string& base64Data, int expectedSize) {
    std::vector<uint8_t> decoded = Base64Decode(base64Data);
    std::vector<uint32_t> result;

    size_t numGids = decoded.size() / 4;
    result.reserve(numGids);

    for (size_t i = 0; i < numGids; ++i) {
        uint32_t gid = static_cast<uint32_t>(decoded[i * 4]) |
                      static_cast<uint32_t>(decoded[i * 4 + 1]) << 8 |
                      static_cast<uint32_t>(decoded[i * 4 + 2]) << 16 |
                      static_cast<uint32_t>(decoded[i * 4 + 3]) << 24;
        result.push_back(gid);
    }

    return result;
}

// ============================================================================
// Base64 + zlib 解压
// ============================================================================

std::vector<uint8_t> TileDataDecoder::DecompressZlib(
    const uint8_t* compressedData,
    size_t compressedSize,
    size_t expectedSize
) {
    std::vector<uint8_t> result;

    z_stream stream = {};
    stream.next_in = const_cast<uint8_t*>(compressedData);
    stream.avail_in = static_cast<uInt>(compressedSize);

    // 初始化解压
    if (inflateInit(&stream) != Z_OK) {
        return result;
    }

    // 预分配空间
    result.reserve(expectedSize > 0 ? expectedSize : compressedSize * 4);

    // 解压循环
    int ret;
    std::array<uint8_t, 16384> buffer;
    do {
        stream.next_out = buffer.data();
        stream.avail_out = buffer.size();

        ret = inflate(&stream, Z_NO_FLUSH);

        if (ret == Z_OK || ret == Z_STREAM_END) {
            size_t have = buffer.size() - stream.avail_out;
            result.insert(result.end(), buffer.data(), buffer.data() + have);
        }
    } while (ret == Z_OK);

    inflateEnd(&stream);

    if (ret != Z_STREAM_END) {
        // 解压失败
        return {};
    }

    return result;
}

std::vector<uint32_t> TileDataDecoder::ParseBase64Zlib(const std::string& base64Data, int expectedSize) {
    std::vector<uint8_t> compressed = Base64Decode(base64Data);

    if (compressed.empty()) {
        return {};
    }

    std::vector<uint8_t> decompressed = DecompressZlib(
        compressed.data(),
        compressed.size(),
        expectedSize > 0 ? static_cast<size_t>(expectedSize * 4) : 0
    );

    std::vector<uint32_t> result;
    size_t numGids = decompressed.size() / 4;
    result.reserve(numGids);

    for (size_t i = 0; i < numGids; ++i) {
        uint32_t gid = static_cast<uint32_t>(decompressed[i * 4]) |
                      static_cast<uint32_t>(decompressed[i * 4 + 1]) << 8 |
                      static_cast<uint32_t>(decompressed[i * 4 + 2]) << 16 |
                      static_cast<uint32_t>(decompressed[i * 4 + 3]) << 24;
        result.push_back(gid);
    }

    return result;
}

// ============================================================================
// Base64 + zstd 解压
// ============================================================================

#ifdef PRISMA_USE_ZSTD
std::vector<uint8_t> TileDataDecoder::DecompressZstd(
    const uint8_t* compressedData,
    size_t compressedSize,
    size_t expectedSize
) {
    // 获取解压后的大小
    size_t decompressedSize = ZSTD_getDecompressedSize(compressedData, compressedSize);
    if (decompressedSize == 0 && expectedSize > 0) {
        decompressedSize = expectedSize;
    } else if (decompressedSize == 0) {
        // 预估大小
        decompressedSize = compressedSize * 4;
    }

    std::vector<uint8_t> result(decompressedSize);

    size_t actualSize = ZSTD_decompress(
        result.data(),
        decompressedSize,
        compressedData,
        compressedSize
    );

    if (ZSTD_isError(actualSize)) {
        return {};
    }

    result.resize(actualSize);
    return result;
}

std::vector<uint32_t> TileDataDecoder::ParseBase64Zstd(const std::string& base64Data, int expectedSize) {
    std::vector<uint8_t> compressed = Base64Decode(base64Data);

    if (compressed.empty()) {
        return {};
    }

    std::vector<uint8_t> decompressed = DecompressZstd(
        compressed.data(),
        compressed.size(),
        expectedSize > 0 ? static_cast<size_t>(expectedSize * 4) : 0
    );

    std::vector<uint32_t> result;
    size_t numGids = decompressed.size() / 4;
    result.reserve(numGids);

    for (size_t i = 0; i < numGids; ++i) {
        uint32_t gid = static_cast<uint32_t>(decompressed[i * 4]) |
                      static_cast<uint32_t>(decompressed[i * 4 + 1]) << 8 |
                      static_cast<uint32_t>(decompressed[i * 4 + 2]) << 16 |
                      static_cast<uint32_t>(decompressed[i * 4 + 3]) << 24;
        result.push_back(gid);
    }

    return result;
}

#else // PRISMA_USE_ZSTD

std::vector<uint8_t> TileDataDecoder::DecompressZstd(
    const uint8_t*,
    size_t,
    size_t
) {
    return {};
}

std::vector<uint32_t> TileDataDecoder::ParseBase64Zstd(const std::string&, int) {
    return {};
}

#endif // PRISMA_USE_ZSTD

// ============================================================================
// Base64 + gzip 解压
// ============================================================================

std::vector<uint8_t> TileDataDecoder::DecompressGzip(
    const uint8_t* compressedData,
    size_t compressedSize,
    size_t expectedSize
) {
    std::vector<uint8_t> result;

    // gzip 和 zlib 格式不同，需要使用 inflateInit2
    z_stream stream = {};
    stream.next_in = const_cast<uint8_t*>(compressedData);
    stream.avail_in = static_cast<uInt>(compressedSize);

    // 窗口大小设为 15 + 16 表示 gzip 格式
    if (inflateInit2(&stream, 15 + 16) != Z_OK) {
        return result;
    }

    result.reserve(expectedSize > 0 ? expectedSize : compressedSize * 4);

    int ret;
    std::array<uint8_t, 16384> buffer;
    do {
        stream.next_out = buffer.data();
        stream.avail_out = buffer.size();

        ret = inflate(&stream, Z_NO_FLUSH);

        if (ret == Z_OK || ret == Z_STREAM_END) {
            size_t have = buffer.size() - stream.avail_out;
            result.insert(result.end(), buffer.data(), buffer.data() + have);
        }
    } while (ret == Z_OK);

    inflateEnd(&stream);

    if (ret != Z_STREAM_END) {
        return {};
    }

    return result;
}

std::vector<uint32_t> TileDataDecoder::ParseBase64Gzip(const std::string& base64Data, int expectedSize) {
    std::vector<uint8_t> compressed = Base64Decode(base64Data);

    if (compressed.empty()) {
        return {};
    }

    std::vector<uint8_t> decompressed = DecompressGzip(
        compressed.data(),
        compressed.size(),
        expectedSize > 0 ? static_cast<size_t>(expectedSize * 4) : 0
    );

    std::vector<uint32_t> result;
    size_t numGids = decompressed.size() / 4;
    result.reserve(numGids);

    for (size_t i = 0; i < numGids; ++i) {
        uint32_t gid = static_cast<uint32_t>(decompressed[i * 4]) |
                      static_cast<uint32_t>(decompressed[i * 4 + 1]) << 8 |
                      static_cast<uint32_t>(decompressed[i * 4 + 2]) << 16 |
                      static_cast<uint32_t>(decompressed[i * 4 + 3]) << 24;
        result.push_back(gid);
    }

    return result;
}

// ============================================================================
// 主解码方法
// ============================================================================

std::vector<uint32_t> TileDataDecoder::Decode(
    const std::string& data,
    TileDataEncoding encoding,
    int expectedSize
) {
    switch (encoding) {
        case TileDataEncoding::CSV:
            return ParseCSV(data, expectedSize);

        case TileDataEncoding::Base64:
            return ParseBase64(data, expectedSize);

        case TileDataEncoding::Base64_Zlib:
            return ParseBase64Zlib(data, expectedSize);

        case TileDataEncoding::Base64_Zstd:
            return ParseBase64Zstd(data, expectedSize);

        case TileDataEncoding::Base64_Gzip:
            return ParseBase64Gzip(data, expectedSize);

        default:
            return {};
    }
}

// ============================================================================
// 编码方法
// ============================================================================

std::vector<uint8_t> TileDataDecoder::CompressZlib(const uint8_t* data, size_t dataSize) {
    std::vector<uint8_t> result;

    z_stream stream = {};
    stream.next_in = const_cast<uint8_t*>(data);
    stream.avail_in = static_cast<uInt>(dataSize);

    if (deflateInit(&stream, Z_DEFAULT_COMPRESSION) != Z_OK) {
        return result;
    }

    std::array<uint8_t, 16384> buffer;
    int ret;
    do {
        stream.next_out = buffer.data();
        stream.avail_out = buffer.size();

        ret = deflate(&stream, Z_FINISH);

        if (ret == Z_OK || ret == Z_STREAM_END) {
            size_t have = buffer.size() - stream.avail_out;
            result.insert(result.end(), buffer.data(), buffer.data() + have);
        }
    } while (ret == Z_OK);

    deflateEnd(&stream);

    if (ret != Z_STREAM_END) {
        return {};
    }

    return result;
}

#ifdef PRISMA_USE_ZSTD
std::vector<uint8_t> TileDataDecoder::CompressZstd(const uint8_t* data, size_t dataSize) {
    size_t compressedSize = ZSTD_compressBound(dataSize);
    std::vector<uint8_t> result(compressedSize);

    size_t actualSize = ZSTD_compress(
        result.data(),
        compressedSize,
        data,
        dataSize,
        3  // 压缩级别
    );

    if (ZSTD_isError(actualSize)) {
        return {};
    }

    result.resize(actualSize);
    return result;
}

#else

std::vector<uint8_t> TileDataDecoder::CompressZstd(const uint8_t*, size_t) {
    return {};
}

#endif

std::vector<uint8_t> TileDataDecoder::CompressGzip(const uint8_t* data, size_t dataSize) {
    std::vector<uint8_t> result;

    z_stream stream = {};
    stream.next_in = const_cast<uint8_t*>(data);
    stream.avail_in = static_cast<uInt>(dataSize);

    // 15 + 16 表示 gzip 格式
    if (deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        return result;
    }

    std::array<uint8_t, 16384> buffer;
    int ret;
    do {
        stream.next_out = buffer.data();
        stream.avail_out = buffer.size();

        ret = deflate(&stream, Z_FINISH);

        if (ret == Z_OK || ret == Z_STREAM_END) {
            size_t have = buffer.size() - stream.avail_out;
            result.insert(result.end(), buffer.data(), buffer.data() + have);
        }
    } while (ret == Z_OK);

    deflateEnd(&stream);

    if (ret != Z_STREAM_END) {
        return {};
    }

    return result;
}

std::string TileDataDecoder::Encode(
    const std::vector<uint32_t>& data,
    TileDataEncoding encoding,
    bool compression
) {
    // 转换为字节数组
    std::vector<uint8_t> bytes;
    bytes.reserve(data.size() * 4);

    for (uint32_t gid : data) {
        bytes.push_back(static_cast<uint8_t>(gid & 0xFF));
        bytes.push_back(static_cast<uint8_t>((gid >> 8) & 0xFF));
        bytes.push_back(static_cast<uint8_t>((gid >> 16) & 0xFF));
        bytes.push_back(static_cast<uint8_t>((gid >> 24) & 0xFF));
    }

    std::vector<uint8_t> processed;

    switch (encoding) {
        case TileDataEncoding::CSV: {
            std::string result;
            result.reserve(data.size() * 6);
            for (size_t i = 0; i < data.size(); ++i) {
                if (i > 0) result += ',';
                result += std::to_string(data[i]);
            }
            return result;
        }

        case TileDataEncoding::Base64:
            processed = bytes;
            break;

        case TileDataEncoding::Base64_Zlib:
            if (compression) {
                processed = CompressZlib(bytes.data(), bytes.size());
            } else {
                processed = bytes;
            }
            break;

        case TileDataEncoding::Base64_Zstd:
            if (compression) {
                processed = CompressZstd(bytes.data(), bytes.size());
            } else {
                processed = bytes;
            }
            break;

        case TileDataEncoding::Base64_Gzip:
            if (compression) {
                processed = CompressGzip(bytes.data(), bytes.size());
            } else {
                processed = bytes;
            }
            break;

        default:
            return "";
    }

    return Base64Encode(processed);
}

} // namespace PrismaEngine
