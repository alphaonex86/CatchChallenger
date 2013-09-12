include(../base/client.pri)
include(../../general/general.pri)

TARGET = catchchallenger-multi-server

SOURCES += main.cpp\
	mainwindow.cpp \
    AddServer.cpp

HEADERS  += mainwindow.h \
    AddServer.h

FORMS    += mainwindow.ui \
    AddServer.ui

RESOURCES += ../base/resources/client-resources-multi.qrc

