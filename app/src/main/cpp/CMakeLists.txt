cmake_minimum_required(VERSION 3.10.2)

project(heif_writer)

add_library(heif SHARED IMPORTED)
set_target_properties(heif PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/libheif.so)

target_include_directories(native-lib PRIVATE libheif/include)
target_link_libraries(native-lib heif)