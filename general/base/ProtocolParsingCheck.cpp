#include "ProtocolParsingCheck.h"
#ifdef CATCHCHALLENGER_EXTRA_CHECK

#include <QDebug>

using namespace CatchChallenger;

ProtocolParsingCheck::ProtocolParsingCheck(const PacketModeTransmission &packetModeTransmission) :
    ProtocolParsingBase(packetModeTransmission),
    valid(false)
{
}

void ProtocolParsingCheck::parseMessage(const uint8_t &mainCodeType,const char * const data,const unsigned int &size)
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

void ProtocolParsingCheck::parseFullMessage(const uint8_t &mainCodeType,const uint8_t &subCodeType,const char * const data,const unsigned int &size)
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

void ProtocolParsingCheck::moveClientFastPath(const uint8_t &previousMovedUnit,const uint8_t &direction)
{
}

void ProtocolParsingCheck::parseQuery(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
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

void ProtocolParsingCheck::parseFullQuery(const uint8_t &mainCodeType,const uint8_t &subCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
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

void ProtocolParsingCheck::parseReplyData(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
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

void ProtocolParsingCheck::parseFullReplyData(const uint8_t &mainCodeType,const uint8_t &subCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
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

void ProtocolParsingCheck::errorParsingLayer(const std::string &error)
{
    qDebug() << error;
    abort();
}

void ProtocolParsingCheck::messageParsingLayer(const std::string &message) const
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

bool ProtocolParsingCheck::parseIncommingDataRaw(const char * const commonBuffer, const uint32_t &size,uint32_t &cursor)
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
