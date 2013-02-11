include(../base/client.pri)
include(../../general/general.pri)

TARGET = catchchallenger-single-server

SOURCES += main.cpp\
	mainwindow.cpp \
    ../fight/interface/MapVisualiserPlayerWithFight.cpp

HEADERS  += mainwindow.h \
    ../fight/interface/MapVisualiserPlayerWithFight.h

FORMS    += mainwindow.ui

RESOURCES += ../base/resources/client-resources-multi.qrc
