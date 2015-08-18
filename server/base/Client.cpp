#include "Client.h"
#include "GlobalServerData.h"
#include "../base/PreparedDBQuery.h"
#include "../../general/base/QFakeSocket.h"
#include "../../general/base/GeneralType.h"
#ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
#include "../game-server-alone/LinkToMaster.h"
#endif

#include "BaseServerLogin.h"

#include <QCryptographicHash>

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
    character_loaded(false),
    character_loaded_in_progress(false),
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
    have_send_protocol(false),
    is_logging_in_progess(false),
    stopIt(false),
    #ifdef CATCHCHALLENGER_DDOS_FILTER
    movePacketKickTotalCache(0),
    movePacketKickNewValue(0),
    chatPacketKickTotalCache(0),
    chatPacketKickNewValue(0),
    otherPacketKickTotalCache(0),
    otherPacketKickNewValue(0),
    #endif
    profileIndex(0),
    otherPlayerBattle(NULL),
    battleIsValidated(false),
    mCurrentSkillId(0),
    mHaveCurrentSkill(false),
    mMonsterChange(false),
    botFightCash(0),
    botFightId(0),
    isInCityCapture(false),
    otherPlayerTrade(NULL),
    tradeIsValidated(false),
    clan(NULL),
    #ifdef EPOLLCATCHCHALLENGERSERVER
    socketString(NULL),
    socketStringSize(0),
    #endif
    otherCityPlayerBattle(NULL)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    ClientBase::public_and_private_informations_solo=&public_and_private_informations;
    #endif
    public_and_private_informations.repel_step=0;
    #ifdef CATCHCHALLENGER_DDOS_FILTER
    {
        memset(movePacketKick+(CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue),
               0x00,
               GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue*sizeof(quint8));
        memset(chatPacketKick+(CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue),
               0x00,
               GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue*sizeof(quint8));
        memset(otherPacketKick+(CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue),
               0x00,
               GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue*sizeof(quint8));
    }
    #endif
    queryNumberList.reserve(256);
    {
        int index=0;
        while(index<256)
        {
            queryNumberList << index;
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
    {
        int index=0;
        while(index<callbackRegistred.size())
        {
            callbackRegistred.at(index)->object=NULL;
            index++;
        }
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
        normalOutput(QStringLiteral("error detected for the client: %1 %2").arg(error));
        socket->disconnectFromHost();
    }
}
#endif

/// \warning called in one other thread!!!
void Client::disconnectClient()
{
    closeSocket();
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
    #endif
    GlobalServerData::serverPrivateVariables.db_base->clear();
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
        quint32 index=0;
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
    #endif

    if(character_loaded_in_progress)
    {
        character_loaded_in_progress=false;
        GlobalServerData::serverPrivateVariables.connected_players_id_list.remove(character_id);
        simplifiedIdList << public_and_private_informations.public_informations.simplifiedId;
    }
    else if(character_loaded)
    {
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
        simplifiedIdList << public_and_private_informations.public_informations.simplifiedId;
        GlobalServerData::serverPrivateVariables.connected_players_id_list.remove(character_id);
        #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
        LinkToMaster::linkToMaster->characterDisconnected(character_id);
        #endif
        playerByPseudo.remove(public_and_private_informations.public_informations.pseudo);
        clanChangeWithoutDb(0);
        clientBroadCastList.removeOne(this);
        playerByPseudo.remove(public_and_private_informations.public_informations.pseudo);
        playerById.remove(character_id);
        leaveTheCityCapture();
        const quint32 &addTime=QDateTime::currentDateTime().toMSecsSinceEpoch()/1000-connectedSince.toMSecsSinceEpoch()/1000;
        if(addTime>5)
        {
            dbQueryWriteCommon(PreparedDBQueryCommon::db_query_played_time.arg(character_id).arg(addTime));
            #ifdef CATCHCHALLENGER_CLASS_ALLINONESERVER
            dbQueryWriteCommon(PreparedDBQueryCommon::db_query_update_server_time_played_time.arg(addTime).arg(0).arg(character_id));
            #endif
        }
        //save the monster
        if(GlobalServerData::serverSettings.fightSync==GameServerSettings::FightSync_AtTheEndOfBattle && isInFight())
            saveCurrentMonsterStat();
        if(GlobalServerData::serverSettings.fightSync==GameServerSettings::FightSync_AtTheDisconnexion)
        {
            int index=0;
            const int &size=public_and_private_informations.playerMonster.size();
            while(index<size)
            {
                const PlayerMonster &playerMonster=public_and_private_informations.playerMonster.at(index);
                if(CommonSettingsServer::commonSettingsServer.useSP)
                    dbQueryWriteCommon(PreparedDBQueryCommon::db_query_monster
                                 .arg(playerMonster.id)
                                 .arg(character_id)
                                 .arg(playerMonster.hp)
                                 .arg(playerMonster.remaining_xp)
                                 .arg(playerMonster.level)
                                 .arg(playerMonster.sp)
                                 .arg(index+1)
                                 );
                else
                    dbQueryWriteCommon(PreparedDBQueryCommon::db_query_monster
                                 .arg(playerMonster.id)
                                 .arg(character_id)
                                 .arg(playerMonster.hp)
                                 .arg(playerMonster.remaining_xp)
                                 .arg(playerMonster.level)
                                 .arg(index+1)
                                 );
                int sub_index=0;
                const int &sub_size=playerMonster.skills.size();
                while(sub_index<sub_size)
                {
                    const PlayerMonster::PlayerSkill &playerSkill=playerMonster.skills.at(sub_index);
                    dbQueryWriteCommon(PreparedDBQueryCommon::db_query_monster_skill
                                 .arg(playerSkill.endurance)
                                 .arg(playerMonster.id)
                                 .arg(playerSkill.skill)
                                 );
                    sub_index++;
                }
                index++;
            }
        }
        if(map!=NULL)
            savePosition();
        map=NULL;
        character_loaded=false;
    }

    #ifndef EPOLLCATCHCHALLENGERSERVER
    BroadCastWithoutSender::broadCastWithoutSender.emit_player_is_disconnected(public_and_private_informations.public_informations.pseudo);
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    if(character_id!=0)
        normalOutput("Disconnected client done");
    #endif
}

//input/ouput layer
void Client::errorParsingLayer(const QString &error)
{
    errorOutput(error);
}

void Client::messageParsingLayer(const QString &message) const
{
    normalOutput(message);
}

//* do the message by the general broadcast */
void Client::errorOutput(const QString &errorString)
{
    if(account_id==0)
    {
        normalOutput(QStringLiteral("Kicked by: ")+errorString);
        disconnectClient();
        return;
    }
    if(character_loaded)
        sendSystemMessage(public_and_private_informations.public_informations.pseudo+" have been kicked from server, have try hack",false);

    normalOutput(QStringLiteral("Kicked by: ")+errorString);
    disconnectClient();
}

void Client::kick()
{
    normalOutput(QStringLiteral("kicked()"));
    disconnectClient();
}

void Client::normalOutput(const QString &message) const
{
    if(!public_and_private_informations.public_informations.pseudo.isEmpty())
    {
        if(GlobalServerData::serverSettings.anonymous)
            DebugClass::debugConsole(QStringLiteral("%1: %2").arg(character_id).arg(message));
        else
            DebugClass::debugConsole(QStringLiteral("%1: %2").arg(public_and_private_informations.public_informations.pseudo).arg(message));
    }
    else
    {
        #ifndef EPOLLCATCHCHALLENGERSERVER
        QString ip;
        if(socket==NULL)
            ip="unknown";
        else
        {
            QHostAddress hostAddress=socket->peerAddress();
            if(hostAddress==QHostAddress::LocalHost || hostAddress==QHostAddress::LocalHostIPv6)
                ip=QStringLiteral("localhost:%1").arg(socket->peerPort());
            else if(hostAddress==QHostAddress::Null || hostAddress==QHostAddress::Any || hostAddress==QHostAddress::AnyIPv4 || hostAddress==QHostAddress::AnyIPv6 || hostAddress==QHostAddress::Broadcast)
                ip="internal";
            else
                ip=QStringLiteral("%1:%2").arg(hostAddress.toString()).arg(socket->peerPort());
        }
        #else
        QString ip;
        if(socketString==NULL)
            ip=QStringLiteral("[IP]:[PORT]");
        else
            ip=QString::fromLatin1(QByteArray(socketString,socketStringSize));
        #endif
        if(GlobalServerData::serverSettings.anonymous)
        {
            QCryptographicHash hash(QCryptographicHash::Sha1);
            hash.addData(ip.toUtf8());
            DebugClass::debugConsole(QStringLiteral("%1: %2").arg(QString(hash.result().toHex())).arg(message));
        }
        else
            DebugClass::debugConsole(QStringLiteral("%1: %2").arg(ip).arg(message));
    }
}

QString Client::getPseudo()
{
    return public_and_private_informations.public_informations.pseudo;
}

void Client::dropAllClients()
{
    sendPacket(0xC4);
}

void Client::dropAllBorderClients()
{
    sendPacket(0xC9);
}

QByteArray Client::getRawPseudo() const
{
    return rawPseudo;
}

#ifndef EPOLLCATCHCHALLENGERSERVER
/*void Client::fake_receive_data(QByteArray data)
{
    fake_send_received_data(data);
}*/
#endif

void Client::parseIncommingData()
{
    ProtocolParsingInputOutput::parseIncommingData();
}

void Client::errorFightEngine(const QString &error)
{
    errorOutput(error);
}

void Client::messageFightEngine(const QString &message) const
{
    normalOutput(message);
}

#ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
char *Client::addAuthGetToken(const quint32 &characterId, const quint32 &accountIdRequester)
{
    TokenAuth newEntry;
    newEntry.characterId=characterId;
    newEntry.accountIdRequester=accountIdRequester;
    newEntry.createTime=QDateTime::currentMSecsSinceEpoch()/1000;
    newEntry.token=new char[CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER];
    const int &returnedSize=fread(newEntry.token,1,CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER,BaseServerLogin::fpRandomFile);
    if(returnedSize!=CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER)
    {
        qDebug() << QStringLiteral("sizeof(newEntry.token) don't match with urandom size: %1").arg(returnedSize);
        delete newEntry.token;
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

quint32 Client::getMonsterId(bool * const ok)
{
    if(GlobalServerData::serverPrivateVariables.maxMonsterId.size()==0)
    {
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
    if(ok!=NULL)
        *ok=true;
    const quint32 monsterId=GlobalServerData::serverPrivateVariables.maxMonsterId.front();
    GlobalServerData::serverPrivateVariables.maxMonsterId.erase(GlobalServerData::serverPrivateVariables.maxMonsterId.begin());
    if(GlobalServerData::serverPrivateVariables.maxMonsterId.size()<CATCHCHALLENGER_SERVER_MINIDBLOCK)
        LinkToMaster::linkToMaster->askMoreMaxMonsterId();
    return monsterId;
}

quint32 Client::getClanId(bool * const ok)
{
    if(GlobalServerData::serverPrivateVariables.maxClanId.size()==0)
    {
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
    if(ok!=NULL)
        *ok=true;
    const quint32 clanId=GlobalServerData::serverPrivateVariables.maxClanId.front();
    GlobalServerData::serverPrivateVariables.maxClanId.erase(GlobalServerData::serverPrivateVariables.maxClanId.begin());
    if(GlobalServerData::serverPrivateVariables.maxClanId.size()<CATCHCHALLENGER_SERVER_MINCLANIDBLOCK)
        LinkToMaster::linkToMaster->askMoreMaxClanId();
    return clanId;
}
#else
quint32 Client::getMonsterId(bool * const ok)
{
    *ok=true;
    const quint32 monsterId=GlobalServerData::serverPrivateVariables.maxMonsterId;
    GlobalServerData::serverPrivateVariables.maxMonsterId++;
    return monsterId;
}

quint32 Client::getClanId(bool * const ok)
{
    *ok=true;
    const quint32 clanId=GlobalServerData::serverPrivateVariables.maxClanId;
    GlobalServerData::serverPrivateVariables.maxClanId++;
    return clanId;
}
#endif

bool Client::characterConnected(const quint32 &characterId)
{
    return playerById.contains(characterId);
}

void Client::disconnectClientById(const quint32 &characterId)
{
    if(playerById.contains(characterId))
        playerById.value(characterId)->disconnectClient();
}
