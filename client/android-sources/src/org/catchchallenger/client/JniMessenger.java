package org.catchchallenger.client;

import android.content.res.AssetManager;
import android.os.Environment;
import android.util.Log;
import java.io.*;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipEntry;

public class JniMessenger {
    public static void decompressDatapack(String outDir) {
        AssetManager assetManager = Client.getContext().getAssets();
        // String outDir = Client.getContext().getExternalFilesDir(null).getAbsolutePath() + "/";
        Log.i("LAN", "outDir: " + outDir);
        InputStream in = null;
        OutputStream out = null;
        String filename = "datapack.zip";
        try {
            InputStream fIn = assetManager.open(filename);
            ZipInputStream zis = new ZipInputStream(fIn);

            ZipEntry entry;

            while ((entry = zis.getNextEntry()) != null) {

                int size;
                byte[] buffer = new byte[2048];

                File outFile = new File(outDir, entry.getName());
                if (entry.getName().endsWith("/")) {
                    if (!outFile.exists()) {
                        outFile.mkdirs();
                    }
                    continue;
                }
                FileOutputStream fos = new FileOutputStream(outFile);
                BufferedOutputStream bos = new BufferedOutputStream(fos, buffer.length);
                while ((size = zis.read(buffer, 0, buffer.length)) != -1) {
                    bos.write(buffer, 0, size);
                }
                bos.flush();
            }
            Log.v("LAN", "Decompress Success");
        } catch (IOException e) {
            Log.e("LAN", "Failed to decompress: " + filename, e);
        }

        //return outDir + "datapack/";
    }
}
