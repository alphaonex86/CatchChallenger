#include "EpollClientLoginMaster.h"
#include "EpollServerLoginMaster.h"

#include <iostream>
#include <string>
#include <time.h>

using namespace CatchChallenger;

EpollClientLoginMaster::EpollClientLoginMaster(
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
        stat(None),
        uniqueKey(0),
        charactersGroupForGameServer(NULL),
        charactersGroupForGameServerInformation(NULL)
{
    rng.seed(time(0));
}

EpollClientLoginMaster::~EpollClientLoginMaster()
{
    if(stat==EpollClientLoginMasterStat::LoginServer)
    {
        vectorremoveOne(EpollClientLoginMaster::loginServers,this);
        unsigned int index=0;
        while(index<EpollClientLoginMaster::gameServers.size())
        {
            unsigned int sub_index=0;
            while(sub_index<EpollClientLoginMaster::gameServers.at(index)->loginServerReturnForCharaterSelect.size())
            {
                const DataForSelectedCharacterReturn &dataForSelectedCharacterReturn=EpollClientLoginMaster::gameServers.at(index)->loginServerReturnForCharaterSelect.at(sub_index);
                if(dataForSelectedCharacterReturn.loginServer==this)
                    EpollClientLoginMaster::gameServers.at(index)->loginServerReturnForCharaterSelect[sub_index].loginServer=NULL;
                sub_index++;
            }
            index++;
        }
    }
    if(stat==EpollClientLoginMasterStat::GameServer)
    {
        vectorremoveOne(EpollClientLoginMaster::gameServers,this);
        charactersGroupForGameServer->removeGameServerUniqueKey(this);
        unsigned int index=0;
        while(index<loginServerReturnForCharaterSelect.size())
        {
            const DataForSelectedCharacterReturn &dataForSelectedCharacterReturn=loginServerReturnForCharaterSelect.at(index);
            if(dataForSelectedCharacterReturn.loginServer!=NULL)
            {
                charactersGroupForGameServer->unlockTheCharacter(dataForSelectedCharacterReturn.characterId);
                charactersGroupForGameServerInformation->lockedAccount.erase(dataForSelectedCharacterReturn.characterId);
                dataForSelectedCharacterReturn.loginServer->selectCharacter_ReturnFailed(dataForSelectedCharacterReturn.client_query_id,0x04);
            }
            index++;
        }
        EpollServerLoginMaster::epollServerLoginMaster->doTheServerList();
        EpollServerLoginMaster::epollServerLoginMaster->doTheReplyCache();
        EpollClientLoginMaster::broadcastGameServerChange();
    }
    if(EpollClientLoginMaster::lastSizeDisplayGameServers!=gameServers.size() || EpollClientLoginMaster::lastSizeDisplayLoginServers!=loginServers.size())
    {
        EpollClientLoginMaster::lastSizeDisplayGameServers=gameServers.size();
        EpollClientLoginMaster::lastSizeDisplayLoginServers=loginServers.size();
        std::cout << "Online: " << EpollClientLoginMaster::lastSizeDisplayLoginServers << " login server and " << EpollClientLoginMaster::lastSizeDisplayGameServers << " game server" << std::endl;
    }
}

void EpollClientLoginMaster::disconnectClient()
{
    epollSocket.close();
    messageParsingLayer("Disconnected client");
}

//input/ouput layer
void EpollClientLoginMaster::errorParsingLayer(const std::string &error)
{
    std::cerr << socketString/* << ": " already concat to improve the performance*/ << error << std::endl;
    disconnectClient();
}

void EpollClientLoginMaster::messageParsingLayer(const std::string &message) const
{
    std::cout << socketString/* << ": " already concat to improve the performance*/ << message << std::endl;
}

void EpollClientLoginMaster::errorParsingLayer(const char * const error)
{
    std::cerr << socketString/* << ": " already concat to improve the performance*/ << error << std::endl;
    disconnectClient();
}

void EpollClientLoginMaster::messageParsingLayer(const char * const message) const
{
    std::cout << socketString/* << ": " already concat to improve the performance*/ << message << std::endl;
}

BaseClassSwitch::EpollObjectType EpollClientLoginMaster::getType() const
{
    return BaseClassSwitch::EpollObjectType::Client;
}

void EpollClientLoginMaster::parseIncommingData()
{
    ProtocolParsingInputOutput::parseIncommingData();
}

void EpollClientLoginMaster::selectCharacter(const uint8_t &query_id,const uint32_t &serverUniqueKey,const uint8_t &charactersGroupIndex,const uint32_t &characterId,const uint32_t &accountId)
{
    if(charactersGroupIndex>=CharactersGroup::list.size())
    {
        errorParsingLayer("EpollClientLoginMaster::selectCharacter() charactersGroupIndex is out of range");
        return;
    }
    if(!CharactersGroup::list.at(charactersGroupIndex)->containsGameServerUniqueKey(serverUniqueKey))
    {
        //send the network reply
        removeFromQueryReceived(query_id);
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
        posOutput+=1+4;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)0x05;
        posOutput+=1;

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        return;
    }
    if(CharactersGroup::list.at(charactersGroupIndex)->characterIsLocked(characterId))
    {
        //send the network reply
        removeFromQueryReceived(query_id);
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
        posOutput+=1+4;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)0x03;
        posOutput+=1;

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        return;
    }
    EpollClientLoginMaster * gameServer=static_cast<EpollClientLoginMaster *>(CharactersGroup::list.at(charactersGroupIndex)->gameServers.at(serverUniqueKey).link);
    if(!gameServer->trySelectCharacterGameServer(this,query_id,serverUniqueKey,charactersGroupIndex,characterId,accountId))
    {
        //send the network reply
        removeFromQueryReceived(query_id);
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
        posOutput+=1+4;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)0x05;
        posOutput+=1;

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        return;
    }
    CharactersGroup::list[charactersGroupIndex]->lockTheCharacter(characterId);
    gameServer->charactersGroupForGameServerInformation->lockedAccount.insert(characterId);
}

bool EpollClientLoginMaster::trySelectCharacterGameServer(EpollClientLoginMaster * const loginServer,const uint8_t &client_query_id,const uint32_t &serverUniqueKey,const uint8_t &charactersGroupIndex,const uint32_t &characterId, const uint32_t &accountId)
{
    //here you are on game server link

    //check if the characterId is linked to the correct account on login server
    if(queryNumberList.empty())
    {
        std::cerr << "queryNumberList.empty() on game server to server it: "
                  << charactersGroupForGameServerInformation->uniqueKey
                  << ", host: "
                  << charactersGroupForGameServerInformation->host
                  << ":"
                  << charactersGroupForGameServerInformation->port
                  << std::endl;
        return false;
    }
    DataForSelectedCharacterReturn dataForSelectedCharacterReturn;
    dataForSelectedCharacterReturn.loginServer=loginServer;
    dataForSelectedCharacterReturn.client_query_id=client_query_id;
    dataForSelectedCharacterReturn.serverUniqueKey=serverUniqueKey;
    dataForSelectedCharacterReturn.charactersGroupIndex=charactersGroupIndex;
    dataForSelectedCharacterReturn.characterId=characterId;
    loginServerReturnForCharaterSelect.push_back(dataForSelectedCharacterReturn);
    //the data
    const uint8_t &queryNumber=queryNumberList.back();
    registerOutputQuery(queryNumber,0xF8);
    EpollClientLoginMaster::getTokenForCharacterSelect[0x01]=queryNumber;
    *reinterpret_cast<uint32_t *>(EpollClientLoginMaster::getTokenForCharacterSelect+0x02)=htole32(characterId);
    *reinterpret_cast<uint32_t *>(EpollClientLoginMaster::getTokenForCharacterSelect+0x06)=htole32(accountId);
    queryNumberList.pop_back();
    return sendRawBlock(reinterpret_cast<char *>(EpollClientLoginMaster::getTokenForCharacterSelect),sizeof(EpollClientLoginMaster::getTokenForCharacterSelect));
}

void EpollClientLoginMaster::selectCharacter_ReturnToken(const uint8_t &query_id,const char * const token)
{
    //send the network reply
    removeFromQueryReceived(query_id);
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
    posOutput+=1+4;
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER);//set the dynamic size

    memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,token,CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER);
    posOutput+=CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER;

    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void EpollClientLoginMaster::selectCharacter_ReturnFailed(const uint8_t &query_id,const uint8_t &errorCode)
{
    //send the network reply
    removeFromQueryReceived(query_id);
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
    posOutput+=1+4;
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size

    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=errorCode;
    posOutput+=1;

    //you are on login server
    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void EpollClientLoginMaster::broadcastGameServerChange()
{
    unsigned int index=0;
    while(index<EpollClientLoginMaster::loginServers.size())
    {
        EpollClientLoginMaster * const loginServer=EpollClientLoginMaster::loginServers.at(index);
        loginServer->internalSendRawSmallPacket(EpollClientLoginMaster::serverServerList,EpollClientLoginMaster::serverServerListSize);
        index++;
    }
}

bool EpollClientLoginMaster::sendRawBlock(const char * const data,const int &size)
{
    return internalSendRawSmallPacket(data,size);
}

void EpollClientLoginMaster::sendCurrentPlayer()
{
    if(!currentPlayerForGameServerToUpdate)
        return;
    if(EpollClientLoginMaster::gameServers.size()==0)
        return;
    //do the list
    uint32_t posOutput=0;
    {
        //send the network message

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x47;
        posOutput+=1+4;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(EpollClientLoginMaster::gameServers.size()*2);//set the dynamic size

        unsigned int index=0;
        while(index<EpollClientLoginMaster::gameServers.size())
        {
            const EpollClientLoginMaster * const gameServer=EpollClientLoginMaster::gameServers.at(index);
            *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(gameServer->charactersGroupForGameServerInformation->currentPlayer);
            posOutput+=2;
            index++;
        }
    }
    //broadcast all
    {
        unsigned int index=0;
        while(index<EpollClientLoginMaster::loginServers.size())
        {
            EpollClientLoginMaster * const loginServer=EpollClientLoginMaster::loginServers.at(index);
            loginServer->internalSendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
            index++;
        }
    }
}

void EpollClientLoginMaster::disconnectForDuplicateConnexionDetected(const uint32_t &characterId)
{
    *reinterpret_cast<uint32_t *>(EpollClientLoginMaster::duplicateConnexionDetected+0x02)=htole32(characterId);
    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::duplicateConnexionDetected),sizeof(EpollClientLoginMaster::duplicateConnexionDetected));
}
