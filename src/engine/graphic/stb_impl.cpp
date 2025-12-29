// stb 库实现
// 避免符号重定义错误 - 只在这个文件里定义实现宏
// 不定义 STBTT_STATIC/STBRP_STATIC，以便函数可以被其他编译单元链接

// stb_rect_pack.h 需要独立实现
#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

// stb_truetype.h
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

// stb_image.h (独立实现)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
