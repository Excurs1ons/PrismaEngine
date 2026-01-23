# DependencyVersions.cmake
# 第三方依赖版本锁定配置
# 修改这个文件后，运行 scripts/update-deps.sh 来验证兼容性

# ============================================================================
# 依赖版本定义 / Dependency Versions
# ============================================================================

# 使用 GIT_TAG 的最佳实践：
# 1. 优先使用 release tag (如 v1.0.0)
# 2. 对于没有 release 的项目，使用 commit SHA
# 3. 避免使用分支名（如 master/main）

# -------------------------------------------------------------------------------
# 核心依赖 / Core Dependencies
# -------------------------------------------------------------------------------

# GLM - 数学库
# GitHub: https://github.com/g-truc/glm
set(PRISMA_DEP_GLM_VERSION "1.0.2")

# nlohmann/json - JSON库
# GitHub: https://github.com/nlohmann/json
set(PRISMA_DEP_NLOHMANN_JSON_VERSION "v3.12.0")

# stb - 图像加载库 (header-only)
# GitHub: https://github.com/nothings/stb
# 锁定到具体 commit 而非 master 分支
set(PRISMA_DEP_STB_VERSION "af1a5bc352164740c1cc1354942b1c6b72eacb8a")  # 2024-01-15

# tinyxml2 - XML解析库 (用于 TMX 地图格式)
# GitHub: https://github.com/leethomason/tinyxml2
set(PRISMA_DEP_TINYXML2_VERSION "10.0.0")

# Zstandard - 压缩库 (用于 TMX Base64+zstd 格式)
# GitHub: https://github.com/facebook/zstd
set(PRISMA_DEP_ZSTD_VERSION "v1.5.6")

# Tweeny - 补间动画库 (header-only, 用于 UI 动画)
# GitHub: https://github.com/mobius3/tweeny
# 锁定到具体 commit 而非 master 分支
set(PRISMA_DEP_TWEENY_VERSION "3.2.0")

# -------------------------------------------------------------------------------
# 平台抽象 / Platform Abstraction
# -------------------------------------------------------------------------------

# SDL3 - 窗口/输入/音频抽象
# GitHub: https://github.com/libsdl-org/SDL
set(PRISMA_DEP_SDL3_VERSION "release-3.2.28")

# -------------------------------------------------------------------------------
# 图形后端 / Graphics Backends
# -------------------------------------------------------------------------------

# Vulkan-Headers - Vulkan头文件
# GitHub: https://github.com/KhronosGroup/Vulkan-Headers
set(PRISMA_DEP_VULKAN_HEADERS_VERSION "v1.4.328")

# VMA (Vulkan Memory Allocator) - Vulkan内存分配库
# GitHub: https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
set(PRISMA_DEP_VMA_VERSION "v3.1.0")

# vk-bootstrap - Vulkan初始化库
# GitHub: https://github.com/charles-lunarge/vk-bootstrap
set(PRISMA_DEP_VK_BOOTSTRAP_VERSION "v0.9")

# DirectX-Headers (Windows) - DirectX 12 头文件
# GitHub: https://github.com/microsoft/DirectX-Headers
set(PRISMA_DEP_DIRECTX_HEADERS_VERSION "v1.614.1")

# -------------------------------------------------------------------------------
# 编辑器工具 / Editor Tools
# -------------------------------------------------------------------------------

# ImGui - UI框架 (docking分支)
# GitHub: https://github.com/ocornut/imgui
# 使用 docking 分支的最新 commit
set(PRISMA_DEP_IMGUI_VERSION "docking")  # docking 分支相对稳定

# OpenFBX - FBX模型加载 (Windows only)
# GitHub: https://github.com/nem0/OpenFBX
# 锁定到具体 commit 而非 master 分支
set(PRISMA_DEP_OPENFBX_VERSION "c2ac836ed06b7c3fd1a57e6a8b0e3d9a5ebd2e91")  # 2024-01-10

# -------------------------------------------------------------------------------
# 超分辨率 / Upscalers
# -------------------------------------------------------------------------------

# FSR SDK - AMD FidelityFX SDK
# GitHub: https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK
set(PRISMA_DEP_FIDELITYFX_SDK_VERSION "v2.1.0")

# Streamline - NVIDIA DLSS SDK
# GitHub: https://github.com/NVIDIA-RTX/Streamline
set(PRISMA_DEP_STREAMLINE_VERSION "v2.9.0")

# -------------------------------------------------------------------------------
# 依赖兼容性矩阵 / Dependency Compatibility Matrix
# -------------------------------------------------------------------------------

# 记录经过测试的依赖组合
# 格式: "PRISMA_VERSION"
set(PRISMA_DEP_MATRIX_VERSION "1.0.0")

# 最后验证日期
set(PRISMA_DEP_LAST_VERIFIED "2025-01-24")

# 已知问题 / Known Issues
set(PRISMA_DEP_KNOWN_ISSUES
    "SDL3 3.2.28+: Android 构建需要 NDK r25+"
    "VMA 3.1.0: 需要 Vulkan-Headers 1.3.231+"
    "ImGui docking: 频繁更新，建议定期检查兼容性"
)

# ============================================================================
# 版本更新说明 / Version Update Guidelines
# ============================================================================

# 更新依赖版本时：
# 1. 更新对应的 PRISMA_DEP_*_VERSION 变量
# 2. 运行 scripts/update-deps.sh 验证编译
# 3. 运行测试确保没有破坏性变更
# 4. 更新 PRISMA_DEP_LAST_VERIFIED 日期
# 5. 在 PRISMA_DEP_KNOWN_ISSUES 中记录任何新问题

# 推荐更新策略：
# - 核心依赖（GLM, nlohmann/json）：只在有重要更新时升级
# - 图形API相关（Vulkan, DX12）：跟随上游版本更新
# - ImGui：由于使用 docking 分支，可以定期更新
# - 其他库：使用最新的稳定版本
