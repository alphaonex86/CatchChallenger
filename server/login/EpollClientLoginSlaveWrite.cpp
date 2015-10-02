#include "EpollClientLoginSlave.h"
#include <iostream>

using namespace CatchChallenger;

/* not use mainCodeWithoutSubCodeTypeServerToClient because the reply have unknow code */
void EpollClientLoginSlave::sendFullPacket(const uint8_t &mainCodeType,const uint8_t &subCodeType,const char * const data,const unsigned int &size)
{
    if(!ProtocolParsingBase::packFullOutcommingData(mainCodeType,subCodeType,data,size))
        return;
}

void EpollClientLoginSlave::sendPacket(const uint8_t &mainCodeType,const char * const data,const unsigned int &size)
{
    if(!ProtocolParsingBase::packOutcommingData(mainCodeType,data,size))
        return;
}

void EpollClientLoginSlave::sendRawSmallPacket(const char * const data,const unsigned int &size)
{
    if(!ProtocolParsingBase::internalSendRawSmallPacket(data,size))
        return;
}

void EpollClientLoginSlave::sendQuery(const uint8_t &mainIdent,const uint8_t &subIdent,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    if(!ProtocolParsingBase::packFullOutcommingQuery(mainIdent,subIdent,queryNumber,data,size))
        return;
}

//send reply
void EpollClientLoginSlave::postReply(const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    if(!ProtocolParsingBase::postReplyData(queryNumber,data,size))
    {
        std::cerr << "can't send reply: postReply(" << queryNumber << "," << std::vector<char>(data,size).toHex().constData() << ")" << std::endl;
        return;
    }
}
