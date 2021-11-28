CONFIG += c++11
QMAKE_CXXFLAGS+="-std=c++0x"
mac:QMAKE_CXXFLAGS+="-stdlib=libc++"
linux:QMAKE_CXXFLAGS+="-Wno-missing-braces -Wno-delete-non-virtual-dtor -Wall -Wextra"
linux:QMAKE_CFLAGS+="-Wno-missing-braces -Wno-delete-non-virtual-dtor -Wall -Wextra"

QT       -= core

wasm: {
    QMAKE_LFLAGS += -s TOTAL_MEMORY=200015872
    #QMAKE_LFLAGS += -s EMTERPRETIFY_ASYNC=1 -s EMTERPRETIFY=1
    QMAKE_CLFLAGS += -g
    QMAKE_CXXFLAGS += -g
    QMAKE_LFLAGS += -s ASSERTIONS=1 -g
    #CONFIG+=debug
}
android: {
INCLUDEPATH += /opt/android-sdk/ndk-r18b/platforms/android-21/arch-arm/usr/include
}

TEMPLATE = app

SOURCES += $$PWD/base/ProtocolParsingGeneral.cpp \
    $$PWD/base/ProtocolParsingInput.cpp \
    $$PWD/base/ProtocolParsingOutput.cpp \
    $$PWD/base/ProtocolParsingCheck.cpp \
    $$PWD/base/MoveOnTheMap.cpp \
    $$PWD/base/Map_loader.cpp \
    $$PWD/base/Map_loaderMain.cpp \
    $$PWD/base/FacilityLib.cpp \
    $$PWD/base/FacilityLibGeneral.cpp \
    $$PWD/base/DatapackGeneralLoader.cpp \
    $$PWD/base/DatapackGeneralLoaderCrafting.cpp \
    $$PWD/base/DatapackGeneralLoaderIndustry.cpp \
    $$PWD/base/DatapackGeneralLoaderItem.cpp \
    $$PWD/base/DatapackGeneralLoaderMap.cpp \
    $$PWD/base/DatapackGeneralLoaderMonsterDrop.cpp \
    $$PWD/base/DatapackGeneralLoaderPlant.cpp \
    $$PWD/base/DatapackGeneralLoaderQuest.cpp \
    $$PWD/base/DatapackGeneralLoaderReputation.cpp \
    $$PWD/base/cpp11addition.cpp \
    $$PWD/base/cpp11additionstringtointcpp.cpp \
    $$PWD/base/cpp11additionstringtointc.cpp \
    $$PWD/fight/FightLoader.cpp \
    $$PWD/fight/FightLoaderBuff.cpp \
    $$PWD/fight/FightLoaderFight.cpp \
    $$PWD/fight/FightLoaderMonster.cpp \
    $$PWD/fight/FightLoaderSkill.cpp \
    $$PWD/base/CommonMap.cpp \
    $$PWD/base/CommonDatapack.cpp \
    $$PWD/base/CommonDatapackServerSpec.cpp \
    $$PWD/base/CommonSettingsCommon.cpp \
    $$PWD/base/CommonSettingsServer.cpp \
    $$PWD/fight/CommonFightEngine.cpp \
    $$PWD/fight/CommonFightEngineEnd.cpp \
    $$PWD/fight/CommonFightEngineTurn.cpp \
    $$PWD/fight/CommonFightEngineBuff.cpp \
    $$PWD/fight/CommonFightEngineSkill.cpp \
    $$PWD/fight/CommonFightEngineWild.cpp \
    $$PWD/fight/CommonFightEngineBase.cpp \
    $$PWD/base/Version.cpp \
    $$PWD/sha224/sha224.cpp \
    $$PWD/xxhash/xxhash.c

SOURCES += \
    $$PWD/libzstd/lib/common/pool.c \
    $$PWD/libzstd/lib/common/zstd_common.c \
    $$PWD/libzstd/lib/common/entropy_common.c \
    $$PWD/libzstd/lib/common/fse_decompress.c \
    $$PWD/libzstd/lib/common/threading.c \
    $$PWD/libzstd/lib/common/error_private.c \
    $$PWD/libzstd/lib/common/xxbhash.c \
    $$PWD/libzstd/lib/compress/zstd_compress_sequences.c \
    $$PWD/libzstd/lib/compress/huf_compress.c \
    $$PWD/libzstd/lib/compress/zstd_compress.c \
    $$PWD/libzstd/lib/compress/zstd_compress_superblock.c \
    $$PWD/libzstd/lib/compress/zstd_lazy.c \
    $$PWD/libzstd/lib/compress/hist.c \
    $$PWD/libzstd/lib/compress/zstd_fast.c \
    $$PWD/libzstd/lib/compress/zstd_opt.c \
    $$PWD/libzstd/lib/compress/zstd_ldm.c \
    $$PWD/libzstd/lib/compress/zstdmt_compress.c \
    $$PWD/libzstd/lib/compress/fse_compress.c \
    $$PWD/libzstd/lib/compress/zstd_compress_literals.c \
    $$PWD/libzstd/lib/compress/zstd_double_fast.c \
    $$PWD/libzstd/lib/decompress/zstd_ddict.c \
    $$PWD/libzstd/lib/decompress/huf_decompress.c \
    $$PWD/libzstd/lib/decompress/zstd_decompress.c \
    $$PWD/libzstd/lib/decompress/zstd_decompress_block.c \
    $$PWD/libzstd/lib/dictBuilder/cover.c \
    $$PWD/libzstd/lib/dictBuilder/divsufsort.c \
    $$PWD/libzstd/lib/dictBuilder/fastcover.c \
    $$PWD/libzstd/lib/dictBuilder/zdict.c \

HEADERS  += $$PWD/base/GeneralStructures.hpp \
    $$PWD/base/ClientBase.hpp \
    $$PWD/base/ProtocolParsing.hpp \
    $$PWD/base/ProtocolParsingCheck.hpp \
    $$PWD/base/MoveOnTheMap.hpp \
    $$PWD/base/Map_loader.hpp \
    $$PWD/base/CommonMap.hpp \
    $$PWD/base/GeneralVariable.hpp \
    $$PWD/base/FacilityLib.hpp \
    $$PWD/base/FacilityLibGeneral.hpp \
    $$PWD/base/DatapackGeneralLoader.hpp \
    $$PWD/base/CommonDatapack.hpp \
    $$PWD/base/CommonDatapackServerSpec.hpp \
    $$PWD/base/CommonSettingsCommon.hpp \
    $$PWD/base/CommonSettingsServer.hpp \
    $$PWD/base/GeneralType.hpp \
    $$PWD/base/cpp11addition.hpp \
    $$PWD/base/GeneralStructuresXml.hpp \
    $$PWD/base/PortableEndian.hpp \
    $$PWD/fight/FightLoader.hpp \
    $$PWD/fight/CommonFightEngine.hpp \
    $$PWD/fight/CommonFightEngineBase.hpp \
    $$PWD/sha224/sha224.hpp \
    $$PWD/xxhash/xxhash.h

HEADERS += \
    $$PWD/libzstd/lib/zstd.h \
    $$PWD/libzstd/lib/common/compiler.h \
    $$PWD/libzstd/lib/common/threading.h \
    $$PWD/libzstd/lib/common/xxhash.h \
    $$PWD/libzstd/lib/common/error_private.h \
    $$PWD/libzstd/lib/common/bitstream.h \
    $$PWD/libzstd/lib/common/fse.h \
    $$PWD/libzstd/lib/common/mem.h \
    $$PWD/libzstd/lib/common/huf.h \
    $$PWD/libzstd/lib/common/zstd_internal.h \
    $$PWD/libzstd/lib/common/pool.h \
    $$PWD/libzstd/lib/common/cpu.h \
    $$PWD/libzstd/lib/common/zstd_errors.h \
    $$PWD/libzstd/lib/compress/zstd_fast.h \
    $$PWD/libzstd/lib/compress/zstd_lazy.h \
    $$PWD/libzstd/lib/compress/zstd_compress_internal.h \
    $$PWD/libzstd/lib/compress/zstdmt_compress.h \
    $$PWD/libzstd/lib/compress/zstd_cwksp.h \
    $$PWD/libzstd/lib/compress/zstd_opt.h \
    $$PWD/libzstd/lib/compress/hist.h \
    $$PWD/libzstd/lib/compress/zstd_compress_sequences.h \
    $$PWD/libzstd/lib/compress/zstd_ldm.h \
    $$PWD/libzstd/lib/compress/zstd_double_fast.h \
    $$PWD/libzstd/lib/compress/zstd_compress_literals.h \
    $$PWD/libzstd/lib/decompress/zstd_ddict.h \
    $$PWD/libzstd/lib/decompress/zstd_decompress_block.h \
    $$PWD/libzstd/lib/decompress/zstd_decompress_internal.h \
    $$PWD/libzstd/lib/dictBuilder/cover.h \
    $$PWD/libzstd/lib/dictBuilder/zdict.h \
    $$PWD/libzstd/lib/dictBuilder/divsufsort.h \

DEFINES += ZSTD_STATIC_LINKING_ONLY
INCLUDEPATH += $$PWD/libzstd/lib/

#only linux is C only, mac, windows, other is in Qt for compatibility
win32:RESOURCES += $$PWD/base/resources/resources-windows-qt-plugin.qrc
mac:INCLUDEPATH += /usr/local/include/
mac:LIBS += -L/usr/local/lib/
linux:DEFINES += __linux__
