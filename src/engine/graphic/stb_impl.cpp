// stb 库实现
// 避免符号重定义错误 - 只在这个文件里定义实现宏

// 定义 STB 库为静态链接，避免符号导出问题
#define STBTT_STATIC
#define STBRP_STATIC

// 首先包含 stb_rect_pack.h 的声明部分（不定义 IMPLEMENTATION）
#include "stb_rect_pack.h"

// 然后定义并实现 stb_truetype（它内部会处理 rect pack）
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

// stb_image.h (独立实现)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
