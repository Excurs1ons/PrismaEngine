#!/bin/bash
# PrismaEngine CMake 工具库
# 提供 CMake 预设和构建的辅助函数

# 检查 CMake 是否可用
function check_cmake() {
    if ! command -v cmake &> /dev/null; then
        print_error "CMake not found"
        echo "Please install CMake:"
        echo "  Ubuntu/Debian: sudo apt install cmake"
        echo "  macOS: brew install cmake"
        echo "  Windows: Download from https://cmake.org/"
        return 1
    fi

    # 检查 CMake 版本
    local cmake_version=$(cmake --version | grep -oP '\d+\.\d+' | head -1)
    local major=$(echo $cmake_version | cut -d. -f1)
    local minor=$(echo $cmake_version | cut -d. -f2)

    if [ "$major" -lt 3 ] || ([ "$major" -eq 3 ] && [ "$minor" -lt 20 ]); then
        print_error "CMake version 3.20 or higher required (found $cmake_version)"
        return 1
    fi

    return 0
}

# 列出所有可用的 CMake 预设
function cmake_list_presets() {
    if [ ! -f "CMakePresets.json" ] && [ ! -f "CMakeUserPresets.json" ]; then
        return 1
    fi

    cmake --list-presets 2>/dev/null | grep '"' | sed 's/.*"\(.*\)".*/\1/' | sort
}

# 检查预设是否存在
function cmake_preset_exists() {
    local preset="$1"

    if [ -z "$preset" ]; then
        return 1
    fi

    local presets=$(cmake_list_presets)
    if echo "$presets" | grep -q "^${preset}$"; then
        return 0
    fi

    return 1
}

# 使用预设配置项目
function cmake_configure_preset() {
    local preset="$1"
    shift

    if ! cmake_preset_exists "$preset"; then
        print_error "Preset '$preset' not found"
        echo "Available presets:"
        cmake_list_presets | sed 's/^/  /'
        return 1
    fi

    cmake --preset "$preset" "$@"
}

# 使用预设构建项目
function cmake_build_preset() {
    local preset="$1"
    shift

    if ! cmake_preset_exists "$preset"; then
        print_error "Build preset '$preset' not found"
        echo "Available presets:"
        cmake --list-presets 2>&1 | grep -- "--build " | sed 's/.*"\(.*\)".*/\1/' | sed 's/^/  /'
        return 1
    fi

    cmake --build --preset "$preset" "$@"
}

# 手动配置项目（不使用预设）
function cmake_configure_manual() {
    local build_dir="$1"
    shift

    if [ -z "$build_dir" ]; then
        print_error "Build directory not specified"
        return 1
    fi

    cmake -B "$build_dir" "$@"
}

# 手动构建项目（不使用预设）
function cmake_build_manual() {
    local build_dir="$1"
    shift

    if [ ! -d "$build_dir" ]; then
        print_error "Build directory '$build_dir' does not exist"
        return 1
    fi

    cmake --build "$build_dir" "$@"
}

# 获取构建目录
function get_build_dir() {
    local preset="$1"

    if [ -f "CMakePresets.json" ]; then
        # 从 CMakePresets.json 读取 binaryDir
        local binary_dir=$(python3 -c "
import json
import sys
try:
    with open('CMakePresets.json', 'r') as f:
        presets = json.load(f)
    for preset_data in presets.get('configurePresets', []):
        if preset_data.get('name') == '$preset':
            # 处理继承
            binary_dir = preset_data.get('binaryDir', '')
            inherits = preset_data.get('inherits', [])
            while inherits and '\${sourceDir}' in binary_dir:
                for base in presets.get('configurePresets', []):
                    if base.get('name') in inherits:
                        base_dir = base.get('binaryDir', '')
                        if base_dir:
                            binary_dir = binary_dir.replace('\${sourceDir}', base_dir.split('\${sourceDir}')[0] if '\${sourceDir}' in base_dir else '')
                        inherits.remove(base.get('name'))
                        break
            print(binary_dir.replace('\${sourceDir}', '.'))
except:
    print('')
" 2>/dev/null)

        if [ -n "$binary_dir" ]; then
            echo "$binary_dir"
            return 0
        fi
    fi

    # 默认构建目录
    echo "build/$preset"
    return 0
}

# 清理构建目录
function clean_build_dir() {
    local build_dir="$1"

    if [ -z "$build_dir" ]; then
        print_error "Build directory not specified"
        return 1
    fi

    if [ -d "$build_dir" ]; then
        print_warning "Removing build directory: $build_dir"
        rm -rf "$build_dir"
        return 0
    fi

    return 0
}

# 显示 CMake 帮助
function show_cmake_help() {
    print_header "CMake Presets Help"
    echo ""
    echo "Available presets:"
    cmake_list_presets | sed 's/^/  /' || echo "  (No presets found)"
    echo ""
}
