package com.meijsoft.cameraadvance;

import android.graphics.Bitmap;
import android.util.Log;

public class RawProcessor {
    private static final String TAG = "RawProcessor";
    static {
        try {
            System.loadLibrary("opencamera_heif");
            Log.d(TAG, "native library loaded for RawProcessor");
        }
        catch( UnsatisfiedLinkError e ) {
            Log.e(TAG, "failed to load native library for RawProcessor");
            e.printStackTrace();
        }
    }

    // Decode a DNG file at the given path to a Bitmap (ARGB_8888)
    public static native Bitmap decodeDng(String dngPath);
}
