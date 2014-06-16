#include "Client.h"

using namespace CatchChallenger;

/* not use mainCodeWithoutSubCodeTypeServerToClient because the reply have unknow code */
void Client::sendFullPacket(const quint8 &mainCodeType,const quint16 &subCodeType,const QByteArray &data)
{
    if(!isConnected)
    {
        normalOutput(QStringLiteral("sendPacket(%1,%2,%3) when is not connected").arg(mainCodeType).arg(subCodeType).arg(QString(data.toHex())));
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    normalOutput(QStringLiteral("sendPacket(%1,%2,%3)").arg(mainCodeType).arg(subCodeType).arg(QString(data.toHex())));
    #endif
    if(!ProtocolParsingInputOutput::packFullOutcommingData(mainCodeType,subCodeType,data))
        return;
    if(!socket->isValid())
    {
        errorOutput("device is not valid at sendPacket(mainCodeType,subCodeType)");
        return;
    }
}

void Client::sendPacket(const quint8 &mainCodeType,const QByteArray &data)
{
    if(!isConnected)
    {
        normalOutput(QStringLiteral("sendPacket(%1,%2) when is not connected").arg(mainCodeType).arg(QString(data.toHex())));
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    normalOutput(QStringLiteral("sendPacket(%1,%2)").arg(mainCodeType).arg(QString(data.toHex())));
    #endif
    if(!ProtocolParsingInputOutput::packOutcommingData(mainCodeType,data))
        return;
    if(!socket->isValid())
    {
        errorOutput("device is not valid at sendPacket(mainCodeType)");
        return;
    }
}

void Client::sendRawSmallPacket(const QByteArray &data)
{
    if(!isConnected)
    {
        normalOutput(QStringLiteral("sendRawSmallPacket(%1) when is not connected").arg(QString(data.toHex())));
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    normalOutput(QStringLiteral("sendRawSmallPacket(%1)").arg(QString(data.toHex())));
    #endif
    if(!ProtocolParsingInputOutput::internalSendRawSmallPacket(data))
        return;
    if(!socket->isValid())
    {
        errorOutput("device is not valid at sendPacket(mainCodeType)");
        return;
    }
}

void Client::sendQuery(const quint8 &mainIdent,const quint16 &subIdent,const quint8 &queryNumber,const QByteArray &data)
{
    if(!isConnected)
    {
        normalOutput(QStringLiteral("sendQuery(%1,%2,%3,%4) when is not connected").arg(mainIdent).arg(subIdent).arg(queryNumber).arg(QString(data.toHex())));
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    normalOutput(QStringLiteral("sendQuery(%1,%2,%3)").arg(mainIdent).arg(subIdent).arg(QString(data.toHex())));
    #endif
    if(!ProtocolParsingInputOutput::packFullOutcommingQuery(mainIdent,subIdent,queryNumber,data))
        return;
    if(!socket->isValid())
    {
        errorOutput("device is not valid at sendPacket(mainCodeType)");
        return;
    }
}

//send reply
void Client::postReply(const quint8 &queryNumber,const QByteArray &data)
{
    if(!isConnected)
    {
        normalOutput(QStringLiteral("postReply(%1,%2) when is not connected").arg(queryNumber).arg(QString(data.toHex())));
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    normalOutput(QStringLiteral("postReply(%1,%2)").arg(queryNumber).arg(QString(data.toHex())));
    #endif
    if(!ProtocolParsingInputOutput::postReplyData(queryNumber,data))
    {
        normalOutput(QStringLiteral("can't' send reply: postReply(%1,%2)").arg(queryNumber).arg(QString(data.toHex())));
        return;
    }
    if(!socket->isValid())
    {
        errorOutput("device is not valid at postReply()");
        return;
    }
}
