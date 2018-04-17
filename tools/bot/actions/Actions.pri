SOURCES += $$PWD/ActionsBotInterface.cpp \
    $$PWD/ActionsAction.cpp \
    $$PWD/ActionsMap.cpp \
    $$PWD/ActionsMapStep1.cpp \
    $$PWD/MapServerMini.cpp \
    $$PWD/ActionsBot.cpp \
    $$PWD/MapServerMiniStep1.cpp \
    $$PWD/MapServerMiniStep2.cpp \
    $$PWD/MapServerMiniNear.cpp \
    $$PWD/ActionsCheck.cpp \
    $$PWD/ActionsFight.cpp \
    $$PWD/ActionsQuests.cpp \
    $$PWD/ActionsAPIReturn.cpp

HEADERS  += $$PWD/ActionsBotInterface.h \
    $$PWD/ActionsAction.h \
    $$PWD/MapServerMini.h

#needed for QColor into MultipleBotConnectionImplForGui
QT       += gui
