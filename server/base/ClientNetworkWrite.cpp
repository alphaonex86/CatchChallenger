#include "ClientNetworkWrite.h"

using namespace Pokecraft;

ClientNetworkWrite::ClientNetworkWrite(Player_internal_informations *player_informations,ConnectedSocket * socket) :
    ProtocolParsingOutput(socket,PacketModeTransmission_Server)
{
    this->socket=socket;
    this->player_informations=player_informations;
}

ClientNetworkWrite::~ClientNetworkWrite()
{
}

/* not use mainCodeWithoutSubCodeTypeServerToClient because the reply have unknow code */
void ClientNetworkWrite::sendPacket(const quint8 &mainCodeType,const quint16 &subCodeType,const QByteArray &data)
{
    if(!player_informations->isConnected)
    {
        emit message(QString("sendPacket(%1,%2,%3) when is not connected").arg(mainCodeType).arg(subCodeType).arg(QString(data.toHex())));
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    emit message(QString("sendPacket(%1,%2,%3)").arg(mainCodeType).arg(subCodeType).arg(QString(data.toHex())));
    #endif
    if(!ProtocolParsingOutput::packOutcommingData(mainCodeType,subCodeType,data))
        return;
    if(!socket->isValid())
    {
        emit error("device is not valid at sendPacket(mainCodeType,subCodeType)");
        return;
    }
}

void ClientNetworkWrite::sendPacket(const quint8 &mainCodeType,const QByteArray &data)
{
    if(!player_informations->isConnected)
    {
        emit message(QString("sendPacket(%1,%3) when is not connected").arg(mainCodeType).arg(QString(data.toHex())));
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    emit message(QString("sendPacket(%1,%2)").arg(mainCodeType).arg(QString(data.toHex())));
    #endif
    if(!ProtocolParsingOutput::packOutcommingData(mainCodeType,data))
        return;
    if(!socket->isValid())
    {
        emit error("device is not valid at sendPacket(mainCodeType)");
        return;
    }
}

//send reply
void ClientNetworkWrite::postReply(const quint8 &queryNumber,const QByteArray &data)
{
    if(!player_informations->isConnected)
    {
        emit message(QString("postReply(%1,%2) when is not connected").arg(queryNumber).arg(QString(data.toHex())));
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    emit message(QString("postReply(%1,%2)").arg(queryNumber).arg(QString(data.toHex())));
    #endif
    if(!ProtocolParsingOutput::postReplyData(queryNumber,data))
        return;
    if(!socket->isValid())
    {
        emit error("device is not valid at postReply()");
        return;
    }
}

void ClientNetworkWrite::askIfIsReadyToStop()
{
    emit isReadyToStop();
}
