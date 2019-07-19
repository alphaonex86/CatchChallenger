SOURCES += main.cpp\
    mainwindow.cpp \
    AddServer.cpp \
    AskKey.cpp

HEADERS  += mainwindow.h \
    AddServer.h \
    AskKey.h

FORMS    += mainwindow.ui \
    AddServer.ui \
    AskKey.ui

TRANSLATIONS    = $$PWD/languages/en/specific.ts \
    $$PWD/languages/fr/specific.ts

DEFINES += CATCHCHALLENGER_VERSION_ULTIMATE

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/../base/android-sources

DISTFILES += \
    ../base/android-sources/AndroidManifest.xml
