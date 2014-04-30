#include "Client.h"
#include "GlobalServerData.h"
#include "../../general/base/QFakeSocket.h"

using namespace CatchChallenger;

/// \warning never cross the signals from and to the different client, complexity ^2
/// \todo drop instant player number notification, and before do the signal without signal/slot, check if the number have change
/// \todo change push position recording, from ClientMapManagement to ClientLocalCalcule, to disable ALL operation for MapVisibilityAlgorithm_None

#define OBJECTTOSTOP 7

Client::Client(ConnectedSocket *socket, ClientMapManagement *clientMapManagement) :
    ask_is_ready_to_stop(false),
    stopped_object(0),
    socket(socket),
    clientBroadCast(),
    clientHeavyLoad(),
    clientNetworkRead(&player_informations,socket),
    clientNetworkWrite(&player_informations,socket),
    localClientHandler(),
    clientLocalBroadcast(),
    clientMapManagement(clientMapManagement)
{
    player_informations.character_loaded=false;
    player_informations.character_id=0;
    player_informations.account_id=0;
    player_informations.number_of_character=0;
    player_informations.isConnected=true;
    player_informations.public_and_private_informations.repel_step=0;

    Qt::ConnectionType coTypeAsync=Qt::DirectConnection;
    {
        int index=1;
        while(index<GlobalServerData::serverPrivateVariables.eventThreaderList.size())
        {
            if(Q_LIKELY(GlobalServerData::serverPrivateVariables.eventThreaderList.at(0)!=GlobalServerData::serverPrivateVariables.eventThreaderList.at(index)))
            {
                coTypeAsync=Qt::QueuedConnection;
                break;
            }
            index++;
        }
    }

    if(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm!=MapVisibilityAlgorithmSelection_None)
    {
        connect(clientMapManagement,	&ClientMapManagement::sendFullPacket,   &clientNetworkWrite,     &ClientNetworkWrite::sendFullPacket,    coTypeAsync);
        connect(clientMapManagement,	&ClientMapManagement::sendPacket,       &clientNetworkWrite,     &ClientNetworkWrite::sendPacket,        coTypeAsync);
        connect(&clientHeavyLoad,        &ClientHeavyLoad::put_on_the_map,       clientMapManagement,	&ClientMapManagement::put_on_the_map,   coTypeAsync);
        connect(&clientNetworkRead,      &ClientNetworkRead::moveThePlayer,		clientMapManagement,	&ClientMapManagement::moveThePlayer,	coTypeAsync);
        connect(&clientNetworkRead,      &ClientNetworkRead::teleportValidatedTo,clientMapManagement,	&ClientMapManagement::teleportValidatedTo,coTypeAsync);
        connect(clientMapManagement,	&ClientMapManagement::error,			this,                   &Client::errorOutput,                   coTypeAsync);
        connect(clientMapManagement,	&ClientMapManagement::message,			this,                   &Client::normalOutput,                  coTypeAsync);
    }

    connect(socket,	static_cast<void(ConnectedSocket::*)(QAbstractSocket::SocketError)>(&ConnectedSocket::error),         this, &Client::connectionError);
    connect(socket,	&ConnectedSocket::disconnected,	this, &Client::disconnectClient);

    clientBroadCast.moveToThread(GlobalServerData::serverPrivateVariables.eventThreaderList.at(0));
    clientMapManagement->moveToThread(GlobalServerData::serverPrivateVariables.eventThreaderList.at(1));
    clientNetworkRead.moveToThread(GlobalServerData::serverPrivateVariables.eventThreaderList.at(2));
    clientHeavyLoad.moveToThread(GlobalServerData::serverPrivateVariables.eventThreaderList.at(3));
    localClientHandler.moveToThread(GlobalServerData::serverPrivateVariables.eventThreaderList.at(4));
    clientLocalBroadcast.moveToThread(GlobalServerData::serverPrivateVariables.eventThreaderList.at(5));

    //set variables
    clientBroadCast.setVariable(&player_informations);
    clientMapManagement->setVariable(&player_informations);
    clientHeavyLoad.setVariable(&player_informations);
    localClientHandler.setVariable(&player_informations);
    clientLocalBroadcast.setVariable(&player_informations);

    //connect input/ouput
    connect(&clientNetworkRead,     &ClientNetworkRead::newFullInputQuery,      &clientNetworkWrite,    &ClientNetworkWrite::newFullInputQuery, coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::newInputQuery,          &clientNetworkWrite,    &ClientNetworkWrite::newInputQuery,     coTypeAsync);
    connect(&clientNetworkWrite,	&ClientNetworkWrite::newFullOutputQuery,	&clientNetworkRead,     &ClientNetworkRead::newFullOutputQuery, coTypeAsync);
    connect(&clientNetworkWrite,	&ClientNetworkWrite::newOutputQuery,		&clientNetworkRead,     &ClientNetworkRead::newOutputQuery,     coTypeAsync);

    //connect the write, to send packet on the network
    connect(&clientNetworkRead,     &ClientNetworkRead::sendFullPacket,         &clientNetworkWrite,    &ClientNetworkWrite::sendFullPacket,    coTypeAsync);
    connect(&clientBroadCast,       &ClientBroadCast::sendFullPacket,           &clientNetworkWrite,    &ClientNetworkWrite::sendFullPacket,    coTypeAsync);
    connect(&clientHeavyLoad,       &ClientHeavyLoad::sendFullPacket,           &clientNetworkWrite,    &ClientNetworkWrite::sendFullPacket,    coTypeAsync);
    connect(&localClientHandler,	&MapBasicMove::sendFullPacket,              &clientNetworkWrite,    &ClientNetworkWrite::sendFullPacket,    coTypeAsync);
    connect(&clientLocalBroadcast,  &MapBasicMove::sendFullPacket,              &clientNetworkWrite,    &ClientNetworkWrite::sendFullPacket,    coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::sendPacket,             &clientNetworkWrite,    &ClientNetworkWrite::sendPacket,        coTypeAsync);
    connect(&clientBroadCast,       &ClientBroadCast::sendPacket,               &clientNetworkWrite,    &ClientNetworkWrite::sendPacket,        coTypeAsync);
    connect(&clientHeavyLoad,       &ClientHeavyLoad::sendPacket,               &clientNetworkWrite,    &ClientNetworkWrite::sendPacket,        coTypeAsync);
    connect(&localClientHandler,	&MapBasicMove::sendPacket,                  &clientNetworkWrite,    &ClientNetworkWrite::sendPacket,        coTypeAsync);
    connect(&clientLocalBroadcast,  &MapBasicMove::sendPacket,                  &clientNetworkWrite,    &ClientNetworkWrite::sendPacket,        coTypeAsync);

    connect(&clientBroadCast,       &ClientBroadCast::sendRawSmallPacket,       &clientNetworkWrite,    &ClientNetworkWrite::sendRawSmallPacket,coTypeAsync);
    connect(&clientLocalBroadcast,	&ClientLocalBroadcast::sendRawSmallPacket,  &clientNetworkWrite,    &ClientNetworkWrite::sendRawSmallPacket,coTypeAsync);

    connect(&clientNetworkRead,     &ClientNetworkRead::sendQuery,              &clientNetworkWrite,    &ClientNetworkWrite::sendQuery,         coTypeAsync);

    connect(&clientNetworkRead,     &ClientNetworkRead::postReply,              &clientNetworkWrite,    &ClientNetworkWrite::postReply,         coTypeAsync);
    connect(&clientHeavyLoad,       &ClientHeavyLoad::postReply,                &clientNetworkWrite,    &ClientNetworkWrite::postReply,         coTypeAsync);
    connect(&clientLocalBroadcast,  &ClientLocalBroadcast::postReply,           &clientNetworkWrite,    &ClientNetworkWrite::postReply,         coTypeAsync);
    connect(&localClientHandler,    &LocalClientHandler::postReply,             &clientNetworkWrite,    &ClientNetworkWrite::postReply,         coTypeAsync);

    connect(&clientNetworkRead,     &ClientNetworkRead::addCharacter,           &clientHeavyLoad,       &ClientHeavyLoad::addCharacter,         coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::removeCharacter,        &clientHeavyLoad,       &ClientHeavyLoad::removeCharacter,      coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::selectCharacter,        &clientHeavyLoad,       &ClientHeavyLoad::selectCharacter,      coTypeAsync);

    connect(&localClientHandler,    &LocalClientHandler::dbQuery,               &clientHeavyLoad,       &ClientHeavyLoad::dbQuery,              coTypeAsync);
    connect(&clientLocalBroadcast,  &ClientLocalBroadcast::dbQuery,             &clientHeavyLoad,       &ClientHeavyLoad::dbQuery,              coTypeAsync);
    connect(&localClientHandler,    &LocalClientHandler::askRandomNumber,       &clientHeavyLoad,       &ClientHeavyLoad::askedRandomNumber,    coTypeAsync);

    //connect for the seed
    connect(&localClientHandler,    &LocalClientHandler::seedValidated,         &clientLocalBroadcast,  &ClientLocalBroadcast::seedValidated,   coTypeAsync);
    connect(&clientLocalBroadcast,  &ClientLocalBroadcast::useSeed,             &localClientHandler,    &LocalClientHandler::useSeed,           coTypeAsync);
    connect(&clientLocalBroadcast,  &ClientLocalBroadcast::addObjectAndSend,    &localClientHandler,    &LocalClientHandler::addObjectAndSend,  coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::plantSeed,              &clientLocalBroadcast,  &ClientLocalBroadcast::plantSeed,       coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::collectPlant,           &clientLocalBroadcast,  &ClientLocalBroadcast::collectPlant,    coTypeAsync);

    //connect for crafting
    connect(&clientNetworkRead,     &ClientNetworkRead::useRecipe,              &localClientHandler,    &LocalClientHandler::useRecipe,         coTypeAsync);

    //connect for trade
    connect(&localClientHandler,	&LocalClientHandler::sendTradeRequest,      &clientNetworkRead,     &ClientNetworkRead::sendTradeRequest,       coTypeAsync);
    connect(&localClientHandler,	&LocalClientHandler::sendBattleRequest,     &clientNetworkRead,     &ClientNetworkRead::sendBattleRequest,      coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::tradeAccepted,          &localClientHandler,    &LocalClientHandler::tradeAccepted,         coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::tradeCanceled,          &localClientHandler,    &LocalClientHandler::tradeCanceled,         coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::battleAccepted,         &localClientHandler,    &LocalClientHandler::battleAccepted,        coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::battleCanceled,         &localClientHandler,    &LocalClientHandler::battleCanceled,        coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::tradeFinished,          &localClientHandler,    &LocalClientHandler::tradeFinished,         coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::tradeAddTradeCash,      &localClientHandler,    &LocalClientHandler::tradeAddTradeCash,     coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::tradeAddTradeObject,    &localClientHandler,    &LocalClientHandler::tradeAddTradeObject,   coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::tradeAddTradeMonster,   &localClientHandler,    &LocalClientHandler::tradeAddTradeMonster,  coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::newQuestAction,         &localClientHandler,    &LocalClientHandler::newQuestAction,        coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::clanAction,             &localClientHandler,    &LocalClientHandler::clanAction,            coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::clanInvite,             &localClientHandler,    &LocalClientHandler::clanInvite,            coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::waitingForCityCaputre,  &localClientHandler,    &LocalClientHandler::waitingForCityCaputre, coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::moveMonster,            &localClientHandler,    &LocalClientHandler::moveMonster,           coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::changeOfMonsterInFight, &localClientHandler,    &LocalClientHandler::changeOfMonsterInFight,coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::confirmEvolution,       &localClientHandler,    &LocalClientHandler::confirmEvolution,      coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::useObjectOnMonster,     &localClientHandler,    &LocalClientHandler::useObjectOnMonster,    coTypeAsync);

    //connect the player information
    connect(&clientHeavyLoad,       &ClientHeavyLoad::send_player_informations,	&clientBroadCast,       &ClientBroadCast::send_player_informations, coTypeAsync);
    connect(&clientHeavyLoad,       &ClientHeavyLoad::put_on_the_map,           &localClientHandler,	&LocalClientHandler::put_on_the_map,        coTypeAsync);
    connect(&clientHeavyLoad,       &ClientHeavyLoad::put_on_the_map,           &clientLocalBroadcast,  &ClientLocalBroadcast::put_on_the_map,      coTypeAsync);
    connect(&clientHeavyLoad,       &ClientHeavyLoad::send_player_informations,	this,                   &Client::send_player_informations,          coTypeAsync);
    connect(&clientHeavyLoad,       &ClientHeavyLoad::newRandomNumber,          &localClientHandler,	&LocalClientHandler::newRandomNumber,       coTypeAsync);
    connect(&clientHeavyLoad,       &ClientHeavyLoad::haveClanInfo,             &localClientHandler,	&LocalClientHandler::haveClanInfo,          coTypeAsync);
    connect(&localClientHandler,	&LocalClientHandler::clanChange,            &clientBroadCast,       &ClientBroadCast::clanChange,               coTypeAsync);

    //packet parsed (heavy)
    connect(&clientNetworkRead,     &ClientNetworkRead::askLogin,               &clientHeavyLoad,       &ClientHeavyLoad::askLogin,                 coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::datapackList,           &clientHeavyLoad,       &ClientHeavyLoad::datapackList,             coTypeAsync);

    //packet parsed (map management)
    connect(&clientNetworkRead,     &ClientNetworkRead::moveThePlayer,			&localClientHandler,	&LocalClientHandler::moveThePlayer,         coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::moveThePlayer,			&clientLocalBroadcast,  &MapBasicMove::moveThePlayer,               coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::teleportValidatedTo,	&localClientHandler,	&LocalClientHandler::teleportValidatedTo,	coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::teleportValidatedTo,	&clientLocalBroadcast,  &ClientLocalBroadcast::teleportValidatedTo, coTypeAsync);
    connect(&localClientHandler,	&LocalClientHandler::teleportTo,            &clientNetworkRead,     &ClientNetworkRead::teleportTo,             coTypeAsync);

    //packet parsed (broadcast)
    connect(&localClientHandler,	&LocalClientHandler::receiveSystemText,     &clientBroadCast,       &ClientBroadCast::receiveSystemText,        coTypeAsync);
    if(CommonSettings::commonSettings.chat_allow_clan || CommonSettings::commonSettings.chat_allow_all)
        connect(&clientNetworkRead,	&ClientNetworkRead::sendChatText,			&clientBroadCast,       &ClientBroadCast::sendChatText,				coTypeAsync);
    if(CommonSettings::commonSettings.chat_allow_local)
        connect(&clientNetworkRead,	&ClientNetworkRead::sendLocalChatText,      &clientLocalBroadcast,  &ClientLocalBroadcast::sendLocalChatText,	coTypeAsync);
    if(CommonSettings::commonSettings.chat_allow_private)
        connect(&clientNetworkRead,	&ClientNetworkRead::sendPM,                 &clientBroadCast,       &ClientBroadCast::sendPM,                   coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::sendBroadCastCommand,	&clientBroadCast,       &ClientBroadCast::sendBroadCastCommand,		coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::sendHandlerCommand,		&localClientHandler,	&LocalClientHandler::sendHandlerCommand,	coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::destroyObject,          &localClientHandler,	&LocalClientHandler::destroyObject,         coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::useObject,              &localClientHandler,	&LocalClientHandler::useObject,             coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::wareHouseStore,         &localClientHandler,	&LocalClientHandler::wareHouseStore,        coTypeAsync);
    connect(&clientBroadCast,       &ClientBroadCast::kicked,                   this,                   &Client::kick,                            coTypeAsync);

    //shops
    connect(&clientNetworkRead,     &ClientNetworkRead::getShopList,            &localClientHandler,	&LocalClientHandler::getShopList,           coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::buyObject,              &localClientHandler,	&LocalClientHandler::buyObject,             coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::sellObject,             &localClientHandler,	&LocalClientHandler::sellObject,            coTypeAsync);

    //factory
    connect(&clientNetworkRead,     &ClientNetworkRead::getFactoryList,         &localClientHandler,	&LocalClientHandler::getFactoryList,        coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::buyFactoryObject,       &localClientHandler,	&LocalClientHandler::buyFactoryProduct,     coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::sellFactoryObject,      &localClientHandler,	&LocalClientHandler::sellFactoryResource,   coTypeAsync);

    //fight
    connect(&clientNetworkRead,     &ClientNetworkRead::tryEscape,              &localClientHandler,	&LocalClientHandler::tryEscape,             coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::useSkill,               &localClientHandler,	&LocalClientHandler::useSkill,              coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::learnSkill,             &localClientHandler,	&LocalClientHandler::learnSkill,            coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::heal,                   &localClientHandler,	&LocalClientHandler::heal,                  coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::requestFight,           &localClientHandler,	&LocalClientHandler::requestFight,          coTypeAsync);

    //market
    connect(&clientNetworkRead,     &ClientNetworkRead::getMarketList,          &localClientHandler,	&LocalClientHandler::getMarketList,         coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::buyMarketObject,        &localClientHandler,	&LocalClientHandler::buyMarketObject,       coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::buyMarketMonster,       &localClientHandler,	&LocalClientHandler::buyMarketMonster,      coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::putMarketObject,        &localClientHandler,	&LocalClientHandler::putMarketObject,       coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::putMarketMonster,       &localClientHandler,	&LocalClientHandler::putMarketMonster,      coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::recoverMarketCash,      &localClientHandler,	&LocalClientHandler::recoverMarketCash,     coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::withdrawMarketObject,   &localClientHandler,	&LocalClientHandler::withdrawMarketObject,  coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::withdrawMarketMonster,  &localClientHandler,	&LocalClientHandler::withdrawMarketMonster, coTypeAsync);

    //connect the message
    connect(&clientBroadCast,       &ClientBroadCast::error,					this,	&Client::errorOutput,coTypeAsync);
    connect(&clientHeavyLoad,       &ClientHeavyLoad::error,					this,	&Client::errorOutput,coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::error,					this,	&Client::errorOutput,coTypeAsync);
    connect(&clientNetworkWrite,	&ClientNetworkWrite::error,					this,	&Client::errorOutput,coTypeAsync);
    connect(&localClientHandler,	&MapBasicMove::error,						this,	&Client::errorOutput,coTypeAsync);
    connect(&clientLocalBroadcast,  &ClientLocalBroadcast::error,				this,	&Client::errorOutput,coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::needDisconnectTheClient,this,	&Client::disconnectClient,coTypeAsync);
    connect(&clientBroadCast,       &ClientBroadCast::message,					this,	&Client::normalOutput,coTypeAsync);
    connect(&clientHeavyLoad,       &ClientHeavyLoad::message,					this,	&Client::normalOutput,coTypeAsync);
    connect(&clientNetworkRead,     &ClientNetworkRead::message,				this,	&Client::normalOutput,coTypeAsync);
    connect(&clientNetworkWrite,	&ClientNetworkWrite::message,				this,	&Client::normalOutput,coTypeAsync);
    connect(&localClientHandler,	&MapBasicMove::message,                     this,	&Client::normalOutput,coTypeAsync);
    connect(&clientLocalBroadcast,  &ClientLocalBroadcast::message,			this,	&Client::normalOutput,coTypeAsync);

    //connect to quit
    connect(&clientNetworkRead,     &ClientNetworkRead::isReadyToStop,          this,&Client::disconnectNextStep,coTypeAsync);
    connect(clientMapManagement,    &ClientMapManagement::isReadyToStop,        this,&Client::disconnectNextStep,coTypeAsync);
    connect(&clientBroadCast,       &ClientBroadCast::isReadyToStop,            this,&Client::disconnectNextStep,coTypeAsync);
    connect(&clientHeavyLoad,       &ClientHeavyLoad::isReadyToStop,            this,&Client::disconnectNextStep,coTypeAsync);
    connect(&clientNetworkWrite,	&ClientNetworkWrite::isReadyToStop,         this,&Client::disconnectNextStep,coTypeAsync);
    connect(&localClientHandler,	&LocalClientHandler::isReadyToStop,         this,&Client::disconnectNextStep,coTypeAsync);
    connect(&clientLocalBroadcast,  &ClientLocalBroadcast::isReadyToStop,      this,&Client::disconnectNextStep,coTypeAsync);
    //stop
    connect(this,&Client::askIfIsReadyToStop,&clientNetworkRead,         &ClientNetworkRead::askIfIsReadyToStop,     coTypeAsync);
    connect(this,&Client::askIfIsReadyToStop,clientMapManagement,        &ClientMapManagement::askIfIsReadyToStop,   coTypeAsync);
    connect(this,&Client::askIfIsReadyToStop,&clientBroadCast,           &ClientBroadCast::askIfIsReadyToStop,       coTypeAsync);
    connect(this,&Client::askIfIsReadyToStop,&clientHeavyLoad,           &ClientHeavyLoad::askIfIsReadyToStop,       coTypeAsync);
    connect(this,&Client::askIfIsReadyToStop,&clientNetworkWrite,        &ClientNetworkWrite::askIfIsReadyToStop,    coTypeAsync);
    connect(this,&Client::askIfIsReadyToStop,&localClientHandler,        &MapBasicMove::askIfIsReadyToStop,          coTypeAsync);
    connect(this,&Client::askIfIsReadyToStop,&clientLocalBroadcast,      &MapBasicMove::askIfIsReadyToStop,          coTypeAsync);

    clientNetworkRead.purgeReadBuffer();
}

//need be call after isReadyToDelete() emited
Client::~Client()
{
    disconnectClient();
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    //normalOutput("Destroyed client");
    #endif
    disconnect(this);
    if(socket!=NULL)
    {
        delete socket;
        socket=NULL;
    }
}

/// \brief new error at connexion
void Client::connectionError(QAbstractSocket::SocketError error)
{
    player_informations.isConnected=false;
    ConnectedSocket *socket=qobject_cast<ConnectedSocket *>(QObject::sender());
    if(socket==NULL)
    {
        normalOutput("Unlocated client socket at error");
        return;
    }
    if(error!=QAbstractSocket::RemoteHostClosedError)
    {
        normalOutput(QStringLiteral("error detected for the client: %1").arg(error));
        socket->disconnectFromHost();
    }
}

/// \warning called in one other thread!!!
void Client::disconnectClient()
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    if(player_informations.account_id!=0)
        normalOutput("Disconnected client");
    #endif
    player_informations.isConnected=false;
    if(ask_is_ready_to_stop)
        return;
    ask_is_ready_to_stop=true;
    if(socket!=NULL)
    {
        socket->disconnectFromHost();
        //commented because if in closing state, block in waitForDisconnected
        /*if(socket->state()!=QAbstractSocket::UnconnectedState)
            socket->waitForDisconnected();*/
        socket=NULL;
    }
    clientNetworkRead.stopRead();

    emit askIfIsReadyToStop();

    BroadCastWithoutSender::broadCastWithoutSender.emit_player_is_disconnected(player_informations.public_and_private_informations.public_informations.pseudo);
}

void Client::disconnectNextStep()
{
    stopped_object++;
    if(stopped_object==OBJECTTOSTOP)
    {
        //remove the player
        if(player_informations.character_loaded)
        {
            GlobalServerData::serverPrivateVariables.connected_players--;
            if(GlobalServerData::serverSettings.sendPlayerNumber)
                GlobalServerData::serverPrivateVariables.player_updater.removeConnectedPlayer();
        }
        player_informations.character_loaded=false;

        emit askIfIsReadyToStop();
        return;
    }
    if(stopped_object==(OBJECTTOSTOP*2))
    {
        clientMapManagement->deleteLater();

        emit isReadyToDelete();
        return;
    }
    if(stopped_object>(OBJECTTOSTOP*2))
    {
        DebugClass::debugConsole(QStringLiteral("remove count error"));
        return;
    }
}

//* do the message by the general broadcast */
void Client::errorOutput(const QString &errorString)
{
    if(player_informations.character_loaded)
        clientBroadCast.sendSystemMessage(player_informations.public_and_private_informations.public_informations.pseudo+" have been kicked from server, have try hack",false);

    normalOutput("Kicked by: "+errorString);
    disconnectClient();
}

void Client::kick()
{
    normalOutput("kicked()");
    disconnectClient();
}

void Client::normalOutput(const QString &message)
{
    if(!player_informations.public_and_private_informations.public_informations.pseudo.isEmpty())
    {
        if(GlobalServerData::serverSettings.anonymous)
            DebugClass::debugConsole(QStringLiteral("%1: %2").arg(player_informations.character_id).arg(message));
        else
            DebugClass::debugConsole(QStringLiteral("%1: %2").arg(player_informations.public_and_private_informations.public_informations.pseudo).arg(message));
    }
    else
    {
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

void Client::send_player_informations()
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(QStringLiteral("load the normal player id: %1, simplified id: %2").arg(player_informations.character_id).arg(player_informations.public_and_private_informations.public_informations.simplifiedId));
    #endif
    BroadCastWithoutSender::broadCastWithoutSender.emit_new_player_is_connected(player_informations);
    this->player_informations=player_informations;
    GlobalServerData::serverPrivateVariables.connected_players++;
    if(GlobalServerData::serverSettings.sendPlayerNumber)
        GlobalServerData::serverPrivateVariables.player_updater.addConnectedPlayer();
}

QString Client::getPseudo()
{
    return player_informations.public_and_private_informations.public_informations.pseudo;
}

void Client::fake_receive_data(QByteArray data)
{
    emit fake_send_received_data(data);
}
