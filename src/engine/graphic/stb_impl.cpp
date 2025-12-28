// stb 库实现
// 避免符号重定义错误 - 只在这个文件里定义实现宏

// 定义 STB 库为静态链接，避免符号导出问题
#define STBTT_STATIC
#define STBRP_STATIC

// stb_rect_pack.h 需要独立实现
#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

// stb_truetype.h
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

// stb_image.h (独立实现)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
