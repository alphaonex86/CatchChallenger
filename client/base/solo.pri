QT       += sql

SOURCES += $$PWD/solo/InternalServer.cpp \
    $$PWD/solo/SoloWindow.cpp

HEADERS  += $$PWD/solo/InternalServer.h \
    $$PWD/solo/SoloWindow.h

FORMS    += $$PWD/solo/solowindow.ui

RESOURCES += $$PWD/resources/resources-single-player.qrc \
    $$PWD/../../server/databases/resources-db-sqlite.qrc

TRANSLATIONS    = $$PWD/resources/languages/en/solo.ts \
    $$PWD/languages/fr/solo.ts

DEFINES += CATCHCHALLENGER_SOLO
