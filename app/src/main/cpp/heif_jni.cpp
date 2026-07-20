#include <jni.h>
#include "libheif/heif.h"
#include <android/log.h>
#include <cstring>
#include <android/bitmap.h>
#include "libraw/libraw.h"

#define TAG "libheif"

// Global configurable LibRaw output color space (default 1 = sRGB)
static int g_libraw_output_color = 1;
static int g_libraw_use_auto_wb = 1;
static int g_libraw_use_camera_wb = 0;
static int g_libraw_use_camera_matrix = 1;
static int g_libraw_output_bps = 8;
static float g_libraw_gamma0 = 1.0f / 2.2f;
static float g_libraw_gamma1 = 4.5f;
static int g_libraw_user_qual = 3;
static int g_libraw_fbdd_noiserd = 0;
static int g_libraw_med_passes = 0;
static int g_libraw_dcb_iterations = 0;
static int g_libraw_dcb_enhance_fl = 0;

extern "C"
JNIEXPORT void JNICALL
Java_com_meijsoft_cameraadvance_RawProcessor_setLibRawOutputColor(JNIEnv *env, jclass type, jint value) {
    g_libraw_output_color = value;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_meijsoft_cameraadvance_RawProcessor_configureRawProcessor(JNIEnv *env, jclass type, jboolean useAutoWB, jboolean useCameraWB) {
    g_libraw_use_auto_wb = useAutoWB ? 1 : 0;
    g_libraw_use_camera_wb = useCameraWB ? 1 : 0;
}

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
    const char *dngPath = env->GetStringUTFChars(dngPath_, 0);

    LibRaw RawProcessor;
    RawProcessor.open_file(dngPath);
    RawProcessor.unpack();

    // Default to configurable output color space and processing parameters
    RawProcessor.imgdata.params.output_color = g_libraw_output_color;
    RawProcessor.imgdata.params.use_auto_wb = g_libraw_use_auto_wb;
    RawProcessor.imgdata.params.use_camera_wb = g_libraw_use_camera_wb;
    RawProcessor.imgdata.params.output_bps = g_libraw_output_bps;
    RawProcessor.imgdata.params.gamm[0] = g_libraw_gamma0;
    RawProcessor.imgdata.params.gamm[1] = g_libraw_gamma1;
    RawProcessor.imgdata.params.user_qual = g_libraw_user_qual;
    RawProcessor.imgdata.params.fbdd_noiserd = g_libraw_fbdd_noiserd;
    RawProcessor.imgdata.params.med_passes = g_libraw_med_passes;
    RawProcessor.imgdata.params.dcb_iterations = g_libraw_dcb_iterations;
    RawProcessor.imgdata.params.dcb_enhance_fl = g_libraw_dcb_enhance_fl;

    int width = RawProcessor.imgdata.sizes.width;
    int height = RawProcessor.imgdata.sizes.height;

    RawProcessor.dcraw_process();

    libraw_processed_image_t *processed_image = RawProcessor.dcraw_make_mem_image();

    jclass bitmapConfig = env->FindClass("android/graphics/Bitmap$Config");
    jfieldID rgba8888FieldID = env->GetStaticFieldID(bitmapConfig, "ARGB_8888", "Landroid/graphics/Bitmap$Config;");
    jobject rgba8888Obj = env->GetStaticObjectField(bitmapConfig, rgba8888FieldID);

    jclass bitmapClass = env->FindClass("android/graphics/Bitmap");
    jmethodID createBitmapMethodID = env->GetStaticMethodID(bitmapClass, "createBitmap", "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
    jobject bitmap = env->CallStaticObjectMethod(bitmapClass, createBitmapMethodID, width, height, rgba8888Obj);

    AndroidBitmapInfo info;
    void* pixels;
    AndroidBitmap_getInfo(env, bitmap, &info);
    AndroidBitmap_lockPixels(env, bitmap, &pixels);

    if (processed_image->type == LIBRAW_IMAGE_BITMAP && processed_image->bits == 8) {
        memcpy(pixels, processed_image->data, processed_image->data_size);
    }

    AndroidBitmap_unlockPixels(env, bitmap);

    LibRaw::dcraw_clear_mem(processed_image);
    RawProcessor.recycle();

    env->ReleaseStringUTFChars(dngPath_, dngPath);

    return bitmap;
}

extern "C"
JNIEXPORT jboolean
Java_com_meijsoft_cameraadvance_HeifSaver_saveRawToHeic(JNIEnv *env, jclass clazz,
                                                         jobject image_obj, jstring outputPath_, jint quality) {
    // Get Image class and methods
    jclass image_class = env->GetObjectClass(image_obj);
    jmethodID get_planes_mid = env->GetMethodID(image_class, "getPlanes", "()[Landroid/media/Image$Plane;");
    jmethodID get_format_mid = env->GetMethodID(image_class, "getFormat", "()I");

    // Get image format
    jint format = env->CallIntMethod(image_obj, get_format_mid);
    const int RAW_SENSOR = 0x20;
    if (format != RAW_SENSOR) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Image format is not RAW_SENSOR");
        return JNI_FALSE;
    }

    // Get planes
    jobjectArray planes = (jobjectArray) env->CallObjectMethod(image_obj, get_planes_mid);
    jobject plane = env->GetObjectArrayElement(planes, 0);

    // Get Plane class and methods
    jclass plane_class = env->GetObjectClass(plane);
    jmethodID get_buffer_mid = env->GetMethodID(plane_class, "getBuffer", "()Ljava/nio/ByteBuffer;");

    // Get buffer
    jobject buffer_obj = env->CallObjectMethod(plane, get_buffer_mid);
    jlong buffer_size = env->GetDirectBufferCapacity(buffer_obj);
    auto* raw_buffer = (uint8_t*) env->GetDirectBufferAddress(buffer_obj);

    // Use LibRaw to process the RAW data
    LibRaw RawProcessor;
    int ret = RawProcessor.open_buffer(raw_buffer, buffer_size);
    if (ret != LIBRAW_SUCCESS) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Failed to open raw buffer: %s", libraw_strerror(ret));
        return JNI_FALSE;
    }

    if ((ret = RawProcessor.unpack()) != LIBRAW_SUCCESS) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Failed to unpack raw data: %s", libraw_strerror(ret));
        return JNI_FALSE;
    }

    // Default to configurable output color space (g_libraw_output_color)
    RawProcessor.imgdata.params.output_color = g_libraw_output_color;

    if ((ret = RawProcessor.dcraw_process()) != LIBRAW_SUCCESS) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Failed to process raw data: %s", libraw_strerror(ret));
        return JNI_FALSE;
    }

    libraw_processed_image_t *processed_image = RawProcessor.dcraw_make_mem_image();
    if (!processed_image) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Failed to make memory image");
        return JNI_FALSE;
    }

    if (processed_image->type != LIBRAW_IMAGE_BITMAP || processed_image->bits != 8) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Processed image is not 8-bit RGB");
        LibRaw::dcraw_clear_mem(processed_image);
        return JNI_FALSE;
    }

    int width = processed_image->width;
    int height = processed_image->height;

    // Use libheif to save the processed image
    heif_image* heif_image;
    heif_image_create(width, height, heif_colorspace_RGB, heif_chroma_interleaved_RGB, &heif_image);
    heif_image_add_plane(heif_image, heif_channel_interleaved, width, height, 24);

    int stride = 0;
    uint8_t* p = heif_image_get_plane(heif_image, heif_channel_interleaved, &stride);

    if (stride == width * 3) {
        memcpy(p, processed_image->data, (size_t)width * height * 3);
    } else {
        for (int y = 0; y < height; y++) {
            memcpy(p + y * stride, processed_image->data + y * width * 3, (size_t)width * 3);
        }
    }

    heif_context* ctx = heif_context_alloc();
    heif_encoder* encoder;
    heif_context_get_encoder_for_format(ctx, heif_compression_HEVC, &encoder);

    if (quality >= 100) {
        heif_encoder_set_lossless(encoder, 1);
    } else {
        heif_encoder_set_lossy_quality(encoder, quality);
    }

    heif_image_handle* handle;
    heif_context_encode_image(ctx, heif_image, encoder, nullptr, &handle);
    heif_encoder_release(encoder);

    const char *outputPath = env->GetStringUTFChars(outputPath_, 0);
    heif_context_write_to_file(ctx, outputPath);
    env->ReleaseStringUTFChars(outputPath_, outputPath);

    heif_image_handle_release(handle);
    heif_context_free(ctx);
    heif_image_release(heif_image);

    LibRaw::dcraw_clear_mem(processed_image);

    return JNI_TRUE;
}

static bool copyYuvPlane(JNIEnv *env, jobject plane_obj, uint8_t *dst, int dst_width, int dst_height) {
    jclass plane_class = env->GetObjectClass(plane_obj);
    jmethodID get_buffer_mid = env->GetMethodID(plane_class, "getBuffer", "()Ljava/nio/ByteBuffer;");
    jmethodID get_row_stride_mid = env->GetMethodID(plane_class, "getRowStride", "()I");
    jmethodID get_pixel_stride_mid = env->GetMethodID(plane_class, "getPixelStride", "()I");

    jobject buffer_obj = env->CallObjectMethod(plane_obj, get_buffer_mid);
    auto *src = reinterpret_cast<uint8_t *>(env->GetDirectBufferAddress(buffer_obj));
    const int row_stride = env->CallIntMethod(plane_obj, get_row_stride_mid);
    const int pixel_stride = env->CallIntMethod(plane_obj, get_pixel_stride_mid);

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
    const int row_stride = env->CallIntMethod(plane_obj, get_row_stride_mid);
    const int pixel_stride = env->CallIntMethod(plane_obj, get_pixel_stride_mid);

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

    if( normalized_rotation == 180 ) {
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
    const char *dngPath = env->GetStringUTFChars(dngPath_, 0);

    LibRaw RawProcessor;
    int ret = RawProcessor.open_file(dngPath);
    if (ret != LIBRAW_SUCCESS) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Failed to open DNG file: %s", libraw_strerror(ret));
        env->ReleaseStringUTFChars(dngPath_, dngPath);
        return JNI_FALSE;
    }

    if ((ret = RawProcessor.unpack()) != LIBRAW_SUCCESS) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Failed to unpack DNG: %s", libraw_strerror(ret));
        env->ReleaseStringUTFChars(dngPath_, dngPath);
        return JNI_FALSE;
    }

    // Apply white balance preferences from Java
    RawProcessor.imgdata.params.use_auto_wb = use_auto_wb ? 1 : 0;
    RawProcessor.imgdata.params.use_camera_wb = use_camera_wb ? 1 : 0;
    RawProcessor.imgdata.params.use_camera_matrix = g_libraw_use_camera_matrix;

    // Use a more natural demosaic and color rendering profile for closer bitmap-like output
    RawProcessor.imgdata.params.user_qual = g_libraw_user_qual;
    RawProcessor.imgdata.params.fbdd_noiserd = g_libraw_fbdd_noiserd;
    RawProcessor.imgdata.params.med_passes = g_libraw_med_passes;
    RawProcessor.imgdata.params.dcb_iterations = g_libraw_dcb_iterations;
    RawProcessor.imgdata.params.dcb_enhance_fl = g_libraw_dcb_enhance_fl;

    // Apply gamma ~2.2 (dcraw uses gamm[0]=1/gamma, gamm[1]=toe)
    RawProcessor.imgdata.params.gamm[0] = g_libraw_gamma0;
    RawProcessor.imgdata.params.gamm[1] = g_libraw_gamma1;

    // Keep output as 8-bit RGB (we convert to YUV420 later)
    RawProcessor.imgdata.params.output_bps = g_libraw_output_bps;
    // Default output color space (configurable from Java via RawProcessor.setLibRawOutputColor)
    RawProcessor.imgdata.params.output_color = g_libraw_output_color;

    // Log processing parameters to help diagnose behavior
    __android_log_print(ANDROID_LOG_DEBUG, TAG, "LibRaw params before dcraw_process: use_auto_wb=%d, use_camera_wb=%d, use_camera_matrix=%d, output_color=%d, output_bps=%d, user_qual=%d, fbdd_noiserd=%d, med_passes=%d, iterations=%d, dcb_enhance=%d",
                        RawProcessor.imgdata.params.use_auto_wb,
                        RawProcessor.imgdata.params.use_camera_wb,
                        RawProcessor.imgdata.params.use_camera_matrix,
                        RawProcessor.imgdata.params.output_color,
                        RawProcessor.imgdata.params.output_bps,
                        RawProcessor.imgdata.params.user_qual,
                        RawProcessor.imgdata.params.fbdd_noiserd,
                        RawProcessor.imgdata.params.med_passes,
                        RawProcessor.imgdata.params.dcb_iterations,
                        RawProcessor.imgdata.params.dcb_enhance_fl);
    __android_log_print(ANDROID_LOG_DEBUG, TAG, "LibRaw cam_mul: %f, %f, %f, %f",
                        RawProcessor.imgdata.color.cam_mul[0],
                        RawProcessor.imgdata.color.cam_mul[1],
                        RawProcessor.imgdata.color.cam_mul[2],
                        RawProcessor.imgdata.color.cam_mul[3]);

    if ((ret = RawProcessor.dcraw_process()) != LIBRAW_SUCCESS) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Failed to process DNG: %s", libraw_strerror(ret));
        env->ReleaseStringUTFChars(dngPath_, dngPath);
        return JNI_FALSE;
    }

    libraw_processed_image_t *processed_image = RawProcessor.dcraw_make_mem_image();
    if (!processed_image) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Failed to make memory image");
        env->ReleaseStringUTFChars(dngPath_, dngPath);
        return JNI_FALSE;
    }

    if (processed_image->type != LIBRAW_IMAGE_BITMAP) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Processed image is not bitmap type");
        LibRaw::dcraw_clear_mem(processed_image);
        env->ReleaseStringUTFChars(dngPath_, dngPath);
        return JNI_FALSE;
    }

    int width = processed_image->width;
    int height = processed_image->height;

    // Convert processed RGB (8-bit per channel) to YUV420 (8-bit) for maximum decoder compatibility
    // Allocate Y, U, V planes
    const uint8_t* rgb = (const uint8_t*) processed_image->data;
    size_t y_size = (size_t) width * height;
    size_t c_size = (size_t) (width/2) * (height/2);
    uint8_t* planeY = (uint8_t*) malloc(y_size);
    uint8_t* planeU = (uint8_t*) malloc(c_size);
    uint8_t* planeV = (uint8_t*) malloc(c_size);
    if (!planeY || !planeU || !planeV) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Failed to allocate YUV buffers");
        if (planeY) free(planeY);
        if (planeU) free(planeU);
        if (planeV) free(planeV);
        LibRaw::dcraw_clear_mem(processed_image);
        env->ReleaseStringUTFChars(dngPath_, dngPath);
        return JNI_FALSE;
    }

    // We'll perform BT.601 limited range conversion and 4:2:0 chroma subsampling (average 2x2)
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            int pix_index = (j * width + i) * 3;
            int R = rgb[pix_index + 0];
            int G = rgb[pix_index + 1];
            int B = rgb[pix_index + 2];
            int Y = (( 66 * R + 129 * G +  25 * B + 128) >> 8) + 16;
            if (Y < 0) Y = 0; if (Y > 255) Y = 255;
            planeY[j * width + i] = (uint8_t) Y;
        }
    }
    // Compute U/V by averaging 2x2 blocks
    for (int j = 0; j < height; j += 2) {
        for (int i = 0; i < width; i += 2) {
            int sumU = 0, sumV = 0, count = 0;
            for (int y_off = 0; y_off < 2; ++y_off) {
                int y = j + y_off;
                if (y >= height) continue;
                for (int x_off = 0; x_off < 2; ++x_off) {
                    int x = i + x_off;
                    if (x >= width) continue;
                    int pix_index = (y * width + x) * 3;
                    int R = rgb[pix_index + 0];
                    int G = rgb[pix_index + 1];
                    int B = rgb[pix_index + 2];
                    int U = ((-38 * R -  74 * G + 112 * B + 128) >> 8) + 128;
                    int V = ((112 * R -  94 * G -  18 * B + 128) >> 8) + 128;
                    if (U < 0) U = 0; if (U > 255) U = 255;
                    if (V < 0) V = 0; if (V > 255) V = 255;
                    sumU += U;
                    sumV += V;
                    ++count;
                }
            }
            int idx = (j/2) * (width/2) + (i/2);
            planeU[idx] = (uint8_t) (sumU / count);
            planeV[idx] = (uint8_t) (sumV / count);
        }
    }

    // Build heif image with YCbCr 4:2:0 planes (8-bit)
    heif_image* heif_image_out;
    heif_image_create(width, height, heif_colorspace_YCbCr, heif_chroma_420, &heif_image_out);
    heif_image_add_plane(heif_image_out, heif_channel_Y, width, height, 8);
    heif_image_add_plane(heif_image_out, heif_channel_Cb, width/2, height/2, 8);
    heif_image_add_plane(heif_image_out, heif_channel_Cr, width/2, height/2, 8);

    int strideY = 0, strideU = 0, strideV = 0;
    uint8_t* pY = heif_image_get_plane(heif_image_out, heif_channel_Y, &strideY);
    uint8_t* pU = heif_image_get_plane(heif_image_out, heif_channel_Cb, &strideU);
    uint8_t* pV = heif_image_get_plane(heif_image_out, heif_channel_Cr, &strideV);

    // copy Y plane
    for (int y = 0; y < height; ++y) {
        memcpy(pY + y * strideY, planeY + y * width, width);
    }
    // copy subsampled Cb/Cr planes
    for (int y = 0; y < height/2; ++y) {
        memcpy(pU + y * strideU, planeU + y * (width/2), width/2);
        memcpy(pV + y * strideV, planeV + y * (width/2), width/2);
    }

    free(planeY);
    free(planeU);
    free(planeV);

    heif_context* ctx = heif_context_alloc();
    heif_encoder* encoder;
    heif_context_get_encoder_for_format(ctx, heif_compression_HEVC, &encoder);

    if (quality >= 100) {
        heif_encoder_set_lossless(encoder, 1);
    } else {
        heif_encoder_set_lossy_quality(encoder, quality);
    }

    heif_image_handle* handle;
    heif_error herr = heif_context_encode_image(ctx, heif_image_out, encoder, nullptr, &handle);
    heif_encoder_release(encoder);

    const char *outputPath = env->GetStringUTFChars(outputPath_, 0);
    heif_error werr = heif_context_write_to_file(ctx, outputPath);
    if (werr.code != heif_error_Ok) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "write to file failed: %s", werr.message);
    }

    heif_image_handle_release(handle);
    heif_context_free(ctx);
    heif_image_release(heif_image_out);

    LibRaw::dcraw_clear_mem(processed_image);

    env->ReleaseStringUTFChars(dngPath_, dngPath);
    env->ReleaseStringUTFChars(outputPath_, outputPath);

    return (werr.code == heif_error_Ok) ? JNI_TRUE : JNI_FALSE;
}
