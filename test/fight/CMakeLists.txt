# test/fight/CMakeLists.txt — testfightengine binary.
#
# Self-contained, one binary per CMakeLists.txt (per CLAUDE.md). Pulls
# the catchchallenger_general INTERFACE lib (which includes the fight
# engine sources via general/general.cmake) and links a tiny
# MockFightEngine subclass + main.cpp.
#
# Built by test/testingfight.py via cmake_helpers.build_project, which
# expects a single executable target name matching the binary file.

cmake_minimum_required(VERSION 3.20)
project(testfightengine LANGUAGES C CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../general/CCCommon.cmake)

include(${CC_REPO_ROOT}/general/general.cmake)

find_package(Threads REQUIRED)

add_executable(testfightengine
    main.cpp
    MockFightEngine.cpp
)

target_include_directories(testfightengine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CC_REPO_ROOT}
)

target_link_libraries(testfightengine PRIVATE
    catchchallenger_common_flags
    catchchallenger_general
    Threads::Threads
    ZLIB::ZLIB
)

# CommonFightEngine is gated by class flags; the test exercises it as
# a plain library subclass (no server / client / master class). No
# CATCHCHALLENGER_CLASS_* define is set so every protected method
# stays available. NOXML must NOT be set — parseDatapack() needs
# tinyxml2 to load the on-disk datapack.
target_compile_definitions(testfightengine PRIVATE
    CATCHCHALLENGER_EXTRA_CHECK
)

if(EXTERNALLIBZSTD)
    target_link_libraries(testfightengine PRIVATE ${ZSTD_LIBRARY})
endif()

# Linker driver: use the test-link-verified accelerator that
# CCCommon.cmake picked for THIS target (mold on native ELF,
# lld/bfd where mold has no backend, e.g. MIPS). Never hard-code
# a specific linker here — CCCommon is the single source of truth.
if(CC_LINKER)
    target_link_options(testfightengine PRIVATE -fuse-ld=${CC_LINKER})
endif()
