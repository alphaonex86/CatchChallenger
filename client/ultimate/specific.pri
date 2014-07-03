SOURCES += main.cpp\
        mainwindow.cpp \
    AddServer.cpp

HEADERS  += mainwindow.h \
    AddServer.h

FORMS    += mainwindow.ui \
    AddServer.ui

TRANSLATIONS    = $$PWD/languages/en/specific.ts \
    $$PWD/languages/fr/specific.ts

DEFINES += CATCHCHALLENGER_VERSION_ULTIMATE
