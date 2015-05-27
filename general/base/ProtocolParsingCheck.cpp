#include "ProtocolParsingCheck.h"
#ifdef CATCHCHALLENGER_EXTRA_CHECK

#include <QDebug>

using namespace CatchChallenger;

ProtocolParsingCheck::ProtocolParsingCheck(const PacketModeTransmission &packetModeTransmission) :
    ProtocolParsingBase(packetModeTransmission),
    valid(false)
{
}

void ProtocolParsingCheck::parseMessage(const quint8 &mainCodeType,const char * const data,const unsigned int &size)
{
    Q_UNUSED(mainCodeType);
    Q_UNUSED(data);
    Q_UNUSED(size);
    if(valid)
    {
        qDebug() << "Double valid call!";
        abort();
    }
    valid=true;
}

void ProtocolParsingCheck::parseFullMessage(const quint8 &mainCodeType,const quint8 &subCodeType,const char * const data,const unsigned int &size)
{
    Q_UNUSED(mainCodeType);
    Q_UNUSED(subCodeType);
    Q_UNUSED(data);
    Q_UNUSED(size);
    if(valid)
    {
        qDebug() << "Double valid call!";
        abort();
    }
    valid=true;
}

void ProtocolParsingCheck::parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size)
{
    Q_UNUSED(mainCodeType);
    Q_UNUSED(queryNumber);
    Q_UNUSED(data);
    Q_UNUSED(size);
    if(valid)
    {
        qDebug() << "Double valid call!";
        abort();
    }
    valid=true;
}

void ProtocolParsingCheck::parseFullQuery(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size)
{
    Q_UNUSED(mainCodeType);
    Q_UNUSED(subCodeType);
    Q_UNUSED(queryNumber);
    Q_UNUSED(data);
    Q_UNUSED(size);
    if(valid)
    {
        qDebug() << "Double valid call!";
        abort();
    }
    valid=true;
}

void ProtocolParsingCheck::parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size)
{
    Q_UNUSED(mainCodeType);
    Q_UNUSED(queryNumber);
    Q_UNUSED(data);
    Q_UNUSED(size);
    if(valid)
    {
        qDebug() << "Double valid call!";
        abort();
    }
    valid=true;
}

void ProtocolParsingCheck::parseFullReplyData(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size)
{
    Q_UNUSED(mainCodeType);
    Q_UNUSED(subCodeType);
    Q_UNUSED(queryNumber);
    Q_UNUSED(data);
    Q_UNUSED(size);
    if(valid)
    {
        qDebug() << "Double valid call!";
        abort();
    }
    valid=true;
}

void ProtocolParsingCheck::errorParsingLayer(const QString &error)
{
    qDebug() << error;
    abort();
}

void ProtocolParsingCheck::messageParsingLayer(const QString &message) const
{
    qDebug() << message;
}

void ProtocolParsingCheck::disconnectClient()
{
    qDebug() << "Disconnect at control";
    abort();
}

ssize_t ProtocolParsingCheck::read(char * data, const size_t &size)
{
    Q_UNUSED(data);
    Q_UNUSED(size);
    qDebug() << "Read at check";
    abort();
}

ssize_t ProtocolParsingCheck::write(const char * const data, const size_t &size)
{
    Q_UNUSED(data);
    Q_UNUSED(size);
    qDebug() << "Write at check";
    abort();
}

bool ProtocolParsingCheck::parseIncommingDataRaw(const char * const commonBuffer, const quint32 &size,quint32 &cursor)
{
    const bool &returnVar=ProtocolParsingBase::parseIncommingDataRaw(commonBuffer,size,cursor);
    if(!header_cut.isEmpty())
    {
        qDebug() << "Header cut is not empty";
        abort();
    }
    return returnVar;
}

#ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
ProtocolParsing::CompressionType ProtocolParsingCheck::getCompressType() const
{
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(isClient)
        return compressionTypeServer;
    else
        return compressionTypeClient;
    #else
        return compressionTypeServer;
    #endif
}
#endif

#endif
