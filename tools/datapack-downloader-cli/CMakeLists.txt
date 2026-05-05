# datapack-downloader — minimal Qt6 client that downloads only the
# datapack from a server. Self-contained, one binary.

cmake_minimum_required(VERSION 3.20)
project(datapack_downloader LANGUAGES C CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../general/CCCommon.cmake)

find_package(Qt6 COMPONENTS Core Network Sql WebSockets Gui Widgets REQUIRED)

include(${CC_REPO_ROOT}/general/general.cmake)
include(${CC_REPO_ROOT}/client/libcatchchallenger/lib.cmake)
include(${CC_REPO_ROOT}/client/libqtcatchchallenger/libqt.cmake)

add_executable(datapack-downloader
    main.cpp
    MainWindow.cpp
)

target_compile_definitions(datapack-downloader PRIVATE
    CATCHCHALLENGER_NOAUDIO
)

set_target_properties(datapack-downloader PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

target_link_libraries(datapack-downloader PRIVATE
    catchchallenger_qt_lib
    Qt6::Core
    Qt6::Network
    Qt6::Sql
    Qt6::WebSockets
    Qt6::Gui
    Qt6::Widgets
)

find_program(MOLD_EXE mold)
if(MOLD_EXE)
    target_link_options(datapack-downloader PRIVATE -fuse-ld=mold)
endif()
