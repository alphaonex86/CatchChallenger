# bot-bench — Qt-FREE connect/login benchmark client. Self-contained, one
# binary. NO find_package(Qt6), NO libqtcatchchallenger: the whole transport
# is POSIX sockets + select() (tools/libbot/cli/) on top of the Qt-free
# client/libcatchchallenger/ protocol code.
#
# Do NOT add Qt here. If a symbol pulls Qt in, fix the call site in
# tools/libbot/cli/ instead — linking libqtcatchchallenger would defeat the
# purpose of this binary (measuring the protocol, not the GUI toolkit).

cmake_minimum_required(VERSION 3.20)
project(bot_bench LANGUAGES C CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../general/CCCommon.cmake)

include(${CC_REPO_ROOT}/general/general.cmake)
include(${CC_REPO_ROOT}/client/libcatchchallenger/lib.cmake)

find_package(Threads REQUIRED)

add_executable(bot-bench
    main.cpp
    ../libbot/cli/CliSocket.cpp
    ../libbot/cli/CliDatapack.cpp
    ../libbot/cli/CliApiClient.cpp
    ../libbot/cli/CliApiClientNotifications.cpp
    ../libbot/cli/CliEventLoop.cpp
    # client-side latency instrumentation (--latency). The whole file expands
    # to nothing unless CATCHCHALLENGER_BENCHMARK is set below.
    ../libbot/cli/CliLatency.cpp
)

# Client-side latency benchmark instrumentation (CliLatency + --latency).
# OFF by default -> zero overhead in normal/dev builds. benchmarkclientlatency.py
# configures with -DCATCHCHALLENGER_BENCHMARK=ON.
option(CATCHCHALLENGER_BENCHMARK
       "Client-side latency instrumentation for benchmark*.py (benchmark only)"
       OFF)
if(CATCHCHALLENGER_BENCHMARK)
    target_compile_definitions(bot-bench PRIVATE CATCHCHALLENGER_BENCHMARK)
endif()

target_include_directories(bot-bench PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CC_REPO_ROOT}/tools/libbot
)

target_link_libraries(bot-bench PRIVATE
    catchchallenger_common_flags
    catchchallenger_general
    catchchallenger_client_lib
    Threads::Threads
)

# Linker driver: use the test-link-verified accelerator that
# CCCommon.cmake picked for THIS target (mold on native ELF,
# lld/bfd where mold has no backend, e.g. MIPS). Never hard-code
# a specific linker here — CCCommon is the single source of truth.
if(CC_LINKER)
    target_link_options(bot-bench PRIVATE -fuse-ld=${CC_LINKER})
endif()
