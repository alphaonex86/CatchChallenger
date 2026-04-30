# Phase 3 entry point. Each subproject is opt-in via top-level
# options so a server-only checkout doesn't pull Qt6.

option(CATCHCHALLENGER_BUILD_CLIENT_LIB
       "Build the non-Qt client lib (libcatchchallenger)" OFF)
option(CATCHCHALLENGER_BUILD_CLIENT_QT_LIB
       "Build the Qt-based client middleware (libqtcatchchallenger)" OFF)
option(CATCHCHALLENGER_BUILD_CLIENT_QTOPENGL
       "Build the qtopengl Qt6 OpenGL client executable" OFF)
option(CATCHCHALLENGER_BUILD_CLIENT_QTCPU800X600
       "Build the qtcpu800x600 fixed-widget client (legacy)" OFF)

# Pull-in dependencies automatically: building the qtopengl exec
# requires both libs.
if(CATCHCHALLENGER_BUILD_CLIENT_QTOPENGL OR CATCHCHALLENGER_BUILD_CLIENT_QTCPU800X600)
    set(CATCHCHALLENGER_BUILD_CLIENT_QT_LIB ON)
endif()
# When any client target is built with single-player, the qt_lib must
# also include the in-process server pieces (SoloDatabaseInit etc.).
if(CATCHCHALLENGER_BUILD_QTOPENGL_SINGLEPLAYER OR CATCHCHALLENGER_BUILD_QTCPU800X600_SINGLEPLAYER)
    set(CATCHCHALLENGER_SOLO ON CACHE BOOL "" FORCE)
endif()
if(CATCHCHALLENGER_BUILD_CLIENT_QT_LIB)
    set(CATCHCHALLENGER_BUILD_CLIENT_LIB ON)
endif()

if(CATCHCHALLENGER_BUILD_CLIENT_LIB)
    add_subdirectory(libcatchchallenger)
endif()
if(CATCHCHALLENGER_BUILD_CLIENT_QT_LIB)
    add_subdirectory(libqtcatchchallenger)
endif()
if(CATCHCHALLENGER_BUILD_CLIENT_QTOPENGL)
    add_subdirectory(qtopengl)
endif()
if(CATCHCHALLENGER_BUILD_CLIENT_QTCPU800X600)
    add_subdirectory(qtcpu800x600)
endif()
