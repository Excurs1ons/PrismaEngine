#pragma once
#include "Export.h"
#include <string>

namespace Prisma::Utils {

/**
 * @brief 图像处理工具类
 */
class ENGINE_API ImageUtils {
public:
    /**
     * @brief 将图像数据保存为 PNG 文件
     * @param filename 文件名
     * @param w 宽度
     * @param h 高度
     * @param comp 通道数 (例如 3 为 RGB, 4 为 RGBA)
     * @param data 图像像素数据
     * @param stride_in_bytes 每行字节数 (0 则自动计算)
     * @return 是否成功
     */
    static bool SavePNG(const std::string& filename, int w, int h, int comp, const void* data, int stride_in_bytes = 0);
};

} // namespace Prisma::Utils
