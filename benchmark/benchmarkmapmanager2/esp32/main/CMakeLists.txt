# ESP-IDF "main" component of the benchmarkmapmanager2 firmware. Mirrors
# benchmark/benchmarkmapmanager/esp32/main/CMakeLists.txt (same NOXML source set
# and flags, see there for the reasoning) with stage 2 + the generated workload
# in place of the synthetic harness.

get_filename_component(CC_REPO "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)

if(NOT DEFINED ENV{CC_WORKLOAD_CPP} OR NOT EXISTS "$ENV{CC_WORKLOAD_CPP}")
    message(FATAL_ERROR
        "CC_WORKLOAD_CPP is not set or does not exist: the firmware has no workload.\n"
        "Generate one with benchmarkmapmanager2/stage1 and export it.")
endif()

# esp32_bench_config.h (player counts + budget) is generated next to it.
get_filename_component(_cc_workload_dir "$ENV{CC_WORKLOAD_CPP}" DIRECTORY)
if(NOT EXISTS "${_cc_workload_dir}/esp32_bench_config.h")
    message(FATAL_ERROR "esp32_bench_config.h missing beside the workload: "
                        "benchmarkmapmanager2.py writes it with the workload.")
endif()

include(${CC_REPO}/general/esp32-idf-common.cmake)

set(_cc_bench
    ${CC_REPO}/benchmark/benchmarkmapmanager2/stage2/main.cpp
    $ENV{CC_WORKLOAD_CPP}
    ${CC_REPO}/test/testingmapmanagement/Stubs.cpp
    ${CC_REPO}/server/base/MapManagement/MapVisibilityAlgorithm.cpp
    ${CC_REPO}/server/base/MapManagement/MapVisibilityAlgorithmCpu.cpp
    ${CC_REPO}/server/base/MapManagement/MapVisibilityAlgorithmBalanced.cpp
    ${CC_REPO}/server/base/MapManagement/MapVisibilityAlgorithmNetwork.cpp
    ${CC_REPO}/server/base/MapServer.cpp
)

idf_component_register(
    SRCS "app_main.cpp"
         ${_cc_bench} ${CC_ESP32_GENERAL_SRCS} ${CC_ESP32_VENDOR_SRCS}
    INCLUDE_DIRS "." "${CC_REPO}" "${_cc_workload_dir}"
                 "${CC_REPO}/benchmark/benchmarkmapmanager2/stage2"
                 "${CC_REPO}/test/testingmapmanagement"
                 ${CC_ESP32_VENDOR_INCLUDE_DIRS}
    REQUIRES esp_timer esp_app_format app_update
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE
    CATCHCHALLENGER_TESTING
    CATCHCHALLENGER_HARDENED
    CATCHCHALLENGER_BENCHMARK
    CATCHCHALLENGER_DB_FILE
    CATCHCHALLENGER_NOXML
    CC_TARGET_ESP32
)

cc_esp32_common_component_flags(${COMPONENT_LIB})

# No exceptions, no RTTI (CatchChallenger uses neither; sdkconfig.defaults
# turns both off): app_main checks the heap before each count instead.
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=format)
