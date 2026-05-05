# filedb-converter — bidirectional binary <-> XML converter for the
# filedb database (server-side debug utility). Self-contained, one binary.
# Pure C++, no Qt — just hps + tinyXML2 from general/.

cmake_minimum_required(VERSION 3.20)
project(filedb_converter LANGUAGES C CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../../general/CCCommon.cmake)

add_executable(filedb-converter
    main.cpp
    FiledbConverter.cpp
    # tinyXML2 sources (vendored, three-file split — see general/).
    ${CC_REPO_ROOT}/general/tinyXML2/tinyxml2.cpp
    ${CC_REPO_ROOT}/general/tinyXML2/tinyxml2b.cpp
    ${CC_REPO_ROOT}/general/tinyXML2/tinyxml2c.cpp
)

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

find_program(MOLD_EXE mold)
if(MOLD_EXE)
    target_link_options(filedb-converter PRIVATE -fuse-ld=mold)
endif()
