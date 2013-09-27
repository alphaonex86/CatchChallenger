QT       += sql

SOURCES += $$PWD/solo/NewProfile.cpp \
    $$PWD/solo/NewGame.cpp \
    $$PWD/solo/InternalServer.cpp \
    $$PWD/solo/SoloWindow.cpp

HEADERS  += $$PWD/solo/NewProfile.h \
    $$PWD/solo/NewGame.h \
    $$PWD/solo/InternalServer.h \
    $$PWD/solo/SoloWindow.h

FORMS    += $$PWD/solo/solowindow.ui \
    $$PWD/solo/NewProfile.ui \
    $$PWD/solo/NewGame.ui

RESOURCES += $$PWD/resources/resources-single-player.qrc \
    $$PWD/../../server/databases/resources-db-sqlite.qrc

TRANSLATIONS    = $$PWD/resources/languages/en/solo.ts \
    $$PWD/languages/fr/solo.ts
