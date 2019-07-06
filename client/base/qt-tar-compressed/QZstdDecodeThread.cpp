#include "QZstdDecodeThread.h"
#include <zstd.h>

QZstdDecodeThread::QZstdDecodeThread()
{
    #ifndef NOTHREADS
    moveToThread(this);
    #endif
}

QZstdDecodeThread::~QZstdDecodeThread()
{
}

void QZstdDecodeThread::setData(std::vector<char> data)
{
    mDataToDecode=data;
}

bool QZstdDecodeThread::errorFound()
{
    return !mErrorString.isEmpty();
}

QString QZstdDecodeThread::errorString()
{
    return mErrorString;
}

std::vector<char> QZstdDecodeThread::decodedData()
{
    return mDataToDecode;
}

void QZstdDecodeThread::run()
{
    std::vector<char> dataToDecoded;

    unsigned long long const rSize = ZSTD_getDecompressedSize(mDataToDecode.data(), mDataToDecode.size());
    if (rSize==ZSTD_CONTENTSIZE_ERROR) {
        mErrorString="it was not compressed by zstd";
        emit decodedIsFinish();
        return;
    } else if (rSize==ZSTD_CONTENTSIZE_UNKNOWN) {
        mErrorString="original size unknown. Use streaming decompression instead.";
        emit decodedIsFinish();
        return;
    }

    dataToDecoded.resize(rSize);

    size_t const dSize = ZSTD_decompress(dataToDecoded.data(), rSize, mDataToDecode.data(), mDataToDecode.size());

    if (dSize != rSize) {
        mErrorString=QString("error decoding: ")+ZSTD_getErrorName(dSize);
        emit decodedIsFinish();
        return;
    }
    dataToDecoded.resize(dSize);

    mDataToDecode=dataToDecoded;

    emit decodedIsFinish();
}

