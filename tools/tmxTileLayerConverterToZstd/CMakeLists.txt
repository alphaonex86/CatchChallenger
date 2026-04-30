# tmxTileLayerConverterToZstd — single-file CLI tool that converts
# Tiled .tmx tile layer compression from gzip/zlib to zstd.

find_package(Qt6 COMPONENTS Core Xml REQUIRED)
find_package(ZLIB REQUIRED)

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
