#include "EpollClientLoginSlave.h"
#include <iostream>

using namespace CatchChallenger;

/* not use mainCodeWithoutSubCodeTypeServerToClient because the reply have unknow code */
void EpollClientLoginSlave::sendFullPacket(const quint8 &mainCodeType,const quint8 &subCodeType,const char * const data,const unsigned int &size)
{
    if(!ProtocolParsingBase::packFullOutcommingData(mainCodeType,subCodeType,data,size))
        return;
}

void EpollClientLoginSlave::sendPacket(const quint8 &mainCodeType,const char * const data,const unsigned int &size)
{
    if(!ProtocolParsingBase::packOutcommingData(mainCodeType,data,size))
        return;
}

void EpollClientLoginSlave::sendRawSmallPacket(const char * const data,const unsigned int &size)
{
    if(!ProtocolParsingBase::internalSendRawSmallPacket(data,size))
        return;
}

void EpollClientLoginSlave::sendQuery(const quint8 &mainIdent,const quint8 &subIdent,const quint8 &queryNumber,const char * const data,const unsigned int &size)
{
    if(!ProtocolParsingBase::packFullOutcommingQuery(mainIdent,subIdent,queryNumber,data,size))
        return;
}

//send reply
void EpollClientLoginSlave::postReply(const quint8 &queryNumber,const char * const data,const unsigned int &size)
{
    if(!ProtocolParsingBase::postReplyData(queryNumber,data,size))
    {
        std::cerr << "can't send reply: postReply(" << queryNumber << "," << QByteArray(data,size).toHex().constData() << ")" << std::endl;
        return;
    }
}
