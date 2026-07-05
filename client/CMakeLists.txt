# client/CMakeLists.txt — catchchallenger (qtopengl) binary.
#
# Self-contained: opening this file in Qt Creator (or
# `cmake -S client -B build/client`) configures and builds the qtopengl
# client. Sources live in client/qtopengl/, but the CMakeLists.txt that
# produces the binary is HERE at /client/ — that matches the
# one-binary-per-CMakeLists rule (the qtcpu800x600 binary lives at
# /client/qtcpu800x600/CMakeLists.txt).
#
# Pulls in:
#   general/                              → catchchallenger_general
#   client/libcatchchallenger/            → catchchallenger_client_lib
#   client/libqtcatchchallenger/          → catchchallenger_qt_lib
#                                           (libtiled / libogg / libopus
#                                           / libopusfile cascade from
#                                           there as needed)
# Single-player (CATCHCHALLENGER_BUILD_QTOPENGL_SINGLEPLAYER=ON) also
# embeds the in-process Qt server:
#   server/base/                          → catchchallenger_server_base /
#                                           catchchallenger_server_sql
#   server/qt/                            → catchchallenger_server_qt /
#                                           catchchallenger_qfakesocket

cmake_minimum_required(VERSION 3.20)
project(catchchallenger LANGUAGES C CXX)

# Explicit LANGUAGES list omits RC, so CMake would silently skip
# resources-windows.rc and ship an icon-less .exe. Enable RC on Windows
# targets (MXE toolchain supplies windres) so icon.ico is embedded.
if(WIN32)
    enable_language(RC)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/../general/CCCommon.cmake)

find_package(Qt6 COMPONENTS Core Gui Network Widgets OpenGL OpenGLWidgets Xml REQUIRED)

option(CATCHCHALLENGER_BUILD_QTOPENGL_WEBSOCKETS
       "Link Qt6::WebSockets into qtopengl (matches default qmake build)"
       ON)
option(CATCHCHALLENGER_BUILD_QTOPENGL_SINGLEPLAYER
       "Embed the in-process Qt server (CATCHCHALLENGER_CLASS_QT + CATCHCHALLENGER_DB_SQLITE)"
       ON)
option(CATCHCHALLENGER_NOAUDIO
       "Disable QtMultimedia audio (Audio.cpp / Audio.hpp dropped)"
       OFF)

# Pull SOLO into the libqtcatchchallenger lib build before we add it as a
# subdirectory — libqtcatchchallenger.cmakefile reads CATCHCHALLENGER_SOLO
# at configure time and includes SoloDatabaseInit.cpp + Audio.cpp behaviour
# accordingly. Allow either spelling — `-DCATCHCHALLENGER_SOLO=ON` and
# `-DCATCHCHALLENGER_BUILD_QTOPENGL_SINGLEPLAYER=ON` are equivalent.
if(CATCHCHALLENGER_SOLO)
    set(CATCHCHALLENGER_BUILD_QTOPENGL_SINGLEPLAYER ON CACHE BOOL "" FORCE)
endif()
if(CATCHCHALLENGER_BUILD_QTOPENGL_SINGLEPLAYER)
    set(CATCHCHALLENGER_SOLO ON CACHE BOOL "" FORCE)
endif()

# When the Qt6::WebSockets module isn't installed (Android Qt-for-Android
# default), libqtcatchchallenger forces CATCHCHALLENGER_NO_WEBSOCKET ON;
# turn the qtopengl-side toggle OFF in that case so we don't try to
# find_package(WebSockets REQUIRED) — find_package(... QUIET) is enough.
find_package(Qt6 COMPONENTS WebSockets QUIET)
if(NOT Qt6WebSockets_FOUND)
    set(CATCHCHALLENGER_BUILD_QTOPENGL_WEBSOCKETS OFF CACHE BOOL "" FORCE)
endif()
if(CATCHCHALLENGER_BUILD_QTOPENGL_WEBSOCKETS)
    find_package(Qt6 COMPONENTS WebSockets REQUIRED)
endif()
if(CATCHCHALLENGER_BUILD_QTOPENGL_SINGLEPLAYER)
    find_package(Qt6 COMPONENTS Sql REQUIRED)
endif()

# Library .cmake fragments — mirrors the qmake-era include() of .pri
# files. Each fragment defines its INTERFACE / STATIC libs idempotently
# (guarded by `if(NOT TARGET …)`).
include(${CC_REPO_ROOT}/general/general.cmake)
include(${CC_REPO_ROOT}/client/libcatchchallenger/lib.cmake)
include(${CC_REPO_ROOT}/client/libqtcatchchallenger/libqt.cmake)
if(CATCHCHALLENGER_BUILD_QTOPENGL_SINGLEPLAYER)
    # Single-player needs the in-process Qt server. server-qt.cmake must
    # be include()'d BEFORE libqt.cmake's SOLO branch links qfakesocket
    # to qt_lib — but libqt.cmake guards that link with TARGET checks, so
    # the order is OK either way.
    include(${CC_REPO_ROOT}/server/catchchallenger-server.cmake)
    include(${CC_REPO_ROOT}/server/qt/catchchallenger-server-qt.cmake)
endif()

set(_qtopengl_dir ${CMAKE_CURRENT_SOURCE_DIR}/qtopengl)
set(_qtopengl_sources
    # client.pri SOURCES (UI + game flow)
    ${_qtopengl_dir}/CCGraphicsTextItem.cpp
    ${_qtopengl_dir}/CCSliderH.cpp
    ${_qtopengl_dir}/MultiItem.cpp
    ${_qtopengl_dir}/ConnexionInfo.cpp
    ${_qtopengl_dir}/CCScrollZone.cpp
    ${_qtopengl_dir}/LineEdit.cpp
    ${_qtopengl_dir}/ComboBox.cpp
    ${_qtopengl_dir}/SpinBox.cpp
    ${_qtopengl_dir}/above/AddOrEditServer.cpp
    ${_qtopengl_dir}/above/OptionsDialog.cpp
    ${_qtopengl_dir}/above/Login.cpp
    ${_qtopengl_dir}/CheckBox.cpp
    ${_qtopengl_dir}/foreground/Battle.cpp
    ${_qtopengl_dir}/foreground/MainScreen.cpp
    ${_qtopengl_dir}/foreground/Multi.cpp
    ${_qtopengl_dir}/foreground/CharacterList.cpp
    ${_qtopengl_dir}/foreground/SubServer.cpp
    ${_qtopengl_dir}/foreground/QGraphicsTextItemOutline.cpp
    ${_qtopengl_dir}/above/AddCharacter.cpp
    ${_qtopengl_dir}/above/NewGame.cpp
    ${_qtopengl_dir}/FacilityLibClient.cpp
    ${_qtopengl_dir}/background/CCMap.cpp
    ${_qtopengl_dir}/foreground/OverMap.cpp
    ${_qtopengl_dir}/CustomText.cpp
    ${_qtopengl_dir}/foreground/widgets/MapMonsterPreview.cpp
    ${_qtopengl_dir}/ImagesStrechMiddle.cpp
    ${_qtopengl_dir}/MyApplication.cpp
    ${_qtopengl_dir}/above/DebugDialog.cpp
    ${_qtopengl_dir}/foreground/OverMapLogic.cpp
    ${_qtopengl_dir}/foreground/OverMapLogicTrade.cpp
    ${_qtopengl_dir}/foreground/OverMapLogicShop.cpp
    ${_qtopengl_dir}/foreground/OverMapLogicBot.cpp
    ${_qtopengl_dir}/foreground/OverMapLogicFactory.cpp
    ${_qtopengl_dir}/foreground/OverMapLogicClan.cpp
    ${_qtopengl_dir}/above/inventory/Inventory.cpp
    ${_qtopengl_dir}/above/inventory/Plant.cpp
    ${_qtopengl_dir}/above/inventory/Crafting.cpp
    ${_qtopengl_dir}/above/Animation.cpp
    ${_qtopengl_dir}/above/Encyclopedia.cpp
    ${_qtopengl_dir}/above/Shop.cpp
    ${_qtopengl_dir}/above/Factory.cpp
    ${_qtopengl_dir}/above/Trade.cpp
    ${_qtopengl_dir}/above/Warehouse.cpp
    ${_qtopengl_dir}/above/player/Player.cpp
    ${_qtopengl_dir}/above/player/MonsterSelect.cpp
    ${_qtopengl_dir}/above/player/Reputations.cpp
    ${_qtopengl_dir}/above/player/Quests.cpp
    ${_qtopengl_dir}/above/player/FinishedQuests.cpp
    ${_qtopengl_dir}/QGraphicsPixmapItemClick.cpp
    ${_qtopengl_dir}/above/MonsterDetails.cpp
    ${_qtopengl_dir}/GameLoader.cpp
    ${_qtopengl_dir}/ChatParsing.cpp
    ${_qtopengl_dir}/GameLoaderThread.cpp
    ${_qtopengl_dir}/ProgressBarPixel.cpp
    ${_qtopengl_dir}/main.cpp
    ${_qtopengl_dir}/CCTitle.cpp
    ${_qtopengl_dir}/ConnexionManager.cpp
    ${_qtopengl_dir}/foreground/LoadingScreen.cpp
    ${_qtopengl_dir}/CustomButton.cpp
    ${_qtopengl_dir}/ScreenTransition.cpp
    ${_qtopengl_dir}/background/CCBackground.cpp
    ${_qtopengl_dir}/CCprogressbar.cpp
    ${_qtopengl_dir}/ScreenInput.cpp
    ${_qtopengl_dir}/AudioGL.cpp
)

set(_qtopengl_resources
    ${_qtopengl_dir}/resources/client-resources.qrc
)
if(CATCHCHALLENGER_BUILD_QTOPENGL_SINGLEPLAYER)
    # SoloDatabaseInit reads `:/catchchallenger-sqlite.sql` to bootstrap
    # the savegame DB. The qrc lives next to the canonical schema in
    # server/databases/ so server + client embed the same file.
    list(APPEND _qtopengl_resources
        ${CC_REPO_ROOT}/server/databases/resources-db-sqlite.qrc
    )
endif()

# Windows-only resource script: embeds icon.ico into the .exe via
# windres. CMake handles the .rc -> .res step automatically once the
# source is added to the target (RC language enabled above).
#
# The .rc references "icon.ico" relatively. SINGLEPLAYER puts
# git/server on the target's include path, and server/icon.ico would
# otherwise shadow it (windres searches -I dirs for resource data
# files) — every client would embed the server icon. Scoping the .rc's
# own resources/ dir as a source-level RC include dir makes CMake emit
# it as the FIRST -I, so the intended qtopengl icon wins.
if(WIN32)
    list(APPEND _qtopengl_sources
        ${_qtopengl_dir}/resources/resources-windows.rc)
    set_source_files_properties(
        ${_qtopengl_dir}/resources/resources-windows.rc
        PROPERTIES INCLUDE_DIRECTORIES ${_qtopengl_dir}/resources)
endif()

# ── translation: compile the tracked .ts into the .qm embedded by the qrc ──
# client-resources.qrc embeds languages/en/translation.qm, but a .qm is a
# GENERATED artifact (.gitignore: *.qm) so a fresh `git clone` ships only
# translation.ts. Without producing the .qm, RCC aborts the build with
# "translation.qm ... missing and no known rule to make it". The qmake
# build compiled it via `CONFIG += lrelease`; the CMake port dropped that.
# The .qm must land NEXT TO the .ts because the qrc references it by that
# relative path (RCC resolves <file> paths against the qrc's source dir).
set(_qtopengl_ts ${_qtopengl_dir}/resources/languages/en/translation.ts)
set(_qtopengl_qm ${_qtopengl_dir}/resources/languages/en/translation.qm)
# LinguistTools (lrelease) is OPTIONAL: keep the "vanilla VPS, no extra
# packages" promise. When present we compile the real translation and
# rebuild it whenever the .ts changes; when absent we drop an empty
# placeholder .qm so RCC still succeeds (QTranslator::load() then just
# returns false at runtime → app runs untranslated, same degraded
# behaviour as the old committed 1-byte placeholder).
find_package(Qt6 COMPONENTS LinguistTools QUIET)
# Pick an lrelease that can actually RUN on the build host. A .qm is platform-
# INDEPENDENT, so the host tool is the correct one for EVERY target. The
# imported Qt6::lrelease may be a TARGET-arch binary the host cannot exec: on
# osxcross it is a macOS Mach-O ("Exec format error", which aborted the qtopengl
# build) AND that toolchain does not even set CMAKE_CROSSCOMPILING, so we cannot
# gate on it — instead PROBE executability with `-version`. Order: (1) the
# imported Qt6::lrelease if it runs here (covers native, incl. a non-PATH Qt
# prefix); (2) a host lrelease from QT_HOST_PATH / PATH that runs (covers MXE +
# Android cross-builds, whose host Qt is on the build machine); (3) an empty
# placeholder .qm so RCC still succeeds — app runs untranslated, the same
# degraded behaviour as the committed placeholder (osxcross has no host lrelease).
set(_lrelease_exe "")
if(TARGET Qt6::lrelease)
    get_target_property(_lr_loc Qt6::lrelease LOCATION)
    if(_lr_loc)
        execute_process(COMMAND ${_lr_loc} -version
                        RESULT_VARIABLE _lr_rc OUTPUT_QUIET ERROR_QUIET)
        if(_lr_rc EQUAL 0)
            set(_lrelease_exe ${_lr_loc})
        endif()
    endif()
endif()
if(NOT _lrelease_exe)
    find_program(_host_lrelease NAMES lrelease-qt6 lrelease lrelease6
                 HINTS ${QT_HOST_PATH}/bin ${QT_HOST_PATH}/libexec
                 NO_CMAKE_FIND_ROOT_PATH)
    if(_host_lrelease)
        execute_process(COMMAND ${_host_lrelease} -version
                        RESULT_VARIABLE _hr_rc OUTPUT_QUIET ERROR_QUIET)
        if(_hr_rc EQUAL 0)
            set(_lrelease_exe ${_host_lrelease})
        endif()
    endif()
endif()
if(_lrelease_exe)
    add_custom_command(
        OUTPUT ${_qtopengl_qm}
        COMMAND ${_lrelease_exe} -silent ${_qtopengl_ts} -qm ${_qtopengl_qm}
        DEPENDS ${_qtopengl_ts}
        COMMENT "lrelease translation.ts -> translation.qm"
        VERBATIM)
    set_source_files_properties(${_qtopengl_qm} PROPERTIES GENERATED TRUE)
else()
    if(NOT EXISTS ${_qtopengl_qm})
        file(WRITE ${_qtopengl_qm} "")
    endif()
endif()

# Use Qt's executable wrapper when available — on Android it produces a
# shared library + a -deployment-settings.json suitable for androiddeployqt.
# On host platforms it behaves the same as add_executable.
if(COMMAND qt_add_executable)
    qt_add_executable(catchchallenger ${_qtopengl_sources} ${_qtopengl_resources})
else()
    add_executable(catchchallenger ${_qtopengl_sources} ${_qtopengl_resources})
endif()

# Build the generated .qm before AUTORCC runs rcc on the qrc that embeds
# it. AUTOGEN_TARGET_DEPENDS is the documented hook for "the autogen step
# consumes this generated file" — with the GENERATED property set above,
# CMake wires the custom command into the <target>_autogen dependency so
# ninja produces translation.qm first instead of failing on it.
if(_lrelease_exe)
    set_property(TARGET catchchallenger APPEND PROPERTY
                 AUTOGEN_TARGET_DEPENDS ${_qtopengl_qm})
endif()

# WIN32_EXECUTABLE → linker `-mwindows`, so launching the .exe does
# NOT open a console window alongside the Qt GUI.
if(WIN32)
    set_target_properties(catchchallenger PROPERTIES
        WIN32_EXECUTABLE TRUE)
endif()

target_compile_definitions(catchchallenger PRIVATE
    CATCHCHALLENGER_CLIENT
    CATCHCHALLENGER_CLASS_QT
    CATCHCHALLENGER_CLASS_CLIENT
    CATCHCHALLENGER_MULTI
    CATCHCHALLENGER_DB_SQLITE
)
if(CATCHCHALLENGER_CACHE_HPS)
    target_compile_definitions(catchchallenger PRIVATE CATCHCHALLENGER_CACHE_HPS)
endif()
if(CATCHCHALLENGER_BUILD_QTOPENGL_SINGLEPLAYER)
    target_compile_definitions(catchchallenger PRIVATE CATCHCHALLENGER_SOLO)
endif()
if(CATCHCHALLENGER_NOAUDIO)
    target_compile_definitions(catchchallenger PRIVATE CATCHCHALLENGER_NOAUDIO)
endif()
if(NOT CATCHCHALLENGER_BUILD_QTOPENGL_WEBSOCKETS)
    target_compile_definitions(catchchallenger PRIVATE CATCHCHALLENGER_NO_WEBSOCKET)
endif()

set_target_properties(catchchallenger PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

target_include_directories(catchchallenger PRIVATE
    ${_qtopengl_dir}
)

# NOTE: catchchallenger_common_flags is NOT linked here. That target
# carries `-fno-exceptions` (suitable for the epoll server / general lib
# consumers), but Qt UI code relies on exceptions (MyApplication.cpp's
# try/catch around bad_alloc, qFatal-with-exception flow, etc.). Use the
# Qt6 default flags instead.
target_link_libraries(catchchallenger PRIVATE
    catchchallenger_qt_lib
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    Qt6::Xml
)
if(CATCHCHALLENGER_BUILD_QTOPENGL_WEBSOCKETS)
    target_link_libraries(catchchallenger PRIVATE Qt6::WebSockets)
endif()
if(CATCHCHALLENGER_BUILD_QTOPENGL_SINGLEPLAYER)
    target_link_libraries(catchchallenger PRIVATE
        catchchallenger_general
        catchchallenger_server_base
        catchchallenger_server_sql
        catchchallenger_server_qt
        catchchallenger_qfakesocket
        Qt6::Sql
    )
endif()

# Linker driver: use the test-link-verified accelerator that
# CCCommon.cmake picked for THIS target (mold on native ELF,
# lld/bfd where mold has no backend, e.g. MIPS). Never hard-code
# a specific linker here — CCCommon is the single source of truth.
if(CC_LINKER)
    target_link_options(catchchallenger PRIVATE -fuse-ld=${CC_LINKER})
endif()

# Android-only: install the qt_add_executable-produced shared library to
# the layout androiddeployqt expects.
if(ANDROID)
    install(TARGETS catchchallenger
        LIBRARY DESTINATION "libs/${CMAKE_ANDROID_ARCH_ABI}"
        RUNTIME DESTINATION "libs/${CMAKE_ANDROID_ARCH_ABI}"
    )

    # Branding + behaviour for the .apk: a custom package source dir
    # carrying our AndroidManifest.xml (label "CatchChallenger", landscape
    # lock) and res/drawable-*/icon.png. androiddeployqt overlays this on
    # top of the stock Qt template, so anything we don't ship here
    # (qtprovider_paths.xml, libs.xml, gradle bits) still comes from Qt.
    set_property(TARGET catchchallenger PROPERTY
        QT_ANDROID_PACKAGE_SOURCE_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/qtopengl/resources/android-package-source")
    # Belt-and-suspenders: even though the manifest hard-codes the label,
    # set the app-name property so any gradle/strings.xml path agrees.
    set_property(TARGET catchchallenger PROPERTY QT_ANDROID_APP_NAME "CatchChallenger")

    # Optional OpenSSL runtime libs for Qt-for-Android.  Qt ships only the
    # openssl TLS *backend plugin* (libplugins_tls_qopensslbackend) — not
    # libssl_3.so / libcrypto_3.so — so without these every QSslSocket /
    # https:// (server-list, updater, wss) fails with
    # "TLS initialization failed".  We bundle KDAB's prebuilt arm64 libs
    # via QT_ANDROID_EXTRA_LIBS when the helper is available, and fall back
    # silently to a TLS-less build otherwise so the tree still configures
    # out-of-the-box on a host without it (same policy as ninja/mold/etc.).
    set(CATCHCHALLENGER_ANDROID_OPENSSL_DIR "" CACHE PATH
        "Path to a KDAB android_openssl checkout (contains android_openssl.cmake)")
    set(_cc_openssl_dir "${CATCHCHALLENGER_ANDROID_OPENSSL_DIR}")
    if(_cc_openssl_dir STREQUAL "" AND DEFINED ENV{CC_ANDROID_OPENSSL_DIR})
        set(_cc_openssl_dir "$ENV{CC_ANDROID_OPENSSL_DIR}")
    endif()
    # Auto-pick a KDAB android_openssl checkout from well-known host
    # locations (same "use it when present, fall back silently"
    # policy as ninja/mold/MXE) so the APK ships TLS out-of-the-box
    # without every builder having to pass -DCATCHCHALLENGER_ANDROID_OPENSSL_DIR.
    if(_cc_openssl_dir STREQUAL "")
        foreach(_cc_openssl_guess
                "/mnt/data/perso/progs/CatchChallenger-android/android_openssl"
                "$ENV{HOME}/android_openssl"
                "$ENV{HOME}/Android/android_openssl")
            if(EXISTS "${_cc_openssl_guess}/android_openssl.cmake")
                set(_cc_openssl_dir "${_cc_openssl_guess}")
                break()
            endif()
        endforeach()
    endif()
    if(NOT _cc_openssl_dir STREQUAL ""
       AND EXISTS "${_cc_openssl_dir}/android_openssl.cmake")
        include("${_cc_openssl_dir}/android_openssl.cmake")
        add_android_openssl_libraries(catchchallenger)
        message(STATUS "CatchChallenger: bundling Android OpenSSL from ${_cc_openssl_dir}")
    else()
        message(STATUS "CatchChallenger: no Android OpenSSL dir — APK built "
                       "without TLS (set CATCHCHALLENGER_ANDROID_OPENSSL_DIR)")
    endif()
endif()
