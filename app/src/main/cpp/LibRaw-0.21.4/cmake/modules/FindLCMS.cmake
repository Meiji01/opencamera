# - Try to find Little CMS
# Once done this will define
#
#  LCMS_FOUND - system has Little CMS
#  LCMS_INCLUDE_DIR - the Little CMS include directory
#  LCMS_LIBRARIES - Link these to use Little CMS
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

# Copyright (c) 2006, Alexander Neundorf, <neundorf@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (LCMS_INCLUDE_DIR AND LCMS_LIBRARIES)
  # in cache already
  set(LCMS_FOUND TRUE)
else (LCMS_INCLUDE_DIR AND LCMS_LIBRARIES)

  find_path(LCMS_INCLUDE_DIR lcms.h
  PATHS
  /usr/include
  /usr/local/include
  /opt/local/include
  /sw/include
  )

  find_library(LCMS_LIBRARIES NAMES lcms PATHS /usr/lib /usr/local/lib /opt/local/lib /sw/lib)

  if (LCMS_INCLUDE_DIR AND LCMS_LIBRARIES)
    set(LCMS_FOUND TRUE)
  endif (LCMS_INCLUDE_DIR AND LCMS_LIBRARIES)


  if (LCMS_FOUND)
    if (NOT LCMS_FIND_QUIETLY)
      message(STATUS "Found lcms: ${LCMS_LIBRARIES}")
    endif (NOT LCMS_FIND_QUIETLY)
  else (LCMS_FOUND)
    if (LCMS_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find lcms")
    endif (LCMS_FIND_REQUIRED)
  endif (LCMS_FOUND)

  # show the LCMS_INCLUDE_DIR and LCMS_LIBRARIES variables only in the advanced view
  mark_as_advanced(LCMS_INCLUDE_DIR LCMS_LIBRARIES)

endif (LCMS_INCLUDE_DIR AND LCMS_LIBRARIES)
