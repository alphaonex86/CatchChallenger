include(qtopengl/client.pri)
include(../general/general.pri)
include(qt/qt.pri)

linux:QMAKE_CXXFLAGS+="-Wno-deprecated-declarations"
linux:QMAKE_CFLAGS+="-Wno-deprecated-declarations"

DEFINES += OPENGL CATCHCHALLENGER_CACHE_HPS
TARGET = catchchallenger
