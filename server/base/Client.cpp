#include "Client.h"
#include "GlobalServerData.h"
#include "../base/PreparedDBQuery.h"
#include "../../general/base/QFakeSocket.h"
#include "../../general/base/GeneralType.h"
#ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
#include "../game-server-alone/LinkToMaster.h"
#else
#include <openssl/sha.h>
#endif

#include "BaseServerLogin.h"

#include <chrono>

using namespace CatchChallenger;

/// \warning never cross the signals from and to the different client, complexity ^2
/// \todo drop instant player number notification, and before do the signal without signal/slot, check if the number have change
/// \todo change push position recording, from ClientMapManagement to ClientLocalCalcule, to disable ALL operation for MapVisibilityAlgorithm_None

Client::Client(
        #ifdef EPOLLCATCHCHALLENGERSERVER
            #ifdef SERVERSSL
                const int &infd, SSL_CTX *ctx
            #else
                const int &infd
            #endif
        #else
        ConnectedSocket *socket
        #endif
        ) :
    ProtocolParsingInputOutput(
        #ifdef EPOLLCATCHCHALLENGERSERVER
            #ifdef SERVERSSL
                infd,ctx
            #else
                infd
            #endif
        #else
        socket
        #endif
        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
        ,PacketModeTransmission_Server
        #endif
        ),
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    stat_client(false),
    #endif
    stat(ClientStat::None),
    account_id(0),
    character_id(0),
    market_cash(0),
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
    botFightId(0),
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
    public_and_private_informations.recipes=NULL;
    public_and_private_informations.encyclopedia_monster=NULL;
    public_and_private_informations.encyclopedia_item=NULL;
    public_and_private_informations.bot_already_beaten=NULL;
    queryNumberList.reserve(CATCHCHALLENGER_MAXPROTOCOLQUERY);
    {
        int index=0;
        while(index<CATCHCHALLENGER_MAXPROTOCOLQUERY)
        {
            queryNumberList.push_back(index);
            index++;
        }
    }
}

//need be call after isReadyToDelete() emited
Client::~Client()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    ClientBase::public_and_private_informations_solo=NULL;
    #endif
    if(public_and_private_informations.recipes!=NULL)
    {
        delete public_and_private_informations.recipes;
        public_and_private_informations.recipes=NULL;
    }
    if(public_and_private_informations.encyclopedia_monster!=NULL)
    {
        delete public_and_private_informations.encyclopedia_monster;
        public_and_private_informations.encyclopedia_monster=NULL;
    }
    if(public_and_private_informations.encyclopedia_item!=NULL)
    {
        delete public_and_private_informations.encyclopedia_item;
        public_and_private_informations.encyclopedia_item=NULL;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    //normalOutput("Destroyed client");
    #endif
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(socket!=NULL)
    {
        delete socket;
        socket=NULL;
    }
    #else
    if(socketString!=NULL)
        delete socketString;
    #endif
    //SQL
    {
        while(!callbackRegistred.empty())
        {
            callbackRegistred.front()->object=NULL;
            callbackRegistred.pop();
        }
        //crash with heap-buffer-overflow if not flush before the end of destructor
        while(!callbackRegistred.empty())
            callbackRegistred.pop();
        while(!paramToPassToCallBack.empty())
            paramToPassToCallBack.pop();
    }
}

#ifdef EPOLLCATCHCHALLENGERSERVER
BaseClassSwitch::EpollObjectType Client::getType() const
{
    return BaseClassSwitch::EpollObjectType::Client;
}
#endif

#ifndef EPOLLCATCHCHALLENGERSERVER
/// \brief new error at connexion
void Client::connectionError(QAbstractSocket::SocketError error)
{
    isConnected=false;
    if(error!=QAbstractSocket::RemoteHostClosedError)
    {
        normalOutput("error detected for the client: "+std::to_string(error));
        socket->disconnectFromHost();
    }
}
#endif

/// \warning called in one other thread!!!
void Client::disconnectClient()
{
    closeSocket();
    if(stat==ClientStat::None)
        return;
    if(account_id==0)
        return;
    account_id=0;
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    if(character_id!=0)
        normalOutput("Disconnected client");
    #endif
    /// \warning global clear from client call?
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    GlobalServerData::serverPrivateVariables.db_login->clear();
    GlobalServerData::serverPrivateVariables.db_base->clear();
    #endif
    GlobalServerData::serverPrivateVariables.db_common->clear();
    GlobalServerData::serverPrivateVariables.db_server->clear();
    #ifndef EPOLLCATCHCHALLENGERSERVER
    isConnected=false;
    if(socket!=NULL)
    {
        socket->disconnectFromHost();
        //commented because if in closing state, block in waitForDisconnected
        /*if(socket->state()!=QAbstractSocket::UnconnectedState)
            socket->waitForDisconnected();*/
        socket=NULL;
    }
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
        stat=ClientStat::CharacterSelected;
        GlobalServerData::serverPrivateVariables.connected_players_id_list.erase(character_id);
        simplifiedIdList.push_back(public_and_private_informations.public_informations.simplifiedId);
    }
    else if(stat==ClientStat::CharacterSelected)
    {
        const std::string &character_id_string=std::to_string(character_id);
        if(map!=NULL)
            removeClientOnMap(map
                              #ifndef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
                              ,true
                              #endif
                              );
        if(GlobalServerData::serverSettings.sendPlayerNumber)
            GlobalServerData::serverPrivateVariables.player_updater.removeConnectedPlayer();
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
        }
        //save the monster
        if(GlobalServerData::serverSettings.fightSync==GameServerSettings::FightSync_AtTheEndOfBattle && isInFight())
            saveCurrentMonsterStat();
        if(GlobalServerData::serverSettings.fightSync==GameServerSettings::FightSync_AtTheDisconnexion)
        {
            unsigned int index=0;
            const unsigned int &size=public_and_private_informations.playerMonster.size();
            while(index<size)
            {
                const PlayerMonster &playerMonster=public_and_private_informations.playerMonster.at(index);
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

    #ifndef EPOLLCATCHCHALLENGERSERVER
    BroadCastWithoutSender::broadCastWithoutSender.emit_player_is_disconnected(public_and_private_informations.public_informations.pseudo);
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    if(character_id!=0)
        normalOutput("Disconnected client done");
    #endif
    #ifdef EPOLLCATCHCHALLENGERSERVER
    recordDisconnectByServer(this);
    #endif
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
        //std::cerr << headerOutput() << "Kicked by: " << errorString << std::endl;//silent if protocol not passed, to not flood the log if other client like http client (browser) is connected
        disconnectClient();
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
        std::string ip;
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
        #else
        std::string ip;
        if(socketString==NULL)
            ip="[IP]:[PORT]";
        else
            ip=socketString;
        #endif
        /// \todo fast hide the ip
        /*if(GlobalServerData::serverSettings.anonymous)
        {
            QCryptographicHash hash(QCryptographicHash::Sha224);
            hash.addData(ip.data(),ip.size());
            return std::string(hash.result().toHex())+": ";
        }
        else*/
            return ip+": ";
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
    TokenAuth newEntry;
    newEntry.characterId=characterId;
    newEntry.accountIdRequester=accountIdRequester;
    newEntry.createTime=sFrom1970();
    newEntry.token=new char[CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER];
    const int &returnedSize=fread(newEntry.token,1,CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER,BaseServerLogin::fpRandomFile);
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
uint32_t Client::getMonsterId(bool * const ok)
{
    *ok=true;
    const uint32_t monsterId=GlobalServerData::serverPrivateVariables.maxMonsterId;
    GlobalServerData::serverPrivateVariables.maxMonsterId++;
    return monsterId;
}

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
        playerById.at(characterId)->disconnectClient();
}

#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
void Client::askStatClient(const uint8_t &query_id,const char *rawdata)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(PreparedDBQueryLogin::db_query_login.empty())
    {
        errorParsingLayer("askLogin() Query login is empty, bug");
        return;
    }
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
                SHA224(Client::private_token_statclient,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT,reinterpret_cast<unsigned char *>(ProtocolParsingBase::tempBigBufferForOutput));

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

