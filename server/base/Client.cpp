#include "Client.hpp"
#include "GlobalServerData.hpp"
#include "../base/PreparedDBQuery.hpp"
#ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
#include "../game-server-alone/LinkToMaster.hpp"
#else
#include "../../general/sha224/sha224.hpp"
#endif
#include "../../general/base/CommonSettingsServer.hpp"

#include "BaseServerLogin.hpp"

#ifdef CATCHCHALLENGER_DB_FILE
#include <fstream>
#include <string>
#include "../../general/hps/hps.h"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"
#include "DictionaryServer.hpp"
#endif

#include <chrono>
#include <cstring>

using namespace CatchChallenger;

/// \warning never cross the signals from and to the different client, complexity ^2
/// \todo drop instant player number notification, and before do the signal without signal/slot, check if the number have change
/// \todo change push position recording, from ClientMapManagement to ClientLocalCalcule, to disable ALL operation for MapVisibilityAlgorithm_None

Client::Client() :
    ProtocolParsingInputOutput(
        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
        PacketModeTransmission_Server
        #endif
        ),
    mapSyncMiss(false),
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    stat_client(false),
    #endif
    stat(ClientStat::None),
    lastdaillygift(0),
    pingInProgress(0),
    account_id(0),
    character_id(0),
    #ifndef EPOLLCATCHCHALLENGERSERVER
    isConnected(true),
    #endif
    randomIndex(0),
    randomSize(0),
    number_of_character(0),
    #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    datapackStatus(DatapackStatus::Main),
    #else
    datapackStatus(DatapackStatus::Base),
    #endif
    connected_players(0),
    stopIt(false),
    profileIndex(0),
    otherPlayerBattle(NULL),
    battleIsValidated(false),
    mCurrentSkillId(0),
    mHaveCurrentSkill(false),
    mMonsterChange(false),
    botFightCash(0),
    botFight(std::pair<CATCHCHALLENGER_TYPE_MAPID/*mapId*/,uint8_t/*botId*/>(0,0)),
    #ifndef EPOLLCATCHCHALLENGERSERVER
    isInCityCapture(false),
    #endif
    otherPlayerTrade(NULL),
    tradeIsValidated(false),
    clan(NULL)
    #ifdef EPOLLCATCHCHALLENGERSERVER
    ,socketString(NULL),
    socketStringSize(0)
    #endif
    #ifndef EPOLLCATCHCHALLENGERSERVER
    ,otherCityPlayerBattle(NULL)
    #endif
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    ClientBase::public_and_private_informations_solo=&public_and_private_informations;
    #endif
    public_and_private_informations.repel_step=0;
    queryNumberList.reserve(CATCHCHALLENGER_MAXPROTOCOLQUERY);
    {
        uint8_t index=0;
        while(index<CATCHCHALLENGER_MAXPROTOCOLQUERY)
        {
            queryNumberList.push_back(index);
            index++;
        }
    }
    public_and_private_informations.recipes=nullptr;
    public_and_private_informations.encyclopedia_monster=nullptr;
    public_and_private_informations.encyclopedia_item=nullptr;
    public_and_private_informations.bot_already_beaten=nullptr;
}

//need be call after isReadyToDelete() emited
Client::~Client()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    ClientBase::public_and_private_informations_solo=NULL;
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    //normalOutput("Destroyed client");
    #endif
    /*#ifndef EPOLLCATCHCHALLENGERSERVER
    if(socket!=NULL)
    {
        delete socket;
        socket=NULL;
    }
    #else
    if(socketString!=NULL)
        delete socketString;
    #endif*/
    //SQL
    {
        #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
        while(!callbackRegistred.empty())
        {
            callbackRegistred.front()->object=NULL;
            callbackRegistred.pop();
        }
        //crash with heap-buffer-overflow if not flush before the end of destructor
        while(!callbackRegistred.empty())
            callbackRegistred.pop();
        #elif CATCHCHALLENGER_DB_BLACKHOLE
        #elif CATCHCHALLENGER_DB_FILE
        #else
        #error Define what do here
        #endif
        while(!paramToPassToCallBack.empty())
            paramToPassToCallBack.pop();
    }
}

/// \warning called in one other thread!!!
bool Client::disconnectClient()
{
    //closeSocket();-> done into above layer
    if(stat==ClientStat::None)
    {
        closeSocket();
        return false;
    }
    if(account_id==0)
    {
        closeSocket();
        return false;
    }
    account_id=0;
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    if(character_id!=0)
        normalOutput("Disconnected client");
    #endif
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    /// \warning global clear from client call?
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    GlobalServerData::serverPrivateVariables.db_login->clear();
    GlobalServerData::serverPrivateVariables.db_base->clear();
    #endif
    GlobalServerData::serverPrivateVariables.db_common->clear();
    GlobalServerData::serverPrivateVariables.db_server->clear();
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif
    #ifndef EPOLLCATCHCHALLENGERSERVER
    isConnected=false;
    /*if(socket!=NULL)
    {
        socket->disconnectFromHost();
        socket=NULL;
    }*/
    #endif
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
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
    if(stat_client)
    {
        unsigned int index=0;
        while(index<stat_client_list.size())
        {
            const Client * const client=stat_client_list.at(index);
            if(this==client)
            {
                stat_client_list.erase(stat_client_list.begin()+index);
                break;
            }
            index++;
        }
    }
    #endif

    if(stat==ClientStat::CharacterSelecting)
    {
        stat=ClientStat::CharacterSelected;//to block the new connection util th destructor is invocked
        GlobalServerData::serverPrivateVariables.connected_players_id_list.erase(character_id);
        simplifiedIdList.push_back(public_and_private_informations.public_informations.simplifiedId);
        #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
        if(character_id==0)
            std::cerr << "character_id==0, set it before to correctly unload to master" << std::endl;
        else
            LinkToMaster::linkToMaster->characterDisconnected(character_id);
        #endif
    }
    else if(stat==ClientStat::CharacterSelected)
    {
        const std::string &character_id_string=std::to_string(character_id);
        if(map!=NULL)
            removeClientOnMap(map);
        if(GlobalServerData::serverSettings.sendPlayerNumber)
            GlobalServerData::serverPrivateVariables.player_updater->removeConnectedPlayer();
        extraStop();
        tradeCanceled();
        battleCanceled();
        removeFromClan();
        GlobalServerData::serverPrivateVariables.connected_players--;
        simplifiedIdList.push_back(public_and_private_informations.public_informations.simplifiedId);
        GlobalServerData::serverPrivateVariables.connected_players_id_list.erase(character_id);
        #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
        LinkToMaster::linkToMaster->characterDisconnected(character_id);
        #endif
        playerByPseudo.erase(public_and_private_informations.public_informations.pseudo);
        clanChangeWithoutDb(0);
        if(!vectorremoveOne(clientBroadCastList,this))
            std::cout << "Client::disconnectClient(): vectorremoveOne(clientBroadCastList,this)" << std::endl;
        playerByPseudo.erase(public_and_private_informations.public_informations.pseudo);
        playerById.erase(character_id);
        characterCreationDateList.erase(character_id);
        #ifndef EPOLLCATCHCHALLENGERSERVER
        leaveTheCityCapture();
        #endif
        const uint64_t &addTime=sFrom1970()-connectedSince;
        if(addTime>5)
        {
            #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
            GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_played_time.asyncWrite({
                        std::to_string(addTime),
                        character_id_string
                        });
            #ifdef CATCHCHALLENGER_CLASS_ALLINONESERVER
            GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_server_time_played_time.asyncWrite({
                        std::to_string(addTime),
                        std::to_string(0),
                        character_id_string
                        });
            #endif
            #elif CATCHCHALLENGER_DB_BLACKHOLE
            #elif CATCHCHALLENGER_DB_FILE
            #else
            #error Define what do here
            #endif
        }
        //save the monster
        if(GlobalServerData::serverSettings.fightSync==GameServerSettings::FightSync_AtTheEndOfBattle && isInFight())
            saveCurrentMonsterStat();
        if(GlobalServerData::serverSettings.fightSync==GameServerSettings::FightSync_AtTheDisconnexion)
        {
            unsigned int index=0;
            while(index<public_and_private_informations.playerMonster.size())
            {
                const PlayerMonster &playerMonster=public_and_private_informations.playerMonster.at(index);
                #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
                if(CommonSettingsServer::commonSettingsServer.useSP)
                    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_monster_xp_hp_level.asyncWrite({
                                std::to_string(playerMonster.hp),
                                std::to_string(playerMonster.remaining_xp),
                                std::to_string(playerMonster.level),
                                std::to_string(playerMonster.sp),
                                std::to_string(playerMonster.id)
                                });
                else
                    GlobalServerData::serverPrivateVariables.preparedDBQueryCommon.db_query_update_monster_xp_hp_level.asyncWrite({
                                std::to_string(playerMonster.hp),
                                std::to_string(playerMonster.remaining_xp),
                                std::to_string(playerMonster.level),
                                std::to_string(playerMonster.id)
                                });
                #elif CATCHCHALLENGER_DB_BLACKHOLE
                #elif CATCHCHALLENGER_DB_FILE
                #else
                #error Define what do here
                #endif
                syncMonsterSkillAndEndurance(playerMonster);
                index++;
            }
        }
        if(map!=NULL)
            savePosition();
        map=NULL;
        character_id=0;
        stat=ClientStat::None;
    }

    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    if(character_id!=0)
        normalOutput("Disconnected client done");
    #endif
    #ifdef EPOLLCATCHCHALLENGERSERVER
    recordDisconnectByServer(this);
    #endif

    return true;
}

//input/ouput layer
void Client::errorParsingLayer(const std::string &error)
{
    errorOutput(error);
}

void Client::messageParsingLayer(const std::string &message) const
{
    normalOutput(message);
}

//* do the message by the general broadcast */
void Client::errorOutput(const std::string &errorString)
{
    if(stat==ClientStat::None)
    {
        #ifndef NOSINGLEPLAYER
        std::cerr << headerOutput() << "Kicked when stat==ClientStat::None by: " << errorString << std::endl;//silent if protocol not passed, to not flood the log if other client like http client (browser) is connected
        #endif
        disconnectClient();//include closeSocket();, but do it again:
        closeSocket();
        return;
    }
    if(stat==ClientStat::CharacterSelected)
        sendSystemMessage(public_and_private_informations.public_informations.pseudo+" have been kicked from server, have try hack",false);

    std::cerr << headerOutput() << "Kicked by: " << errorString << std::endl;
    disconnectClient();
}

void Client::kick()
{
    std::cerr << headerOutput() << "kicked()" << std::endl;
    disconnectClient();
}

std::string Client::headerOutput() const
{
    if(!public_and_private_informations.public_informations.pseudo.empty())
    {
        if(GlobalServerData::serverSettings.anonymous)
            return std::to_string(character_id)+": ";
        else
            return public_and_private_informations.public_informations.pseudo+": ";
    }
    else
    {
        #ifndef EPOLLCATCHCHALLENGERSERVER
        /*std::string ip;
        if(socket==NULL)
            ip="unknown";
        else
        {
            QHostAddress hostAddress=socket->peerAddress();
            if(hostAddress==QHostAddress::LocalHost || hostAddress==QHostAddress::LocalHostIPv6)
                ip="localhost:"+std::to_string(socket->peerPort());
            else if(hostAddress==QHostAddress::Null || hostAddress==QHostAddress::Any || hostAddress==QHostAddress::AnyIPv4 || hostAddress==QHostAddress::AnyIPv6 || hostAddress==QHostAddress::Broadcast)
                ip="internal";
            else
                ip=hostAddress.toString().toStdString()+":"+std::to_string(socket->peerPort());
        }
        return ip+": ";*/
        return "";
        #else
        std::string ip;
        if(socketString==NULL)
            ip="[IP]:[PORT]";
        else
            ip=socketString;
        return ip+": ";
        #endif
    }
}

void Client::normalOutput(const std::string &message) const
{
    std::cout << headerOutput() << message << std::endl;
}

std::string Client::getPseudo() const
{
    return public_and_private_informations.public_informations.pseudo;
}

void Client::dropAllClients()
{
    ProtocolParsingBase::tempBigBufferForOutput[0x00]=0x65;
    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,0x01);
}

void Client::dropAllBorderClients()
{
    ProtocolParsingBase::tempBigBufferForOutput[0x00]=0x67;
    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,0x01);
}

#ifndef EPOLLCATCHCHALLENGERSERVER
/*void Client::fake_receive_data(std::vector<char> data)
{
    fake_send_received_data(data);
}*/
#endif

void Client::parseIncommingData()
{
    ProtocolParsingInputOutput::parseIncommingData();
}

void Client::errorFightEngine(const std::string &error)
{
    errorOutput(error);
}

void Client::messageFightEngine(const std::string &message) const
{
    normalOutput(message);
}

#ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
char *Client::addAuthGetToken(const uint32_t &characterId, const uint32_t &accountIdRequester)
{
    if(characterId==0)
        std::cerr << "Client::addAuthGetToken() call with characterId=0" << std::endl;
    TokenAuth newEntry;
    newEntry.characterId=characterId;
    newEntry.accountIdRequester=accountIdRequester;
    newEntry.createTime=sFrom1970();
    newEntry.token=new char[CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER];
    const int &returnedSize=static_cast<int32_t>(fread(newEntry.token,1,CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER,BaseServerLogin::fpRandomFile));
    if(returnedSize!=CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER)
    {
        std::cerr << "sizeof(newEntry.token) don't match with random size: " << returnedSize << std::endl;
        delete[] newEntry.token;
        return NULL;
    }
    tokenAuthList.push_back(newEntry);

    if(tokenAuthList.size()>50)
    {
        LinkToMaster::linkToMaster->characterDisconnected(tokenAuthList.at(0).characterId);
        tokenAuthList.erase(tokenAuthList.begin());
    }
    return newEntry.token;
}

#ifndef CATCHCHALLENGER_DB_FILE
uint32_t Client::getMonsterId(bool * const ok)
{
    if(GlobalServerData::serverPrivateVariables.maxMonsterId.size()==0)
    {
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
    if(ok!=NULL)
        *ok=true;
    const uint32_t monsterId=GlobalServerData::serverPrivateVariables.maxMonsterId.front();
    GlobalServerData::serverPrivateVariables.maxMonsterId.erase(GlobalServerData::serverPrivateVariables.maxMonsterId.begin());
    if(GlobalServerData::serverPrivateVariables.maxMonsterId.size()<CATCHCHALLENGER_SERVER_MINIDBLOCK)
        LinkToMaster::linkToMaster->askMoreMaxMonsterId();
    return monsterId;
}
#endif

uint32_t Client::getClanId(bool * const ok)
{
    if(GlobalServerData::serverPrivateVariables.maxClanId.size()==0)
    {
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
    if(ok!=NULL)
        *ok=true;
    const uint32_t clanId=GlobalServerData::serverPrivateVariables.maxClanId.front();
    GlobalServerData::serverPrivateVariables.maxClanId.erase(GlobalServerData::serverPrivateVariables.maxClanId.begin());
    if(GlobalServerData::serverPrivateVariables.maxClanId.size()<CATCHCHALLENGER_SERVER_MINCLANIDBLOCK)
        LinkToMaster::linkToMaster->askMoreMaxClanId();
    return clanId;
}
#else
#ifndef CATCHCHALLENGER_DB_FILE
uint32_t Client::getMonsterId(bool * const ok)
{
    *ok=true;
    const uint32_t monsterId=GlobalServerData::serverPrivateVariables.maxMonsterId;
    GlobalServerData::serverPrivateVariables.maxMonsterId++;
    return monsterId;
}
#endif

uint32_t Client::getClanId(bool * const ok)
{
    *ok=true;
    const uint32_t clanId=GlobalServerData::serverPrivateVariables.maxClanId;
    GlobalServerData::serverPrivateVariables.maxClanId++;
    return clanId;
}
#endif

bool Client::characterConnected(const uint32_t &characterId)
{
    return playerById.find(characterId)!=playerById.cend();
}

void Client::disconnectClientById(const uint32_t &characterId)
{
    if(playerById.find(characterId)!=playerById.cend())
    {
        Client * client=playerById.at(characterId);
        client->disconnectClient();
    }
}

bool Client::haveBeatBot(const CATCHCHALLENGER_TYPE_MAPID &mapId,const CATCHCHALLENGER_TYPE_BOTID &botId) const
{
    if(mapData.find(mapId)==mapData.cend())
        return false;
    const Player_private_and_public_informations_Map &tempMapData=mapData.at(mapId);
    if(tempMapData.bot_already_beaten.find(botId)==tempMapData.bot_already_beaten.cend())
        return false;
    return true;
}

#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
void Client::askStatClient(const uint8_t &query_id,const char *rawdata)
{
    #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryLogin::db_query_login.empty())
    {
        errorParsingLayer("askLogin() Query login is empty, bug");
        return;
    }
    #endif
    #elif CATCHCHALLENGER_DB_BLACKHOLE
    #elif CATCHCHALLENGER_DB_FILE
    #else
    #error Define what do here
    #endif

    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    std::vector<char> tempAddedToken;
    std::vector<char> secretTokenBinary;
    #endif
    {
        int32_t tokenForAuthIndex=0;
        while((uint32_t)tokenForAuthIndex<BaseServerLogin::tokenForAuthSize)
        {
            const BaseServerLogin::TokenLink &tokenLink=BaseServerLogin::tokenForAuth[tokenForAuthIndex];
            if(tokenLink.client==this)
            {
                //append the token
                memcpy(Client::private_token_statclient+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT,tokenLink.value,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);

                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                secretTokenBinary.resize(TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                memcpy(secretTokenBinary.data(),tokenLink.value,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                tempAddedToken.resize(TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                memcpy(tempAddedToken.data(),tokenLink.value,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                if(secretTokenBinary.size()!=(TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT))
                {
                    std::cerr << "secretTokenBinary.size()!=(CATCHCHALLENGER_SHA224HASH_SIZE+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT)" << std::endl;
                    abort();
                }
                #endif
                SHA224 ctx = SHA224();
                ctx.init();
                ctx.update(Client::private_token_statclient,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                ctx.final(reinterpret_cast<unsigned char *>(ProtocolParsingBase::tempBigBufferForOutput));

                BaseServerLogin::tokenForAuthSize--;
                //see to do with SIMD
                if(BaseServerLogin::tokenForAuthSize>0)
                {
                    while((uint32_t)tokenForAuthIndex<BaseServerLogin::tokenForAuthSize)
                    {
                        BaseServerLogin::tokenForAuth[tokenForAuthIndex]=BaseServerLogin::tokenForAuth[tokenForAuthIndex+1];
                        tokenForAuthIndex++;
                    }
                    //don't work:memmove(BaseServerLogin::tokenForAuth+index*sizeof(TokenLink),BaseServerLogin::tokenForAuth+index*sizeof(TokenLink)+sizeof(TokenLink),sizeof(TokenLink)*(BaseServerLogin::tokenForAuthSize-index));
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(BaseServerLogin::tokenForAuth[0].client==NULL)
                        abort();
                    #endif
                }
                tokenForAuthIndex--;
                break;
            }
            tokenForAuthIndex++;
        }
        if(tokenForAuthIndex>=(int32_t)BaseServerLogin::tokenForAuthSize)
        {
            removeFromQueryReceived(query_id);//all list dropped at client destruction
            ProtocolParsingBase::tempBigBufferForOutput[0x00]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
            ProtocolParsingBase::tempBigBufferForOutput[0x01]=query_id;
            ProtocolParsingBase::tempBigBufferForOutput[0x02]=0x02;
            internalSendRawSmallPacket(reinterpret_cast<char *>(ProtocolParsingBase::tempBigBufferForOutput),3);
            errorParsingLayer("No temp auth token found");
            return;
        }
    }
    if(memcmp(ProtocolParsingBase::tempBigBufferForOutput,rawdata,CATCHCHALLENGER_SHA224HASH_SIZE)!=0)
    {
        removeFromQueryReceived(query_id);//all list dropped at client destruction
        ProtocolParsingBase::tempBigBufferForOutput[0x00]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
        ProtocolParsingBase::tempBigBufferForOutput[0x01]=query_id;
        ProtocolParsingBase::tempBigBufferForOutput[0x02]=0x02;
        internalSendRawSmallPacket(reinterpret_cast<char *>(ProtocolParsingBase::tempBigBufferForOutput),3);
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        errorParsingLayer("Password wrong: "+
                     binarytoHexa(secretTokenBinary)+
                     " + token "+
                     binarytoHexa(tempAddedToken)+
                     " = "+
                     " hashedToken: "+
                     binarytoHexa(ProtocolParsingBase::tempBigBufferForOutput,CATCHCHALLENGER_SHA224HASH_SIZE)+
                     "sended pass + token: "+
                     binarytoHexa(rawdata,CATCHCHALLENGER_SHA224HASH_SIZE)
                     );
        #else
        errorParsingLayer("Password wrong: "+
                     binarytoHexa(rawdata,CATCHCHALLENGER_SHA224HASH_SIZE)
                     );
        #endif
        return;
    }
    else
    {
        removeFromQueryReceived(query_id);//all list dropped at client destruction
        ProtocolParsingBase::tempBigBufferForOutput[0x00]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
        ProtocolParsingBase::tempBigBufferForOutput[0x01]=query_id;
        ProtocolParsingBase::tempBigBufferForOutput[0x02]=0x01;
        internalSendRawSmallPacket(reinterpret_cast<char *>(ProtocolParsingBase::tempBigBufferForOutput),3);
        internalSendRawSmallPacket((char *)Client::protocolMessageLogicalGroupAndServerList,Client::protocolMessageLogicalGroupAndServerListSize);

        stat_client=true;
        //flags|=0x08;->just listen

        stat_client_list.push_back(this);

        stat=ClientStat::LoggedStatClient;
    }
}
#endif

void Client::breakNeedMoreData()
{
    if(stat==Client::None)
    {
        disconnectClient();
        return;
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    //std::cerr << "Break due to need more in parse data" << std::endl;
    #endif
}

//Client::sendPing() can be call by another function using the buffer like Map_server_MapVisibility_Simple_StoreOnSender::purgeBuffer(), then use their own
void Client::sendPing()
{
    if(queryNumberList.empty())
    {
        errorOutput("Sorry, no free query number to send this query of teleportation");
        return;
    }

    if(pingInProgress<255)
        pingInProgress++;

    //send the network reply
    char buffer[2];//Client::sendPing() can be call by another function using the buffer like Map_server_MapVisibility_Simple_StoreOnSender::purgeBuffer(), then use their own
    buffer[0x00]=0xE3;
    buffer[0x01]=queryNumberList.back();
    registerOutputQuery(queryNumberList.back(),0xE3);

    sendRawBlock(buffer,sizeof(buffer));

    queryNumberList.pop_back();
}

uint8_t Client::pingCountInProgress() const
{
    return pingInProgress;
}

#ifdef CATCHCHALLENGER_DB_FILE
#ifdef CATCHCHALLENGER_CACHE_HPS
void Client::serialize(hps::StreamOutputBuffer& buf) const {
    /// \warning use dictionary

    buf << character_id;
    std::string recipesS;
    if(public_and_private_informations.recipes!=nullptr)
        recipesS=std::string(public_and_private_informations.recipes,CommonDatapack::commonDatapack.get_craftingRecipesMaxId()/8+1);
    std::string encyclopedia_monsterS;
    if(public_and_private_informations.encyclopedia_monster!=nullptr)
        encyclopedia_monsterS=std::string(public_and_private_informations.encyclopedia_monster,CommonDatapack::commonDatapack.get_monstersMaxId()/8+1);
    std::string encyclopedia_itemS;
    if(public_and_private_informations.encyclopedia_item!=nullptr)
        encyclopedia_itemS=std::string(public_and_private_informations.encyclopedia_item,CommonDatapack::commonDatapack.get_items().item.size()/8+1);
    std::string bot_already_beatenS;

    //serialise list of bot fight in form: map DB id + id
    if(public_and_private_informations.bot_already_beaten!=nullptr)
        bot_already_beatenS=std::string(public_and_private_informations.bot_already_beaten,CommonDatapackServerSpec::commonDatapackServerSpec.get_botFightsMaxId()/8+1);

    buf << public_and_private_informations.public_informations << public_and_private_informations.cash << public_and_private_informations.warehouse_cash << recipesS
        << public_and_private_informations.playerMonster << public_and_private_informations.warehouse_playerMonster << encyclopedia_monsterS << encyclopedia_itemS
        << public_and_private_informations.repel_step << public_and_private_informations.clan_leader << bot_already_beatenS << public_and_private_informations.itemOnMap
        << public_and_private_informations.plantOnMap << public_and_private_informations.quests << public_and_private_informations.reputation
        << public_and_private_informations.items << public_and_private_informations.warehouse_items;
    buf << mapData;
    const uint8_t allowSize=public_and_private_informations.allow.size();
    buf << allowSize;
    std::set<ActionAllow>::iterator it;
    for (it = public_and_private_informations.allow.begin(); it != public_and_private_informations.allow.end(); ++it) {
        buf << (uint8_t)*it;
    }

    buf << ableToFight;
    buf << wildMonsters;
    buf << botFightMonsters;
    buf << randomIndex << randomSize << number_of_character;
    buf << questsDrop << connectedSince << profileIndex << queryNumberList;
    buf << botFightCash << botFight << isInCityCapture;

    uint32_t map_file_database_id=0;
    uint32_t rescue_map_file_database_id=0;
    uint32_t unvalidated_rescue_map_file_database_id=0;
    if(map_entry.map!=nullptr)
        map_file_database_id=static_cast<MapServer *>(map_entry.map)->id_db;
    if(rescue.map!=nullptr)
        rescue_map_file_database_id=static_cast<MapServer *>(rescue.map)->id_db;
    if(unvalidated_rescue.map!=nullptr)
        unvalidated_rescue_map_file_database_id=static_cast<MapServer *>(unvalidated_rescue.map)->id_db;
    buf << map_file_database_id << map_entry.x << map_entry.y << (uint8_t)map_entry.orientation;
    buf << rescue_map_file_database_id << rescue.x << rescue.y << (uint8_t)rescue.orientation;
    buf << unvalidated_rescue_map_file_database_id << unvalidated_rescue.x << unvalidated_rescue.y << (uint8_t)unvalidated_rescue.orientation;

    uint32_t map_id=0;
    if(map!=nullptr)
        map_id=static_cast<MapServer *>(map)->id_db;
    buf << map_id << x << y << (uint8_t)last_direction;
}

void Client::parse(hps::StreamInputBuffer& buf) {
    /// \warning use dictionary

    auto temp_character_id=character_id;
    buf >> temp_character_id;
    std::string recipesS;
    std::string encyclopedia_monsterS;
    std::string encyclopedia_itemS;
    std::string bot_already_beatenS;
    buf >> public_and_private_informations.public_informations >> public_and_private_informations.cash >> public_and_private_informations.warehouse_cash
        >> recipesS >> public_and_private_informations.playerMonster >> public_and_private_informations.warehouse_playerMonster
        >> encyclopedia_monsterS >> encyclopedia_itemS
        >> public_and_private_informations.repel_step >> public_and_private_informations.clan_leader >> bot_already_beatenS;
    buf >> public_and_private_informations.itemOnMap >> public_and_private_informations.plantOnMap >> public_and_private_informations.quests
        >> public_and_private_informations.reputation >> public_and_private_informations.items >> public_and_private_informations.warehouse_items;
    buf >> mapData;
    uint8_t allowSize=0;
    buf >> allowSize;
    {
        unsigned int index=0;
        while(index<allowSize)
        {
            uint8_t t=0;
            buf >> t;
            public_and_private_informations.allow.insert((ActionAllow)t);
            index++;
        }
    }
    public_and_private_informations.recipes=(char *)malloc(CommonDatapack::commonDatapack.get_craftingRecipesMaxId()/8+1);
    memset(public_and_private_informations.recipes,0x00,CommonDatapack::commonDatapack.get_craftingRecipesMaxId()/8+1);
    size_t min=CommonDatapack::commonDatapack.get_craftingRecipesMaxId()/8+1;
    if(min>recipesS.size())
        min=recipesS.size();
    memcpy(public_and_private_informations.recipes,recipesS.data(),min);
    public_and_private_informations.encyclopedia_monster=(char *)malloc(CommonDatapack::commonDatapack.get_monstersMaxId()/8+1);
    memset(public_and_private_informations.encyclopedia_monster,0x00,CommonDatapack::commonDatapack.get_monstersMaxId()/8+1);
    min=CommonDatapack::commonDatapack.get_monstersMaxId()/8+1;
    if(min>encyclopedia_monsterS.size())
        min=encyclopedia_monsterS.size();
    memcpy(public_and_private_informations.encyclopedia_monster,encyclopedia_monsterS.data(),min);
    public_and_private_informations.encyclopedia_item=(char *)malloc(CommonDatapack::commonDatapack.get_items().item.size()/8+1);
    memset(public_and_private_informations.encyclopedia_item,0x00,CommonDatapack::commonDatapack.get_items().item.size()/8+1);
    min=CommonDatapack::commonDatapack.get_items().item.size()/8+1;
    if(min>encyclopedia_itemS.size())
        min=encyclopedia_itemS.size();
    memcpy(public_and_private_informations.encyclopedia_item,encyclopedia_itemS.data(),min);
    public_and_private_informations.bot_already_beaten=(char *)malloc(CommonDatapackServerSpec::commonDatapackServerSpec.get_botFightsMaxId()/8+1);
    memset(public_and_private_informations.bot_already_beaten,0x00,CommonDatapackServerSpec::commonDatapackServerSpec.get_botFightsMaxId()/8+1);
    min=CommonDatapackServerSpec::commonDatapackServerSpec.get_botFightsMaxId()/8+1;
    if(min>bot_already_beatenS.size())
        min=bot_already_beatenS.size();
    memcpy(public_and_private_informations.bot_already_beaten,bot_already_beatenS.data(),min);

    buf >> ableToFight;
    buf >> wildMonsters;
    buf >> botFightMonsters;
    buf >> randomIndex >> randomSize >> number_of_character;
    buf >> questsDrop >> connectedSince >> profileIndex >> queryNumberList;
    buf >> botFightCash >> botFightId >> isInCityCapture;

    uint8_t value=0;
    uint32_t map_file_database_id=0;
    uint32_t rescue_map_file_database_id=0;
    uint32_t unvalidated_rescue_map_file_database_id=0;
    buf >> map_file_database_id >> map_entry.x >> map_entry.y >> value;
    map_entry.orientation=(Orientation)value;
    buf >> rescue_map_file_database_id >> rescue.x >> rescue.y >> value;
    rescue.orientation=(Orientation)value;
    buf >> unvalidated_rescue_map_file_database_id >> unvalidated_rescue.x >> unvalidated_rescue.y >> value;
    unvalidated_rescue.orientation=(Orientation)value;

    if(map_file_database_id>=(uint32_t)DictionaryServer::dictionary_map_database_to_internal.size())
    {
        std::cerr << "map_file_database_id out of range" << __FILE__ << ":" << __LINE__ << std::endl;
        abort();
    }
    map_entry.map=static_cast<CommonMap *>(DictionaryServer::dictionary_map_database_to_internal.at(map_file_database_id));
    if(map_entry.map==nullptr)
    {
        std::cerr << "map_file_database_id have not reverse: " << std::to_string(map_file_database_id) << ", mostly due to start previously start with another mainDatapackCode " << __FILE__ << ":" << __LINE__ << std::endl;
        unsigned int index=0;
        while(index<DictionaryServer::dictionary_map_database_to_internal.size())
        {
            const MapServer * const s=DictionaryServer::dictionary_map_database_to_internal.at(index);
            if(s!=nullptr)
                std::cerr << "map_file_database_id["+std::to_string(index)+"]="+s->map_file << std::endl;
            index++;
        }
        abort();
    }
    if(rescue_map_file_database_id>=(uint32_t)DictionaryServer::dictionary_map_database_to_internal.size())
    {
        std::cerr << "rescue_map_file_database_id out of range" << __FILE__ << ":" << __LINE__ << std::endl;
        abort();
    }
    rescue.map=static_cast<CommonMap *>(DictionaryServer::dictionary_map_database_to_internal.at(rescue_map_file_database_id));
    if(rescue.map==nullptr)
    {
        std::cerr << "rescue_map_file_database_id have not reverse: " << std::to_string(rescue_map_file_database_id) << ", mostly due to start previously start with another mainDatapackCode " << __FILE__ << ":" << __LINE__ << std::endl;
        unsigned int index=0;
        while(index<DictionaryServer::dictionary_map_database_to_internal.size())
        {
            const MapServer * const s=DictionaryServer::dictionary_map_database_to_internal.at(index);
            if(s!=nullptr)
                std::cerr << "map_file_database_id["+std::to_string(index)+"]="+s->map_file << std::endl;
            index++;
        }
        abort();
    }
    if(unvalidated_rescue_map_file_database_id>=(uint32_t)DictionaryServer::dictionary_map_database_to_internal.size())
    {
        std::cerr << "unvalidated_rescue_map_file_database_id out of range" << __FILE__ << ":" << __LINE__ << std::endl;
        abort();
    }
    unvalidated_rescue.map=static_cast<CommonMap *>(DictionaryServer::dictionary_map_database_to_internal.at(unvalidated_rescue_map_file_database_id));
    if(unvalidated_rescue.map==nullptr)
    {
        std::cerr << "unvalidated_rescue_map_file_database_id have not reverse: " << std::to_string(unvalidated_rescue_map_file_database_id) << ", mostly due to start previously start with another mainDatapackCode " << __FILE__ << ":" << __LINE__ << std::endl;
        unsigned int index=0;
        while(index<DictionaryServer::dictionary_map_database_to_internal.size())
        {
            const MapServer * const s=DictionaryServer::dictionary_map_database_to_internal.at(index);
            if(s!=nullptr)
                std::cerr << "map_file_database_id["+std::to_string(index)+"]="+s->map_file << std::endl;
            index++;
        }
        abort();
    }

    uint32_t map_id=0;
    uint8_t temp_direction=0;
    buf >> map_id >> x >> y >> temp_direction;
    if(map_id>=(uint32_t)DictionaryServer::dictionary_map_database_to_internal.size())
    {
        std::cerr << "map_file_database_id out of range" << __FILE__ << ":" << __LINE__ << std::endl;
        abort();
    }
    map=static_cast<CommonMap *>(DictionaryServer::dictionary_map_database_to_internal.at(map_id));
    if(map==nullptr)
    {
        std::cerr << "map_file_database_id have not reverse: " << std::to_string(map_file_database_id) << ", mostly due to start previously start with another mainDatapackCode " << __FILE__ << ":" << __LINE__ << std::endl;
        unsigned int index=0;
        while(index<DictionaryServer::dictionary_map_database_to_internal.size())
        {
            const MapServer * const s=DictionaryServer::dictionary_map_database_to_internal.at(index);
            if(s!=nullptr)
                std::cerr << "map_file_database_id["+std::to_string(index)+"]="+s->map_file << std::endl;
            index++;
        }
        abort();
    }
}
#endif
#endif
