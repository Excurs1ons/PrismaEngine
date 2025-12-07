import io
import os
import json
import sys
import subprocess
from pathlib import Path

# 修改console编码为 utf8
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf8')
print(f"Python Version = {sys.version}")
print(f"Current Work Dir = {os.getcwd()}")


def check_dxc_compiler():
    """检查DXC编译器是否存在"""
    # 检查常见的DXC路径
    possible_paths = [
        "tools/dxc_2025_07_14/bin/x64/dxc.exe",
        "tools/dxc_2025_07_14/dxc.exe",
        "third_party/dxc/dxc.exe",
        "dxc.exe"
    ]

    for path in possible_paths:
        if os.path.exists(path):
            pass
            return path


    # 检查系统PATH中是否有dxc
    try:
        subprocess.run(["dxc", "--version"], capture_output=True, check=True)
        return "dxc"
    except (subprocess.CalledProcessError, FileNotFoundError):
        pass

    return None


# 设置DXC编译命令
cmd_dxc_vert = "{dxcPath} -spirv -T {0} -E {1} {2} -Fo {3}"
cmd_dxc_vert = cmd_dxc_vert.replace("{dxcPath}",check_dxc_compiler())
print(cmd_dxc_vert)
def load_config():
    """加载配置文件"""
    try:
        with open('tools/python/config.json', 'r', encoding='utf-8') as f:
            return json.load(f)
    except FileNotFoundError:
        print("错误: 找不到配置文件 tools/python/config.json")
        return None


def find_shader_files(shader_dir):
    """查找目录中所有的着色器源文件"""
    shader_files = []
    extensions = ['.hlsl', '.vert', '.frag']

    for file_path in Path(shader_dir).rglob('*'):
        if file_path.suffix in extensions:
            shader_files.append(file_path)

    return shader_files


def compile_shader_dxcompiler(shader_path, output_path, profile):
    """使用DXCompiler编译着色器为SPIR-V"""
    dxc_path = check_dxc_compiler()

    if not dxc_path:
        print(f"警告: DXC编译器未找到，跳过编译 {shader_path}")
        return False

    cmd = [
        dxc_path,
        "-spirv",
        "-T", profile,
        "-E", "main",
        str(shader_path),
        "-Fo", str(output_path)
    ]

    try:
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode != 0:
            print(f"编译失败 {shader_path}: {result.stderr}")
            return False
        else:
            print(f"成功编译: {shader_path} -> {output_path}")
            return True
    except Exception as e:
        print(f"执行编译命令时出错: {e}")
        return False
# 顶点着色器的入口函数
vert_patterns=["VSMain","main_vs","main_vert","main_vertex","vert"]
# 像素着色器的入口函数
pixel_patterns=["PSMain","main_frag","main_ps","main_pixel","frag"]
# 同时出现则说明是SM60的

def get_shader_profile(file_path):
    """根据文件内容确定入口函数和类型"""
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            lines = f.readlines()
            for line in lines:

    """根据文件名确定着色器类型和配置文件"""
    name = file_path.name.lower()
    if name.endswith('.vert.hlsl') or name.endswith('.vert'):
        return "vs_5_0", "vert"
    elif name.endswith('.frag.hlsl') or name.endswith('.frag'):
        return "ps_5_0", "frag"
    elif 'vertex' in name or 'vert' in name:
        return "vs_5_0", "vert"
    elif 'pixel' in name or 'fragment' in name or 'frag' in name:
        return "ps_5_0", "frag"
    else:
        return "vs_5_0", "vert"  # 默认为顶点着色器


def show_shader_info(shader_files):
    """显示着色器文件信息"""
    print("\n着色器文件详情:")
    print("-" * 50)

    for shader_file in shader_files:
        stat = shader_file.stat()
        print(f"文件: {shader_file}")
        print(f"  大小: {stat.st_size} 字节")
        print(f"  修改时间: {stat.st_mtime}")

        # 显示前几行内容
        try:
            with open(shader_file, 'r', encoding='utf-8') as f:
                lines = f.readlines()
                print(f"  内容预览 (前5行):")
                for i, line in enumerate(lines[:5]):
                    print(f"    {line.rstrip()}")
                if len(lines) > 5:
                    print("    ...")
        except Exception as e:
            print(f"  无法读取文件内容: {e}")
        print()


def get_compile_command(shader_files):
    print("\n生成编译命令:")
    commands = dict()
    for shader_file in shader_files:
        dx_profile, shader_type = get_shader_profile(shader_file)
        output_path = shader_file.with_suffix('.spv')
        command = cmd_dxc_vert.format(dx_profile, shader_type, shader_file, output_path)
        commands[shader_file] = command
    return commands


def main():
    config = load_config()
    if not config:
        return

    shader_dir = config.get("path", "assets/shaders")
    # 使用绝对路径
    full_shader_dir = Path(os.getcwd()) / shader_dir

    if not full_shader_dir.exists():
        print(f"着色器目录不存在: {full_shader_dir}")
        return

    print(f"正在扫描着色器目录: {full_shader_dir}")
    shader_files = find_shader_files(full_shader_dir)

    if not shader_files:
        print("未找到任何着色器文件")
        return

    print(f"找到 {len(shader_files)} 个着色器文件:")
    for shader in shader_files:
        print(f"  - {shader}")

    # 生成编译命令
    get_compile_command(shader_files)
    # 显示着色器文件详细信息
    show_shader_info(shader_files)

    # 检查编译器
    dxc_path = check_dxc_compiler()
    if dxc_path:
        print(f"找到DXC编译器: {dxc_path}")

        # 编译每个着色器
        compiled_count = 0
        for shader_file in shader_files:
            # 生成SPIR-V输出路径 - 直接在原文件基础上替换扩展名
            spv_output = shader_file.with_suffix('.spv')
            print(f"正在编译: {shader_file.name}")

            dx_profile, shader_type = get_shader_profile(shader_file)

            # 编译为SPIR-V
            if compile_shader_dxcompiler(shader_file, spv_output, dx_profile):
                compiled_count += 1

        print(f"\n编译完成! 成功编译 {compiled_count}/{len(shader_files)} 个着色器文件")
    else:
        print("\n未找到DXC编译器，跳过编译步骤")
        print("请安装DXC编译器以启用着色器编译功能:")
        print("1. 从 https://github.com/microsoft/DirectXShaderCompiler/releases 下载")
        print("2. 将dxc.exe放置在tools/dxc_2025_07_14/目录下")
        print("3. 或者确保dxc在系统PATH环境变量中")


if __name__ == "__main__":
    main()
