#include "ECS.h"

namespace PrismaEngine {
namespace Core {
namespace ECS {

// 静态成员定义
// 注意：由于 ComponentRegistry 的方法现在是 header-only，其静态成员 m_nextID 已经在类定义中初始化 (static inline)
// 如果编译器版本不支持 C++17 static inline，则需要在这里定义

} // namespace ECS
} // namespace Core
} // namespace PrismaEngine
