include(catchchallenger-server-normal.pri)

QT       -= gui widgets
LIBS += -lzstd

#QMAKE_CFLAGS="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops"
#QMAKE_CXXFLAGS="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops"

TARGET = catchchallenger-server-cli
CONFIG   += console

SOURCES += ProcessControler.cpp \
    main-cli.cpp
HEADERS  += ProcessControler.h
