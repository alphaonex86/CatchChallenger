#include "TestUnitMessageParsing.h"
#include <iostream>

TestUnitMessageParsing::TestUnitMessageParsing() :
    CatchChallenger::ProtocolParsingBase(PacketModeTransmission_Server)
{
    ProtocolParsing::initialiseTheVariable();
}

void TestUnitMessageParsing::testDropAllPlayer()
{
    int8_t returnVar=0;
    flags|=0x08;
    finalResult=true;
    {
        const char data[]={65};
        const uint32_t size=sizeof(data);
        uint32_t cursor=0;
        do
        {
            const uint32_t oldcursor=cursor;
            returnVar=parseIncommingDataRaw(data,size,cursor);
            if(finalResult==false)
                return;
            if(oldcursor==cursor && returnVar==1)
            {
                std::cerr << "testDropAllPlayer 1x: Cursor don't move but the function have returned 1" << std::endl;
                finalResult=false;
                return;
            }
            if(returnVar!=1)
            {
                std::cerr << "testDropAllPlayer 1x: Don't have returned 1 for complete query" << std::endl;
                finalResult=false;
                return;
            }
        } while(cursor<size);
        std::cout << "testDropAllPlayer 1x: Ok" << std::endl;
    }
    {
        const char data[]={65,65,65};
        const uint32_t size=sizeof(data);
        uint32_t cursor=0;
        do
        {
            const uint32_t oldcursor=cursor;
            returnVar=parseIncommingDataRaw(data,size,cursor);
            if(finalResult==false)
                return;
            if(oldcursor==cursor && returnVar==1)
            {
                std::cerr << "testDropAllPlayer 3x: Cursor don't move but the function have returned 1" << std::endl;
                finalResult=false;
                return;
            }
            if(returnVar!=1)
            {
                std::cerr << "testDropAllPlayer 3x: Don't have returned 1 for complete query" << std::endl;
                finalResult=false;
                return;
            }
        } while(cursor<size);
        std::cout << "testDropAllPlayer 3x: Ok" << std::endl;
    }
}

void TestUnitMessageParsing::errorParsingLayer(const std::string &error)
{
    std::cerr << error << std::endl;
    finalResult=false;
}

void TestUnitMessageParsing::messageParsingLayer(const std::string &message) const
{
    std::cout << message << std::endl;
}

void TestUnitMessageParsing::reset()
{
}

ssize_t TestUnitMessageParsing::read(char *, const size_t &)
{
    std::cerr << "Should not read the socket here" << std::endl;
    finalResult=false;
    return 0;
}

ssize_t TestUnitMessageParsing::write(const char * const, const size_t &)
{
    std::cerr << "Should not write the socket here" << std::endl;
    finalResult=false;
    return 0;
}

void TestUnitMessageParsing::registerOutputQuery(const uint8_t &, const uint8_t &)
{
}

//have message without reply
bool TestUnitMessageParsing::parseMessage(const uint8_t &packetCode,const char * const data,const unsigned int &size)
{
    (void)packetCode;
    (void)data;
    (void)size;
    return true;
}

//have query with reply
bool TestUnitMessageParsing::parseQuery(const uint8_t &packetCode,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    (void)packetCode;
    (void)queryNumber;
    (void)data;
    (void)size;
    return true;
}

//send reply
bool TestUnitMessageParsing::parseReplyData(const uint8_t &packetCode,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    (void)packetCode;
    (void)queryNumber;
    (void)data;
    (void)size;
    return true;
}

void TestUnitMessageParsing::disconnectClient()
{
    std::cerr << "Should not disconnect the socket here" << std::endl;
    finalResult=false;
}

#ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
CatchChallenger::ProtocolParsing::CompressionType TestUnitMessageParsing::getCompressType() const
{
    return CatchChallenger::ProtocolParsing::compressionTypeClient;
}
#endif
