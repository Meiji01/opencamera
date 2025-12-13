cmake_minimum_required(VERSION 3.10.2)

project("heif_saver")

include_directories(src/main/cpp
                    src/main/cpp/libheif
                    src/main/cpp/LibRaw-0.21.4/libraw)

add_subdirectory(LibRaw-0.21.4)

add_library( # Sets the name of the library.
             opencamera_heif

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             heif_saver.cpp
             heif_jni.cpp )

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

# Define an imported shared library for libheif
add_library(heif_prebuilt SHARED IMPORTED)
set_target_properties(heif_prebuilt PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/${ANDROID_ABI}/libheif.so
)

# Find the Android library
find_library(
    android-lib
    android
)

# Find the JNI Graphics library
find_library(
    jnigraphics-lib
    jnigraphics
)

target_link_libraries( # Specifies the target library.
                       opencamera_heif

                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib}
                       heif_prebuilt
                       ${android-lib}
                       ${jnigraphics-lib} 
                       raw_r)