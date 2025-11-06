cmake_minimum_required(VERSION 3.10.2)

project(heif_writer)

add_library(heif SHARED IMPORTED)
set_target_properties(heif PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../jniLibs/${ANDROID_ABI}/libheif.so)

add_library(heif_writer SHARED heif_writer.cpp)

target_link_libraries(heif_writer heif android log)
