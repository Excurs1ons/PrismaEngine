#pragma once

// 警告：此文件已废弃，请使用 #include "input/InputManager.h"
// 此文件将在未来版本中移除
#if defined(_MSC_VER)
#pragma message("Warning: KeyCode.h is deprecated. Use #include \"input/InputManager.h\" instead")
#elif defined(__GNUC__)
#warning "KeyCode.h is deprecated. Use #include \"input/InputManager.h\" instead"
#endif

// 转发到新的 InputManager.h
#include "input/InputManager.h"

// 注意：新的 KeyCode 是 enum class 类型，需要使用 KeyCode::Value 而非直接的 Value
// 例如：KeyCode::W 而非 W
