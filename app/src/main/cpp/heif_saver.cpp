#include <jni.h>
#include <string>
#include <android/log.h>
#include <android/bitmap.h>
#include "heif_jni.h"

#define TAG "HeifSaver"

extern "C"
JNIEXPORT jboolean JNICALL
Java_net_sourceforge_opencamera_HeifSaver_saveBitmapAsHeic(JNIEnv *env, jobject thiz,
                                                              jobject bitmap, jstring path) {
    // TODO: Implement this
    return false;
}
