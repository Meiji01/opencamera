package com.meijsoft.cameraadvance;

import android.graphics.Bitmap;
import android.util.Log;

public class HeifSaver {
    private static final String TAG = "HeifSaver";

    static {
        try {
            System.loadLibrary("opencamera_heif");
            Log.d(TAG, "heif library loaded");
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "failed to load heif library");
            e.printStackTrace();
        }
    }

    public static native boolean saveBitmapAsHeic(Bitmap bitmap, String path, byte[] exifData, int quality);
}
