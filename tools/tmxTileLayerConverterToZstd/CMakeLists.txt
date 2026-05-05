# tmxTileLayerConverterToZstd — single-file CLI tool that converts
# Tiled .tmx tile layer compression from gzip/zlib to zstd. Self-contained,
# one binary. The only catchchallenger lib it touches is via CCCommon's
# zstd discovery (system or vendored static) — no general/, libtiled, etc.

cmake_minimum_required(VERSION 3.20)
project(tmxTileLayerConverterToZstd LANGUAGES C CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../general/CCCommon.cmake)

find_package(Qt6 COMPONENTS Core Xml REQUIRED)

add_executable(tmxTileLayerConverterToZstd main.cpp)

target_link_libraries(tmxTileLayerConverterToZstd PRIVATE
    Qt6::Core
    Qt6::Xml
    ZLIB::ZLIB
    ${ZSTD_LIBRARY}
)

set_target_properties(tmxTileLayerConverterToZstd PROPERTIES
    AUTOMOC ON
)

find_program(MOLD_EXE mold)
if(MOLD_EXE)
    target_link_options(tmxTileLayerConverterToZstd PRIVATE -fuse-ld=mold)
endif()
