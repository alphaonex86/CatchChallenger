# filedb-converter — bidirectional binary <-> XML converter for the
# filedb database (server-side debug utility). Self-contained, one binary.
# Pure C++, no Qt — just hps + tinyXML2 from general/.

cmake_minimum_required(VERSION 3.20)
project(filedb_converter LANGUAGES C CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../../general/CCCommon.cmake)

add_executable(filedb-converter
    main.cpp
    FiledbConverter.cpp
)

# tinyXML2: pull from system when CCCommon.cmake detected it (>=v9), else
# fall back to the vendored 3-file split shipped in general/tinyXML2/.
# The .hpp shim and CC_TINYXML2_SYSTEM macro come from CCCommon.cmake.
if(CC_USE_SYSTEM_TINYXML2)
    target_link_libraries(filedb-converter PRIVATE ${TINYXML2_LIBRARY})
    target_include_directories(filedb-converter PRIVATE
        ${TINYXML2_INCLUDE_DIR}
        ${CC_TINYXML2_SHIM_DIR})
else()
    target_sources(filedb-converter PRIVATE
        ${CC_REPO_ROOT}/general/tinyXML2/tinyxml2.cpp
    )
endif()

target_compile_definitions(filedb-converter PRIVATE
    CATCHCHALLENGER_DB_FILE
    CATCHCHALLENGER_CACHE_HPS
)

target_include_directories(filedb-converter PRIVATE
    ${CC_REPO_ROOT}/general
    ${CC_REPO_ROOT}/general/hps
    ${CC_REPO_ROOT}/general/tinyXML2
)

target_link_libraries(filedb-converter PRIVATE
    catchchallenger_common_flags
)

# Linker driver: use the test-link-verified accelerator that
# CCCommon.cmake picked for THIS target (mold on native ELF,
# lld/bfd where mold has no backend, e.g. MIPS). Never hard-code
# a specific linker here — CCCommon is the single source of truth.
if(CC_LINKER)
    target_link_options(filedb-converter PRIVATE -fuse-ld=${CC_LINKER})
endif()
