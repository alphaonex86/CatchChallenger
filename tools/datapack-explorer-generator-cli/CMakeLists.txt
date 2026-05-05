# datapack-explorer-generator — generate static HTML pages summarising
# the datapack content. Self-contained, one binary. Qt6 Core+Xml+Gui.

cmake_minimum_required(VERSION 3.20)
project(datapack_explorer_generator LANGUAGES C CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../general/CCCommon.cmake)

find_package(Qt6 COMPONENTS Core Xml Gui REQUIRED)

include(${CC_REPO_ROOT}/general/general.cmake)
include(${CC_REPO_ROOT}/client/libqtcatchchallenger/libtiled.cmake)

add_executable(datapack-explorer-generator
    main.cpp
    Helper.cpp
    MapStore.cpp
    Generator.cpp
    GeneratorItems.cpp
    GeneratorMonsters.cpp
    GeneratorPlants.cpp
    GeneratorSkills.cpp
    GeneratorTypes.cpp
    GeneratorBuffs.cpp
    GeneratorCrafting.cpp
    GeneratorIndustries.cpp
    GeneratorMaps.cpp
    GeneratorQuests.cpp
    GeneratorIndex.cpp
    GeneratorStart.cpp
    GeneratorTree.cpp
    ${CC_REPO_ROOT}/client/libcatchchallenger/DatapackChecksum.cpp
    ${CC_REPO_ROOT}/client/libcatchchallenger/DatapackClientLoader.cpp
    ${CC_REPO_ROOT}/client/libqtcatchchallenger/Language.cpp
    ${CC_REPO_ROOT}/client/libqtcatchchallenger/QtDatapackClientLoader.cpp
    ${CC_REPO_ROOT}/client/libqtcatchchallenger/QtDatapackClientLoaderThread.cpp
    ${CC_REPO_ROOT}/client/libqtcatchchallenger/Settings.cpp
)

target_compile_definitions(datapack-explorer-generator PRIVATE
    CATCHCHALLENGER_NO_WEBSOCKET
    CATCHCHALLENGER_ONLYMAPRENDER
    NOTHREADS
    CATCHCHALLENGER_CLIENT
    CATCHCHALLENGER_NOAUDIO
    SRC_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(datapack-explorer-generator PRIVATE
    ${CC_REPO_ROOT}/client/libcatchchallenger
    ${CC_REPO_ROOT}/client/libqtcatchchallenger
)

set_target_properties(datapack-explorer-generator PROPERTIES AUTOMOC ON)

target_link_libraries(datapack-explorer-generator PRIVATE
    catchchallenger_general
    catchchallenger_tiled
    Qt6::Core
    Qt6::Xml
    Qt6::Gui
)

find_program(MOLD_EXE mold)
if(MOLD_EXE)
    target_link_options(datapack-explorer-generator PRIVATE -fuse-ld=mold)
endif()
