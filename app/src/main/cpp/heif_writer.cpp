#include <jni.h>
#include <string>

extern "C" JNIEXPORT jboolean JNICALL
Java_net_sourceforge_opencamera_HeifWriter_saveHeif(JNIEnv *env, jobject /* this */, jobject image, jstring path) {
    // TODO: Implement HEIF writing using libheif
    return false;
}
