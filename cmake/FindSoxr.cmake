# FindSoxr.cmake
# Find the SoX Resampler library (libsoxr)
#
# This module defines:
#  SOXR_FOUND - True if libsoxr is found
#  SOXR_INCLUDE_DIRS - Include directories for libsoxr
#  SOXR_LIBRARIES - Libraries to link against
#  SOXR_VERSION - Version string of libsoxr
#  SOXR_VERSION_MAJOR - Major version number
#  SOXR_VERSION_MINOR - Minor version number
#  SOXR_VERSION_PATCH - Patch version number
#
# Also creates the imported target:
#  soxr::soxr - The libsoxr library

# Copyright The Mumble Developers. All rights reserved.
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file at the root of the
# Mumble source tree or at <https://www.mumble.info/LICENSE>.

include(FindPackageHandleStandardArgs)

# Try to find libsoxr using pkg-config first
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_SOXR QUIET soxr)
endif()

# Find the header file
find_path(SOXR_INCLUDE_DIR
    NAMES soxr.h
    HINTS
        ${PC_SOXR_INCLUDEDIR}
        ${PC_SOXR_INCLUDE_DIRS}
    PATHS
        /usr/include
        /usr/local/include
        /opt/local/include
        ${CMAKE_PREFIX_PATH}/include
    PATH_SUFFIXES
        soxr
)

# Find the library
find_library(SOXR_LIBRARY
    NAMES soxr libsoxr
    HINTS
        ${PC_SOXR_LIBDIR}
        ${PC_SOXR_LIBRARY_DIRS}
    PATHS
        /usr/lib
        /usr/local/lib
        /opt/local/lib
        ${CMAKE_PREFIX_PATH}/lib
    PATH_SUFFIXES
        x86_64-linux-gnu
        i386-linux-gnu
        arm-linux-gnueabihf
        aarch64-linux-gnu
)

# Extract version information
if(SOXR_INCLUDE_DIR AND EXISTS "${SOXR_INCLUDE_DIR}/soxr.h")
    file(READ "${SOXR_INCLUDE_DIR}/soxr.h" SOXR_H_CONTENT)

    # Try to extract version from header
    string(REGEX MATCH "#define[ \t]+SOXR_VERSION_MAJOR[ \t]+([0-9]+)" _ "${SOXR_H_CONTENT}")
    if(CMAKE_MATCH_1)
        set(SOXR_VERSION_MAJOR ${CMAKE_MATCH_1})
    endif()

    string(REGEX MATCH "#define[ \t]+SOXR_VERSION_MINOR[ \t]+([0-9]+)" _ "${SOXR_H_CONTENT}")
    if(CMAKE_MATCH_1)
        set(SOXR_VERSION_MINOR ${CMAKE_MATCH_1})
    endif()

    string(REGEX MATCH "#define[ \t]+SOXR_VERSION_PATCH[ \t]+([0-9]+)" _ "${SOXR_H_CONTENT}")
    if(CMAKE_MATCH_1)
        set(SOXR_VERSION_PATCH ${CMAKE_MATCH_1})
    endif()

    # Fallback to pkg-config version if header parsing failed
    if(NOT SOXR_VERSION_MAJOR AND PC_SOXR_VERSION)
        string(REPLACE "." ";" VERSION_LIST ${PC_SOXR_VERSION})
        list(LENGTH VERSION_LIST VERSION_LIST_LENGTH)
        if(VERSION_LIST_LENGTH GREATER_EQUAL 1)
            list(GET VERSION_LIST 0 SOXR_VERSION_MAJOR)
        endif()
        if(VERSION_LIST_LENGTH GREATER_EQUAL 2)
            list(GET VERSION_LIST 1 SOXR_VERSION_MINOR)
        endif()
        if(VERSION_LIST_LENGTH GREATER_EQUAL 3)
            list(GET VERSION_LIST 2 SOXR_VERSION_PATCH)
        endif()
    endif()

    # Construct version string
    if(SOXR_VERSION_MAJOR)
        set(SOXR_VERSION "${SOXR_VERSION_MAJOR}")
        if(SOXR_VERSION_MINOR)
            string(APPEND SOXR_VERSION ".${SOXR_VERSION_MINOR}")
            if(SOXR_VERSION_PATCH)
                string(APPEND SOXR_VERSION ".${SOXR_VERSION_PATCH}")
            endif()
        endif()
    else()
        # Use pkg-config version as fallback
        set(SOXR_VERSION "${PC_SOXR_VERSION}")
    endif()
endif()

# Handle standard arguments
find_package_handle_standard_args(Soxr
    REQUIRED_VARS
        SOXR_LIBRARY
        SOXR_INCLUDE_DIR
    VERSION_VAR
        SOXR_VERSION
)

if(SOXR_FOUND)
    set(SOXR_LIBRARIES ${SOXR_LIBRARY})
    set(SOXR_INCLUDE_DIRS ${SOXR_INCLUDE_DIR})

    # Create imported target
    if(NOT TARGET soxr::soxr)
        add_library(soxr::soxr UNKNOWN IMPORTED)
        set_target_properties(soxr::soxr PROPERTIES
            IMPORTED_LOCATION "${SOXR_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${SOXR_INCLUDE_DIR}"
        )

        # Add compile definitions if available from pkg-config
        if(PC_SOXR_CFLAGS_OTHER)
            set_property(TARGET soxr::soxr PROPERTY
                INTERFACE_COMPILE_OPTIONS ${PC_SOXR_CFLAGS_OTHER}
            )
        endif()

        # Add link flags if available from pkg-config
        if(PC_SOXR_LDFLAGS_OTHER)
            set_property(TARGET soxr::soxr PROPERTY
                INTERFACE_LINK_OPTIONS ${PC_SOXR_LDFLAGS_OTHER}
            )
        endif()
    endif()

    # Mark variables as advanced
    mark_as_advanced(
        SOXR_INCLUDE_DIR
        SOXR_LIBRARY
    )

    # Set compatibility variables for legacy usage
    set(soxr_FOUND ${SOXR_FOUND})
    set(soxr_INCLUDE_DIRS ${SOXR_INCLUDE_DIRS})
    set(soxr_LIBRARIES ${SOXR_LIBRARIES})
    set(soxr_VERSION ${SOXR_VERSION})
endif()
