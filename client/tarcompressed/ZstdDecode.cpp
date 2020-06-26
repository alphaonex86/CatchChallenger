#include "ZstdDecode.hpp"
#include "../../general/libzstd/lib/zstd.h"

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

    bool streamed=false;
    unsigned long long rSize = ZSTD_getFrameContentSize(mDataToDecode.data(), mDataToDecode.size());
    if (rSize==ZSTD_CONTENTSIZE_ERROR) {
        mErrorString="it was not compressed by zstd";
        return;
    } else if (rSize==ZSTD_CONTENTSIZE_UNKNOWN) {
        rSize=64*1024*1024;
        streamed=true;
    } else if (rSize==0) {
        mErrorString="original size 0. Use streaming decompression instead.";
        return;
    }

    dataToDecoded.resize(rSize);

    size_t const dSize = ZSTD_decompress(dataToDecoded.data(), rSize, mDataToDecode.data(), mDataToDecode.size());

    if (!streamed && dSize != rSize) {
        mErrorString=std::string("error decoding: ")+ZSTD_getErrorName(dSize);
        return;
    }
    dataToDecoded.resize(dSize);

    mDataToDecode=dataToDecoded;
}

