INCLUDEPATH += $$PWD/libzstd/lib/ \
    $$PWD/libzstd/lib/common/

SOURCES += \
    $$PWD/libzstd/lib/common/debug.c \
    $$PWD/libzstd/lib/common/entropy_common.c \
    $$PWD/libzstd/lib/common/error_private.c \
    $$PWD/libzstd/lib/common/fse_decompress.c \
    $$PWD/libzstd/lib/common/pool.c \
    $$PWD/libzstd/lib/common/threading.c \
    $$PWD/libzstd/lib/common/xxhash.c \
    $$PWD/libzstd/lib/common/zstd_common.c \
    $$PWD/libzstd/lib/compress/fse_compress.c \
    $$PWD/libzstd/lib/compress/hist.c \
    $$PWD/libzstd/lib/compress/huf_compress.c \
    $$PWD/libzstd/lib/compress/zstd_compress.c \
    $$PWD/libzstd/lib/compress/zstd_compress_literals.c \
    $$PWD/libzstd/lib/compress/zstd_compress_sequences.c \
    $$PWD/libzstd/lib/compress/zstd_compress_superblock.c \
    $$PWD/libzstd/lib/compress/zstd_double_fast.c \
    $$PWD/libzstd/lib/compress/zstd_fast.c \
    $$PWD/libzstd/lib/compress/zstd_lazy.c \
    $$PWD/libzstd/lib/compress/zstd_ldm.c \
    $$PWD/libzstd/lib/compress/zstdmt_compress.c \
    $$PWD/libzstd/lib/compress/zstd_opt.c \
    $$PWD/libzstd/lib/compress/zstd_preSplit.c \
    $$PWD/libzstd/lib/decompress/huf_decompress.c \
    $$PWD/libzstd/lib/decompress/zstd_ddict.c \
    $$PWD/libzstd/lib/decompress/zstd_decompress_block.c \
    $$PWD/libzstd/lib/decompress/zstd_decompress.c

HEADERS += $$PWD/libzstd/lib/zstd.h

DEFINES += ZSTD_MULTITHREAD=0 ZSTD_DISABLE_ASM
