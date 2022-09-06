#include "EpollClientLoginSlave.hpp"
#include "LinkToGameServer.hpp"
#include "LinkToMaster.hpp"
#include "CharactersGroupForLogin.hpp"
#include "../base/BaseServerLogin.hpp"

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
        EpollClient(infd),
        ProtocolParsingInputOutput(
           #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            PacketModeTransmission_Server
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
    lastActivity=LinkToGameServer::msFrom1970();
}

EpollClientLoginSlave::~EpollClientLoginSlave()
{
    //std::cerr << "EpollClientLoginSlave::~EpollClientLoginSlave() " << this << " (" << infd << ")" << std::endl;
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
        delete[] socketString;
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

uint64_t EpollClientLoginSlave::get_lastActivity() const
{
    return lastActivity;
}

bool EpollClientLoginSlave::disconnectClient()
{
    //std::cerr << "EpollClientLoginSlave::disconnectClient() " << this << " (" << infd << ")" << std::endl;
    if(linkToGameServer!=NULL)
    {
        linkToGameServer->closeSocket();
        //break the link
        linkToGameServer->client=NULL;
        linkToGameServer=NULL;
    }
    EpollClient::close();

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
    return true;
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
    lastActivity=LinkToGameServer::msFrom1970();
    ProtocolParsingInputOutput::parseIncommingData();
}

bool EpollClientLoginSlave::removeFromQueryReceived(const uint8_t &queryNumber)
{
    return ProtocolParsingBase::removeFromQueryReceived(queryNumber);
}

void EpollClientLoginSlave::breakNeedMoreData()
{
    std::cerr << "EpollClientLoginSlave::breakNeedMoreData()" << std::endl;
    if(stat==EpollClientLoginStat::None)
    {
        disconnectClient();
        return;
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    std::cerr << "Break due to need more in parse data" << std::endl;
    #endif
}

ssize_t EpollClientLoginSlave::read(char * data, const size_t &size)
{
    lastActivity=LinkToGameServer::msFrom1970();
    return EpollClient::read(data,size);
}

ssize_t EpollClientLoginSlave::write(const char * const data, const size_t &size)
{
    lastActivity=LinkToGameServer::msFrom1970();
    //do some basic check on low level protocol (message split, ...)
    if(ProtocolParsingInputOutput::write(data,size)<0)
        return -1;
    return EpollClient::write(data,size);
}

void EpollClientLoginSlave::closeSocket()
{
    disconnectClient();
}
