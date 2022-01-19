#include "QZstdDecodeThread.hpp"
#include "../../general/libzstd/lib/zstd.h"

QZstdDecodeThread::QZstdDecodeThread()
{
    #ifndef NOTHREADS
    moveToThread(this);
    #endif
}

QZstdDecodeThread::~QZstdDecodeThread()
{
}

void QZstdDecodeThread::run()
{
    ZstdDecode::run();
    emit decodedIsFinish();
}

