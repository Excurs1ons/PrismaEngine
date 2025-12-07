package com.example.myapplication;

import android.os.Build;
import android.os.Bundle;
import android.view.Window;
import android.view.WindowManager;

import org.libsdl.app.SDLActivity;

public class MainActivity extends SDLActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // 请求最高刷新率
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            Window window = getWindow();

            // 1. 请求最高可用刷新率
            WindowManager.LayoutParams params = window.getAttributes();
            params.preferredRefreshRate = 60f;
            window.setAttributes(params);

            // 2. 明确告知系统期望的渲染帧率
            // 这里的 120.0f 可以是你期望的目标帧率，例如120Hz。
            // 系统会尝试选择一个最匹配的显示刷新率。

        }
    }

    @Override
    protected String[] getLibraries() {
        return new String[] {
                "SDL3",
                "myapplication"
        };
    }
}
