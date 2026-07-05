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

    /**
     * Configure LibRaw output color space used by native processing.
     * Typical values: 1 = sRGB. See LibRaw documentation for other values.
     */
    public static native void setLibRawOutputColor(int outputColor);

    /**
     * Configure the LibRaw decode parameters used by native DNG decoding.
     */
    public static native void configureRawProcessor(boolean useAutoWB, boolean useCameraWB);
}
