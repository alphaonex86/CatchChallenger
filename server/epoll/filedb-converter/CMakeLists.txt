# filedb-converter — bidirectional binary <-> XML converter for the
# filedb database (server-side debug utility). Mirrors the .pro:
# pure C++, no Qt, just hps + tinyXML2.

add_executable(filedb-converter
    main.cpp
    FiledbConverter.cpp
    # tinyXML2 sources (vendored, three-file split — see general/).
    ${CMAKE_SOURCE_DIR}/general/tinyXML2/tinyxml2.cpp
    ${CMAKE_SOURCE_DIR}/general/tinyXML2/tinyxml2b.cpp
    ${CMAKE_SOURCE_DIR}/general/tinyXML2/tinyxml2c.cpp
)

target_compile_definitions(filedb-converter PRIVATE
    CATCHCHALLENGER_DB_FILE
    CATCHCHALLENGER_CACHE_HPS
)

target_include_directories(filedb-converter PRIVATE
    ${CMAKE_SOURCE_DIR}/general
    ${CMAKE_SOURCE_DIR}/general/hps
    ${CMAKE_SOURCE_DIR}/general/tinyXML2
)

target_link_libraries(filedb-converter PRIVATE
    catchchallenger_common_flags
)

find_program(MOLD_EXE mold)
if(MOLD_EXE)
    target_link_options(filedb-converter PRIVATE -fuse-ld=mold)
endif()
