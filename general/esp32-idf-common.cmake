# general/esp32-idf-common.cmake — sources + flags EVERY ESP-IDF firmware in
# this repo needs, in one place.
#
# Included by the "main" component of each ESP32 project:
#   server/cli/esp32/main/CMakeLists.txt              (the all-in-one server)
#   benchmark/benchmarkmapmanager/esp32/main/CMakeLists.txt  (on-device bench)
#
# It owns ONLY what is genuinely identical between them — before this file the
# NOXML filter regex below was duplicated byte-for-byte in both, which is a
# trap: a refactor that moves a source has to be mirrored in every copy or one
# firmware silently stops compiling the right TUs. Everything that legitimately
# differs stays in the caller: the per-project source sets, the defines, the
# ESP-IDF REQUIRES, and -f{no-}exceptions / -f{no-}rtti (the server builds
# without them, the benchmark needs them to catch std::bad_alloc per scenario).
#
# Provides:
#   CC_ESP32_GENERAL_SRCS         general/base/*.cpp, NOXML-filtered
#   CC_ESP32_ZSTD_SRCS            the vendored zstd subset (needs ZSTD_DISABLE_ASM)
#   CC_ESP32_VENDOR_SRCS          blake3 + xxhash + zlib + zstd
#   CC_ESP32_VENDOR_INCLUDE_DIRS  include dirs for the vendored libs
#   cc_esp32_common_component_flags(<target>)   -Os + -Werror relaxations
#                                               + XXH_INLINE_ALL + ZSTD_DISABLE_ASM
#
# The caller must set CC_REPO (absolute repo root) BEFORE including this.

if(NOT DEFINED CC_REPO)
    message(FATAL_ERROR "esp32-idf-common.cmake: set CC_REPO (absolute repo root) before include()")
endif()

# --- shared game-logic sources, NOXML-filtered --------------------------------
# Mirrors the host NOXML build's source set (general/general.cmake NOXML branch).
# Under CATCHCHALLENGER_NOXML the host compiles only CommonDatapack.cpp for
# datapack loading; the XML *loaders* are not compiled at all — their headers
# guard the whole class body with #ifndef CATCHCHALLENGER_NOXML, so their .cpp
# definitions would fail with "no declaration matches" / "tinyxml2 does not name
# a type". Specifically dropped:
#   - tinyXML2 + any Qt TU (no Qt, no XML),
#   - the XML datapack loaders: Map_loader.cpp / Map_loaderMain.cpp,
#     general/base/DatapackGeneralLoader/*.cpp, and the XML FightLoaders
#     (FightLoaderBuff/Monster/Skill.cpp). NB: FightLoader.cpp (no suffix) IS
#     compiled under NOXML (it carries #ifdef stubs) — keep it,
#   - GUI-only helpers (CCGuiLog/CCGuiStats) and ConnectedSocket.cpp.
file(GLOB_RECURSE CC_ESP32_GENERAL_SRCS "${CC_REPO}/general/base/*.cpp")
list(FILTER CC_ESP32_GENERAL_SRCS EXCLUDE REGEX
    "/tinyXML2/|/Qt|Qt\\.cpp$|/Map_loader(Main)?\\.cpp$|/DatapackGeneralLoader/|/FightLoader(Buff|Monster|Skill)\\.cpp$|/CCGui(Log|Stats)\\.cpp$|/ConnectedSocket\\.cpp$")

# --- vendored third-party (the xtensa sysroot has no system copy) -------------
# blake3: only the portable C path. The x86 SSE/AVX (blake3_*_x86*.c / .S) and
# NEON files don't build for xtensa; general.cmake's vendored fallback lists
# exactly these three, so mirror that.
set(_cc_blake3
    ${CC_REPO}/general/blake3/blake3.c
    ${CC_REPO}/general/blake3/blake3_dispatch.c
    ${CC_REPO}/general/blake3/blake3_portable.c
)
# xxhash: one TU (XXH_INLINE_ALL is set by the macro below), mirrors general.cmake.
set(_cc_xxhash ${CC_REPO}/general/libxxhash/xxhash.c)

# zlib: vendored fallback. Mirrors the vendored branch of general/libzlib.cmake.
# CatchChallenger only uses the in-memory inflate/uncompress path, but the gz*
# sources are kept for parity with the host build.
set(_cc_zlib_dir ${CC_REPO}/general/libzlib)
set(_cc_zlib
    ${_cc_zlib_dir}/adler32.c
    ${_cc_zlib_dir}/compress.c
    ${_cc_zlib_dir}/crc32.c
    ${_cc_zlib_dir}/deflate.c
    ${_cc_zlib_dir}/infback.c
    ${_cc_zlib_dir}/inffast.c
    ${_cc_zlib_dir}/inflate.c
    ${_cc_zlib_dir}/inftrees.c
    ${_cc_zlib_dir}/trees.c
    ${_cc_zlib_dir}/uncompr.c
    ${_cc_zlib_dir}/zutil.c
)

# zstd: vendored static fallback. Mirrors general/libzstd/build/cmake/lib —
# common + compress + decompress C sources, no amd64 .S (xtensa), no
# dictBuilder / deprecated (CompressionProtocol.cpp only calls
# ZSTD_compress/decompress/isError/getErrorName).
set(_cc_zstd_dir ${CC_REPO}/general/libzstd/lib)
file(GLOB _cc_zstd_common     "${_cc_zstd_dir}/common/*.c")
file(GLOB _cc_zstd_compress   "${_cc_zstd_dir}/compress/*.c")
file(GLOB _cc_zstd_decompress "${_cc_zstd_dir}/decompress/*.c")
set(CC_ESP32_ZSTD_SRCS ${_cc_zstd_common} ${_cc_zstd_compress} ${_cc_zstd_decompress})

set(CC_ESP32_VENDOR_SRCS
    ${_cc_blake3} ${_cc_xxhash} ${_cc_zlib} ${CC_ESP32_ZSTD_SRCS})
set(CC_ESP32_VENDOR_INCLUDE_DIRS
    "${CC_REPO}/general/blake3" "${CC_REPO}/general/libxxhash"
    "${_cc_zlib_dir}" "${_cc_zstd_dir}")

# --- flags every ESP32 component needs ----------------------------------------
# Call AFTER idf_component_register(), passing ${COMPONENT_LIB}.
#
# CMAKE_CURRENT_SOURCE_DIR, not CMAKE_CURRENT_LIST_DIR: inside an include()d
# file (and inside a macro body) CMAKE_CURRENT_LIST_DIR is THIS file's
# directory, which would attach the zstd source property to the wrong
# directory scope and silently lose ZSTD_DISABLE_ASM.
macro(cc_esp32_common_component_flags _cc_target)
    # -Os: flash is the binding constraint on these boards.
    target_compile_options(${_cc_target} PRIVATE -Os)

    # ESP-IDF forces -Werror=all (the host CatchChallenger build does not),
    # which turns long-standing, host-tolerated warnings in SHARED code into
    # hard errors. Relax them to plain warnings for the ESP32 "main" component
    # only — it changes no other target's build, and shared sources must not be
    # edited just to satisfy a stricter cross flag:
    #   - overloaded-virtual: ClientWithMapEventLoop::reset(int) intentionally
    #     shadows ProtocolParsingBase::reset() (different signature and role).
    #   - maybe-uninitialized: a gcc-13 xtensa false positive in the fight
    #     engine (CommonFightEngineTurn.cpp 'quantity').
    target_compile_options(${_cc_target} PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-error=overloaded-virtual>
        -Wno-error=maybe-uninitialized
    )

    # XXH_INLINE_ALL: required by the vendored xxhash.c TU so call sites resolve
    # the xxh symbols inline (same as general.cmake's vendored branch). Applying
    # it to the whole component is harmless — only xxhash.c reacts to it.
    target_compile_definitions(${_cc_target} PRIVATE XXH_INLINE_ALL)

    # ZSTD_DISABLE_ASM: xtensa has no amd64 huf_decompress_amd64.S; this define
    # selects the C fallback (mirrors the non-amd64 branch of zstd's cmake).
    set_source_files_properties(${CC_ESP32_ZSTD_SRCS}
        PROPERTIES COMPILE_DEFINITIONS "ZSTD_DISABLE_ASM"
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endmacro()
