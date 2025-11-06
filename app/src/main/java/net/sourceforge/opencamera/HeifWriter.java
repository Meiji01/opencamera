package net.sourceforge.opencamera;

import android.media.Image;

public class HeifWriter {
    static {
        System.loadLibrary("heif");
    }

    public native boolean saveHeif(Image image, String path);
}
