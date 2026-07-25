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

# Shared ESP-IDF sources + flags (NOXML-filtered general/base, vendored
# blake3 / xxhash / zlib / zstd, -Werror relaxations). Same include the server
# firmware uses, so a source move is mirrored in one place instead of two.
include(${CC_REPO}/general/esp32-idf-common.cmake)

# --- the benchmark harness (the production server-side algorithm under test) --
set(_cc_bench
    ${CC_REPO}/benchmark/benchmarkmapmanager/main.cpp
    ${CC_REPO}/test/testingmapmanagement/Stubs.cpp
    ${CC_REPO}/server/base/MapManagement/MapVisibilityAlgorithm.cpp
    ${CC_REPO}/server/base/MapServer.cpp
)

idf_component_register(
    SRCS "app_main.cpp"
         ${_cc_bench} ${CC_ESP32_GENERAL_SRCS} ${CC_ESP32_VENDOR_SRCS}
    INCLUDE_DIRS "." "${CC_REPO}"
                 "${CC_REPO}/benchmark/benchmarkmapmanager"
                 "${CC_REPO}/test/testingmapmanagement"
                 ${CC_ESP32_VENDOR_INCLUDE_DIRS}
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

# Shared flags: -Os, the -Werror relaxations, XXH_INLINE_ALL, ZSTD_DISABLE_ASM.
cc_esp32_common_component_flags(${COMPONENT_LIB})

# Specific to THIS firmware: C++ exceptions + RTTI stay ON (sdkconfig enables
# them globally too) so app_main can catch std::bad_alloc per scenario and SKIP
# the OOM player counts instead of aborting the whole sweep. The server
# firmware deliberately builds -fno-exceptions/-fno-rtti instead.
target_compile_options(${COMPONENT_LIB} PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-fexceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-frtti>
)

# format: main.cpp's snprintf("p%u", id) where id is uint32_t == 'long unsigned
# int' on xtensa (host-fine '%u'). Warning on the native build; only downgraded
# from -Werror here, the shared source is never touched.
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=format)
