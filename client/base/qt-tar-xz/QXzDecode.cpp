/** \file QXzDecode.cpp
\brief To decompress a xz stream
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include "QXzDecode.h"

#include <cstring>

extern "C" {
#include "xz.h"
}

static uint8_t out[BUFSIZ];

QXzDecode::QXzDecode(std::vector<char> data,uint64_t maxSize)
{
    error="Unknow error";
    this->data=data;
    this->maxSize=maxSize;
    isDecoded=false;
}

bool QXzDecode::decode()
{
    if(data.size() < 32) // check the minimal size
        error="The input data is too short";

    isDecoded=false;
    struct xz_buf b;
    struct xz_dec *s;
    enum xz_ret ret;

    xz_crc32_init();

    /*
     * Support up to 64 MiB dictionary. The actually needed memory
     * is allocated once the headers have been parsed.
     */
    s = xz_dec_init(XZ_DYNALLOC, 1 << 26);
    if (s == NULL) {
        error="Memory allocation failed";
        xz_dec_end(s);
        return isDecoded;
    }

    b.in = reinterpret_cast<const unsigned char *>(data.data());
    b.in_pos = 0;
    b.in_size = data.size();
    b.out = out;
    b.out_pos = 0;
    b.out_size = BUFSIZ;

    while (true) {
        ret = xz_dec_run(s, &b);

        if (ret == XZ_OK)
            continue;

#ifdef XZ_DEC_ANY_CHECK
        if (ret == XZ_UNSUPPORTED_CHECK) {
            continue;
        }
#endif

        switch (ret) {
        case XZ_STREAM_END:
            xz_dec_end(s);
            isDecoded=true;
            return isDecoded;
        case XZ_MEM_ERROR:
            error="Memory allocation failed";
            xz_dec_end(s);
            return isDecoded;
        case XZ_MEMLIMIT_ERROR:
            error="Memory usage limit reached";
            xz_dec_end(s);
            return isDecoded;
        case XZ_FORMAT_ERROR:
            error="Not a .xz file";
            xz_dec_end(s);
            return isDecoded;
        case XZ_OPTIONS_ERROR:
            error="Unsupported options in the .xz headers";
            xz_dec_end(s);
            return isDecoded;
        case XZ_DATA_ERROR:
        case XZ_BUF_ERROR:
            error="The file is corrupted";
            xz_dec_end(s);
            return isDecoded;
        default:
            error="Bug!";
            xz_dec_end(s);
            return isDecoded;
        }
    }

    data.resize(b.out_pos);
    memcpy(data.data(),b.out,b.out_pos);
    return isDecoded;
}

std::vector<char> QXzDecode::decodedData()
{
    if(isDecoded)
        return data;
    else
        return std::vector<char>();
}

std::string QXzDecode::errorString()
{
    return error;
}


