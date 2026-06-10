# tuxemon2catchchallenger — convert the Tuxemon game data (YAML db + sprites)
# into a full CatchChallenger datapack (types/skills/buffs/monsters/items XML +
# monster sprites).  Self-contained, one binary.  Reads YAML with the system
# yaml-cpp (system-lib-first; no vendored copy), writes the datapack XML by hand
# and extracts sprites with QImage (Qt6 Gui).

cmake_minimum_required(VERSION 3.20)
project(tuxemon2catchchallenger LANGUAGES C CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../general/CCCommon.cmake)

find_package(Qt6 COMPONENTS Core Gui REQUIRED)
find_package(ZLIB REQUIRED)

# yaml-cpp: prefer the system package (provides the imported target yaml-cpp).
# Fall back to a bare find_library so the build still works on hosts that ship
# the .so without the CMake config.
find_package(yaml-cpp QUIET)
if(NOT TARGET yaml-cpp)
    find_library(YAMLCPP_LIBRARY NAMES yaml-cpp)
    find_path(YAMLCPP_INCLUDE_DIR NAMES yaml-cpp/yaml.h)
    if(NOT YAMLCPP_LIBRARY OR NOT YAMLCPP_INCLUDE_DIR)
        message(FATAL_ERROR "yaml-cpp not found; install libyaml-cpp-dev (system lib, never auto-installed).")
    endif()
    add_library(yaml-cpp INTERFACE IMPORTED)
    set_target_properties(yaml-cpp PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${YAMLCPP_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${YAMLCPP_LIBRARY}")
endif()

# tinyxml2: reuse the vendored copy to parse the Tuxemon .tmx maps (self-contained,
# no link dependency).
set(CC_REPO_ROOT_LOCAL ${CMAKE_CURRENT_LIST_DIR}/../..)

add_executable(tuxemon2catchchallenger
    main.cpp
    TuxemonDb.cpp
    Localization.cpp
    SpriteExtractor.cpp
    DatapackWriter.cpp
    MapConverter.cpp
    ${CC_REPO_ROOT_LOCAL}/general/tinyXML2/tinyxml2.cpp
)

target_include_directories(tuxemon2catchchallenger PRIVATE
    ${CC_REPO_ROOT_LOCAL}/general/tinyXML2
)

target_link_libraries(tuxemon2catchchallenger PRIVATE
    Qt6::Core
    Qt6::Gui
    yaml-cpp
    ZLIB::ZLIB
)

# Linker driver: use the test-link-verified accelerator CCCommon.cmake picked.
if(CC_LINKER)
    target_link_options(tuxemon2catchchallenger PRIVATE -fuse-ld=${CC_LINKER})
endif()
