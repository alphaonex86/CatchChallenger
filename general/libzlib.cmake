# general/libzlib.cmake — zlib lookup with vendored fallback.
#
# Prefer system zlib (Linux distros, macOS Homebrew, etc.). Fall back to the
# vendored copy under general/libzlib/ when none is found, e.g. MinGW
# cross-compile or a bare VPS without -dev packages.
#
# EXTERNALLIBZLIB toggle (mirrors EXTERNALLIBZSTD pattern):
#   ON      → require system zlib (find_package(ZLIB REQUIRED), no fallback).
#   OFF     → use vendored copy; never call find_package(ZLIB).
#   unset   → try system first (QUIET), fall back to vendored.

# Declare the option only once. Default unset-sentinel: use a CACHE STRING with
# empty default so we can distinguish "not set" from explicit ON/OFF.
if(NOT DEFINED EXTERNALLIBZLIB)
    set(EXTERNALLIBZLIB "" CACHE STRING "Force system zlib (ON), force vendored (OFF), or auto (empty)")
    set_property(CACHE EXTERNALLIBZLIB PROPERTY STRINGS "" "ON" "OFF")
endif()

if(NOT TARGET catchchallenger_zlib)
    set(_cc_zlib_use_system FALSE)
    set(_cc_zlib_use_vendored FALSE)
    if("${EXTERNALLIBZLIB}" STREQUAL "")
        # Auto: try system first, fall back to vendored.
        find_package(ZLIB QUIET)
        if(ZLIB_FOUND)
            set(_cc_zlib_use_system TRUE)
        else()
            set(_cc_zlib_use_vendored TRUE)
        endif()
    elseif(EXTERNALLIBZLIB)
        # Forced system: hard-require it.
        find_package(ZLIB REQUIRED)
        set(_cc_zlib_use_system TRUE)
    else()
        # Forced vendored: never look at the system copy.
        set(_cc_zlib_use_vendored TRUE)
    endif()

    if(_cc_zlib_use_system)
        message(STATUS "zlib: using system copy (${ZLIB_LIBRARIES})")
        add_library(catchchallenger_zlib INTERFACE)
        target_link_libraries(catchchallenger_zlib INTERFACE ZLIB::ZLIB)
    else()
        message(STATUS "zlib: building vendored zlib 1.3.2")
        set(ZLIB_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/libzlib)
        set(ZLIB_SOURCES
            ${ZLIB_SRC_DIR}/adler32.c
            ${ZLIB_SRC_DIR}/compress.c
            ${ZLIB_SRC_DIR}/crc32.c
            ${ZLIB_SRC_DIR}/deflate.c
            ${ZLIB_SRC_DIR}/infback.c
            ${ZLIB_SRC_DIR}/inffast.c
            ${ZLIB_SRC_DIR}/inflate.c
            ${ZLIB_SRC_DIR}/inftrees.c
            ${ZLIB_SRC_DIR}/trees.c
            ${ZLIB_SRC_DIR}/uncompr.c
            ${ZLIB_SRC_DIR}/zutil.c
        )
        # The gz* file API (gzopen/gzread/…) needs POSIX errno values
        # (EWOULDBLOCK) DJGPP lacks. CatchChallenger only uses the in-memory
        # inflate/uncompress path (tiled map layers), so skip gz* on MS-DOS.
        if(NOT CC_TARGET_MSDOS)
            list(APPEND ZLIB_SOURCES
                ${ZLIB_SRC_DIR}/gzclose.c
                ${ZLIB_SRC_DIR}/gzlib.c
                ${ZLIB_SRC_DIR}/gzread.c
                ${ZLIB_SRC_DIR}/gzwrite.c
            )
        endif()
        add_library(catchchallenger_zlib STATIC ${ZLIB_SOURCES})
        target_include_directories(catchchallenger_zlib PUBLIC ${ZLIB_SRC_DIR})
        # Silence well-known harmless warnings in vendored zlib so it compiles
        # cleanly under modern toolchains without modifying upstream sources.
        if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(catchchallenger_zlib PRIVATE
                -Wno-implicit-function-declaration
                -Wno-deprecated-non-prototype)
        endif()
        # Provide an alias matching find_package(ZLIB) so unmodified
        # consumers (target_link_libraries(... ZLIB::ZLIB)) keep working.
        add_library(ZLIB::ZLIB ALIAS catchchallenger_zlib)
        set(ZLIB_FOUND TRUE)
    endif()
endif()
