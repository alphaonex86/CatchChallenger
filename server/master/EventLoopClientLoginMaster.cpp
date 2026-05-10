#include "EventLoopClientLoginMaster.hpp"
#include <cstring>
#include "EventLoopServerLoginMaster.hpp"
#include "../../general/base/FacilityLibGeneral.hpp"
#include "../../general/base/cpp11addition.hpp"

#include <iostream>
#include <string>
#include <time.h>

using namespace CatchChallenger;

std::unordered_set<std::string> EventLoopClientLoginMaster::wrongProtocolEmited;

EventLoopClientLoginMaster::EventLoopClientLoginMaster(
            const int &infd
        ) :
        EventLoopClient(infd),
        ProtocolParsingInputOutput(
           #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            PacketModeTransmission_Server
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

EventLoopClientLoginMaster::~EventLoopClientLoginMaster()
{
    resetToDisconnect();
}

void EventLoopClientLoginMaster::resetToDisconnect()
{
    if(stat==EventLoopClientLoginMasterStat::LoginServer)
    {
        vectorremoveOne(EventLoopClientLoginMaster::loginServers,this);
        unsigned int index=0;
        while(index<EventLoopClientLoginMaster::gameServers.size())
        {
            unsigned int sub_index=0;
            while(sub_index<EventLoopClientLoginMaster::gameServers.at(index)->loginServerReturnForCharaterSelect.size())
            {
                const DataForSelectedCharacterReturn &dataForSelectedCharacterReturn=EventLoopClientLoginMaster::gameServers.at(index)->loginServerReturnForCharaterSelect.at(sub_index);
                if(dataForSelectedCharacterReturn.loginServer==this)
                    EventLoopClientLoginMaster::gameServers.at(index)->loginServerReturnForCharaterSelect[sub_index].loginServer=NULL;
                sub_index++;
            }
            index++;
        }
    }
    if(stat==EventLoopClientLoginMasterStat::GameServer)
    {
        addToRemoveList();
        vectorremoveOne(EventLoopClientLoginMaster::gameServers,this);
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
        EventLoopServerLoginMaster::unixServerLoginMaster->doTheServerList();
        EventLoopServerLoginMaster::unixServerLoginMaster->doTheReplyCache();
        //EventLoopClientLoginMaster::broadcastGameServerChange();
    }
    if(inConflicWithTheMainServer!=NULL)
    {
        vectorremoveOne(EventLoopClientLoginMaster::gameServers,this);
        vectorremoveOne(inConflicWithTheMainServer->secondServerInConflict,this);
        inConflicWithTheMainServer=NULL;
    }

    stat=EventLoopClientLoginMasterStat::None;

    updateConsoleCountServer();
}

void EventLoopClientLoginMaster::passUniqueKeyToNextGameServer()
{
    if(stat!=EventLoopClientLoginMasterStat::GameServer)
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

        EventLoopClientLoginMaster * newServerToBeMaster=secondServerInConflict.front();
        secondServerInConflict.erase(secondServerInConflict.cbegin());

        newServerToBeMaster->sendGameServerRegistrationReply(newServerToBeMaster->queryNumberInConflicWithTheMainServer,false);

        /*done after the ping:
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
                    newServerToBeMaster,tempData.uniqueKey,tempData.host,tempData.port,tempData.metaData,tempData.logicalGroupIndex,tempData.currentPlayer,tempData.maxPlayer,tempData.lockedAccountByGameserver);*/

        if(!secondServerInConflict.empty())
        {
            std::cout << "Server with same unique key in waiting... and even more" << std::endl;

            newServerToBeMaster->secondServerInConflict=secondServerInConflict;

            unsigned int indexServerToUpdate=0;
            while(indexServerToUpdate<newServerToBeMaster->secondServerInConflict.size())
            {
                EventLoopClientLoginMaster * const serverToMasterUpdate=newServerToBeMaster->secondServerInConflict.at(indexServerToUpdate);
                serverToMasterUpdate->inConflicWithTheMainServer=newServerToBeMaster;
                indexServerToUpdate++;
            }
        }
        newServerToBeMaster->sendGameServerPing(msFrom1970());//to enable it into the server list
        secondServerInConflict.clear();
    }
}

bool EventLoopClientLoginMaster::disconnectClient()
{
    resetToDisconnect();
    updateConsoleCountServer();
    EventLoopClient::close();
    //messageParsingLayer("Disconnected client");
    return true;
}

void EventLoopClientLoginMaster::updateConsoleCountServer()
{
    /** \warning done by the stats client, better way to do (no logs flood, instant, no text to parse)
    if(EventLoopClientLoginMaster::lastSizeDisplayGameServers!=gameServers.size() || EventLoopClientLoginMaster::lastSizeDisplayLoginServers!=loginServers.size())
    {
        EventLoopClientLoginMaster::lastSizeDisplayGameServers=gameServers.size();
        EventLoopClientLoginMaster::lastSizeDisplayLoginServers=loginServers.size();
        std::cout << "Online: " << EventLoopClientLoginMaster::lastSizeDisplayLoginServers << " login server and " << EventLoopClientLoginMaster::lastSizeDisplayGameServers << " game server" << std::endl;
    }*/
}

//input/ouput layer
void EventLoopClientLoginMaster::errorParsingLayer(const std::string &error)
{
    if(stat==EventLoopClientLoginMasterStat::None)
    {
        //std::cerr << headerOutput() << "Kicked by: " << errorString << std::endl;//silent if protocol not passed, to not flood the log if other client like http client (browser) is connected
        disconnectClient();
        return;
    }
    std::cerr << socketString/* << ": " already concat to improve the performance*/ << sanitizeUtf8String(error) << std::endl;
    disconnectClient();
}

void EventLoopClientLoginMaster::messageParsingLayer(const std::string &message) const
{
    std::cout << socketString/* << ": " already concat to improve the performance*/ << sanitizeUtf8String(message) << std::endl;
}

void EventLoopClientLoginMaster::errorParsingLayer(const char * const error)
{
    std::cerr << socketString/* << ": " already concat to improve the performance*/ << sanitizeUtf8String(std::string(error)) << std::endl;
    disconnectClient();
}

void EventLoopClientLoginMaster::messageParsingLayer(const char * const message) const
{
    std::cout << socketString/* << ": " already concat to improve the performance*/ << sanitizeUtf8String(std::string(message)) << std::endl;
}

BaseClassSwitch::EventLoopObjectType EventLoopClientLoginMaster::getType() const
{
    return BaseClassSwitch::EventLoopObjectType::Client;
}

void EventLoopClientLoginMaster::parseIncommingData()
{
    ProtocolParsingInputOutput::parseIncommingData();
}

void EventLoopClientLoginMaster::selectCharacter(const uint8_t &query_id,const uint32_t &serverUniqueKey,const uint8_t &charactersGroupIndex,const uint32_t &characterId,const uint32_t &accountId)
{
    if(characterId==0)
        std::cerr << "EventLoopClientLoginMaster::selectCharacter() call with characterId=0" << std::endl;
    if(charactersGroupIndex>=CharactersGroup::list.size())
    {
        errorParsingLayer("EventLoopClientLoginMaster::selectCharacter() charactersGroupIndex is out of range");
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
        {const uint32_t _tmp_le=(htole32(1));memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+1,&_tmp_le,sizeof(_tmp_le));}//set the dynamic size

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)0x05;
        posOutput+=1;

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        return;
    }
    {
        const CharactersGroup::CharacterLock &lockResult=CharactersGroup::list.at(charactersGroupIndex)->characterIsLocked(characterId);
        if(lockResult!=CharactersGroup::CharacterLock::Unlocked)
        {
            #ifdef CATCHCHALLENGER_HARDENED
            messageParsingLayer("Lock result wrong: "+std::to_string(lockResult)+" CharactersGroup::list.at("+std::to_string(charactersGroupIndex)+")->characterIsLocked("+std::to_string(characterId)+"), CharactersGroup name: "+CharactersGroup::list.at(charactersGroupIndex)->name);
            #endif
            //send the network reply
            removeFromQueryReceived(query_id);
            uint32_t posOutput=0;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
            posOutput+=1+4;
            {const uint32_t _tmp_le=(htole32(1));memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+1,&_tmp_le,sizeof(_tmp_le));}//set the dynamic size

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
    EventLoopClientLoginMaster * gameServer=static_cast<EventLoopClientLoginMaster *>(CharactersGroup::list.at(charactersGroupIndex)->gameServers.at(serverUniqueKey).link);
    if(!gameServer->trySelectCharacterGameServer(this,query_id,serverUniqueKey,charactersGroupIndex,characterId,accountId))
    {
        #ifdef CATCHCHALLENGER_HARDENED
        messageParsingLayer("Unable to lock: CharactersGroup::list.at("+std::to_string(charactersGroupIndex)+")->trySelectCharacterGameServer("+std::to_string(characterId)+"), CharactersGroup name: "+CharactersGroup::list.at(charactersGroupIndex)->name+": "+std::to_string(characterId));
        #endif
        //send the network reply
        removeFromQueryReceived(query_id);
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
        posOutput+=1+4;
        {const uint32_t _tmp_le=(htole32(1));memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+1,&_tmp_le,sizeof(_tmp_le));}//set the dynamic size

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)0x05;
        posOutput+=1;

        sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        return;
    }

    CharactersGroup::list[charactersGroupIndex]->lockTheCharacter(characterId);
    gameServer->charactersGroupForGameServerInformation->lockedAccountByGameserver.insert(characterId);
    /*#ifdef CATCHCHALLENGER_HARDENED
    messageParsingLayer("Locked: CharactersGroup::list.at("+std::to_string(charactersGroupIndex)+")->characterIsLocked("+std::to_string(characterId)+"), CharactersGroup name: "+CharactersGroup::list.at(charactersGroupIndex)->name+" on server: "+gameServer->charactersGroupForGameServerInformation->host+":"+std::to_string(gameServer->charactersGroupForGameServerInformation->port)+" "+std::to_string(gameServer->uniqueKey));
    #endif*/

    //reply send at trySelectCharacterGameServer() above
}

bool EventLoopClientLoginMaster::trySelectCharacterGameServer(EventLoopClientLoginMaster * const loginServer,const uint8_t &client_query_id,const uint32_t &serverUniqueKey,const uint8_t &charactersGroupIndex,const uint32_t &characterId, const uint32_t &accountId)
{
    //here you are on game server link
    /*std::cout << "EventLoopClientLoginMaster::trySelectCharacterGameServer(), query send on game server to server it: "
              << charactersGroupForGameServerInformation->uniqueKey
              << ", host: "
              << charactersGroupForGameServerInformation->host
              << ":"
              << charactersGroupForGameServerInformation->port
              << std::endl;*/

    if(EventLoopClientLoginMaster::getTokenForCharacterSelect[0x00]!=0xF8)
    {
        std::cerr << "EventLoopClientLoginMaster::getTokenForCharacterSelect[0x00]!=0xF8: "
                  << uniqueKey
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
                  << uniqueKey
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
    EventLoopClientLoginMaster::getTokenForCharacterSelect[0x01]=queryNumber;
    {const uint32_t _tmp_le=(htole32(characterId));memcpy(EventLoopClientLoginMaster::getTokenForCharacterSelect+0x02,&_tmp_le,sizeof(_tmp_le));}

    {const uint32_t _tmp_le=(htole32(accountId));memcpy(EventLoopClientLoginMaster::getTokenForCharacterSelect+0x06,&_tmp_le,sizeof(_tmp_le));}

    queryNumberList.pop_back();
    return sendRawBlock(reinterpret_cast<char *>(EventLoopClientLoginMaster::getTokenForCharacterSelect),sizeof(EventLoopClientLoginMaster::getTokenForCharacterSelect));
}

void EventLoopClientLoginMaster::selectCharacter_ReturnToken(const uint8_t &query_id,const char * const token)
{
    //send the network reply
    removeFromQueryReceived(query_id);
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
    posOutput+=1+4;
    {const uint32_t _tmp_le=(htole32(CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER));memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+1,&_tmp_le,sizeof(_tmp_le));}//set the dynamic size

    memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,token,CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER);
    posOutput+=CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER;

    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void EventLoopClientLoginMaster::selectCharacter_ReturnFailed(const uint8_t &query_id,const uint8_t &errorCode)
{
    //send the network reply
    removeFromQueryReceived(query_id);
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
    posOutput+=1;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=query_id;
    posOutput+=1+4;
    {const uint32_t _tmp_le=(htole32(1));memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+1,&_tmp_le,sizeof(_tmp_le));}//set the dynamic size

    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=errorCode;
    posOutput+=1;

    //you are on login server
    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}

void EventLoopClientLoginMaster::broadcastGameServerChange()
{
    unsigned int index=0;
    while(index<EventLoopClientLoginMaster::loginServers.size())
    {
        EventLoopClientLoginMaster * const loginServer=EventLoopClientLoginMaster::loginServers.at(index);
        loginServer->internalSendRawSmallPacket(EventLoopClientLoginMaster::serverServerList,EventLoopClientLoginMaster::serverServerListSize);
        index++;
    }
}

bool EventLoopClientLoginMaster::sendRawBlock(const char * const data,const int &size)
{
    return internalSendRawSmallPacket(data,size);
}

void EventLoopClientLoginMaster::sendServerChange()
{
    if(EventLoopClientLoginMaster::dataForUpdatedServers.addServer.empty() && EventLoopClientLoginMaster::dataForUpdatedServers.removeServer.empty())
        return;
    if(EventLoopClientLoginMaster::loginServers.size()==0)
    {
        EventLoopClientLoginMaster::dataForUpdatedServers.addServer.clear();
        EventLoopClientLoginMaster::dataForUpdatedServers.removeServer.clear();
        return;
    }
    //do the list
    uint32_t posOutput=0;
    bool useTheDiff=false;
    if(EventLoopClientLoginMaster::dataForUpdatedServers.removeServer.size()<255 && EventLoopClientLoginMaster::dataForUpdatedServers.addServer.size()<255)
    {
        useTheDiff=true;
        //send the network message

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x48;
        posOutput+=1+4;

        //the remove
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=EventLoopClientLoginMaster::dataForUpdatedServers.removeServer.size();
        posOutput+=1;
        unsigned int index=0;
        while(index<EventLoopClientLoginMaster::dataForUpdatedServers.removeServer.size())
        {
            const uint8_t &flatIndex=EventLoopClientLoginMaster::dataForUpdatedServers.removeServer.at(index);
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=flatIndex;
            posOutput+=1;
            index++;
        }

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=EventLoopClientLoginMaster::dataForUpdatedServers.addServer.size();
        posOutput+=1;
        index=0;
        while(index<EventLoopClientLoginMaster::dataForUpdatedServers.addServer.size())
        {
            //the add
            const EventLoopClientLoginMaster * const gameServerOnEpollClientLoginMaster=EventLoopClientLoginMaster::dataForUpdatedServers.addServer.at(index);
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
                {const uint32_t _tmp_le=(htole32(gameServerOnEpollClientLoginMaster->uniqueKey));memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&_tmp_le,sizeof(_tmp_le));}

                posOutput+=sizeof(gameServerOnEpollClientLoginMaster->uniqueKey);
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
                        {const uint16_t _tmp_le=(0);memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&_tmp_le,sizeof(_tmp_le));}

                        posOutput+=2;
                    }
                    else
                    {
                        {const uint16_t _tmp_le=(htole16(xmlString.size()));memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&_tmp_le,sizeof(_tmp_le));}

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
        while(index<EventLoopClientLoginMaster::dataForUpdatedServers.addServer.size())
        {
            const EventLoopClientLoginMaster * const gameServerOnEpollClientLoginMaster=EventLoopClientLoginMaster::dataForUpdatedServers.addServer.at(index);
            const CharactersGroup::InternalGameServer * const gameServerOnCharactersGroup=gameServerOnEpollClientLoginMaster->charactersGroupForGameServerInformation;
            //connected player
            *reinterpret_cast<unsigned short int *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=(unsigned short int)htole16(gameServerOnCharactersGroup->currentPlayer);
            posOutput+=sizeof(unsigned short int);

            index++;
        }

        {const uint32_t _tmp_le=(htole32(posOutput-1-4));memcpy(ProtocolParsingBase::tempBigBufferForOutput+1,&_tmp_le,sizeof(_tmp_le));}//set the dynamic size
        if(posOutput>=EventLoopClientLoginMaster::serverServerListSize)
            useTheDiff=false;
    }
    EventLoopClientLoginMaster::dataForUpdatedServers.addServer.clear();
    EventLoopClientLoginMaster::dataForUpdatedServers.removeServer.clear();
    //broadcast all
    {
        if(!useTheDiff)
            broadcastGameServerChange();
        else
        {
            unsigned int index=0;
            while(index<EventLoopClientLoginMaster::loginServers.size())
            {
                EventLoopClientLoginMaster * const loginServer=EventLoopClientLoginMaster::loginServers.at(index);
                loginServer->internalSendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                index++;
            }
        }
    }
}

//return the number if player
int EventLoopClientLoginMaster::sendCurrentPlayer()
{
    if(!currentPlayerForGameServerToUpdate)
        return -1;
    if(EventLoopClientLoginMaster::gameServers.size()==0)
        return -1;
    if(EventLoopClientLoginMaster::loginServers.size()==0)
        return -1;
    int numberOfPlayer=0;
    //do the list
    uint32_t posOutput=0;
    {
        //send the network message

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x47;
        posOutput+=1+4;
        {const uint32_t _tmp_le=(htole32(EventLoopClientLoginMaster::gameServers.size()*2));memcpy(ProtocolParsingBase::tempBigBufferForOutput+1,&_tmp_le,sizeof(_tmp_le));}//set the dynamic size

        unsigned int index=0;
        while(index<EventLoopClientLoginMaster::gameServers.size())
        {
            const EventLoopClientLoginMaster * const gameServer=EventLoopClientLoginMaster::gameServers.at(index);
            {const uint16_t _tmp_le=(htole16(gameServer->charactersGroupForGameServerInformation->currentPlayer));memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&_tmp_le,sizeof(_tmp_le));}

            numberOfPlayer+=gameServer->charactersGroupForGameServerInformation->currentPlayer;
            posOutput+=2;
            index++;
        }
    }
    //broadcast all
    {
        unsigned int index=0;
        while(index<EventLoopClientLoginMaster::loginServers.size())
        {
            EventLoopClientLoginMaster * const loginServer=EventLoopClientLoginMaster::loginServers.at(index);
            loginServer->internalSendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
            index++;
        }
    }
    return numberOfPlayer;
}

void EventLoopClientLoginMaster::sendTimeRangeEvent()
{
    if(EventLoopClientLoginMaster::gameServers.size()==0)
        return;

    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x4E;
    posOutput+=1+8;
    const uint64_t msFrom1970littleendian=htole64(msFrom1970()/1000);
    memcpy(ProtocolParsingBase::tempBigBufferForOutput+1,&msFrom1970littleendian,8);

    unsigned int index=0;
    while(index<EventLoopClientLoginMaster::gameServers.size())
    {
        EventLoopClientLoginMaster * const gameServer=EventLoopClientLoginMaster::gameServers.at(index);
        gameServer->internalSendRawSmallPacket(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        index++;
    }
}

void EventLoopClientLoginMaster::disconnectForDuplicateConnexionDetected(const uint32_t &characterId)
{
    {const uint32_t _tmp_le=(htole32(characterId));memcpy(EventLoopClientLoginMaster::duplicateConnexionDetected+1,&_tmp_le,sizeof(_tmp_le));}

    internalSendRawSmallPacket(reinterpret_cast<char *>(EventLoopClientLoginMaster::duplicateConnexionDetected),sizeof(EventLoopClientLoginMaster::duplicateConnexionDetected));
}

/// \todo check the 255> size
void EventLoopClientLoginMaster::addToRemoveList()
{
    EventLoopClientLoginMaster::havePlayerCountChange=true;
    if(!charactersGroupForGameServerInformation->addSend)
        return;
    bool foundIntoAdd=false;
    {
        unsigned int index=0;
        while(index<EventLoopClientLoginMaster::dataForUpdatedServers.addServer.size())
        {
            const EventLoopClientLoginMaster * const server=EventLoopClientLoginMaster::dataForUpdatedServers.addServer.at(index);
            if(server==this)
            {
                EventLoopClientLoginMaster::dataForUpdatedServers.addServer.erase(EventLoopClientLoginMaster::dataForUpdatedServers.addServer.cbegin()+index);
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
        while(index<EventLoopClientLoginMaster::gameServers.size())
        {
            const EventLoopClientLoginMaster * gameServer=EventLoopClientLoginMaster::gameServers.at(index);
            if(gameServer==this)
            {
                if(gameServer->charactersGroupForGameServerInformation->addSend)
                    found=true;
                break;
            }
            index++;
        }
        if(found)
        {
            if(EventLoopClientLoginMaster::gameServers.size()>EventLoopClientLoginMaster::dataForUpdatedServers.addServer.size())
            {
                if(index<=(EventLoopClientLoginMaster::gameServers.size()-EventLoopClientLoginMaster::dataForUpdatedServers.addServer.size()))
                    EventLoopClientLoginMaster::dataForUpdatedServers.removeServer.push_back(index);
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
void EventLoopClientLoginMaster::addToInserList()
{
    EventLoopClientLoginMaster::havePlayerCountChange=true;
    charactersGroupForGameServerInformation->addSend=true;
    bool foundIntoAdd=false;
    unsigned int index=0;
    while(index<EventLoopClientLoginMaster::dataForUpdatedServers.addServer.size())
    {
        const EventLoopClientLoginMaster * const server=EventLoopClientLoginMaster::dataForUpdatedServers.addServer.at(index);
        if(server==this)
        {
            //to respect the order of connected player. move at the end
            EventLoopClientLoginMaster::dataForUpdatedServers.addServer.erase(EventLoopClientLoginMaster::dataForUpdatedServers.addServer.cbegin()+index);
            EventLoopClientLoginMaster::dataForUpdatedServers.addServer.push_back(this);
            foundIntoAdd=true;
            break;
        }
        #ifdef CATCHCHALLENGER_HARDENED
        else
        {
            if(server->charactersGroupForGameServer->index==charactersGroupForGameServer->index && server->uniqueKey==uniqueKey)
                std::cerr << "different object but same content" << std::endl;
        }
        #endif
        index++;
    }
    if(!foundIntoAdd)
        EventLoopClientLoginMaster::dataForUpdatedServers.addServer.push_back(this);
}

bool EventLoopClientLoginMaster::sendGameServerRegistrationReply(const uint8_t queryNumber, bool generateNewUniqueKey)
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
        std::cerr << "Generate new unique key for a game server: " << std::to_string(newUniqueKey) << std::endl;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x02;
        posOutput+=1;
        {const uint32_t _tmp_le=((uint32_t)htole32(newUniqueKey));memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&_tmp_le,sizeof(_tmp_le));}

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
            {const uint32_t _tmp_le=((uint32_t)htole32(charactersGroupForGameServer->maxMonsterId+1+index));memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput+index*4/*size of int*/,&_tmp_le,sizeof(_tmp_le));}

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
            {const uint32_t _tmp_le=((uint32_t)htole32(charactersGroupForGameServer->maxClanId+1+index));memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput+index*4/*size of int*/,&_tmp_le,sizeof(_tmp_le));}

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
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,EventLoopServerLoginMaster::fixedValuesRawDictionaryCacheForGameserver,EventLoopServerLoginMaster::fixedValuesRawDictionaryCacheForGameserverSize);
        posOutput+=EventLoopServerLoginMaster::fixedValuesRawDictionaryCacheForGameserverSize;
    }

    {const uint32_t _tmp_le=(htole32(posOutput-1-1-4));memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+1,&_tmp_le,sizeof(_tmp_le));}//set the dynamic size
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
    //std::cout << "new game server added: " << std::to_string((uint64_t)this) << std::endl;
    EventLoopClientLoginMaster::gameServers.push_back(this);
    stat=EventLoopClientLoginMasterStat::GameServer;
    currentPlayerForGameServerToUpdate=false;

    return true;
}

bool EventLoopClientLoginMaster::sendGameServerPing(const uint64_t &msecondFrom1970)
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

void EventLoopClientLoginMaster::breakNeedMoreData()
{
    if(stat==EventLoopClientLoginMaster::None)
    {
        disconnectClient();
        return;
    }
    #ifdef CATCHCHALLENGER_HARDENED
    //std::cerr << "Break due to need more in parse data" << std::endl;
    #endif
}

ssize_t EventLoopClientLoginMaster::readFromSocket(char * data, const size_t &size)
{
    return EventLoopClient::read(data,size);
}

ssize_t EventLoopClientLoginMaster::writeToSocket(const char * const data, const size_t &size)
{
    return EventLoopClient::write(data,size);
}

void EventLoopClientLoginMaster::closeSocket()
{
    disconnectClient();
}


