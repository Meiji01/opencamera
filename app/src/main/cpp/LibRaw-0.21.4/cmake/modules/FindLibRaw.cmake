# - Find LibRaw
# This module finds an installed LibRaw library.
#
# This module sets the following variables:
#  LibRaw_FOUND               - set to true if the library is found
#  LibRaw_VERSION_STRING      - the version of the library (x.y.z)
#  LibRaw_INCLUDE_DIR         - the directory where libraw/libraw.h is
#  LibRaw_LIBRARIES           - the library (libraw)
#  LibRaw_r_LIBRARIES         - the thread-safe library (libraw_r)

# Copyright (c) 2009, Clemens Wacha <reflex-support@cadwork.ch>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(NOT WIN32)
  find_package(PkgConfig)
  pkg_check_modules(PC_LIBRAW QUIET libraw)
  pkg_check_modules(PC_LIBRAW_R QUIET libraw_r)
endif(NOT WIN32)

find_path(LibRaw_INCLUDE_DIR libraw/libraw.h
          PATHS ${PC_LIBRAW_INCLUDE_DIRS}
          )
find_library(LibRaw_LIBRARIES NAMES raw libraw
             PATHS ${PC_LIBRAW_LIBRARY_DIRS}
             )
find_library(LibRaw_r_LIBRARIES NAMES raw_r libraw_r
             PATHS ${PC_LIBRAW_R_LIBRARY_DIRS}
             )

if(LibRaw_INCLUDE_DIR)
  file(READ "${LibRaw_INCLUDE_DIR}/libraw/libraw_version.h" _libraw_version_content)

  string(REGEX MATCH "LIBRAW_VERSION_MAJOR +[0-9]+"
         _major_version "${_libraw_version_content}")
  string(REGEX MATCH "LIBRAW_VERSION_MINOR +[0-9]+"
         _minor_version "${_libraw_version_content}")
  string(REGEX MATCH "LIBRAW_VERSION_PATCH +[0-9]+"
         _patch_version "${_libraw_version_content}")

  string(REPLACE "LIBRAW_VERSION_MAJOR" "" _major_version "${_major_version}")
  string(REPLACE "LIBRAW_VERSION_MINOR" "" _minor_version "${_minor_version}")
  string(REPLACE "LIBRAW_VERSION_PATCH" "" _patch_version "${_patch_version}")

  string(STRIP "${_major_version}" _major_version)
  string(STRIP "${_minor_version}" _minor_version)
  string(STRIP "${_patch_version}" _patch_version)

  set(LibRaw_VERSION_STRING "${_major_version}.${_minor_version}.${_patch_version}")
endif(LibRaw_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibRaw
                                  REQUIRED_VARS LibRaw_LIBRARIES LibRaw_INCLUDE_DIR
                                  VERSION_VAR LibRaw_VERSION_STRING)

mark_as_advanced(LibRaw_INCLUDE_DIR LibRaw_LIBRARIES LibRaw_r_LIBRARIES)
