#ifndef TESTUNITMESSAGEPARSING_H
#define TESTUNITMESSAGEPARSING_H

#include "../../general/base/ProtocolParsing.h"

class TestUnitMessageParsing : public CatchChallenger::ProtocolParsingBase
{
public:
    TestUnitMessageParsing();
public:
    bool finalResult;

    void testDropAllPlayer();

protected:
    void errorParsingLayer(const std::string &error);
    void messageParsingLayer(const std::string &message) const;
    void reset();
    ssize_t read(char *, const size_t &);
    ssize_t write(const char * const, const size_t &);
    void registerOutputQuery(const uint8_t &queryNumber, const uint8_t &);
    //have message without reply
    bool parseMessage(const uint8_t &packetCode,const char * const data,const unsigned int &size);
    //have query with reply
    bool parseQuery(const uint8_t &packetCode,const uint8_t &queryNumber,const char * const data,const unsigned int &size);
    //send reply
    bool parseReplyData(const uint8_t &packetCode,const uint8_t &queryNumber,const char * const data,const unsigned int &size);
    void disconnectClient();
    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
    CatchChallenger::ProtocolParsing::CompressionType getCompressType() const;
    #endif
};

#endif // TESTUNITMESSAGEPARSING_H
