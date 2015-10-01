/** \file QXzDecodeThread.cpp
\brief To have thread to decode the data
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include "QXzDecodeThread.h"

QXzDecodeThread::QXzDecodeThread()
{
    #ifndef QT_NO_EMIT
    moveToThread(this);
    #endif
    DataToDecode=NULL;
    error=false;
}

QXzDecodeThread::~QXzDecodeThread()
{
    if(DataToDecode!=NULL)
        delete DataToDecode;
}

void QXzDecodeThread::setData(std::vector<char>, uint64_t maxSize)
{
    if(DataToDecode!=NULL)
        delete DataToDecode;
    DataToDecode=new QXzDecode(data,maxSize);
}

bool QXzDecodeThread::errorFound()
{
    return error;
}

std::string QXzDecodeThread::errorString()
{
    return DataToDecode->errorString();
}

std::vector<char> QXzDecodeThread::decodedData()
{
    return DataToDecode->decodedData();
}

void QXzDecodeThread::run()
{
    if(DataToDecode!=NULL)
        error=!DataToDecode->decode();
    else
        error=true;
    #ifndef QT_NO_EMIT
    emit decodedIsFinish();
    #endif
}

