# server/CMakeLists.txt — catchchallenger-server-gui binary.
#
# This is a self-contained CMake project: opening server/CMakeLists.txt
# in Qt Creator (or running `cmake -S server -B build/server`) configures
# and builds exactly ONE executable, catchchallenger-server-gui — the Qt
# admin server that listens on the network and ships a SQLite-by-default
# DB. No libtiled, no client libs, no other server execs. Game-server-
# alone, login, master, gateway, and the epoll game server live in their
# own sibling CMakeLists.txt files (server/epoll/, server/login/, …) and
# are NOT pulled in here.
#
# The ONE-binary-per-CMakeLists rule means each binary has its own
# private compile defines (CATCHCHALLENGER_DB_SQLITE, _CLASS_QT, _SOLO,
# _DYNAMIC_MAP_LIST). INTERFACE libs (catchchallenger_server_base,
# catchchallenger_server_qt) propagate sources but NOT defines, so the
# same library compiles correctly for every binary that links it.

cmake_minimum_required(VERSION 3.20)
project(catchchallenger_server_gui LANGUAGES C CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../general/CCCommon.cmake)

# Qt6::Widgets + Qt6::Gui for the MainWindow admin GUI; Sql for the
# SQLite backend; Network for the listening server.
find_package(Qt6 COMPONENTS Core Gui Widgets Network Sql REQUIRED)
find_package(Threads REQUIRED)

# Pull in the dependency libraries via .cmake fragment files (mirroring
# the qmake-era include of .pri fragments). Each fragment defines its
# INTERFACE / STATIC libs guarded by `if(NOT TARGET …)` so the includes
# are idempotent across binaries that share dependencies.
include(${CC_REPO_ROOT}/general/general.cmake)
include(${CC_REPO_ROOT}/server/catchchallenger-server.cmake)
include(${CC_REPO_ROOT}/server/qt/catchchallenger-server-qt.cmake)

# catchchallenger-server-gui executable.
# Standalone Qt-based admin server with a Qt Widgets GUI (MainWindow).
# Reuses NormalServer + the InternalServer/QtServer machinery from
# catchchallenger_server_qt. Auto-creates an empty SQLite DB on first
# launch via QtDatabaseSQLite::syncConnect (schema from the embedded
# :/catchchallenger-sqlite.sql resource).
#
# `--autostart` (used by testingqtserver.py) auto-clicks the GUI's
# "start server" button on the first event-loop tick so headless CI
# can boot the server without a clicker.
add_executable(catchchallenger-server-gui
    ${CMAKE_CURRENT_SOURCE_DIR}/qt/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/qt/MainWindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/qt/MainWindow.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/qt/MainWindow.ui
    # Schema for SQLite auto-init in QtDatabaseSQLite::syncConnect.
    ${CMAKE_CURRENT_SOURCE_DIR}/databases/resources-db-sqlite.qrc
)

set_target_properties(catchchallenger-server-gui PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

target_compile_definitions(catchchallenger-server-gui PRIVATE
    CATCHCHALLENGER_DB_SQLITE
    CATCHCHALLENGER_CLASS_QT
    CATCHCHALLENGER_SOLO
    CATCHCHALLENGER_DYNAMIC_MAP_LIST
)

target_link_libraries(catchchallenger-server-gui PRIVATE
    # NOTE: catchchallenger_common_flags carries `-fno-exceptions`, which
    # is fine for the headless server but breaks Qt Widgets — std::bad_alloc
    # propagation, QPushButton::clicked emission etc. all rely on
    # exceptions. Drop the common-flags link here; the Qt-default flag
    # set is correct for a Widgets-using exec.
    catchchallenger_general
    catchchallenger_server_base
    catchchallenger_server_sql
    catchchallenger_server_qt
    catchchallenger_qfakesocket
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    Threads::Threads
    ZLIB::ZLIB
    ${ZSTD_LIBRARY}
)

target_include_directories(catchchallenger-server-gui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/qt
    ${CC_REPO_ROOT}
)

# Use mold if available — same default as the qmake-driven build.
find_program(MOLD_EXE mold)
if(MOLD_EXE AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_options(catchchallenger-server-gui PRIVATE -fuse-ld=mold)
endif()
