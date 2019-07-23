QT       += sql

SOURCES += $$PWD/solo/InternalServer.cpp \
    $$PWD/solo/SoloWindow.cpp

HEADERS  += $$PWD/solo/InternalServer.h \
    $$PWD/solo/SoloWindow.h

FORMS    += $$PWD/solo/solowindow.ui

# done into all qt server: RESOURCES += $$PWD/../../server/databases/resources-db-sqlite.qrc

#RESOURCES += $$PWD/../resources/resources-single-player.qrc

TRANSLATIONS    = $$PWD/resources/languages/en/solo.ts \
    $$PWD/languages/fr/solo.ts

DEFINES += CATCHCHALLENGER_SOLO
