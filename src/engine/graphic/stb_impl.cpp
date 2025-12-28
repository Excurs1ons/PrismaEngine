// stb 库实现
// 避免符号重定义错误 - 只在这个文件里定义实现宏

#define STB_TRUETYPE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_RECT_PACK_IMPLEMENTATION

// stb 头文件（由 FetchContent 下载，通过 CMake include 目录添加）
#include "stb_truetype.h"
#include "stb_image.h"
#include "stb_rect_pack.h"
