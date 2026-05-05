# client/libqtcatchchallenger/libtiled.cmake — mirrors libtiled.pri.
#
# Embedded libtiled (Tiled map editor library) — vendored sources;
# do NOT modify upstream layout (per CLAUDE.md).
#
# Sources live in client/libqtcatchchallenger/libtiled/ (subdir) but the
# .cmake file sits next to libtiled.pri at the libqtcatchchallenger/
# level — same as the qmake-era layout.
#
# include() this fragment from a binary's CMakeLists.txt; it produces
# a static library `catchchallenger_tiled` that consumers link.

if(NOT TARGET catchchallenger_tiled)
    find_package(Qt6 COMPONENTS Core Gui REQUIRED)
    find_package(ZLIB REQUIRED)

    set(_tiled_dir ${CMAKE_CURRENT_LIST_DIR}/libtiled)
    set(_tiled_sources
        ${_tiled_dir}/compression.cpp
        ${_tiled_dir}/fileformat.cpp
        ${_tiled_dir}/filesystemwatcher.cpp
        ${_tiled_dir}/gidmapper.cpp
        ${_tiled_dir}/grouplayer.cpp
        ${_tiled_dir}/hex.cpp
        ${_tiled_dir}/hexagonalrenderer.cpp
        ${_tiled_dir}/imagecache.cpp
        ${_tiled_dir}/imagelayer.cpp
        ${_tiled_dir}/imagereference.cpp
        ${_tiled_dir}/isometricrenderer.cpp
        ${_tiled_dir}/layer.cpp
        ${_tiled_dir}/logginginterface.cpp
        ${_tiled_dir}/map.cpp
        ${_tiled_dir}/mapformat.cpp
        ${_tiled_dir}/mapobject.cpp
        ${_tiled_dir}/mapreader.cpp
        ${_tiled_dir}/maprenderer.cpp
        ${_tiled_dir}/maptovariantconverter.cpp
        ${_tiled_dir}/mapwriter.cpp
        ${_tiled_dir}/minimaprenderer.cpp
        ${_tiled_dir}/object.cpp
        ${_tiled_dir}/objectgroup.cpp
        ${_tiled_dir}/objecttemplate.cpp
        ${_tiled_dir}/objecttemplateformat.cpp
        ${_tiled_dir}/objecttypes.cpp
        ${_tiled_dir}/obliquerenderer.cpp
        ${_tiled_dir}/orthogonalrenderer.cpp
        ${_tiled_dir}/plugin.cpp
        ${_tiled_dir}/pluginmanager.cpp
        ${_tiled_dir}/properties.cpp
        ${_tiled_dir}/propertytype.cpp
        ${_tiled_dir}/savefile.cpp
        ${_tiled_dir}/staggeredrenderer.cpp
        ${_tiled_dir}/templatemanager.cpp
        ${_tiled_dir}/tile.cpp
        ${_tiled_dir}/tileanimationdriver.cpp
        ${_tiled_dir}/tiled.cpp
        ${_tiled_dir}/tilelayer.cpp
        ${_tiled_dir}/tileset.cpp
        ${_tiled_dir}/tilesetformat.cpp
        ${_tiled_dir}/tilesetmanager.cpp
        ${_tiled_dir}/tmxmapformat.cpp
        ${_tiled_dir}/varianttomapconverter.cpp
        ${_tiled_dir}/wangset.cpp
        ${_tiled_dir}/world.cpp
    )

    add_library(catchchallenger_tiled STATIC ${_tiled_sources})

    target_include_directories(catchchallenger_tiled PUBLIC
        ${_tiled_dir}
        ${CMAKE_CURRENT_LIST_DIR}
    )

    # TILED_ZSTD_SUPPORT and TILED_ZLIB are baked-in (always on).
    # TILED_LIB_DIR is referenced by pluginmanager.cpp for plugin search;
    # we never use plugins so the value is cosmetic — keep it.
    # TIXML_USE_STL: vendored libtiled's tinyxml uses std::string when set.
    # TILED_LIBRARY: tiled_global.h flips TILEDSHARED_EXPORT between
    # Q_DECL_EXPORT (when set) and Q_DECL_IMPORT (otherwise). MinGW's
    # Q_DECL_IMPORT path expands to __declspec(dllimport) and creates
    # `__imp_<symbol>` references that don't resolve from a static archive,
    # so set it PUBLIC so consumers (qtcpu800x600 / qtopengl) compile their
    # tiled-using TUs with the EXPORT path too. Q_DECL_EXPORT is harmless
    # on Linux.
    target_compile_definitions(catchchallenger_tiled PUBLIC
        TILED_ZSTD_SUPPORT
        TILED_LIB_DIR="lib"
        TIXML_USE_STL
        TILED_LIBRARY
    )

    target_link_libraries(catchchallenger_tiled PUBLIC
        Qt6::Core
        Qt6::Gui
        ZLIB::ZLIB
        ${ZSTD_LIBRARY}
    )

    set_target_properties(catchchallenger_tiled PROPERTIES
        AUTOMOC ON
    )
endif()
