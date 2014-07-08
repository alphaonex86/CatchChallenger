#include "Client.h"
#include "GlobalServerData.h"
#include "../../general/base/QFakeSocket.h"

using namespace CatchChallenger;

/// \warning never cross the signals from and to the different client, complexity ^2
/// \todo drop instant player number notification, and before do the signal without signal/slot, check if the number have change
/// \todo change push position recording, from ClientMapManagement to ClientLocalCalcule, to disable ALL operation for MapVisibilityAlgorithm_None

Client::Client(
        #ifdef EPOLLCATCHCHALLENGERSERVER
            #ifndef SERVERNOSSL
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
            #ifndef SERVERNOSSL
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
    account_id(0),
    number_of_character(0),
    character_id(0),
    market_cash(0),
    #ifndef EPOLLCATCHCHALLENGERSERVER
    isConnected(true),
    #endif
    randomIndex(0),
    randomSize(0),
    connected_players(0),
    have_send_protocol(false),
    is_logging_in_progess(false),
    stopIt(false),
    movePacketKickTotalCache(0),
    movePacketKickNewValue(0),
    chatPacketKickTotalCache(0),
    chatPacketKickNewValue(0),
    otherPacketKickTotalCache(0),
    otherPacketKickNewValue(0),
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
    otherCityPlayerBattle(NULL)
{
    public_and_private_informations.repel_step=0;
    {
        /* \warning don't work!
        memset(movePacketKick,0x00,CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE);
        memset(chatPacketKick,0x00,CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE);
        memset(otherPacketKick,0x00,CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE);*/
        memset(movePacketKick+(CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue),
               0x00,
               GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue*sizeof(quint8));
        memset(chatPacketKick+(CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue),
               0x00,
               GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue*sizeof(quint8));
        memset(otherPacketKick+(CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue),
               0x00,
               GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue*sizeof(quint8));

        /*int index=CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE-GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue;
        while(index<CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE)
        {
            movePacketKick[index]=0;
            chatPacketKick[index]=0;
            otherPacketKick[index]=0;
            index++;
        }*/
    }
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
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    //normalOutput("Destroyed client");
    #endif
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(socket!=NULL)
    {
        delete socket;
        socket=NULL;
    }
    #endif
}

#ifdef EPOLLCATCHCHALLENGERSERVER
BaseClassSwitch::Type Client::getType() const
{
    return BaseClassSwitch::Type::Client;
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
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    if(account_id!=0)
        normalOutput("Disconnected client");
    #endif
    GlobalServerData::serverPrivateVariables.db.clear();
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

    if(character_loaded)
    {
        if(map!=NULL)
            removeClientOnMap(map,true);
        extraStop();
        tradeCanceled();
        battleCanceled();
        removeFromClan();
        simplifiedIdList << public_and_private_informations.public_informations.simplifiedId;
        GlobalServerData::serverPrivateVariables.connected_players_id_list.remove(character_id);
        playerByPseudo.remove(public_and_private_informations.public_informations.pseudo);
        clanChangeWithoutDb(0);
        clientBroadCastList.removeOne(this);
        playerByPseudo.remove(public_and_private_informations.public_informations.pseudo);
        playerById.remove(character_id);
        leaveTheCityCapture();
        dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_played_time.arg(character_id).arg(QDateTime::currentDateTime().toMSecsSinceEpoch()/1000-connectedSince.toMSecsSinceEpoch()/1000));
        //save the monster
        if(GlobalServerData::serverSettings.database.fightSync==ServerSettings::Database::FightSync_AtTheEndOfBattle && isInFight())
            saveCurrentMonsterStat();
        if(GlobalServerData::serverSettings.database.fightSync==ServerSettings::Database::FightSync_AtTheDisconnexion)
        {
            int index=0;
            const int &size=public_and_private_informations.playerMonster.size();
            while(index<size)
            {
                const PlayerMonster &playerMonster=public_and_private_informations.playerMonster.at(index);
                if(CommonSettings::commonSettings.useSP)
                    dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_monster
                                 .arg(playerMonster.id)
                                 .arg(character_id)
                                 .arg(playerMonster.hp)
                                 .arg(playerMonster.remaining_xp)
                                 .arg(playerMonster.level)
                                 .arg(playerMonster.sp)
                                 .arg(index+1)
                                 );
                else
                    dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_monster
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
                    dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_monster_skill
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
    if(character_loaded)
        sendSystemMessage(public_and_private_informations.public_informations.pseudo+" have been kicked from server, have try hack",false);

    normalOutput("Kicked by: "+errorString);
    disconnectClient();
}

void Client::kick()
{
    normalOutput("kicked()");
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
        QString ip(QStringLiteral("[IP]:[PORT]"));
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
