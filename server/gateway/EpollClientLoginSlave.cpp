#include "EpollClientLoginSlave.h"
#include "CharactersGroupForLogin.h"

#include <iostream>
#include <QString>

using namespace CatchChallenger;

EpollClientLoginSlave::EpollClientLoginSlave(
        #ifdef SERVERSSL
            const int &infd, SSL_CTX *ctx
        #else
            const int &infd
        #endif
        ) :
        ProtocolParsingInputOutput(
            #ifdef SERVERSSL
                infd,ctx
            #else
                infd
            #endif
           #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            ,PacketModeTransmission_Server
            #endif
            ),
        stat(EpollClientLoginStat::None),
        linkToGameServer(NULL),
        charactersGroupIndex(0),
        serverUniqueKey(0),
        socketString(NULL),
        socketStringSize(0),
        account_id(0),
        characterListForReplyInSuspend(0),
        serverListForReplyRawData(NULL),
        serverListForReplyRawDataSize(0),
        serverListForReplyInSuspend(0),
        movePacketKickTotalCache(0),
        movePacketKickNewValue(0),
        chatPacketKickTotalCache(0),
        chatPacketKickNewValue(0),
        otherPacketKickTotalCache(0),
        otherPacketKickNewValue(0)
{
    {
        memset(movePacketKick,
               0x00,
               CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE*sizeof(quint8));
        memset(chatPacketKick,
               0x00,
               CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE*sizeof(quint8));
        memset(otherPacketKick,
               0x00,
               CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE*sizeof(quint8));
    }
    client_list.push_back(this);
}

EpollClientLoginSlave::~EpollClientLoginSlave()
{
    {
        unsigned int index=0;
        while(index<client_list.size())
        {
            const EpollClientLoginSlave * const client=client_list.at(index);
            if(this==client)
            {
                client_list.erase(client_list.begin()+index);
                break;
            }
            index++;
        }
    }

    if(socketString!=NULL)
        delete socketString;
    if(linkToGameServer!=NULL)
    {
        linkToGameServer->closeSocket();
        //break the link
        linkToGameServer->client=NULL;
        linkToGameServer=NULL;
    }
}

void EpollClientLoginSlave::disconnectClient()
{
    if(linkToGameServer!=NULL)
    {
        linkToGameServer->closeSocket();
        //break the link
        linkToGameServer->client=NULL;
        linkToGameServer=NULL;
    }
    epollSocket.close();
    messageParsingLayer("Disconnected client");
}

//input/ouput layer
void EpollClientLoginSlave::errorParsingLayer(const QString &error)
{
    std::cerr << socketString << ": " << error.toLocal8Bit().constData() << std::endl;
    disconnectClient();
}

void EpollClientLoginSlave::messageParsingLayer(const QString &message) const
{
    std::cout << socketString << ": " << message.toLocal8Bit().constData() << std::endl;
}

void EpollClientLoginSlave::errorParsingLayer(const char * const error)
{
    std::cerr << socketString << ": " << error << std::endl;
    disconnectClient();
}

void EpollClientLoginSlave::messageParsingLayer(const char * const message) const
{
    std::cout << socketString << ": " << message << std::endl;
}

BaseClassSwitch::Type EpollClientLoginSlave::getType() const
{
    return BaseClassSwitch::Type::Client;
}

void EpollClientLoginSlave::parseIncommingData()
{
    ProtocolParsingInputOutput::parseIncommingData();
}
