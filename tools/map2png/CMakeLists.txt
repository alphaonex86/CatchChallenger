# map2png — render TMX maps to PNG. Qt6 + libtiled + general/.
# CATCHCHALLENGER_ONLYMAPRENDER trims the protocol/datapack pipeline.

find_package(Qt6 COMPONENTS Core Gui Xml Network OpenGL Widgets REQUIRED)

add_executable(map2png
    main.cpp
    map2png.cpp
    MapDoor.cpp
    Map_client.cpp
    TriggerAnimation.cpp
    ${CMAKE_SOURCE_DIR}/client/libcatchchallenger/DatapackChecksum.cpp
    ${CMAKE_SOURCE_DIR}/client/libcatchchallenger/DatapackClientLoader.cpp
    ${CMAKE_SOURCE_DIR}/client/libqtcatchchallenger/Language.cpp
    ${CMAKE_SOURCE_DIR}/client/libqtcatchchallenger/QtDatapackClientLoader.cpp
    ${CMAKE_SOURCE_DIR}/client/libqtcatchchallenger/QtDatapackClientLoaderThread.cpp
    ${CMAKE_SOURCE_DIR}/client/libqtcatchchallenger/Settings.cpp
    resources.qrc
)

target_compile_definitions(map2png PRIVATE
    CATCHCHALLENGER_ONLYMAPRENDER
    CATCHCHALLENGER_NO_WEBSOCKET
)

target_include_directories(map2png PRIVATE
    ${CMAKE_SOURCE_DIR}/client/libcatchchallenger
    ${CMAKE_SOURCE_DIR}/client/libqtcatchchallenger
)

set_target_properties(map2png PROPERTIES
    AUTOMOC ON
    AUTORCC ON
)

target_link_libraries(map2png PRIVATE
    catchchallenger_general
    catchchallenger_tiled
    Qt6::Core
    Qt6::Gui
    Qt6::Xml
    Qt6::Network
    Qt6::OpenGL
    Qt6::Widgets
)

find_program(MOLD_EXE mold)
if(MOLD_EXE)
    target_link_options(map2png PRIVATE -fuse-ld=mold)
endif()
