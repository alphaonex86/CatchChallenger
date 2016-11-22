#include "EpollClientLoginMaster.h"
#include "EpollServerLoginMaster.h"
#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/FacilityLibGeneral.h"

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
        charactersGroupForGameServerInformation(NULL),
        inConflicWithTheMainServer(NULL),
        queryNumberInConflicWithTheMainServer(0)
{
    rng.seed(time(0));
}

EpollClientLoginMaster::~EpollClientLoginMaster()
{
    resetToDisconnect();
}

void EpollClientLoginMaster::resetToDisconnect()
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
        addToRemoveList();
        vectorremoveOne(EpollClientLoginMaster::gameServers,this);
        passUniqueKeyToNextGameServer();

        //select in progress: abort
        unsigned int index=0;
        while(index<loginServerReturnForCharaterSelect.size())
        {
            const DataForSelectedCharacterReturn &dataForSelectedCharacterReturn=loginServerReturnForCharaterSelect.at(index);
            if(dataForSelectedCharacterReturn.loginServer!=NULL)
            {
                charactersGroupForGameServer->unlockTheCharacter(dataForSelectedCharacterReturn.characterId);
                charactersGroupForGameServerInformation->lockedAccountByGameserver.erase(dataForSelectedCharacterReturn.characterId);
                dataForSelectedCharacterReturn.loginServer->selectCharacter_ReturnFailed(dataForSelectedCharacterReturn.client_query_id,0x04);
            }
            index++;
        }
        charactersGroupForGameServerInformation=NULL;
        EpollServerLoginMaster::epollServerLoginMaster->doTheServerList();
        EpollServerLoginMaster::epollServerLoginMaster->doTheReplyCache();
        //EpollClientLoginMaster::broadcastGameServerChange();
    }
    if(inConflicWithTheMainServer!=NULL)
    {
        vectorremoveOne(EpollClientLoginMaster::gameServers,this);
        vectorremoveOne(inConflicWithTheMainServer->secondServerInConflict,this);
        inConflicWithTheMainServer=NULL;
    }

    stat=EpollClientLoginMasterStat::None;

    updateConsoleCountServer();
}

void EpollClientLoginMaster::passUniqueKeyToNextGameServer()
{
    if(stat!=EpollClientLoginMasterStat::GameServer)
    {
        std::cerr << "Client is not a game server into passUniqueKeyToNextGameServer()" << std::endl;
        return;
    }
    if(charactersGroupForGameServer==NULL)
    {
        std::cerr << "This game server don't have charactersGroupForGameServer==NULL into passUniqueKeyToNextGameServer()" << std::endl;
        return;
    }
    charactersGroupForGameServer->removeGameServerUniqueKey(this);
    if(!secondServerInConflict.empty())
    {
        std::cout << "Server with same unique key in waiting..." << std::endl;

        EpollClientLoginMaster * newServerToBeMaster=secondServerInConflict.front();
        secondServerInConflict.erase(secondServerInConflict.cbegin());

        newServerToBeMaster->sendGameServerRegistrationReply(newServerToBeMaster->queryNumberInConflicWithTheMainServer,false);

        CharactersGroup::InternalGameServer tempData;
        if(newServerToBeMaster->charactersGroupForGameServerInformation!=NULL)
        {
            tempData=*newServerToBeMaster->charactersGroupForGameServerInformation;
            delete newServerToBeMaster->charactersGroupForGameServerInformation;
            newServerToBeMaster->charactersGroupForGameServerInformation=NULL;
        }
        else
        {
            std::cerr << "newServerToBeMaster->charactersGroupForGameServerInformation==NULL at " << __FILE__ << ":" << __LINE__ << std::endl;
            abort();
        }
        newServerToBeMaster->charactersGroupForGameServerInformation=newServerToBeMaster->charactersGroupForGameServer->addGameServerUniqueKey(
                    newServerToBeMaster,tempData.uniqueKey,tempData.host,tempData.port,tempData.metaData,tempData.logicalGroupIndex,tempData.currentPlayer,tempData.maxPlayer,tempData.lockedAccountByGameserver);

        if(!secondServerInConflict.empty())
        {
            std::cout << "Server with same unique key in waiting... and even more" << std::endl;

            newServerToBeMaster->secondServerInConflict=secondServerInConflict;

            unsigned int indexServerToUpdate=0;
            while(indexServerToUpdate<newServerToBeMaster->secondServerInConflict.size())
            {
                EpollClientLoginMaster * const serverToMasterUpdate=newServerToBeMaster->secondServerInConflict.at(indexServerToUpdate);
                serverToMasterUpdate->inConflicWithTheMainServer=newServerToBeMaster;
                indexServerToUpdate++;
            }

            newServerToBeMaster->sendGameServerPing(msFrom1970());
        }
        secondServerInConflict.clear();
    }
}

void EpollClientLoginMaster::disconnectClient()
{
    resetToDisconnect();
    updateConsoleCountServer();
    epollSocket.close();
    //messageParsingLayer("Disconnected client");
}

void EpollClientLoginMaster::updateConsoleCountServer()
{
    /** \warning done by the stats client, better way to do (no logs flood, instant, no text to parse)
    if(EpollClientLoginMaster::lastSizeDisplayGameServers!=gameServers.size() || EpollClientLoginMaster::lastSizeDisplayLoginServers!=loginServers.size())
    {
        EpollClientLoginMaster::lastSizeDisplayGameServers=gameServers.size();
        EpollClientLoginMaster::lastSizeDisplayLoginServers=loginServers.size();
        std::cout << "Online: " << EpollClientLoginMaster::lastSizeDisplayLoginServers << " login server and " << EpollClientLoginMaster::lastSizeDisplayGameServers << " game server" << std::endl;
    }*/
}

//input/ouput layer
void EpollClientLoginMaster::errorParsingLayer(const std::string &error)
{
    if(stat==EpollClientLoginMasterStat::None)
    {
        //std::cerr << headerOutput() << "Kicked by: " << errorString << std::endl;//silent if protocol not passed, to not flood the log if other client like http client (browser) is connected
        disconnectClient();
        return;
    }
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
        //should be filtred by the login server, then very minimal
        {
            errorParsingLayer("!CharactersGroup::list.at("+std::to_string(charactersGroupIndex)+")->containsGameServerUniqueKey("+std::to_string(serverUniqueKey)+"), CharactersGroup name: "+CharactersGroup::list.at(charactersGroupIndex)->name);
            std::cerr << CharactersGroup::list.at(charactersGroupIndex)->gameServerUniqueKeyHumanReadableList() << std::endl;
        }

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
    {
        const CharactersGroup::CharacterLock &lockResult=CharactersGroup::list.at(charactersGroupIndex)->characterIsLocked(characterId);
        if(lockResult!=CharactersGroup::CharacterLock::Unlocked)
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            messageParsingLayer("Lock result wrong: "+std::to_string(lockResult)+" CharactersGroup::list.at("+std::to_string(charactersGroupIndex)+")->characterIsLocked("+std::to_string(characterId)+"), CharactersGroup name: "+CharactersGroup::list.at(charactersGroupIndex)->name);
            #endif
            //send the network reply
            removeFromQueryReceived(query_id);
            uint32_t posOutput=0;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
            posOutput+=1+4;
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(1);//set the dynamic size

            switch(lockResult)
            {
                case CharactersGroup::CharacterLock::RecentlyUnlocked:
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)0x08;
                break;
                case CharactersGroup::CharacterLock::Locked:
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)0x03;
                break;
                default:
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)0x04;
                break;
            }
            posOutput+=1;

            sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
            return;
        }
    }
    EpollClientLoginMaster * gameServer=static_cast<EpollClientLoginMaster *>(CharactersGroup::list.at(charactersGroupIndex)->gameServers.at(serverUniqueKey).link);
    if(!gameServer->trySelectCharacterGameServer(this,query_id,serverUniqueKey,charactersGroupIndex,characterId,accountId))
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        messageParsingLayer("Unable to lock: CharactersGroup::list.at("+std::to_string(charactersGroupIndex)+")->trySelectCharacterGameServer("+std::to_string(characterId)+"), CharactersGroup name: "+CharactersGroup::list.at(charactersGroupIndex)->name+": "+std::to_string(characterId));
        #endif
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
    gameServer->charactersGroupForGameServerInformation->lockedAccountByGameserver.insert(characterId);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    messageParsingLayer("Locked: CharactersGroup::list.at("+std::to_string(charactersGroupIndex)+")->characterIsLocked("+std::to_string(characterId)+"), CharactersGroup name: "+CharactersGroup::list.at(charactersGroupIndex)->name+" on server: "+gameServer->charactersGroupForGameServerInformation->host+":"+std::to_string(gameServer->charactersGroupForGameServerInformation->port)+" "+std::to_string(gameServer->charactersGroupForGameServerInformation->uniqueKey));
    #endif

    //reply send at trySelectCharacterGameServer() above
}

bool EpollClientLoginMaster::trySelectCharacterGameServer(EpollClientLoginMaster * const loginServer,const uint8_t &client_query_id,const uint32_t &serverUniqueKey,const uint8_t &charactersGroupIndex,const uint32_t &characterId, const uint32_t &accountId)
{
    //here you are on game server link
    /*std::cout << "EpollClientLoginMaster::trySelectCharacterGameServer(), query send on game server to server it: "
              << charactersGroupForGameServerInformation->uniqueKey
              << ", host: "
              << charactersGroupForGameServerInformation->host
              << ":"
              << charactersGroupForGameServerInformation->port
              << std::endl;*/

    if(EpollClientLoginMaster::getTokenForCharacterSelect[0x00]!=0xF8)
    {
        std::cerr << "EpollClientLoginMaster::getTokenForCharacterSelect[0x00]!=0xF8: "
                  << charactersGroupForGameServerInformation->uniqueKey
                  << ", host: "
                  << charactersGroupForGameServerInformation->host
                  << ":"
                  << charactersGroupForGameServerInformation->port
                  << std::endl;
        return false;
    }
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

void EpollClientLoginMaster::sendServerChange()
{
    if(EpollClientLoginMaster::dataForUpdatedServers.addServer.empty() && EpollClientLoginMaster::dataForUpdatedServers.removeServer.empty())
        return;
    if(EpollClientLoginMaster::loginServers.size()==0)
    {
        EpollClientLoginMaster::dataForUpdatedServers.addServer.clear();
        EpollClientLoginMaster::dataForUpdatedServers.removeServer.clear();
        return;
    }
    //do the list
    uint32_t posOutput=0;
    bool useTheDiff=false;
    if(EpollClientLoginMaster::dataForUpdatedServers.removeServer.size()<255 && EpollClientLoginMaster::dataForUpdatedServers.addServer.size()<255)
    {
        useTheDiff=true;
        //send the network message

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x48;
        posOutput+=1+4;

        //the remove
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=EpollClientLoginMaster::dataForUpdatedServers.removeServer.size();
        posOutput+=1;
        unsigned int index=0;
        while(index<EpollClientLoginMaster::dataForUpdatedServers.removeServer.size())
        {
            const uint8_t &flatIndex=EpollClientLoginMaster::dataForUpdatedServers.removeServer.at(index);
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=flatIndex;
            posOutput+=1;
            index++;
        }

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=EpollClientLoginMaster::dataForUpdatedServers.addServer.size();
        posOutput+=1;
        index=0;
        while(index<EpollClientLoginMaster::dataForUpdatedServers.addServer.size())
        {
            //the add
            const EpollClientLoginMaster * const gameServerOnEpollClientLoginMaster=EpollClientLoginMaster::dataForUpdatedServers.addServer.at(index);
            const CharactersGroup::InternalGameServer * const gameServerOnCharactersGroup=gameServerOnEpollClientLoginMaster->charactersGroupForGameServerInformation;
            if(gameServerOnCharactersGroup==NULL)
            {
                std::cerr << "charactersGroup==NULL (abort)" << std::endl;
                abort();
            }
            //charactersGroup
            {
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=gameServerOnEpollClientLoginMaster->charactersGroupForGameServer->index;
                posOutput+=1;
            }
            //key
            {
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(gameServerOnCharactersGroup->uniqueKey);
                posOutput+=sizeof(gameServerOnCharactersGroup->uniqueKey);
            }
            //host
            {
                int newSize=FacilityLibGeneral::toUTF8WithHeader(gameServerOnCharactersGroup->host,ProtocolParsingBase::tempBigBufferForOutput+posOutput);
                if(newSize==0)
                {
                    std::cerr << "host null or unable to translate in utf8 (abort)" << std::endl;
                    abort();
                }
                posOutput+=newSize;
            }
            //port
            {
                *reinterpret_cast<unsigned short int *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=(unsigned short int)htole16((unsigned short int)gameServerOnCharactersGroup->port);
                posOutput+=sizeof(unsigned short int);
            }
            //metaData
            {
                const std::string &xmlString=gameServerOnCharactersGroup->metaData;
                if(xmlString.size()>4*1024)
                {
                    std::cerr << "metaData too hurge (abort)" << std::endl;
                    abort();
                }
                {
                    if(xmlString.size()>65535)
                    {
                        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=0;
                        posOutput+=2;
                    }
                    else
                    {
                        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(xmlString.size());
                        posOutput+=2;
                        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,xmlString.data(),xmlString.size());
                        posOutput+=xmlString.size();
                    }
                }
            }
            //logicalGroup
            {
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=gameServerOnCharactersGroup->logicalGroupIndex;
                posOutput+=1;
            }
            //max player
            *reinterpret_cast<unsigned short int *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=(unsigned short int)htole16(gameServerOnCharactersGroup->maxPlayer);
            posOutput+=sizeof(unsigned short int);

            index++;
        }

        //the new connected player
        index=0;
        while(index<EpollClientLoginMaster::dataForUpdatedServers.addServer.size())
        {
            const EpollClientLoginMaster * const gameServerOnEpollClientLoginMaster=EpollClientLoginMaster::dataForUpdatedServers.addServer.at(index);
            const CharactersGroup::InternalGameServer * const gameServerOnCharactersGroup=gameServerOnEpollClientLoginMaster->charactersGroupForGameServerInformation;
            //connected player
            *reinterpret_cast<unsigned short int *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=(unsigned short int)htole16(gameServerOnCharactersGroup->currentPlayer);
            posOutput+=sizeof(unsigned short int);

            index++;
        }

        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size
        if(posOutput>=EpollClientLoginMaster::serverServerListSize)
            useTheDiff=false;
    }
    EpollClientLoginMaster::dataForUpdatedServers.addServer.clear();
    EpollClientLoginMaster::dataForUpdatedServers.removeServer.clear();
    //broadcast all
    {
        if(!useTheDiff)
            broadcastGameServerChange();
        else
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
}

void EpollClientLoginMaster::sendCurrentPlayer()
{
    if(!currentPlayerForGameServerToUpdate)
        return;
    if(EpollClientLoginMaster::gameServers.size()==0)
        return;
    if(EpollClientLoginMaster::loginServers.size()==0)
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

/// \todo check the 255> size
void EpollClientLoginMaster::addToRemoveList()
{
    bool foundIntoAdd=false;
    {
        unsigned int index=0;
        while(index<EpollClientLoginMaster::dataForUpdatedServers.addServer.size())
        {
            const EpollClientLoginMaster * const server=EpollClientLoginMaster::dataForUpdatedServers.addServer.at(index);
            if(server==this)
            {
                EpollClientLoginMaster::dataForUpdatedServers.addServer.erase(EpollClientLoginMaster::dataForUpdatedServers.addServer.cbegin()+index);
                foundIntoAdd=true;
                break;
            }
            index++;
        }
    }
    if(!foundIntoAdd)
    {
        bool found=false;
        unsigned int index=0;
        while(index<EpollClientLoginMaster::gameServers.size())
        {
            const EpollClientLoginMaster * gameServer=EpollClientLoginMaster::gameServers.at(index);
            if(gameServer==this)
            {
                found=true;
                break;
            }
            index++;
        }
        if(found)
        {
            if(EpollClientLoginMaster::gameServers.size()>EpollClientLoginMaster::dataForUpdatedServers.addServer.size())
            {
                if(index<=(EpollClientLoginMaster::gameServers.size()-EpollClientLoginMaster::dataForUpdatedServers.addServer.size()))
                    EpollClientLoginMaster::dataForUpdatedServers.removeServer.push_back(index);
                else
                {
                    std::cerr << "addToRemoveList() index out of range" << std::endl;
                    abort();
                }
            }
            else
            {
                std::cerr << "addToRemoveList() index out of range into the old value" << std::endl;
                abort();
            }
        }
        else
        {
            std::cerr << "addToRemoveList() server not found to remove" << std::endl;
            abort();
        }
    }
}

/// \todo check the 255> size
void EpollClientLoginMaster::addToInserList()
{
    bool foundIntoAdd=false;
    unsigned int index=0;
    while(index<EpollClientLoginMaster::dataForUpdatedServers.addServer.size())
    {
        const EpollClientLoginMaster * const server=EpollClientLoginMaster::dataForUpdatedServers.addServer.at(index);
        if(server==this)
        {
            //to respect the order of connected player. move at the end
            EpollClientLoginMaster::dataForUpdatedServers.addServer.erase(EpollClientLoginMaster::dataForUpdatedServers.addServer.cbegin()+index);
            EpollClientLoginMaster::dataForUpdatedServers.addServer.push_back(this);
            foundIntoAdd=true;
            break;
        }
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        else
        {
            if(server->charactersGroupForGameServer->index==charactersGroupForGameServer->index && server->charactersGroupForGameServerInformation->uniqueKey==charactersGroupForGameServerInformation->uniqueKey)
                std::cerr << "different object but same content" << std::endl;
        }
        #endif
        index++;
    }
    if(!foundIntoAdd)
        EpollClientLoginMaster::dataForUpdatedServers.addServer.push_back(this);
}

bool EpollClientLoginMaster::sendGameServerRegistrationReply(const uint8_t queryNumber, bool generateNewUniqueKey)
{
    removeFromQueryReceived(queryNumber);

    //send the network reply
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=queryNumber;
    posOutput+=1+4;

    queryNumberInConflicWithTheMainServer=0;
    inConflicWithTheMainServer=NULL;

    if(generateNewUniqueKey)
    {
        uint32_t newUniqueKey;
        do
        {
            newUniqueKey = rng();
        } while(Q_UNLIKELY(charactersGroupForGameServer->gameServers.find(newUniqueKey)!=charactersGroupForGameServer->gameServers.cend()));
        uniqueKey=newUniqueKey;
        std::cerr << "Generate new unique key for a game server" << std::endl;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x02;
        posOutput+=1;
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=(uint32_t)htole32(newUniqueKey);
        posOutput+=4;
    }
    else
    {
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
        posOutput+=1;
    }

    //monster id list
    {
        if((posOutput+4*CATCHCHALLENGER_SERVER_MAXIDBLOCK)>=sizeof(ProtocolParsingBase::tempBigBufferForOutput))
        {
            std::cerr << "ProtocolParsingBase::tempBigBufferForOutput out of buffer, file: " << __FILE__ << ":" << __LINE__ << std::endl;
            abort();
        }
        int index=0;
        while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
        {
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput+index*4/*size of int*/)=(uint32_t)htole32(charactersGroupForGameServer->maxMonsterId+1+index);
            index++;
        }
        posOutput+=4*CATCHCHALLENGER_SERVER_MAXIDBLOCK;
        charactersGroupForGameServer->maxMonsterId+=CATCHCHALLENGER_SERVER_MAXIDBLOCK;
    }
    //clan id list
    {
        if((posOutput+4*CATCHCHALLENGER_SERVER_MAXCLANIDBLOCK)>=sizeof(ProtocolParsingBase::tempBigBufferForOutput))
        {
            std::cerr << "ProtocolParsingBase::tempBigBufferForOutput out of buffer, file: " << __FILE__ << ":" << __LINE__ << std::endl;
            abort();
        }
        int index=0;
        while(index<CATCHCHALLENGER_SERVER_MAXCLANIDBLOCK)
        {
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput+index*4/*size of int*/)=(uint32_t)htole32(charactersGroupForGameServer->maxClanId+1+index);
            index++;
        }
        posOutput+=4*CATCHCHALLENGER_SERVER_MAXCLANIDBLOCK;
        charactersGroupForGameServer->maxClanId+=CATCHCHALLENGER_SERVER_MAXCLANIDBLOCK;
    }
    if((posOutput+1+2+1+2)>=sizeof(ProtocolParsingBase::tempBigBufferForOutput))
    {
        std::cerr << "ProtocolParsingBase::tempBigBufferForOutput out of buffer, file: " << __FILE__ << ":" << __LINE__ << std::endl;
        abort();
    }
    //Max player monsters
    //Max warehouse player monsters
    //Max player items
    //Max warehouse player monsters
    //send the dictionary
    {
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,EpollServerLoginMaster::fixedValuesRawDictionaryCacheForGameserver,EpollServerLoginMaster::fixedValuesRawDictionaryCacheForGameserverSize);
        posOutput+=EpollServerLoginMaster::fixedValuesRawDictionaryCacheForGameserverSize;
    }

    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(posOutput-1-1-4);//set the dynamic size
    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

    //only game server will receive query
    if(queryNumberList.empty())
    {
        queryNumberList.reserve(CATCHCHALLENGER_MAXPROTOCOLQUERY);
        int index=0;
        while(index<CATCHCHALLENGER_MAXPROTOCOLQUERY)
        {
            queryNumberList.push_back(index);
            index++;
        }
    }
    EpollClientLoginMaster::gameServers.push_back(this);
    stat=EpollClientLoginMasterStat::GameServer;
    currentPlayerForGameServerToUpdate=false;

    return true;
}

bool EpollClientLoginMaster::sendGameServerPing(const uint64_t &msecondFrom1970)
{
    if(charactersGroupForGameServerInformation==NULL)
        return false;
    if(charactersGroupForGameServerInformation->pingInProgress==true)
        return false;
    charactersGroupForGameServerInformation->pingInProgress=true;
    charactersGroupForGameServerInformation->lastPingStarted=msecondFrom1970;

    const uint8_t &queryNumber=queryNumberList.back();
    registerOutputQuery(queryNumber,0xF9);
    ProtocolParsingBase::tempBigBufferForOutput[0x00]=0xF9;
    ProtocolParsingBase::tempBigBufferForOutput[0x01]=queryNumber;
    queryNumberList.pop_back();
    return sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,2);
}

void EpollClientLoginMaster::breakNeedMoreData()
{
    if(stat==EpollClientLoginMaster::None)
    {
        disconnectClient();
        return;
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    //std::cerr << "Break due to need more in parse data" << std::endl;
    #endif
}

