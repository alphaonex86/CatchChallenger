#if ! defined (ONLYMAPRENDER)
#include "ProtocolParsingCheck.h"
#ifdef CATCHCHALLENGER_EXTRA_CHECK

#include <iostream>

using namespace CatchChallenger;

ProtocolParsingCheck::ProtocolParsingCheck(const PacketModeTransmission &packetModeTransmission) :
    ProtocolParsingBase(packetModeTransmission),
    valid(false)
{
    flags|=0x08;
}

bool ProtocolParsingCheck::parseMessage(const uint8_t &mainCodeType,const char * const data,const unsigned int &size)
{
    (void)(mainCodeType);
    (void)(data);
    (void)(size);
    if(valid)
    {
        std::cerr << "Double valid call!" << std::endl;
        abort();
    }
    valid=true;
    return true;
}

void ProtocolParsingCheck::moveClientFastPath(const uint8_t &previousMovedUnit,const uint8_t &direction)
{
    (void)previousMovedUnit;
    (void)direction;
}

void ProtocolParsingCheck::breakNeedMoreData()
{
    std::cerr << "ProtocolParsingCheck::breakNeedMoreData(): Break due to need more in parse data" << std::endl;
}

bool ProtocolParsingCheck::parseQuery(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    (void)(mainCodeType);
    (void)(queryNumber);
    (void)(data);
    (void)(size);
    if(valid)
    {
        std::cerr << "Double valid call!" << std::endl;
        abort();
    }
    valid=true;
    return true;
}

bool ProtocolParsingCheck::parseReplyData(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    (void)(mainCodeType);
    (void)(queryNumber);
    (void)(data);
    (void)(size);
    if(valid)
    {
        std::cerr << "Double valid call!" << std::endl;
        abort();
    }
    valid=true;
    return true;
}

void ProtocolParsingCheck::errorParsingLayer(const std::string &error)
{
    std::cerr << "ProtocolParsingCheck: " << error << std::endl;
    abort();
}

void ProtocolParsingCheck::messageParsingLayer(const std::string &message) const
{
    std::cerr << "ProtocolParsingCheck: " << message << std::endl;
}

bool ProtocolParsingCheck::disconnectClient()
{
    std::cerr << "Disconnect at control" << std::endl;
    abort();
    return true;
}

ssize_t ProtocolParsingCheck::read(char * data, const size_t &size)
{
    (void)data;
    (void)size;
    std::cerr << "Read at check" << std::endl;
    abort();
}

ssize_t ProtocolParsingCheck::write(const char * const data, const size_t &size)
{
    (void)data;
    (void)size;
    std::cerr << "Write at check" << std::endl;
    abort();
}

int8_t ProtocolParsingCheck::parseIncommingDataRaw(const char * const commonBuffer, const uint32_t &size,uint32_t &cursor)
{
    const int8_t &returnVar=ProtocolParsingBase::parseIncommingDataRaw(commonBuffer,size,cursor);
    if(!header_cut.empty())
    {
        std::cerr << "Header cut is not empty" << std::endl;
        abort();
    }
    return returnVar;
}

//to revert input and ouput table
void ProtocolParsingCheck::storeInputQuery(const uint8_t &packetCode,const uint8_t &queryNumber)
{
    //register the size of the reply to send
    outputQueryNumberToPacketCode[queryNumber]=packetCode;
}

//to revert input and ouput table
void ProtocolParsingCheck::registerOutputQuery(const uint8_t &queryNumber, const uint8_t &packetCode)
{
    inputQueryNumberToPacketCode[queryNumber]=packetCode;
}

#ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
ProtocolParsing::CompressionType ProtocolParsingCheck::getCompressType() const
{
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(flags & 0x10)
        return compressionTypeServer;
    else
        return compressionTypeClient;
    #else
        return compressionTypeServer;
    #endif
}
#endif

#endif
#endif
