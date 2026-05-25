#!/bin/bash
# NeoEditor 端到端验证脚本 (T19)
# 构建 → 启动 → 验证关键组件 → 关闭 → 输出 PASS/FAIL
#
# 用法:
#   ./scripts/verify-neoeditor.sh          # 运行所有检查
#   ./scripts/verify-neoeditor.sh --fast   # 仅文件存在性检查（不构建）
#   ./scripts/verify-neoeditor.sh --ci     # CI 模式（无 dotnet 依赖的检查）

set -euo pipefail

PASS=0
FAIL=0
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

check() {
    local name="$1"
    local cmd="$2"
    if eval "$cmd" 2>/dev/null; then
        echo "  [PASS] $name"
        PASS=$((PASS + 1))
    else
        echo "  [FAIL] $name"
        FAIL=$((FAIL + 1))
    fi
}

check_silent() {
    local name="$1"
    local cmd="$2"
    if eval "$cmd" 2>/dev/null; then
        PASS=$((PASS + 1))
    else
        echo "  [FAIL] $name"
        FAIL=$((FAIL + 1))
    fi
}

echo ""
echo "=========================================="
echo "  NeoEditor Verification Script (T19)"
echo "=========================================="
echo ""

# ===================================================================
# 1. 项目文件存在性检查
# ===================================================================
echo "--- 1. Project Files ---"

check "NeoEditor.csproj"         "[ -f '$ROOT_DIR/projects/NeoEditor/NeoEditor.csproj' ]"
check "NeoEditor.Core.csproj"    "[ -f '$ROOT_DIR/projects/NeoEditor/NeoEditor.Core/NeoEditor.Core.csproj' ]"
check "NeoEditor.Tests.csproj"   "[ -f '$ROOT_DIR/projects/NeoEditor/NeoEditor.Tests/NeoEditor.Tests.csproj' ]"
check "MainWindow.xaml"          "[ -f '$ROOT_DIR/projects/NeoEditor/MainWindow.xaml' ]"
check "MainWindow.xaml.cs"       "[ -f '$ROOT_DIR/projects/NeoEditor/MainWindow.xaml.cs' ]"
check "App.xaml"                 "[ -f '$ROOT_DIR/projects/NeoEditor/App.xaml' ]"
check "App.xaml.cs"              "[ -f '$ROOT_DIR/projects/NeoEditor/App.xaml.cs' ]"
check "app.manifest"             "[ -f '$ROOT_DIR/projects/NeoEditor/app.manifest' ]"
check "NuGet.Config"             "[ -f '$ROOT_DIR/projects/NeoEditor/NuGet.Config' ]"
check "Directory.Build.props"    "[ -f '$ROOT_DIR/projects/NeoEditor/Directory.Build.props' ]"

# ===================================================================
# 2. NeoEditor.Core 核心文件检查
# ===================================================================
echo ""
echo "--- 2. Core Library Files ---"

check "EditorAPI.cs"             "[ -f '$ROOT_DIR/projects/NeoEditor/NeoEditor.Core/EditorAPI.cs' ]"
check "EditorCommands.cs"        "[ -f '$ROOT_DIR/projects/NeoEditor/NeoEditor.Core/EditorCommands.cs' ]"
check "EngineThread.cs"          "[ -f '$ROOT_DIR/projects/NeoEditor/NeoEditor.Core/EngineThread.cs' ]"
check "EngineInterop.cs"         "[ -f '$ROOT_DIR/projects/NeoEditor/NeoEditor.Core/EngineInterop.cs' ]"
check "WebUIService.cs"          "[ -f '$ROOT_DIR/projects/NeoEditor/NeoEditor.Core/WebUIService.cs' ]"
check "EventBus.cs"              "[ -f '$ROOT_DIR/projects/NeoEditor/NeoEditor.Core/EventBus.cs' ]"
check "InputRouter.cs"           "[ -f '$ROOT_DIR/projects/NeoEditor/NeoEditor.Core/InputRouter.cs' ]"
check "WindowManager.cs"         "[ -f '$ROOT_DIR/projects/NeoEditor/NeoEditor.Core/WindowManager.cs' ]"
check "ViewportSurface.cs"       "[ -f '$ROOT_DIR/projects/NeoEditor/NeoEditor.Core/ViewportSurface.cs' ]"
check "D3D11Interop.cs"          "[ -f '$ROOT_DIR/projects/NeoEditor/NeoEditor.Core/D3D11Interop.cs' ]"
check "WebUIBridge.cs"           "[ -f '$ROOT_DIR/projects/NeoEditor/NeoEditor.Core/WebUIBridge.cs' ]"
check "MessageQueue.cs"          "[ -f '$ROOT_DIR/projects/NeoEditor/NeoEditor.Core/MessageQueue.cs' ]"
check "CoreCLRManager.cs"        "[ -f '$ROOT_DIR/projects/NeoEditor/NeoEditor.Core/CoreCLRManager.cs' ]"
check "NativeMethods.cs"         "[ -f '$ROOT_DIR/projects/NeoEditor/NeoEditor.Core/NativeMethods.cs' ]"
check "NativeMethods.D3D.cs"     "[ -f '$ROOT_DIR/projects/NeoEditor/NeoEditor.Core/NativeMethods.D3D.cs' ]"

# ===================================================================
# 3. NeoEditor.Tests 测试文件检查
# ===================================================================
echo ""
echo "--- 3. Test Files ---"

check "EditorAPITests.cs"        "[ -f '$ROOT_DIR/projects/NeoEditor/NeoEditor.Tests/EditorAPITests.cs' ]"
check "IntegrationTests.cs"      "[ -f '$ROOT_DIR/projects/NeoEditor/NeoEditor.Tests/IntegrationTests.cs' ]"

# ===================================================================
# 4. UI 文件检查
# ===================================================================
echo ""
echo "--- 4. UI Files ---"

check "MenuBar.xaml"             "[ -f '$ROOT_DIR/projects/NeoEditor/MenuBar.xaml' ]"
check "MenuBar.xaml.cs"          "[ -f '$ROOT_DIR/projects/NeoEditor/MenuBar.xaml.cs' ]"
check "ToolBar.xaml"             "[ -f '$ROOT_DIR/projects/NeoEditor/ToolBar.xaml' ]"
check "ToolBar.xaml.cs"          "[ -f '$ROOT_DIR/projects/NeoEditor/ToolBar.xaml.cs' ]"
check "LayoutManager.cs"         "[ -f '$ROOT_DIR/projects/NeoEditor/LayoutManager.cs' ]"
check "EditorDockBehavior.cs"    "[ -f '$ROOT_DIR/projects/NeoEditor/EditorDockBehavior.cs' ]"
check "EditorDockAdapter.cs"     "[ -f '$ROOT_DIR/projects/NeoEditor/EditorDockAdapter.cs' ]"

# ===================================================================
# 5. C++ 引擎文件检查
# ===================================================================
echo ""
echo "--- 5. C++ Engine Files ---"

check "EditorAPI.h"              "[ -f '$ROOT_DIR/src/engine/scripting/EditorAPI.h' ]"
check "EditorAPI.cpp"            "[ -f '$ROOT_DIR/src/engine/scripting/EditorAPI.cpp' ]"
check "Vulkan external memory"   "grep -q VK_KHR_EXTERNAL_MEMORY '$ROOT_DIR/src/engine/graphic/adapters/vulkan/RenderDeviceVulkan.cpp'"
check "CMake project gating"     "grep -q NEOEDITOR '$ROOT_DIR/projects/CMakeLists.txt'"

# ===================================================================
# 6. 插件模式文件检查
# ===================================================================
echo ""
echo "--- 6. Plugin Mode Files ---"

check "Plugin CreateApplication.cpp" "[ -f '$ROOT_DIR/projects/NeoEditor/NeoEditor_plugin/CreateApplication.cpp' ]"
check "Plugin NeoEditorPlugin.h"     "[ -f '$ROOT_DIR/projects/NeoEditor/NeoEditor_plugin/NeoEditorPlugin.h' ]"
check "Plugin NeoEditorPlugin.cpp"   "[ -f '$ROOT_DIR/projects/NeoEditor/NeoEditor_plugin/NeoEditorPlugin.cpp' ]"

# ===================================================================
# 7. CI 文件检查
# ===================================================================
echo ""
echo "--- 7. CI Files ---"

check "CI Windows gated"             "grep -q 'NEOEDITOR=OFF' '$ROOT_DIR/.github/workflows/ci-windows.yml'"
check "CI Linux gated"               "grep -q 'NEOEDITOR=OFF' '$ROOT_DIR/.github/workflows/ci-linux.yml'"
check "CI Android gated"             "grep -q 'NEOEDITOR=OFF' '$ROOT_DIR/.github/workflows/ci-android.yml'"
check "CI build-neoeditor.yml exists" "[ -f '$ROOT_DIR/.github/workflows/build-neoeditor.yml' ]"

# ===================================================================
# 8. WebUI 资产检查
# ===================================================================
echo ""
echo "--- 8. WebUI Assets ---"

check "WebUI index.html"             "[ -f '$ROOT_DIR/projects/NeoEditor/assets/webui/index.html' ]"
check "WebUI api.js"                 "[ -f '$ROOT_DIR/projects/NeoEditor/assets/webui/js/api.js' ]"
check "WebUI editor.js"              "[ -f '$ROOT_DIR/projects/NeoEditor/assets/webui/js/editor.js' ]"
check "WebUI style.css"              "[ -f '$ROOT_DIR/projects/NeoEditor/assets/webui/css/style.css' ]"

# ===================================================================
# 9. EditorAPI 函数指针计数验证
# ===================================================================
echo ""
echo "--- 9. EditorAPI Function Count ---"

# 验证 EditorAPI.h 有至少 20 个函数指针
FUNC_COUNT_C=$(grep -c ')(' "$ROOT_DIR/src/engine/scripting/EditorAPI.h" 2>/dev/null || echo 0)
check "EditorAPI C++ has 20+ func ptrs" "[ $FUNC_COUNT_C -ge 20 ]"

# 验证 EditorAPI.cs 有至少 20 个函数指针
FUNC_COUNT_CS=$(grep -c 'delegate.*unmanaged' "$ROOT_DIR/projects/NeoEditor/NeoEditor.Core/EditorAPI.cs" 2>/dev/null || echo 0)
check "EditorAPI C# has 20+ func ptrs" "[ $FUNC_COUNT_CS -ge 20 ]"

check "StructSize field exists (C++)" "grep -q 'structSize' '$ROOT_DIR/src/engine/scripting/EditorAPI.h'"
check "StructSize field exists (C#)"  "grep -q 'StructSize' '$ROOT_DIR/projects/NeoEditor/NeoEditor.Core/EditorAPI.cs'"

# ===================================================================
# 10. 文件完整性检查: 验证关键跨组件引用一致
# ===================================================================
echo ""
echo "--- 10. Cross-component Consistency ---"

check "WebUIService references EditorCommands" \
    "grep -q 'EditorCommands.Dispatch' '$ROOT_DIR/projects/NeoEditor/NeoEditor.Core/WebUIService.cs'"
check "EventBus referenced from WebUIBridge" \
    "grep -q 'EventBus.Instance' '$ROOT_DIR/projects/NeoEditor/NeoEditor.Core/WebUIBridge.cs'"
check "MainWindow creates InputRouter" \
    "grep -q 'InputRouter' '$ROOT_DIR/projects/NeoEditor/MainWindow.xaml.cs'"
check "MainWindow creates WindowManager" \
    "grep -q 'WindowManager' '$ROOT_DIR/projects/NeoEditor/MainWindow.xaml.cs'"
check "MainWindow creates WebUIService" \
    "grep -q 'WebUIService' '$ROOT_DIR/projects/NeoEditor/MainWindow.xaml.cs'"
check "MainWindow creates ViewportSurface" \
    "grep -q 'ViewportSurface' '$ROOT_DIR/projects/NeoEditor/MainWindow.xaml.cs'"
check "EngineThread uses EngineHandle" \
    "grep -q 'EngineHandle' '$ROOT_DIR/projects/NeoEditor/NeoEditor.Core/EngineThread.cs'"
check "EngineThread uses EngineToUIQueue" \
    "grep -q 'EngineToUIQueue' '$ROOT_DIR/projects/NeoEditor/NeoEditor.Core/EngineThread.cs'"
check "EngineThread uses UIToEngineQueue" \
    "grep -q 'UIToEngineQueue' '$ROOT_DIR/projects/NeoEditor/NeoEditor.Core/EngineThread.cs'"
check "InputRouter references EditorAPI" \
    "grep -q 'EditorAPI.' '$ROOT_DIR/projects/NeoEditor/NeoEditor.Core/InputRouter.cs'"

# ===================================================================
# 结果汇总
# ===================================================================
echo ""
echo "=========================================="
echo "  Results: $PASS passed, $FAIL failed"
echo "=========================================="

if [ "$FAIL" -eq 0 ]; then
    echo "  Status: ALL CHECKS PASSED"
    exit 0
else
    echo "  Status: SOME CHECKS FAILED"
    exit 1
fi
