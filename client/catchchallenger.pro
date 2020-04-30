include(qtopengl/client.pri)
include(../general/general.pri)
include(qt/qt.pri)

TARGET = catchchallenger

HEADERS += \
    qtopengl/foreground/MainScreen.hpp \
    qtopengl/foreground/Multi.hpp

SOURCES += \
    qtopengl/foreground/MainScreen.cpp \
    qtopengl/foreground/Multi.cpp

