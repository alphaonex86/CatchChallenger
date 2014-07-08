#include "Client.h"

using namespace CatchChallenger;

/* not use mainCodeWithoutSubCodeTypeServerToClient because the reply have unknow code */
void Client::sendFullPacket(const quint8 &mainCodeType,const quint16 &subCodeType,const QByteArray &data)
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(!isConnected)
    {
        normalOutput(QStringLiteral("sendPacket(%1,%2,%3) when is not connected").arg(mainCodeType).arg(subCodeType).arg(QString(data.toHex())));
        return;
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    normalOutput(QStringLiteral("sendPacket(%1,%2,%3)").arg(mainCodeType).arg(subCodeType).arg(QString(data.toHex())));
    #endif
    if(!ProtocolParsingInputOutput::packFullOutcommingData(mainCodeType,subCodeType,data.constData(),data.size()))
        return;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(!socket->isValid())
    {
        errorOutput("device is not valid at sendPacket(mainCodeType,subCodeType)");
        return;
    }
    #endif
}

void Client::sendPacket(const quint8 &mainCodeType,const QByteArray &data)
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(!isConnected)
    {
        normalOutput(QStringLiteral("sendPacket(%1,%2) when is not connected").arg(mainCodeType).arg(QString(data.toHex())));
        return;
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    normalOutput(QStringLiteral("sendPacket(%1,%2)").arg(mainCodeType).arg(QString(data.toHex())));
    #endif
    if(!ProtocolParsingInputOutput::packOutcommingData(mainCodeType,data.constData(),data.size()))
        return;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(!socket->isValid())
    {
        errorOutput("device is not valid at sendPacket(mainCodeType)");
        return;
    }
    #endif
}

void Client::sendRawSmallPacket(const char *data,const int &size)
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(!isConnected)
    {
        normalOutput(QStringLiteral("sendRawSmallPacket(%1) when is not connected").arg(QString(QByteArray(data,size).toHex())));
        return;
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    normalOutput(QStringLiteral("sendRawSmallPacket(%1)").arg(QString(QByteArray(data,size).toHex())));
    #endif
    if(!ProtocolParsingInputOutput::internalSendRawSmallPacket(data,size))
        return;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(!socket->isValid())
    {
        errorOutput("device is not valid at sendPacket(mainCodeType)");
        return;
    }
    #endif
}

void Client::sendRawSmallPacket(const QByteArray &data)
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(!isConnected)
    {
        normalOutput(QStringLiteral("sendRawSmallPacket(%1) when is not connected").arg(QString(data.toHex())));
        return;
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    normalOutput(QStringLiteral("sendRawSmallPacket(%1)").arg(QString(data.toHex())));
    #endif
    if(!ProtocolParsingInputOutput::internalSendRawSmallPacket(data.constData(),data.size()))
        return;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(!socket->isValid())
    {
        errorOutput("device is not valid at sendPacket(mainCodeType)");
        return;
    }
    #endif
}

void Client::sendQuery(const quint8 &mainIdent,const quint16 &subIdent,const quint8 &queryNumber,const QByteArray &data)
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(!isConnected)
    {
        normalOutput(QStringLiteral("sendQuery(%1,%2,%3,%4) when is not connected").arg(mainIdent).arg(subIdent).arg(queryNumber).arg(QString(data.toHex())));
        return;
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    normalOutput(QStringLiteral("sendQuery(%1,%2,%3)").arg(mainIdent).arg(subIdent).arg(QString(data.toHex())));
    #endif
    if(!ProtocolParsingInputOutput::packFullOutcommingQuery(mainIdent,subIdent,queryNumber,data.constData(),data.size()))
        return;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(!socket->isValid())
    {
        errorOutput("device is not valid at sendPacket(mainCodeType)");
        return;
    }
    #endif
}

//send reply
void Client::postReply(const quint8 &queryNumber,const QByteArray &data)
{
    postReply(queryNumber,data.constData(),data.size());
}

void Client::postReply(const quint8 &queryNumber,const char *data,const int &size)
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(!isConnected)
    {
        normalOutput(QStringLiteral("postReply(%1,%2) when is not connected").arg(queryNumber).arg(QString(QByteArray(data,size).toHex())));
        return;
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    normalOutput(QStringLiteral("postReply(%1,%2)").arg(queryNumber).arg(QString(QByteArray(data,size).toHex())));
    #endif
    if(!ProtocolParsingInputOutput::postReplyData(queryNumber,data,size))
    {
        normalOutput(QStringLiteral("can't' send reply: postReply(%1,%2)").arg(queryNumber).arg(QString(QByteArray(data,size).toHex())));
        return;
    }
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(!socket->isValid())
    {
        errorOutput("device is not valid at postReply()");
        return;
    }
    #endif
}
