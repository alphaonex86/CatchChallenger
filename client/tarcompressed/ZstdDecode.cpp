#include "ZstdDecode.hpp"
#include "../libzstd/lib/zstd.h"

ZstdDecode::ZstdDecode()
{
}

ZstdDecode::~ZstdDecode()
{
}

void ZstdDecode::setData(std::vector<char> data)
{
    mDataToDecode=data;
}

bool ZstdDecode::errorFound()
{
    return !mErrorString.empty();
}

std::string ZstdDecode::errorString()
{
    return mErrorString;
}

std::vector<char> ZstdDecode::decodedData()
{
    return mDataToDecode;
}

void ZstdDecode::run()
{
    std::vector<char> dataToDecoded;

    unsigned long long const rSize = ZSTD_getDecompressedSize(mDataToDecode.data(), mDataToDecode.size());
    if (rSize==ZSTD_CONTENTSIZE_ERROR) {
        mErrorString="it was not compressed by zstd";
        return;
    } else if (rSize==ZSTD_CONTENTSIZE_UNKNOWN) {
        mErrorString="original size unknown. Use streaming decompression instead.";
        return;
    }

    dataToDecoded.resize(rSize);

    size_t const dSize = ZSTD_decompress(dataToDecoded.data(), rSize, mDataToDecode.data(), mDataToDecode.size());

    if (dSize != rSize) {
        mErrorString=std::string("error decoding: ")+ZSTD_getErrorName(dSize);
        return;
    }
    dataToDecoded.resize(dSize);

    mDataToDecode=dataToDecoded;
}

