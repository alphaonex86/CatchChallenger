#include "EpollClientLoginSlave.h"

using namespace CatchChallenger;

/* not use mainCodeWithoutSubCodeTypeServerToClient because the reply have unknow code */
void EpollClientLoginSlave::sendFullPacket(const quint8 &mainCodeType,const quint16 &subCodeType,const char *data,const int &size)
{
    if(!ProtocolParsingBase::packFullOutcommingData(mainCodeType,subCodeType,data,size))
        return;
}

void EpollClientLoginSlave::sendPacket(const quint8 &mainCodeType,const char *data,const int &size)
{
    if(!ProtocolParsingBase::packOutcommingData(mainCodeType,data,size))
        return;
}

void EpollClientLoginSlave::sendRawSmallPacket(const char *data,const int &size)
{
    if(!ProtocolParsingBase::internalSendRawSmallPacket(data,size))
        return;
}

void EpollClientLoginSlave::sendQuery(const quint8 &mainIdent,const quint16 &subIdent,const quint8 &queryNumber,const char *data,const int &size)
{
    if(!ProtocolParsingBase::packFullOutcommingQuery(mainIdent,subIdent,queryNumber,data,size))
        return;
}

//send reply
void EpollClientLoginSlave::postReply(const quint8 &queryNumber,const char *data,const int &size)
{
    if(!ProtocolParsingBase::postReplyData(queryNumber,data,size))
    {
        normalOutput(QStringLiteral("can't' send reply: postReply(%1,%2)").arg(queryNumber).arg(QString(QByteArray(data,size).toHex())));
        return;
    }
}
