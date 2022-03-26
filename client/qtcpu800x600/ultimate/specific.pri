SOURCES += $$PWD/main.cpp \
    $$PWD/mainwindow.cpp \
    $$PWD/AddServer.cpp \
    $$PWD/AskKey.cpp

HEADERS  += $$PWD/mainwindow.h \
    $$PWD/AddServer.h \
    $$PWD/AskKey.h

FORMS    += $$PWD/mainwindow.ui \
    $$PWD/AddServer.ui \
    $$PWD/AskKey.ui

TRANSLATIONS    = $$PWD/languages/en/specific.ts \
    $$PWD/languages/fr/specific.ts

DEFINES += CATCHCHALLENGER_VERSION_ULTIMATE

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/../base/android-sources

DISTFILES += \
    ../base/android-sources/AndroidManifest.xml

