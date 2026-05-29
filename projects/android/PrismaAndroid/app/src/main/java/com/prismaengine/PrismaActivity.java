package com.prismaengine;

import android.content.res.AssetManager;
import android.os.Bundle;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;

import org.libsdl.app.SDLActivity;

public class PrismaActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL3", "Prisma", "PathTracing3D" };
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        extractAssets("runtime");
        extractAssets("scenes");
        extractAssets("materials");
        extractAssets("shaders");
        extractAssets("models");
        super.onCreate(savedInstanceState);
    }

    private void extractAssets(String path) {
        File outDir = new File(getFilesDir(), path);
        if (!outDir.exists()) {
            outDir.mkdirs();
        }

        AssetManager am = getAssets();
        try {
            String[] assets = am.list(path);
            if (assets == null || assets.length == 0) return;

            for (String asset : assets) {
                String fullPath = path + "/" + asset;
                // Check if it's a directory by trying to list its content
                String[] subAssets = am.list(fullPath);
                if (subAssets != null && subAssets.length > 0) {
                    extractAssets(fullPath);
                } else {
                    File outFile = new File(getFilesDir(), fullPath);
                    // For performance, you might want to check file size or hash here, 
                    // but for a demo, we overwrite if missing.
                    if (outFile.exists()) continue; 

                    try (InputStream in = am.open(fullPath);
                         OutputStream out = new FileOutputStream(outFile)) {
                        byte[] buf = new byte[8192];
                        int len;
                        while ((len = in.read(buf)) > 0) {
                            out.write(buf, 0, len);
                        }
                    }
                }
            }
        } catch (Exception e) {
            android.util.Log.e("PrismaActivity", "extract " + path + " failed: " + e.getMessage());
        }
    }
}
