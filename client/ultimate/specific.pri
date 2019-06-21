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
