# 纯逻辑测试 — 逐个子系统
# 不链接 Engine，不需要 Vulkan SDK

# Wave 2 — 基础模块
add_subdirectory(math)
add_subdirectory(core)
add_subdirectory(memory)
add_subdirectory(logger)
add_subdirectory(serialization)
add_subdirectory(audio)

# Wave 3 — 功能模块
add_subdirectory(animation)
add_subdirectory(ai)
add_subdirectory(navigation)

# Wave 4 — 补充子系统测试
add_subdirectory(render)
add_subdirectory(engine)
add_subdirectory(editor)
