#include "Client.h"

using namespace CatchChallenger;

/* not use mainCodeWithoutSubCodeTypeServerToClient because the reply have unknow code */
void Client::sendFullPacket(const uint8_t &mainCodeType,const uint8_t &subCodeType)
{
    sendFullPacket(mainCodeType,subCodeType,NULL,0);
}

void Client::sendFullPacket(const uint8_t &mainCodeType,const uint8_t &subCodeType,const char * const data,const unsigned int &size)
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(!isConnected)
    {
        normalOutput(std::stringLiteral("sendPacket("+std::to_string()+","+std::to_string()+","+std::to_string()+") when is not connected").arg(mainCodeType).arg(subCodeType).arg(std::string(QByteArray(data,size).toHex())));
        return;
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    normalOutput("sendPacket("+std::to_string(mainCodeType)+","+std::to_string(subCodeType)+","+QString(QByteArray(data,size).toHex()).toStdString()+")");
    #endif
    if(!ProtocolParsingBase::packFullOutcommingData(mainCodeType,subCodeType,data,size))
        return;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(!socket->isValid())
    {
        errorOutput("device is not valid at sendPacket(mainCodeType,subCodeType)");
        return;
    }
    #endif
}

void Client::sendPacket(const uint8_t &mainCodeType)
{
    sendPacket(mainCodeType,NULL,0);
}

void Client::sendPacket(const uint8_t &mainCodeType,const char * const data,const unsigned int &size)
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(!isConnected)
    {
        normalOutput(std::stringLiteral("sendPacket("+std::to_string()+","+std::to_string()+") when is not connected").arg(mainCodeType).arg(std::string(QByteArray(data,size).toHex())));
        return;
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    normalOutput("sendPacket("+std::to_string(mainCodeType)+","+QString(QByteArray(data,size).toHex()).toStdString()+")");
    #endif
    if(!ProtocolParsingBase::packOutcommingData(mainCodeType,data,size))
        return;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(!socket->isValid())
    {
        errorOutput("device is not valid at sendPacket(mainCodeType)");
        return;
    }
    #endif
}

void Client::sendRawSmallPacket(const char * const data,const unsigned int &size)
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(!isConnected)
    {
        normalOutput(std::stringLiteral("sendRawSmallPacket("+std::to_string()+") when is not connected").arg(std::string(QByteArray(data,size).toHex())));
        return;
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    normalOutput("sendRawSmallPacket("+QString(QByteArray(data,size).toHex()).toStdString()+")");
    #endif
    if(!ProtocolParsingBase::internalSendRawSmallPacket(data,size))
        return;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(!socket->isValid())
    {
        errorOutput("device is not valid at sendPacket(mainCodeType)");
        return;
    }
    #endif
}

void Client::sendQuery(const uint8_t &mainIdent,const uint8_t &subIdent,const uint8_t &queryNumber)
{
    sendQuery(mainIdent,subIdent,queryNumber,NULL,0);
}

void Client::sendQuery(const uint8_t &mainIdent,const uint8_t &subIdent,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(!isConnected)
    {
        normalOutput(std::stringLiteral("sendQuery("+std::to_string()+","+std::to_string()+","+std::to_string()+","+std::to_string()+") when is not connected").arg(mainIdent).arg(subIdent).arg(queryNumber).arg(std::string(QByteArray(data,size).toHex())));
        return;
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    normalOutput("sendQuery("+std::to_string(mainIdent)+","+std::to_string(subIdent)+","+QString(QByteArray(data,size).toHex()).toStdString()+")");
    #endif
    if(!ProtocolParsingBase::packFullOutcommingQuery(mainIdent,subIdent,queryNumber,data,size))
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
void Client::postReply(const uint8_t &queryNumber)
{
    postReply(queryNumber,NULL,0);
}

void Client::postReply(const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(!isConnected)
    {
        normalOutput(std::stringLiteral("postReply("+std::to_string()+","+std::to_string()+") when is not connected").arg(queryNumber).arg(std::string(QByteArray(data,size).toHex())));
        return;
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    normalOutput("postReply("+std::to_string(queryNumber)+","+QString(QByteArray(data,size).toHex()).toStdString()+")");
    #endif
    if(!ProtocolParsingBase::postReplyData(queryNumber,data,size))
    {
        normalOutput("can't send reply: postReply("+std::to_string(queryNumber)+","+QString(QByteArray(data,size).toHex()).toStdString()+")");
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
