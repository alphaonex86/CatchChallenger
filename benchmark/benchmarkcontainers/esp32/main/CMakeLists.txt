# ESP-IDF "main" component of the benchmarkcontainers firmware: the shared
# measuring core (../../ContainerBench.hpp) + the host-generated datasets.

get_filename_component(CC_REPO "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)

if(NOT DEFINED ENV{CC_CONTAINERS_DATA_CPP} OR NOT EXISTS "$ENV{CC_CONTAINERS_DATA_CPP}")
    message(FATAL_ERROR
        "CC_CONTAINERS_DATA_CPP is not set or does not exist: the firmware has no data.\n"
        "Generate it with benchmark_containers --emit-esp32 (benchmarkcontainers.py does).")
endif()

include(${CC_REPO}/general/esp32-idf-common.cmake)

idf_component_register(
    SRCS "app_main.cpp" "$ENV{CC_CONTAINERS_DATA_CPP}"
    INCLUDE_DIRS "." "${CC_REPO}/benchmark/benchmarkcontainers"
    REQUIRES esp_timer
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE CC_TARGET_ESP32)

cc_esp32_common_component_flags(${COMPONENT_LIB})
