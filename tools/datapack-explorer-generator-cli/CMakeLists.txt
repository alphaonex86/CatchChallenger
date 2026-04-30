# datapack-explorer-generator — generate static HTML pages summarising
# the datapack content. Qt6 Core+Xml+Gui (no Widgets).

find_package(Qt6 COMPONENTS Core Xml Gui REQUIRED)

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
    ${CMAKE_SOURCE_DIR}/client/libcatchchallenger/DatapackChecksum.cpp
    ${CMAKE_SOURCE_DIR}/client/libcatchchallenger/DatapackClientLoader.cpp
    ${CMAKE_SOURCE_DIR}/client/libqtcatchchallenger/Language.cpp
    ${CMAKE_SOURCE_DIR}/client/libqtcatchchallenger/QtDatapackClientLoader.cpp
    ${CMAKE_SOURCE_DIR}/client/libqtcatchchallenger/QtDatapackClientLoaderThread.cpp
    ${CMAKE_SOURCE_DIR}/client/libqtcatchchallenger/Settings.cpp
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
    ${CMAKE_SOURCE_DIR}/client/libcatchchallenger
    ${CMAKE_SOURCE_DIR}/client/libqtcatchchallenger
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
