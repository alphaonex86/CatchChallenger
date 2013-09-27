SOURCES += main.cpp\
        mainwindow.cpp \
    AddServer.cpp

HEADERS  += mainwindow.h \
    AddServer.h

FORMS    += mainwindow.ui \
    AddServer.ui

RESOURCES += ../base/resources/client-resources-multi.qrc

TRANSLATIONS    = $$PWD/languages/en/specific.ts \
    $$PWD/languages/fr/specific.ts
