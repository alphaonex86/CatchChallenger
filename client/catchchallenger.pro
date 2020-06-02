include(qtopengl/client.pri)
include(../general/general.pri)
include(qt/qt.pri)

QMAKE_CXXFLAGS+="-Wno-deprecated-declarations"
QMAKE_CFLAGS+="-Wno-deprecated-declarations"

DEFINES += OPENGL CATCHCHALLENGER_CACHE_HPS
TARGET = catchchallenger
