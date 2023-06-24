include(catchchallenger-server-normal.pri)

QT       -= gui widgets
DEFINES += CATCHCHALLENGER_DB_MYSQL CATCHCHALLENGER_DB_POSTGRESQL CATCHCHALLENGER_DB_SQLITE

#QMAKE_CFLAGS="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops"
#QMAKE_CXXFLAGS="-pipe -march=native -O2 -fomit-frame-pointer -floop-block -floop-interchange -fgraphite -funroll-loops"
TEMPLATE = app
TARGET = catchchallenger-server-cli
CONFIG   += console

SOURCES += $$PWD/qt/ProcessControler.cpp \
    main-cli.cpp
HEADERS  += $$PWD/qt/ProcessControler.hpp
