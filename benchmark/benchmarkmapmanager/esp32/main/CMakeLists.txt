# benchmark/benchmarkmapmanager/esp32/main/CMakeLists.txt — the ESP-IDF "main"
# component for the MapVisibilityAlgorithm::min_network() pure-CPU benchmark.
#
# Compiles app_main.cpp + the SAME harness the native fleet builds
# (benchmark/benchmarkmapmanager/main.cpp + test/testingmapmanagement/Stubs.cpp +
# server/base/MapManagement/MapVisibilityAlgorithm.cpp + server/base/MapServer.cpp)
# plus the MINIMAL set of general/ sources those pull in. NO WiFi, NO lwIP, NO
# server stack, NO datapack-cpp.
#
# The source set MIRRORS the native CMakeLists.txt
# (benchmark/benchmarkmapmanager/CMakeLists.txt), which links the whole
# catchchallenger_general INTERFACE lib (general/base/*.cpp incl. base/fight/)
# — EXCEPT here we build under CATCHCHALLENGER_NOXML (no XML parsing is needed for
# the pure-CPU loop), so the XML datapack loaders + tinyXML2 are excluded exactly
# as the server firmware (server/cli/esp32/main/CMakeLists.txt) does. The
# benchmark never parses a datapack — the workload is synthetic players in RAM.

get_filename_component(CC_REPO "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)

# --- shared general sources (globbed, then NOXML-filtered like the server fw) -
file(GLOB_RECURSE _cc_general "${CC_REPO}/general/base/*.cpp")
# Drop the XML loaders + tinyXML2 + Qt TUs (CATCHCHALLENGER_NOXML): their class
# bodies are #ifndef'd out under NOXML, so their .cpp definitions would be
# "no declaration matches" / "tinyxml2 does not name a type" errors. Same regex
# as server/cli/esp32/main/CMakeLists.txt. FightLoader.cpp (no suffix) carries
# NOXML stubs — keep it. CCGui* are server-gui-only; ConnectedSocket.cpp isn't in
# this source set.
list(FILTER _cc_general EXCLUDE REGEX
    "/tinyXML2/|/Qt|Qt\\.cpp$|/Map_loader(Main)?\\.cpp$|/DatapackGeneralLoader/|/FightLoader(Buff|Monster|Skill)\\.cpp$|/CCGui(Log|Stats)\\.cpp$|/ConnectedSocket\\.cpp$")

# blake3: portable C path only (the x86 SSE/AVX .c/.S + NEON don't build for
# xtensa). Mirrors general.cmake's vendored fallback list.
set(_cc_blake3
    ${CC_REPO}/general/blake3/blake3.c
    ${CC_REPO}/general/blake3/blake3_dispatch.c
    ${CC_REPO}/general/blake3/blake3_portable.c
)
# xxhash: one TU (XXH_INLINE_ALL set below).
set(_cc_xxhash ${CC_REPO}/general/libxxhash/xxhash.c)

# zlib: vendored fallback (no system zlib in the xtensa sysroot). The CommonMap
# tiled decoder + CompressionProtocol.cpp use inflate/uncompress.
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

# zstd: vendored static fallback (CompressionProtocol.cpp calls ZSTD_*). No
# amd64 .S on xtensa (ZSTD_DISABLE_ASM set below), no dictBuilder / deprecated.
set(_cc_zstd_dir ${CC_REPO}/general/libzstd/lib)
file(GLOB _cc_zstd_common     "${_cc_zstd_dir}/common/*.c")
file(GLOB _cc_zstd_compress   "${_cc_zstd_dir}/compress/*.c")
file(GLOB _cc_zstd_decompress "${_cc_zstd_dir}/decompress/*.c")
set(_cc_zstd ${_cc_zstd_common} ${_cc_zstd_compress} ${_cc_zstd_decompress})

# --- the benchmark harness (the production server-side algorithm under test) --
set(_cc_bench
    ${CC_REPO}/benchmark/benchmarkmapmanager/main.cpp
    ${CC_REPO}/test/testingmapmanagement/Stubs.cpp
    ${CC_REPO}/server/base/MapManagement/MapVisibilityAlgorithm.cpp
    ${CC_REPO}/server/base/MapServer.cpp
)

idf_component_register(
    SRCS "app_main.cpp"
         ${_cc_bench} ${_cc_general} ${_cc_blake3} ${_cc_xxhash}
         ${_cc_zlib} ${_cc_zstd}
    INCLUDE_DIRS "." "${CC_REPO}"
                 "${CC_REPO}/benchmark/benchmarkmapmanager"
                 "${CC_REPO}/test/testingmapmanagement"
                 "${CC_REPO}/general/blake3" "${CC_REPO}/general/libxxhash"
                 "${_cc_zlib_dir}" "${_cc_zstd_dir}"
    REQUIRES esp_timer
)

# Defines: mirror the native benchmark CMakeLists.txt (CATCHCHALLENGER_TESTING
# triggers the Stubs.hpp injection at the top of the server-side .cpp;
# DB_FILE / HARDENED / BENCHMARK match it) + the ESP32 profile defines (NOXML +
# CC_TARGET_ESP32). CC_TARGET_ESP32 compiles out the argv main() so app_main()
# is the single entry.
#
# NOTE: the native build also defines CATCHCHALLENGER_CLIENT, but it is
# DELIBERATELY OMITTED here. Under CATCHCHALLENGER_NOXML the shared
# general/base/CommonDatapackServerSpec.{hpp,cpp} are inconsistent for the
# CLIENT+NOXML combo (the header drops applyMonstersRate()'s declaration under
# NOXML while the .cpp still defines it under CLIENT → "no declaration matches").
# The benchmark's OWN sources (main.cpp / MapVisibilityAlgorithm.cpp /
# MapServer.cpp / Stubs.{hpp,cpp}) never reference CATCHCHALLENGER_CLIENT, so
# dropping it is behaviour-neutral for the measured code and keeps the firmware
# off the broken combo without editing shared code. Validated: the identical
# NOXML-no-CLIENT source set builds + runs natively and prints correct BENCH
# lines. (The native fleet build is untouched — it stays no-NOXML + CLIENT.)
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    CATCHCHALLENGER_TESTING
    CATCHCHALLENGER_HARDENED
    CATCHCHALLENGER_BENCHMARK
    CATCHCHALLENGER_DB_FILE
    CATCHCHALLENGER_NOXML
    CC_TARGET_ESP32
)

# -Os (small flash); C++ exceptions ON for this component (sdkconfig also turns
# them on globally) so app_main can catch std::bad_alloc per scenario and SKIP
# the OOM player counts instead of aborting the whole run.
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Os
    $<$<COMPILE_LANGUAGE:CXX>:-fexceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-frtti>
)

# ESP-IDF forces -Werror=all; relax the host-tolerated warnings the shared code
# trips (same spirit as the server firmware). overloaded-virtual: production
# shadow signatures; maybe-uninitialized: a gcc-13 xtensa false positive in the
# fight engine (compiled here because general/base/fight/* is part of the lib);
# format: main.cpp's `snprintf("p%u", id)` where id is uint32_t == 'long
# unsigned int' on xtensa (host-fine '%u'). These are warnings on the native
# build; we only downgrade them from -Werror here, never touch the shared source.
target_compile_options(${COMPONENT_LIB} PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-error=overloaded-virtual>
    -Wno-error=maybe-uninitialized
    -Wno-error=format
)

# XXH_INLINE_ALL: required by the vendored xxhash.c TU (mirrors general.cmake).
target_compile_definitions(${COMPONENT_LIB} PRIVATE XXH_INLINE_ALL)

# ZSTD_DISABLE_ASM: xtensa has no amd64 huf_decompress_amd64.S; select the C
# fallback (mirrors the non-amd64 branch of zstd's cmake + the server firmware).
set_source_files_properties(${_cc_zstd}
    PROPERTIES COMPILE_DEFINITIONS "ZSTD_DISABLE_ASM"
    DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")
