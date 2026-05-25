#pragma once
#include "Export.h"
#include <string>

namespace Prisma::Utils {

// 图像处理工具类
class ENGINE_API ImageUtils {
public:
    /**
     * @brief 将图像数据保存为 PNG 文件
     */
    static bool SavePNG(const std::string& filename, int w, int h, int comp, const void* data, int stride_in_bytes = 0);
};

} // namespace Prisma::Utils
