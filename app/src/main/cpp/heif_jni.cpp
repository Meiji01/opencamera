#include <jni.h>
#include "libheif/heif.h"
#include <android/log.h>
#include <cstring>
#include <android/bitmap.h>

#define TAG "libheif"

extern "C"
JNIEXPORT jint JNICALL
Java_com_wanghonglin_libheif_HeifNative_encodeBitmap(JNIEnv *env, jclass type, jbyteArray bytes_,
                                                     jint width, jint height, jstring outputPath_) {
    jbyte *bytes = env->GetByteArrayElements(bytes_, NULL);
    jsize length = env->GetArrayLength(bytes_);
    const char *outputPath = env->GetStringUTFChars(outputPath_, 0);

    heif_image* image;
    heif_image_create(width, height, heif_colorspace_RGB, heif_chroma_interleaved_RGBA, &image);
    heif_image_add_plane(image, heif_channel_interleaved, width, height, 32);

    int stride = 0;
    uint8_t* p = heif_image_get_plane(image, heif_channel_interleaved, &stride);
    __android_log_print(ANDROID_LOG_DEBUG, TAG, "stride of image %d, %dx%d, %d", stride, width, height, length);

    std::memcpy(p, bytes, static_cast<size_t>(length));

    heif_context* ctx = heif_context_alloc();
    heif_encoder* encoder;
    heif_context_get_encoder_for_format(ctx, heif_compression_HEVC, &encoder);
    heif_encoder_set_logging_level(encoder, 4);

    heif_encoding_options* encoding_options = heif_encoding_options_alloc();
    encoding_options->save_alpha_channel = 0; // must be turned off for Android

    heif_error error;
    heif_image_handle* handle;
    error = heif_context_encode_image(ctx, image, encoder, encoding_options, &handle);
    if (error.code != heif_error_Ok) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "encode image error");
    } else {
        int ow = heif_image_handle_get_width(handle);
        int oh = heif_image_handle_get_height(handle);
        __android_log_print(ANDROID_LOG_DEBUG, TAG, "encode image done %dx%d", ow, oh);
    }
    heif_encoder_release(encoder);

    error = heif_context_write_to_file(ctx, outputPath);
    if (error.code != heif_error_Ok) {
        __android_log_print(ANDROID_LOG_DEBUG, TAG, "write to file failed");
    } else {
        __android_log_print(ANDROID_LOG_DEBUG, TAG, "write to file success");
    }

    heif_image_handle_release(handle);
    heif_image_release(image);

    heif_context_free(ctx);

    env->ReleaseByteArrayElements(bytes_, bytes, 0);
    env->ReleaseStringUTFChars(outputPath_, outputPath);

    return error.code;
}

extern "C"
JNIEXPORT jbyteArray
Java_com_wanghonglin_libheif_HeifNative_decodeHeif2RGBA(JNIEnv *env, jclass type, jobject outSize,
                                                        jstring srcPath_) {
    const char *srcPath = env->GetStringUTFChars(srcPath_, 0);

    heif_context* ctx = heif_context_alloc();
    heif_context_read_from_file(ctx, srcPath, nullptr);

    heif_image_handle* handle;
    heif_context_get_primary_image_handle(ctx, &handle);

    heif_image* image;
    heif_decode_image(handle, &image, heif_colorspace_RGB, heif_chroma_interleaved_RGBA, nullptr);

    int width = heif_image_handle_get_width(handle);
    int height = heif_image_handle_get_height(handle);

    int stride = 0;
    const uint8_t* data = heif_image_get_plane_readonly(image, heif_channel_interleaved, &stride);
    __android_log_print(ANDROID_LOG_DEBUG, TAG, "decode image %dx%d, stride = %d", width, height, stride);

    jbyteArray array = env->NewByteArray(stride*height);
    env->SetByteArrayRegion(array, 0, stride*height, reinterpret_cast<const jbyte *>(data));

    env->CallVoidMethod(outSize, env->GetMethodID(env->GetObjectClass(outSize), "setWidth", "(I)V"), width);
    env->CallVoidMethod(outSize, env->GetMethodID(env->GetObjectClass(outSize), "setHeight", "(I)V"), height);

    env->ReleaseStringUTFChars(srcPath_, srcPath);
    return array;
}

extern "C"
JNIEXPORT jint
Java_com_wanghonglin_libheif_HeifNative_encodeYUV(JNIEnv *env, jclass type, jbyteArray bytes_,
                                                  jint width, jint height, jstring outputPath_) {
    jbyte *bytes = env->GetByteArrayElements(bytes_, NULL);
    const char *outputPath = env->GetStringUTFChars(outputPath_, 0);

    heif_image* image;
    heif_image_create(width, height, heif_colorspace_YCbCr, heif_chroma_420, &image);
    heif_image_add_plane(image, heif_channel_Y, width, height, 8);
    heif_image_add_plane(image, heif_channel_Cb, width/2, height/2, 8);
    heif_image_add_plane(image, heif_channel_Cr, width/2, height/2, 8);

    int sy, su, sv;
    uint8_t* py = heif_image_get_plane(image, heif_channel_Y, &sy);
    uint8_t* pu = heif_image_get_plane(image, heif_channel_Cb, &su);
    uint8_t* pv = heif_image_get_plane(image, heif_channel_Cr, &sv);

    std::memcpy(py, bytes, static_cast<size_t>(width * height));
    std::memcpy(pu, bytes+(width*height), static_cast<size_t>(width * height / 4));
    std::memcpy(pv, bytes+(width*height+width*height/4), static_cast<size_t>(width * height / 4));

    heif_context* ctx = heif_context_alloc();
    heif_encoder* encoder;
    heif_context_get_encoder_for_format(ctx, heif_compression_HEVC, &encoder);

    heif_encoding_options* options = heif_encoding_options_alloc();
    options->save_alpha_channel = 0;

    heif_image_handle* handle;
    heif_context_encode_image(ctx, image, encoder, options, &handle);
    heif_encoder_release(encoder);

    heif_error error;
    error = heif_context_write_to_file(ctx, outputPath);
    if (error.code != heif_error_Ok) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "YUV write to file error %s", error.message);
    } else {
        __android_log_print(ANDROID_LOG_DEBUG, TAG, "YUV write to file success");
    }

    heif_image_handle_release(handle);
    heif_image_release(image);
    heif_context_free(ctx);

    env->ReleaseByteArrayElements(bytes_, bytes, 0);
    env->ReleaseStringUTFChars(outputPath_, outputPath);

    return error.code;
}



extern "C"
JNIEXPORT jboolean
Java_com_meijsoft_cameraadvance_HeifSaver_saveBitmapAsHeic(JNIEnv *env, jclass clazz,
                                                           jobject bitmap, jstring outputPath_, jbyteArray exifData, jint quality) {
    AndroidBitmapInfo info;
    void* pixels;
    int ret;

    if ((ret = AndroidBitmap_getInfo(env, bitmap, &info)) < 0) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "AndroidBitmap_getInfo() failed ! error=%d", ret);
        return JNI_FALSE;
    }

    if ((ret = AndroidBitmap_lockPixels(env, bitmap, &pixels)) < 0) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "AndroidBitmap_lockPixels() failed ! error=%d", ret);
        return JNI_FALSE;
    }

    int width = info.width;
    int height = info.height;
    const char *outputPath = env->GetStringUTFChars(outputPath_, 0);

    heif_image* image;
    heif_image_create(width, height, heif_colorspace_RGB, heif_chroma_interleaved_RGBA, &image);
    heif_image_add_plane(image, heif_channel_interleaved, width, height, 32);

    int stride = 0;
    uint8_t* p = heif_image_get_plane(image, heif_channel_interleaved, &stride);
    std::memcpy(p, pixels, static_cast<size_t>(width * height * 4)); // Assuming RGBA_8888

    heif_context* ctx = heif_context_alloc();
    heif_encoder* encoder;
    heif_context_get_encoder_for_format(ctx, heif_compression_HEVC, &encoder);
    heif_encoder_set_logging_level(encoder, 4);

    if (quality >= 100) {
        heif_encoder_set_lossless(encoder, 1);
    } else {
        heif_encoder_set_lossy_quality(encoder, quality);
    }

    heif_encoding_options* encoding_options = heif_encoding_options_alloc();
    encoding_options->save_alpha_channel = 0;

    heif_error error;
    heif_image_handle* handle;
    error = heif_context_encode_image(ctx, image, encoder, encoding_options, &handle);
    if (error.code != heif_error_Ok) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "encode image error: %s", error.message);
        AndroidBitmap_unlockPixels(env, bitmap);
        env->ReleaseStringUTFChars(outputPath_, outputPath);
        heif_encoder_release(encoder);
        heif_image_release(image);
        heif_context_free(ctx);
        return JNI_FALSE;
    }

    // Add EXIF metadata
    if (exifData != NULL) {
        jsize exif_size = env->GetArrayLength(exifData);
        if (exif_size > 0) {
            jbyte* exif_bytes = env->GetByteArrayElements(exifData, NULL);
            heif_error exif_error = heif_context_add_exif_metadata(ctx, handle, (uint8_t*)exif_bytes, exif_size);
            if (exif_error.code != heif_error_Ok) {
                __android_log_print(ANDROID_LOG_WARN, TAG, "heif_context_add_exif_metadata() failed: %s", exif_error.message);
            }
            env->ReleaseByteArrayElements(exifData, exif_bytes, JNI_ABORT);
        }
    }

    error = heif_context_write_to_file(ctx, outputPath);
    if (error.code != heif_error_Ok) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "write to file failed: %s", error.message);
        AndroidBitmap_unlockPixels(env, bitmap);
        env->ReleaseStringUTFChars(outputPath_, outputPath);
        heif_image_handle_release(handle);
        heif_image_release(image);
        heif_context_free(ctx);
        return JNI_FALSE;
    }

    __android_log_print(ANDROID_LOG_DEBUG, TAG, "write to file success");

    heif_image_handle_release(handle);
    heif_image_release(image);
    heif_context_free(ctx);

    AndroidBitmap_unlockPixels(env, bitmap);
    env->ReleaseStringUTFChars(outputPath_, outputPath);

    return JNI_TRUE;
}

extern "C"
JNIEXPORT jobject
Java_com_meijsoft_cameraadvance_RawProcessor_decodeDng(JNIEnv *env, jclass clazz, jstring dngPath_) {
    (void) clazz;
    (void) dngPath_;
    __android_log_print(ANDROID_LOG_WARN, TAG, "decodeDng is unavailable because RAW support was removed");
    return nullptr;
}

extern "C"
JNIEXPORT jboolean
Java_com_meijsoft_cameraadvance_HeifSaver_saveRawToHeic(JNIEnv *env, jclass clazz,
                                                         jobject image_obj, jstring outputPath_, jint quality) {
    (void) env;
    (void) clazz;
    (void) image_obj;
    (void) outputPath_;
    (void) quality;
    __android_log_print(ANDROID_LOG_WARN, TAG, "saveRawToHeic is unavailable because RAW support was removed");
    return JNI_FALSE;
}

static bool copyYuvPlane(JNIEnv *env, jobject plane_obj, uint8_t *dst, int dst_width, int dst_height) {
    jclass plane_class = env->GetObjectClass(plane_obj);
    jmethodID get_buffer_mid = env->GetMethodID(plane_class, "getBuffer", "()Ljava/nio/ByteBuffer;");
    jmethodID get_row_stride_mid = env->GetMethodID(plane_class, "getRowStride", "()I");
    jmethodID get_pixel_stride_mid = env->GetMethodID(plane_class, "getPixelStride", "()I");

    jobject buffer_obj = env->CallObjectMethod(plane_obj, get_buffer_mid);
    auto *src = reinterpret_cast<uint8_t *>(env->GetDirectBufferAddress(buffer_obj));
    jbyteArray src_array = nullptr;
    jbyte *src_bytes = nullptr;
    const int row_stride = env->CallIntMethod(plane_obj, get_row_stride_mid);
    const int pixel_stride = env->CallIntMethod(plane_obj, get_pixel_stride_mid);

    if( src == nullptr ) {
        jclass buffer_class = env->GetObjectClass(buffer_obj);
        jmethodID remaining_mid = env->GetMethodID(buffer_class, "remaining", "()I");
        jmethodID get_mid = env->GetMethodID(buffer_class, "get", "([B)Ljava/nio/ByteBuffer;");
        const jint remaining = env->CallIntMethod(buffer_obj, remaining_mid);
        src_array = env->NewByteArray(remaining);
        if( src_array != nullptr ) {
            env->CallObjectMethod(buffer_obj, get_mid, src_array);
            src_bytes = env->GetByteArrayElements(src_array, nullptr);
            src = reinterpret_cast<uint8_t *>(src_bytes);
        }
        env->DeleteLocalRef(buffer_class);
    }

    if( src == nullptr ) {
        env->DeleteLocalRef(buffer_obj);
        env->DeleteLocalRef(plane_class);
        return false;
    }

    for( int y = 0; y < dst_height; ++y ) {
        uint8_t *dst_row = dst + (y * dst_width);
        uint8_t *src_row = src + (y * row_stride);
        for( int x = 0; x < dst_width; ++x ) {
            dst_row[x] = src_row[x * pixel_stride];
        }
    }

    if( src_bytes != nullptr ) {
        env->ReleaseByteArrayElements(src_array, src_bytes, JNI_ABORT);
    }
    if( src_array != nullptr ) {
        env->DeleteLocalRef(src_array);
    }
    env->DeleteLocalRef(buffer_obj);
    env->DeleteLocalRef(plane_class);
    return true;
}

static bool copyYuvPlaneRotated(JNIEnv *env, jobject plane_obj, uint8_t *dst, int dst_stride,
                                int src_width, int src_height, int rotation) {
    jclass plane_class = env->GetObjectClass(plane_obj);
    jmethodID get_buffer_mid = env->GetMethodID(plane_class, "getBuffer", "()Ljava/nio/ByteBuffer;");
    jmethodID get_row_stride_mid = env->GetMethodID(plane_class, "getRowStride", "()I");
    jmethodID get_pixel_stride_mid = env->GetMethodID(plane_class, "getPixelStride", "()I");

    jobject buffer_obj = env->CallObjectMethod(plane_obj, get_buffer_mid);
    auto *src = reinterpret_cast<uint8_t *>(env->GetDirectBufferAddress(buffer_obj));
    jbyteArray src_array = nullptr;
    jbyte *src_bytes = nullptr;
    const int row_stride = env->CallIntMethod(plane_obj, get_row_stride_mid);
    const int pixel_stride = env->CallIntMethod(plane_obj, get_pixel_stride_mid);

    if( src == nullptr ) {
        jclass buffer_class = env->GetObjectClass(buffer_obj);
        jmethodID remaining_mid = env->GetMethodID(buffer_class, "remaining", "()I");
        jmethodID get_mid = env->GetMethodID(buffer_class, "get", "([B)Ljava/nio/ByteBuffer;");
        const jint remaining = env->CallIntMethod(buffer_obj, remaining_mid);
        src_array = env->NewByteArray(remaining);
        if( src_array != nullptr ) {
            env->CallObjectMethod(buffer_obj, get_mid, src_array);
            src_bytes = env->GetByteArrayElements(src_array, nullptr);
            src = reinterpret_cast<uint8_t *>(src_bytes);
        }
        env->DeleteLocalRef(buffer_class);
    }

    if( src == nullptr ) {
        env->DeleteLocalRef(buffer_obj);
        env->DeleteLocalRef(plane_class);
        return false;
    }

    int normalized_rotation = ((rotation % 360) + 360) % 360;
    if( normalized_rotation == 0 ) {
        for( int y = 0; y < src_height; ++y ) {
            uint8_t *dst_row = dst + (y * dst_stride);
            uint8_t *src_row = src + (y * row_stride);
            for( int x = 0; x < src_width; ++x ) {
                dst_row[x] = src_row[x * pixel_stride];
            }
        }
    }
    else if( normalized_rotation == 180 ) {
        for( int y = 0; y < src_height; ++y ) {
            uint8_t *dst_row = dst + (y * dst_stride);
            for( int x = 0; x < src_width; ++x ) {
                int src_x = src_width - 1 - x;
                int src_y = src_height - 1 - y;
                dst_row[x] = src[(src_y * row_stride) + (src_x * pixel_stride)];
            }
        }
    }
    else if( normalized_rotation == 90 ) {
        for( int y = 0; y < src_width; ++y ) {
            uint8_t *dst_row = dst + (y * dst_stride);
            for( int x = 0; x < src_height; ++x ) {
                int src_x = y;
                int src_y = src_height - 1 - x;
                dst_row[x] = src[(src_y * row_stride) + (src_x * pixel_stride)];
            }
        }
    }
    else if( normalized_rotation == 270 ) {
        for( int y = 0; y < src_width; ++y ) {
            uint8_t *dst_row = dst + (y * dst_stride);
            for( int x = 0; x < src_height; ++x ) {
                int src_x = src_width - 1 - y;
                int src_y = x;
                dst_row[x] = src[(src_y * row_stride) + (src_x * pixel_stride)];
            }
        }
    }
    else {
        env->DeleteLocalRef(buffer_obj);
        env->DeleteLocalRef(plane_class);
        return false;
    }

    if( src_bytes != nullptr ) {
        env->ReleaseByteArrayElements(src_array, src_bytes, JNI_ABORT);
    }
    if( src_array != nullptr ) {
        env->DeleteLocalRef(src_array);
    }
    env->DeleteLocalRef(buffer_obj);
    env->DeleteLocalRef(plane_class);
    return true;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_meijsoft_cameraadvance_HeifSaver_saveYuvToHeic(JNIEnv *env, jclass clazz,
                                                         jobject image_obj, jstring outputPath_,
                                                         jbyteArray exifData, jint quality, jint rotation) {
    jclass image_class = env->GetObjectClass(image_obj);
    jmethodID get_planes_mid = env->GetMethodID(image_class, "getPlanes", "()[Landroid/media/Image$Plane;");
    jmethodID get_format_mid = env->GetMethodID(image_class, "getFormat", "()I");
    jmethodID get_width_mid = env->GetMethodID(image_class, "getWidth", "()I");
    jmethodID get_height_mid = env->GetMethodID(image_class, "getHeight", "()I");

    const int yuv_420_888 = 0x23;
    const jint format = env->CallIntMethod(image_obj, get_format_mid);
    if( format != yuv_420_888 ) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Image format is not YUV_420_888");
        env->DeleteLocalRef(image_class);
        return JNI_FALSE;
    }

    const int width = env->CallIntMethod(image_obj, get_width_mid);
    const int height = env->CallIntMethod(image_obj, get_height_mid);
    const int normalized_rotation = ((rotation % 360) + 360) % 360;
    const bool rotate_swap = normalized_rotation == 90 || normalized_rotation == 270;
    const int output_width = rotate_swap ? height : width;
    const int output_height = rotate_swap ? width : height;
    const int chroma_width = (output_width + 1) / 2;
    const int chroma_height = (output_height + 1) / 2;

    jobjectArray planes = (jobjectArray) env->CallObjectMethod(image_obj, get_planes_mid);
    if( planes == nullptr ) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "getPlanes() returned null");
        env->DeleteLocalRef(image_class);
        return JNI_FALSE;
    }

    jobject y_plane = env->GetObjectArrayElement(planes, 0);
    jobject cb_plane = env->GetObjectArrayElement(planes, 1);
    jobject cr_plane = env->GetObjectArrayElement(planes, 2);

    const char *outputPath = env->GetStringUTFChars(outputPath_, 0);

    heif_image *heif_yuv_image = nullptr;
    heif_image_create(output_width, output_height, heif_colorspace_YCbCr, heif_chroma_420, &heif_yuv_image);
    heif_image_add_plane(heif_yuv_image, heif_channel_Y, output_width, output_height, 8);
    heif_image_add_plane(heif_yuv_image, heif_channel_Cb, chroma_width, chroma_height, 8);
    heif_image_add_plane(heif_yuv_image, heif_channel_Cr, chroma_width, chroma_height, 8);

    int y_stride = 0;
    int cb_stride = 0;
    int cr_stride = 0;
    uint8_t *y_dst = heif_image_get_plane(heif_yuv_image, heif_channel_Y, &y_stride);
    uint8_t *cb_dst = heif_image_get_plane(heif_yuv_image, heif_channel_Cb, &cb_stride);
    uint8_t *cr_dst = heif_image_get_plane(heif_yuv_image, heif_channel_Cr, &cr_stride);

    int source_chroma_width = (width + 1) / 2;
    int source_chroma_height = (height + 1) / 2;

    bool copied = copyYuvPlaneRotated(env, y_plane, y_dst, y_stride, width, height, normalized_rotation) &&
                  copyYuvPlaneRotated(env, cb_plane, cb_dst, cb_stride, source_chroma_width, source_chroma_height, normalized_rotation) &&
                  copyYuvPlaneRotated(env, cr_plane, cr_dst, cr_stride, source_chroma_width, source_chroma_height, normalized_rotation);
    if( !copied ) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "failed to copy YUV planes");
        env->ReleaseStringUTFChars(outputPath_, outputPath);
        heif_image_release(heif_yuv_image);
        env->DeleteLocalRef(y_plane);
        env->DeleteLocalRef(cb_plane);
        env->DeleteLocalRef(cr_plane);
        env->DeleteLocalRef(planes);
        env->DeleteLocalRef(image_class);
        return JNI_FALSE;
    }

    heif_context *ctx = heif_context_alloc();
    heif_encoder *encoder;
    heif_context_get_encoder_for_format(ctx, heif_compression_HEVC, &encoder);
    heif_encoder_set_logging_level(encoder, 4);

    if( quality >= 100 ) {
        heif_encoder_set_lossless(encoder, 1);
    } else {
        heif_encoder_set_lossy_quality(encoder, quality);
    }

    heif_encoding_options *encoding_options = heif_encoding_options_alloc();
    encoding_options->save_alpha_channel = 0;

    heif_error error;
    heif_image_handle *handle;
    error = heif_context_encode_image(ctx, heif_yuv_image, encoder, encoding_options, &handle);
    if( error.code != heif_error_Ok ) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "encode YUV image error: %s", error.message);
        env->ReleaseStringUTFChars(outputPath_, outputPath);
        heif_encoder_release(encoder);
        heif_image_release(heif_yuv_image);
        heif_context_free(ctx);
        env->DeleteLocalRef(y_plane);
        env->DeleteLocalRef(cb_plane);
        env->DeleteLocalRef(cr_plane);
        env->DeleteLocalRef(planes);
        env->DeleteLocalRef(image_class);
        return JNI_FALSE;
    }

    if( exifData != NULL ) {
        jsize exif_size = env->GetArrayLength(exifData);
        if( exif_size > 0 ) {
            jbyte *exif_bytes = env->GetByteArrayElements(exifData, NULL);
            heif_error exif_error = heif_context_add_exif_metadata(ctx, handle, (uint8_t *)exif_bytes, exif_size);
            if( exif_error.code != heif_error_Ok ) {
                __android_log_print(ANDROID_LOG_WARN, TAG, "heif_context_add_exif_metadata() failed: %s", exif_error.message);
            }
            env->ReleaseByteArrayElements(exifData, exif_bytes, JNI_ABORT);
        }
    }

    error = heif_context_write_to_file(ctx, outputPath);
    if( error.code != heif_error_Ok ) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "write YUV heic to file failed: %s", error.message);
        env->ReleaseStringUTFChars(outputPath_, outputPath);
        heif_image_handle_release(handle);
        heif_encoder_release(encoder);
        heif_image_release(heif_yuv_image);
        heif_context_free(ctx);
        env->DeleteLocalRef(y_plane);
        env->DeleteLocalRef(cb_plane);
        env->DeleteLocalRef(cr_plane);
        env->DeleteLocalRef(planes);
        env->DeleteLocalRef(image_class);
        return JNI_FALSE;
    }

    __android_log_print(ANDROID_LOG_DEBUG, TAG, "write YUV heic success");

    heif_image_handle_release(handle);
    heif_encoder_release(encoder);
    heif_image_release(heif_yuv_image);
    heif_context_free(ctx);
    env->ReleaseStringUTFChars(outputPath_, outputPath);

    env->DeleteLocalRef(y_plane);
    env->DeleteLocalRef(cb_plane);
    env->DeleteLocalRef(cr_plane);
    env->DeleteLocalRef(planes);
    env->DeleteLocalRef(image_class);

    return JNI_TRUE;
}

extern "C"
JNIEXPORT jboolean
Java_com_meijsoft_cameraadvance_HeifSaver_saveDngToHeic(JNIEnv *env, jclass clazz,
                                                     jstring dngPath_, jstring outputPath_, jint quality, jboolean use_auto_wb, jboolean use_camera_wb) {
    (void) env;
    (void) clazz;
    (void) dngPath_;
    (void) outputPath_;
    (void) quality;
    (void) use_auto_wb;
    (void) use_camera_wb;
    __android_log_print(ANDROID_LOG_WARN, TAG, "saveDngToHeic is unavailable because RAW support was removed");
    return JNI_FALSE;
}
