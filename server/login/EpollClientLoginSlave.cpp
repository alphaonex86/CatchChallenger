#include "EpollClientLoginSlave.h"
#include "CharactersGroupForLogin.h"

#include <iostream>
#include <std::string>

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
               CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE*sizeof(uint8_t));
        memset(chatPacketKick,
               0x00,
               CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE*sizeof(uint8_t));
        memset(otherPacketKick,
               0x00,
               CATCHCHALLENGER_DDOS_COMPUTERAVERAGEVALUE*sizeof(uint8_t));
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
    {
        //SQL
        int index=0;
        while(index<callbackRegistred.size())
        {
            callbackRegistred.at(index)->object=NULL;
            index++;
        }

        //from master
        index=0;
        while(index<CharactersGroupForLogin::list.size())
        {
            CharactersGroupForLogin * const charactersGroupForLogin=CharactersGroupForLogin::list.at(index);

            int sub_index=0;
            while(sub_index<charactersGroupForLogin->clientQueryForReadReturn.size())
            {
                if(charactersGroupForLogin->clientQueryForReadReturn.at(sub_index)==this)
                    charactersGroupForLogin->clientQueryForReadReturn[sub_index]=NULL;
                sub_index++;
            }
            sub_index=0;
            while(sub_index<charactersGroupForLogin->addCharacterParamList.size())
            {
                if(charactersGroupForLogin->addCharacterParamList.at(sub_index).client==this)
                    charactersGroupForLogin->addCharacterParamList[sub_index].client=NULL;
                sub_index++;
            }
            sub_index=0;
            while(sub_index<charactersGroupForLogin->removeCharacterParamList.size())
            {
                if(charactersGroupForLogin->removeCharacterParamList.at(sub_index).client==this)
                    charactersGroupForLogin->removeCharacterParamList[sub_index].client=NULL;
                sub_index++;
            }

            index++;
        }

        //selected char
        /// \todo check by crash with ASSERT failure in std::unordered_map: "Iterating beyond end()"
        {
            std::unordered_mapIterator<uint8_t/*queryNumber*/,LinkToMaster::DataForSelectedCharacterReturn> j(LinkToMaster::linkToMaster->selectCharacterClients);
            while (j.hasNext()) {
                j.next();
                if(j.value().client==this)
                    LinkToMaster::linkToMaster->selectCharacterClients[j.key()].client=NULL;
            }
        }
    }
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
void EpollClientLoginSlave::errorParsingLayer(const std::string &error)
{
    std::cerr << socketString << ": " << error.toLocal8Bit().constData() << std::endl;
    disconnectClient();
}

void EpollClientLoginSlave::messageParsingLayer(const std::string &message) const
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

BaseClassSwitch::EpollObjectType EpollClientLoginSlave::getType() const
{
    return BaseClassSwitch::EpollObjectType::Client;
}

void EpollClientLoginSlave::parseIncommingData()
{
    ProtocolParsingInputOutput::parseIncommingData();
}
