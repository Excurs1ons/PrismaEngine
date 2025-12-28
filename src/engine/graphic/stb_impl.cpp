// stb 库实现
// 避免符号重定义错误 - 只在这个文件里定义实现宏

// STB 库需要在定义 IMPLEMENTATION 宏之前包含相应的头文件
// stb_truetype.h
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

// stb_rect_pack.h (独立实现，不依赖 stb_truetype)
#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

// stb_image.h (独立实现)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
