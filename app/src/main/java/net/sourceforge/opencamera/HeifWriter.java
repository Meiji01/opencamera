package net.sourceforge.opencamera;

import android.media.Image;

public class HeifWriter {
    static {
        System.loadLibrary("heif");
    }

    public native int encodeHeif(byte[] input, String outputPath);

}
