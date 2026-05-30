package com.prismaengine;

import android.os.Bundle;
import org.libsdl.app.SDLActivity;

/**
 * PrismaEngine Android 主 Activity
 * 
 * 极致零解压架构：
 * 1. 原生库 (.so) 通过 jniLibs 打包，Android 系统安装时自动处理磁盘路径，支持 dlopen。
 * 2. 托管库 (.dll) 留在 APK assets 中，通过 C++/C# 内存流加载，不落地磁盘。
 * 3. 游戏资产 (Shader, Scene, etc.) 通过 SDL 内存加载，不落地磁盘。
 */
public class PrismaActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        // 加载核心运行时和项目插件
        return new String[] { "SDL3", "Prisma", "PathTracing3D" };
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // 这里没有任何解压逻辑。真正的零拷贝启动。
        super.onCreate(savedInstanceState);
    }
}
