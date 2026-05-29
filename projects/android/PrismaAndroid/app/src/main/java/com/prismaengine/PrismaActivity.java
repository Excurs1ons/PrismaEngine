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
        return new String[] { "SDL3", "Prisma" };
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        extractRuntimeAssets();
        super.onCreate(savedInstanceState);
    }

    private void extractRuntimeAssets() {
        File runtimeDir = new File(getFilesDir(), "runtime");
        if (runtimeDir.exists()) {
            return;
        }
        runtimeDir.mkdirs();

        AssetManager am = getAssets();
        try {
            String[] assets = am.list("runtime");
            if (assets == null || assets.length == 0) return;

            for (String asset : assets) {
                File outFile = new File(runtimeDir, asset);
                try (InputStream in = am.open("runtime/" + asset);
                     OutputStream out = new FileOutputStream(outFile)) {
                    byte[] buf = new byte[8192];
                    int len;
                    while ((len = in.read(buf)) > 0) {
                        out.write(buf, 0, len);
                    }
                }
            }
        } catch (Exception e) {
            android.util.Log.e("PrismaActivity", "extract runtime failed: " + e.getMessage());
        }
    }
}
