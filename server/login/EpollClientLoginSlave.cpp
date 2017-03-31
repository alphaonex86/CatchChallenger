#include "EpollClientLoginSlave.h"
#include "CharactersGroupForLogin.h"
#include "../base/BaseServerLogin.h"

#include <iostream>
#include <string>
#include <cstring>

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
        serverListForReplyInSuspend(0)
{
    client_list.push_back(this);
}

EpollClientLoginSlave::~EpollClientLoginSlave()
{
    if(stat!=EpollClientLoginStat::LoggedStatClient)
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
    else
    {
        unsigned int index=0;
        while(index<stat_client_list.size())
        {
            const EpollClientLoginSlave * const client=stat_client_list.at(index);
            if(this==client)
            {
                stat_client_list.erase(stat_client_list.begin()+index);
                break;
            }
            index++;
        }
    }

    if(socketString!=NULL)
        delete socketString;
    {
        unsigned int index=0;

        //SQL
        {
            while(!callbackRegistred.empty())
            {
                callbackRegistred.front()->object=NULL;
                callbackRegistred.pop();
            }
        }

        //from master
        index=0;
        while(index<CharactersGroupForLogin::list.size())
        {
            CharactersGroupForLogin * const charactersGroupForLogin=CharactersGroupForLogin::list.at(index);

            unsigned int sub_index=0;
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
            auto j=LinkToMaster::linkToMaster->selectCharacterClients.begin();
            while(j!=LinkToMaster::linkToMaster->selectCharacterClients.cend())
            {
                if(j->second.client==this)
                    LinkToMaster::linkToMaster->selectCharacterClients[j->first].client=NULL;
                ++j;
            }
        }

        //crash with heap-buffer-overflow if not flush before the end of destructor
    }
    disconnectClient();
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

    {
        uint32_t index=0;
        while(index<BaseServerLogin::tokenForAuthSize)
        {
            const BaseServerLogin::TokenLink &tokenLink=BaseServerLogin::tokenForAuth[index];
            if(tokenLink.client==this)
            {
                BaseServerLogin::tokenForAuthSize--;
                if(BaseServerLogin::tokenForAuthSize>0)
                {
                    while(index<BaseServerLogin::tokenForAuthSize)
                    {
                        BaseServerLogin::tokenForAuth[index]=BaseServerLogin::tokenForAuth[index+1];
                        index++;
                    }
                    //don't work:memmove(BaseServerLogin::tokenForAuth+index*sizeof(TokenLink),BaseServerLogin::tokenForAuth+index*sizeof(TokenLink)+sizeof(TokenLink),sizeof(TokenLink)*(BaseServerLogin::tokenForAuthSize-index));
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(BaseServerLogin::tokenForAuth[0].client==NULL)
                        abort();
                    #endif
                }
                break;
            }
            index++;
        }
    }
}

//input/ouput layer
void EpollClientLoginSlave::errorParsingLayer(const std::string &error)
{
    if(stat==EpollClientLoginStat::None)
    {
        //std::cerr << headerOutput() << "Kicked by: " << errorString << std::endl;//silent if protocol not passed, to not flood the log if other client like http client (browser) is connected
        disconnectClient();
        return;
    }
    std::cerr << socketString << ": " << error << std::endl;
    disconnectClient();
}

void EpollClientLoginSlave::messageParsingLayer(const std::string &message) const
{
    std::cout << socketString << ": " << message << std::endl;
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

bool EpollClientLoginSlave::removeFromQueryReceived(const uint8_t &queryNumber)
{
    return ProtocolParsingBase::removeFromQueryReceived(queryNumber);
}

void EpollClientLoginSlave::breakNeedMoreData()
{
    if(stat==EpollClientLoginStat::None)
    {
        disconnectClient();
        return;
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    //std::cerr << "Break due to need more in parse data" << std::endl;
    #endif
}
