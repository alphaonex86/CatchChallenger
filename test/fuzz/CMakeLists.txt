# test/fuzz — OPTIONAL coverage-guided fuzz harness for the protocol parser.
# Self-contained, one binary (per the repo's one-binary-per-CMakeLists rule).
# NOT part of any default build: configure it explicitly, e.g.
#   cmake -S test/fuzz -B build/fuzz -DCMAKE_CXX_COMPILER=clang++
#   cmake --build build/fuzz
#   build/fuzz/fuzz_protocol_parser -dict=test/fuzz/protocol.dict <corpusdir>
# A vanilla VPS build never touches this. Requires a clang with libFuzzer +
# sanitizers; if the toolchain lacks -fsanitize=fuzzer the configure fails fast
# (this target is opt-in only).
cmake_minimum_required(VERSION 3.20)
project(catchchallenger-fuzz-protocol LANGUAGES C CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../general/CCCommon.cmake)
include(${CC_REPO_ROOT}/general/general.cmake)

add_executable(fuzz_protocol_parser fuzz_protocol_parser.cpp)
target_link_libraries(fuzz_protocol_parser PRIVATE catchchallenger_general_minimal)

# Match the audited cli server's packet-table flavour EXACTLY (so the fuzzed
# code path is the real one). zstd is linked automatically: general.cmake
# propagates ${ZSTD_LIBRARY} to every catchchallenger_general_minimal consumer.
# IMPORTANT: do NOT define CATCHCHALLENGER_HARDENED here (its abort() tripwires
# would end the fuzz loop on the first malformed byte and re-route dispatch).
target_compile_definitions(fuzz_protocol_parser PRIVATE
    CATCHCHALLENGER_SERVER
    CATCHCHALLENGER_CLASS_ALLINONESERVER
    CATCHCHALLENGER_DB_INTERNAL_VARS
    CATCHCHALLENGER_DB_FILE)

# libFuzzer + AddressSanitizer + UBSan. UBSan aborts on first UB (no recover).
set(_fuzz_flags -g -O1 -fsanitize=fuzzer,address,undefined -fno-sanitize-recover=undefined)
target_compile_options(fuzz_protocol_parser PRIVATE ${_fuzz_flags})
target_link_options(fuzz_protocol_parser PRIVATE -fsanitize=fuzzer,address,undefined)
