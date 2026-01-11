#include "Client.hpp"
#include "MapManagement/ClientWithMap.hpp"
#include "ClientList.hpp"
#include "GlobalServerData.hpp"
#include "../base/PreparedDBQuery.hpp"
#ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
#include "../game-server-alone/LinkToMaster.hpp"
#else
#include "../../general/sha224/sha224.hpp"
#endif
#include "../../general/base/CommonSettingsServer.hpp"

#include "BaseServer/BaseServerLogin.hpp"
#include "MapManagement/Map_server_MapVisibility_Simple_StoreOnSender.hpp"

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

Client::Client(const uint16_t &index_connected_player) :
    ProtocolParsingInputOutput(
        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
        PacketModeTransmission_Server
        #endif
        )
{
    this->index_connected_player=index_connected_player;
    setToDefault();
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    ClientBase::public_and_private_informations_solo=&public_and_private_informations;
    #endif
    queryNumberList.reserve(CATCHCHALLENGER_MAXPROTOCOLQUERY);
    {
        uint8_t index=0;
        while(index<CATCHCHALLENGER_MAXPROTOCOLQUERY)
        {
            queryNumberList.push_back(index);
            index++;
        }
    }
}

void Client::setToDefault()
{
    //protocol
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    parseIncommingDataCount=0;
    #endif
    #ifndef EPOLLCATCHCHALLENGERSERVER
    RXSize=0;
    TXSize=0;
    #endif
    memset(inputQueryNumberToPacketCode,0x00,sizeof(inputQueryNumberToPacketCode));

    //map move
    mapIndex=65535;
    x=0;
    y=0;
    last_direction=Direction_look_at_bottom;

    //player info
    public_and_private_informations.public_informations.monsterId=0;
    public_and_private_informations.public_informations.pseudo.clear();
    public_and_private_informations.public_informations.skinId=0;
    public_and_private_informations.public_informations.type=Player_type_normal;
    public_and_private_informations.cash=0;
    public_and_private_informations.recipes=nullptr;
    public_and_private_informations.monsters.clear();
    public_and_private_informations.warehouse_monsters.clear();
    public_and_private_informations.clan=0;
    public_and_private_informations.encyclopedia_monster=nullptr;
    public_and_private_informations.encyclopedia_item=nullptr;
    public_and_private_informations.repel_step=0;
    public_and_private_informations.clan_leader=false;
    public_and_private_informations.mapData.clear();
    public_and_private_informations.quests.clear();
    public_and_private_informations.reputation.clear();
    public_and_private_informations.items.clear();

    //Fight engine
    CommonFightEngine::resetAll();
    botFight.first=65535;
    botFight.second=0;

    //local
    mapSyncMiss=false;
    stat=ClientStat::Free;
    lastdaillygift=0;
    pingInProgress=0;

    //disable reserve
    if(otherPlayerBattle!=SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX)
    {
        Client &otherPlayerTrade=ClientList::list->rw(this->otherPlayerTrade);
        otherPlayerTrade.battleCanceled();
        otherPlayerTrade.otherPlayerBattle=SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX;
        otherPlayerBattle=SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX;
    }

    index_on_map=SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX;
    account_id_db=0;
    character_id_db=0;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    isConnected=true;
    #endif
    randomIndex=0;
    randomSize=0;
    number_of_character=0;
    #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    datapackStatus=DatapackStatus::Main;
    #else
    datapackStatus=DatapackStatus::Base;
    #endif
    map_entry.mapIndex=65535;
    map_entry.orientation=Orientation_bottom;
    map_entry.x=0;
    map_entry.y=0;
    rescue.mapIndex=65535;
    rescue.orientation=Orientation_bottom;
    rescue.x=0;
    rescue.y=0;
    unvalidated_rescue.mapIndex=65535;
    unvalidated_rescue.orientation=Orientation_bottom;
    unvalidated_rescue.x=0;
    unvalidated_rescue.y=0;
    #ifdef CATCHCHALLENGER_DDOS_FILTER
    movePacketKick.reset();
    chatPacketKick.reset();
    otherPacketKick.reset();
    #endif
    while(!lastTeleportation.empty())
        lastTeleportation.pop();
    queryNumberList.clear();
    botFight.first=65535;
    botFight.second=255;
    attackReturn.clear();
    deferedEnduranceSync.clear();
    tradeObjects.clear();
    tradeMonster.clear();
    inviteToClanList.clear();
    questsDrop.clear();
    connectedSince=0;
    last_sended_connected_players=0;
    stopIt=false;
    profileIndex=0;
    battleIsValidated=false;
    mCurrentSkillId=0;
    mHaveCurrentSkill=false;
    mMonsterChange=false;

    while(!paramToPassToCallBack.empty())
        paramToPassToCallBack.pop();
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    while(!paramToPassToCallBackType.empty())
        paramToPassToCallBackType.pop();
    #endif

    botFight=std::pair<CATCHCHALLENGER_TYPE_MAPID/*mapId*/,uint8_t/*botId*/>(0,0);
    #ifndef EPOLLCATCHCHALLENGERSERVER
    isInCityCapture=false;
    #endif
    tradeIsValidated=false;
    #ifdef EPOLLCATCHCHALLENGERSERVER
    socketString=NULL;
    socketStringSize=0;
    #endif
    #ifndef EPOLLCATCHCHALLENGERSERVER
    otherCityPlayerBattle=NULL;
    #endif
}

//need be call after isReadyToDelete() emited
Client::~Client()
{
    std::cerr << "should never pass here, client now should just set to free slot" << std::endl;
    abort();
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

SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED Client::getIndexConnect() const
{
    return index_connected_player;
}

Client::ClientStat Client::getClientStat() const
{
    return stat;
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
    if(account_id_db==0)
    {
        closeSocket();
        return false;
    }
    account_id_db=0;
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
    /*if(stat==ClientStat::LoggedStatClient)
    {
        done into ClientList::remove(const Client &client)
         * unsigned int index=0;
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
    }*/
    #endif

    if(stat==ClientStat::CharacterSelecting)
    {
        stat=ClientStat::CharacterSelected;//to block the new connection util th destructor is invocked
        GlobalServerData::serverPrivateVariables.playerById_db.erase(character_id_db);
        //simplifiedIdList.push_back(public_and_private_informations.public_informations.simplifiedId);
        #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
        if(character_id==0)
            std::cerr << "character_id==0, set it before to correctly unload to master" << std::endl;
        else
            LinkToMaster::linkToMaster->characterDisconnected(character_id);
        #endif
    }
    else if(stat==ClientStat::CharacterSelected)
    {
        //const std::string &character_id_string=std::to_string(character_id_db);
        if(mapIndex<65535)
            removeClientOnMap(Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list[mapIndex]);
        /*        if(!vectorremoveOne(clientBroadCastList,this))
            std::cout << "Client::disconnectClient(): vectorremoveOne(clientBroadCastList,this)" << std::endl;*/
        if(GlobalServerData::serverSettings.sendPlayerNumber)
            GlobalServerData::serverPrivateVariables.player_updater->removeConnectedPlayer();
        //extraStop();
        tradeCanceled();
        battleCanceled();
        removeFromClan();
        //simplifiedIdList.push_back(public_and_private_informations.public_informations.simplifiedId);
        GlobalServerData::serverPrivateVariables.playerById_db.erase(character_id_db);
        #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
        LinkToMaster::linkToMaster->characterDisconnected(character_id);
        #endif
        //playerByPseudo.erase(public_and_private_informations.public_informations.pseudo); done by ClientList::remove(const Client &client)
        clanChangeWithoutDb(0);
        ClientList::list->remove(*this);
        /*playerByPseudo.erase(public_and_private_informations.public_informations.pseudo);
        playerById_db.erase(character_id_db);
        characterCreationDateList.erase(character_id_db);*/
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
        if(isInFight())
            saveCurrentMonsterStat();
        if(mapIndex<65535)
            savePosition();
        mapIndex=65535;
        character_id_db=0;
        stat=ClientStat::None;
    }

    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    if(character_id!=0)
        normalOutput("Disconnected client done");
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
            return std::to_string(character_id_db)+": ";
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
    std::cerr << "seam buggy function, see client/libcatchchallenger/Api_protocol_message.cpp" << std::endl;
    abort();
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

uint32_t Client::getMaxClanId(bool * const ok)
{
    *ok=true;
    const uint32_t clanId=GlobalServerData::serverPrivateVariables.maxClanId;
    GlobalServerData::serverPrivateVariables.maxClanId++;
    return clanId;
}
#endif

bool Client::characterConnected_db(const uint32_t &characterId_db)
{
    return GlobalServerData::serverPrivateVariables.playerById_db.find(characterId_db)!=GlobalServerData::serverPrivateVariables.playerById_db.cend();
}

void Client::disconnectClientById_db(const uint32_t &characterId_db)
{
    if(GlobalServerData::serverPrivateVariables.playerById_db.find(characterId_db)!=GlobalServerData::serverPrivateVariables.playerById_db.cend())
    {
        const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED &index=GlobalServerData::serverPrivateVariables.playerById_db.at(characterId_db);
        if(!ClientList::list->empty(index))
            ClientList::list->rw(index).disconnectClient();
    }
}

bool Client::haveBeatBot(const CATCHCHALLENGER_TYPE_MAPID &mapId,const CATCHCHALLENGER_TYPE_BOTID &botId) const
{
    if(public_and_private_informations.mapData.find(mapId)==public_and_private_informations.mapData.cend())
        return false;
    const Player_private_and_public_informations_Map &tempMapData=public_and_private_informations.mapData.at(mapId);
    if(tempMapData.bots_beaten.find(botId)==tempMapData.bots_beaten.cend())
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

        //stat_client=true;
        //flags|=0x08;->just listen

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

size_t Client::sendPing(char * output)
{
    if(queryNumberList.empty())
    {
        errorOutput("Sorry, no free query number to send this query of teleportation");
        return 0;
    }

    if(pingInProgress<255)
        pingInProgress++;

    //send the network reply
    output[0x00]=0xE3;
    output[0x01]=queryNumberList.back();
    registerOutputQuery(queryNumberList.back(),0xE3);

    queryNumberList.pop_back();
    return 2;
}

uint8_t Client::pingCountInProgress() const
{
    return pingInProgress;
}

#ifdef CATCHCHALLENGER_DB_FILE
#ifdef CATCHCHALLENGER_CACHE_HPS
void Client::serialize(hps::StreamOutputBuffer& buf) const {
    /// \warning use dictionary

    buf << character_id_db;
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

    //see Player_private_and_public_informations_Map
    //serialise list of bot fight in form: map DB id + id

    buf << public_and_private_informations.public_informations << public_and_private_informations.cash << recipesS
        << public_and_private_informations.monsters << public_and_private_informations.warehouse_monsters << encyclopedia_monsterS << encyclopedia_itemS
        << public_and_private_informations.repel_step << public_and_private_informations.clan_leader << bot_already_beatenS
        << public_and_private_informations.quests << public_and_private_informations.reputation
        << public_and_private_informations.items;
    buf << public_and_private_informations.mapData << public_and_private_informations.allowCreateClan;

    buf << ableToFight;
    buf << wildMonsters;
    buf << botFightMonsters;
    buf << randomIndex << randomSize << number_of_character;
    buf << questsDrop << connectedSince << profileIndex << queryNumberList;
    buf << botFight << isInCityCapture;

    CATCHCHALLENGER_TYPE_MAPID map_file_database_id=0;
    CATCHCHALLENGER_TYPE_MAPID rescue_map_file_database_id=0;
    CATCHCHALLENGER_TYPE_MAPID unvalidated_rescue_map_file_database_id=0;
    map_file_database_id=Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list.at(map_entry.mapIndex).id_db;
    rescue_map_file_database_id=Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list.at(rescue.mapIndex).id_db;
    unvalidated_rescue_map_file_database_id=Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list.at(unvalidated_rescue.mapIndex).id_db;
    buf << map_file_database_id << map_entry.x << map_entry.y << (uint8_t)map_entry.orientation;
    buf << rescue_map_file_database_id << rescue.x << rescue.y << (uint8_t)rescue.orientation;
    buf << unvalidated_rescue_map_file_database_id << unvalidated_rescue.x << unvalidated_rescue.y << (uint8_t)unvalidated_rescue.orientation;

    CATCHCHALLENGER_TYPE_MAPID map_id=0;
    map_id=Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list.at(mapIndex).id_db;
    buf << map_id << x << y << (uint8_t)last_direction;
}

void Client::parse(hps::StreamInputBuffer& buf) {
    /// \warning use dictionary

    auto temp_character_id=character_id_db;
    buf >> temp_character_id;
    std::string recipesS;
    std::string encyclopedia_monsterS;
    std::string encyclopedia_itemS;
    buf >> public_and_private_informations.public_informations >> public_and_private_informations.cash
        >> recipesS >> public_and_private_informations.monsters >> public_and_private_informations.warehouse_monsters
        >> encyclopedia_monsterS >> encyclopedia_itemS
        >> public_and_private_informations.repel_step >> public_and_private_informations.clan_leader;
    buf >> public_and_private_informations.quests
        >> public_and_private_informations.reputation >> public_and_private_informations.items;
    buf >> public_and_private_informations.mapData >> public_and_private_informations.allowCreateClan;
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

    buf >> ableToFight;
    buf >> wildMonsters;
    buf >> botFightMonsters;
    buf >> randomIndex >> randomSize >> number_of_character;
    buf >> questsDrop >> connectedSince >> profileIndex >> queryNumberList;
    buf >> isInCityCapture;

    uint8_t value=0;
    CATCHCHALLENGER_TYPE_MAPID map_file_database_id=0;
    CATCHCHALLENGER_TYPE_MAPID rescue_map_file_database_id=0;
    CATCHCHALLENGER_TYPE_MAPID unvalidated_rescue_map_file_database_id=0;
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
    if(map_file_database_id>=DictionaryServer::dictionary_map_database_to_internal.size())
    {
        std::cerr << "map_file_database_id out of range" << __FILE__ << ":" << __LINE__ << std::endl;
        abort();
    }
    map_entry.mapIndex=DictionaryServer::dictionary_map_database_to_internal.at(map_file_database_id);
    if(map_entry.mapIndex>=65535)
    {
        std::cerr << "map_entry.mapIndex>=65535, mostly due to start previously start with another mainDatapackCode " << __FILE__ << ":" << __LINE__ << std::endl;
        abort();
    }
    if(rescue_map_file_database_id>=DictionaryServer::dictionary_map_database_to_internal.size())
    {
        std::cerr << "rescue_map_file_database_id out of range" << __FILE__ << ":" << __LINE__ << std::endl;
        abort();
    }
    if(rescue_map_file_database_id>=(uint32_t)DictionaryServer::dictionary_map_database_to_internal.size())
    {
        std::cerr << "rescue_map_file_database_id out of range" << __FILE__ << ":" << __LINE__ << std::endl;
        abort();
    }
    rescue.mapIndex=DictionaryServer::dictionary_map_database_to_internal.at(rescue_map_file_database_id);
    if(rescue.mapIndex>=65535)
    {
        std::cerr << "rescue.mapIndex>=65535, mostly due to start previously start with another mainDatapackCode " << __FILE__ << ":" << __LINE__ << std::endl;
        abort();
    }
    if(map_file_database_id>=DictionaryServer::dictionary_map_database_to_internal.size())
    {
        std::cerr << "map_file_database_id out of range" << __FILE__ << ":" << __LINE__ << std::endl;
        abort();
    }
    if(unvalidated_rescue_map_file_database_id>=(uint32_t)DictionaryServer::dictionary_map_database_to_internal.size())
    {
        std::cerr << "unvalidated_rescue_map_file_database_id out of range" << __FILE__ << ":" << __LINE__ << std::endl;
        abort();
    }
    unvalidated_rescue.mapIndex=DictionaryServer::dictionary_map_database_to_internal.at(unvalidated_rescue_map_file_database_id);
    if(unvalidated_rescue.mapIndex>=65535)
    {
        std::cerr << "unvalidated_rescue.mapIndex>=65535, mostly due to start previously start with another mainDatapackCode " << __FILE__ << ":" << __LINE__ << std::endl;
        abort();
    }

    CATCHCHALLENGER_TYPE_MAPID map_id=0;
    uint8_t temp_direction=0;
    buf >> map_id >> x >> y >> temp_direction;
    if(map_file_database_id>=DictionaryServer::dictionary_map_database_to_internal.size())
    {
        std::cerr << "map_file_database_id out of range" << __FILE__ << ":" << __LINE__ << std::endl;
        abort();
    }
    if(map_id>=(uint32_t)DictionaryServer::dictionary_map_database_to_internal.size())
    {
        std::cerr << "map_file_database_id out of range" << __FILE__ << ":" << __LINE__ << std::endl;
        abort();
    }
    mapIndex=DictionaryServer::dictionary_map_database_to_internal.at(map_id);
    if(mapIndex>=65535)
    {
        std::cerr << "mapIndex>=65535, mostly due to start previously start with another mainDatapackCode " << __FILE__ << ":" << __LINE__ << std::endl;
        abort();
    }
}
#endif
#endif
