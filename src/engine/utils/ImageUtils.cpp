#include "ImageUtils.h"
#include <stb_image_write.h>

namespace Prisma::Utils {

bool ImageUtils::SavePNG(const std::string& filename, int w, int h, int comp, const void* data, int stride_in_bytes) {
    return stbi_write_png(filename.c_str(), w, h, comp, data, stride_in_bytes) != 0;
}

} // namespace Prisma::Utils
