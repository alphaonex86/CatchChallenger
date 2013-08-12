#include "Client.h"
#include "GlobalServerData.h"
#include "../../general/base/QFakeSocket.h"

#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
#include <QTime>
#endif

using namespace CatchChallenger;

/// \warning never cross the signals from and to the different client, complexity ^2
/// \todo drop instant player number notification, and before do the signal without signal/slot, check if the number have change
/// \todo change push position recording, from ClientMapManagement to ClientLocalCalcule, to disable ALL operation for MapVisibilityAlgorithm_None

#define OBJECTTOSTOP 7

Client::Client(ConnectedSocket *socket,bool isFake,ClientMapManagement *clientMapManagement)
{
    this->socket			= socket;
    player_informations.isFake=isFake;
    player_informations.is_logged=false;
    player_informations.isConnected=true;

    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    QTime time;
    time.restart();
    #endif
    clientBroadCast=new ClientBroadCast();
    clientHeavyLoad=new ClientHeavyLoad();
    clientNetworkRead=new ClientNetworkRead(&player_informations,socket);
    clientNetworkWrite=new ClientNetworkWrite(&player_informations,socket);
    localClientHandler=new LocalClientHandler();
    clientLocalBroadcast=new ClientLocalBroadcast();
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(QString("creating object time: %1").arg(time.elapsed()));
    time.restart();
    #endif
    this->clientMapManagement=clientMapManagement;

    if(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm!=MapVisibilityAlgorithm_none)
    {
        connect(clientMapManagement,	&ClientMapManagement::sendFullPacket,   clientNetworkWrite,     &ClientNetworkWrite::sendFullPacket,    Qt::QueuedConnection);
        connect(clientMapManagement,	&ClientMapManagement::sendPacket,       clientNetworkWrite,     &ClientNetworkWrite::sendPacket,        Qt::QueuedConnection);
        connect(clientHeavyLoad,        &ClientHeavyLoad::put_on_the_map,       clientMapManagement,	&ClientMapManagement::put_on_the_map,   Qt::QueuedConnection);
        connect(clientNetworkRead,      &ClientNetworkRead::moveThePlayer,		clientMapManagement,	&ClientMapManagement::moveThePlayer,	Qt::QueuedConnection);
        connect(clientNetworkRead,      &ClientNetworkRead::teleportValidatedTo,clientMapManagement,	&ClientMapManagement::teleportValidatedTo,Qt::QueuedConnection);
        connect(clientMapManagement,	&ClientMapManagement::error,			this,                   &Client::errorOutput,                   Qt::QueuedConnection);
        connect(clientMapManagement,	&ClientMapManagement::message,			this,                   &Client::normalOutput,                  Qt::QueuedConnection);
        connect(&GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove,	&QTimer::timeout,clientMapManagement,&ClientMapManagement::purgeBuffer,Qt::QueuedConnection);
    }

    connect(socket,	static_cast<void(ConnectedSocket::*)(QAbstractSocket::SocketError)>(&ConnectedSocket::error),         this, &Client::connectionError);
    connect(socket,	&ConnectedSocket::disconnected,	this, &Client::disconnectClient);

    ask_is_ready_to_stop=false;

    clientBroadCast->moveToThread(GlobalServerData::serverPrivateVariables.eventThreaderList.at(0));
    clientMapManagement->moveToThread(GlobalServerData::serverPrivateVariables.eventThreaderList.at(1));
    clientNetworkRead->moveToThread(GlobalServerData::serverPrivateVariables.eventThreaderList.at(2));
    clientHeavyLoad->moveToThread(GlobalServerData::serverPrivateVariables.eventThreaderList.at(3));
    localClientHandler->moveToThread(GlobalServerData::serverPrivateVariables.eventThreaderList.at(4));
    clientLocalBroadcast->moveToThread(GlobalServerData::serverPrivateVariables.eventThreaderList.at(5));

    //set variables
    clientBroadCast->setVariable(&player_informations);
    clientMapManagement->setVariable(&player_informations);
    clientHeavyLoad->setVariable(&player_informations);
    localClientHandler->setVariable(&player_informations);
    clientLocalBroadcast->setVariable(&player_informations);

    //previously without ,Qt::QueuedConnection, added
    //connect input/ouput
    connect(clientNetworkRead,	&ClientNetworkRead::newFullInputQuery,      clientNetworkWrite,&ClientNetworkWrite::newFullInputQuery,  Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::newInputQuery,          clientNetworkWrite,&ClientNetworkWrite::newInputQuery,      Qt::QueuedConnection);
    connect(clientNetworkWrite,	&ClientNetworkWrite::newFullOutputQuery,	clientNetworkRead,&ClientNetworkRead::newFullOutputQuery,   Qt::QueuedConnection);
    connect(clientNetworkWrite,	&ClientNetworkWrite::newOutputQuery,		clientNetworkRead,&ClientNetworkRead::newOutputQuery,       Qt::QueuedConnection);

    //connect the write, to send packet on the network
    connect(clientNetworkRead,	&ClientNetworkRead::sendFullPacket, clientNetworkWrite,&ClientNetworkWrite::sendFullPacket,Qt::QueuedConnection);
    connect(clientBroadCast,	&ClientBroadCast::sendFullPacket,   clientNetworkWrite,&ClientNetworkWrite::sendFullPacket,Qt::QueuedConnection);
    connect(clientHeavyLoad,	&ClientHeavyLoad::sendFullPacket,   clientNetworkWrite,&ClientNetworkWrite::sendFullPacket,Qt::QueuedConnection);
    connect(localClientHandler,	&MapBasicMove::sendFullPacket,      clientNetworkWrite,&ClientNetworkWrite::sendFullPacket,Qt::QueuedConnection);
    connect(clientLocalBroadcast,&MapBasicMove::sendFullPacket,     clientNetworkWrite,&ClientNetworkWrite::sendFullPacket,Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::sendPacket,     clientNetworkWrite,&ClientNetworkWrite::sendPacket,Qt::QueuedConnection);
    connect(clientBroadCast,	&ClientBroadCast::sendPacket,       clientNetworkWrite,&ClientNetworkWrite::sendPacket,Qt::QueuedConnection);
    connect(clientHeavyLoad,	&ClientHeavyLoad::sendPacket,       clientNetworkWrite,&ClientNetworkWrite::sendPacket,Qt::QueuedConnection);
    connect(localClientHandler,	&MapBasicMove::sendPacket,          clientNetworkWrite,&ClientNetworkWrite::sendPacket,Qt::QueuedConnection);
    connect(clientLocalBroadcast,&MapBasicMove::sendPacket,         clientNetworkWrite,&ClientNetworkWrite::sendPacket,Qt::QueuedConnection);

    connect(clientNetworkRead,	&ClientNetworkRead::sendQuery,      clientNetworkWrite,&ClientNetworkWrite::sendQuery,Qt::QueuedConnection);

    connect(clientNetworkRead,	&ClientNetworkRead::postReply,      clientNetworkWrite,&ClientNetworkWrite::postReply,Qt::QueuedConnection);
    connect(clientHeavyLoad,	&ClientHeavyLoad::postReply,        clientNetworkWrite,&ClientNetworkWrite::postReply,Qt::QueuedConnection);
    connect(clientLocalBroadcast,&ClientLocalBroadcast::postReply,  clientNetworkWrite,&ClientNetworkWrite::postReply,Qt::QueuedConnection);
    connect(localClientHandler,&LocalClientHandler::postReply,      clientNetworkWrite,&ClientNetworkWrite::postReply,Qt::QueuedConnection);

    connect(localClientHandler,  &LocalClientHandler::dbQuery,      clientHeavyLoad,&ClientHeavyLoad::dbQuery,              Qt::QueuedConnection);
    connect(clientLocalBroadcast,&ClientLocalBroadcast::dbQuery,    clientHeavyLoad,&ClientHeavyLoad::dbQuery,              Qt::QueuedConnection);
    connect(localClientHandler,  &LocalClientHandler::askRandomNumber,clientHeavyLoad,&ClientHeavyLoad::askedRandomNumber,  Qt::QueuedConnection);

    //connect for the seed
    connect(localClientHandler,  &LocalClientHandler::seedValidated,    clientLocalBroadcast,&ClientLocalBroadcast::seedValidated,  Qt::QueuedConnection);
    connect(clientLocalBroadcast,&ClientLocalBroadcast::useSeed,        localClientHandler,&LocalClientHandler::useSeed,            Qt::QueuedConnection);
    connect(clientLocalBroadcast,&ClientLocalBroadcast::addObjectAndSend,localClientHandler,&LocalClientHandler::addObjectAndSend,  Qt::QueuedConnection);
    connect(clientNetworkRead,	 &ClientNetworkRead::plantSeed,         clientLocalBroadcast,&ClientLocalBroadcast::plantSeed,      Qt::QueuedConnection);
    connect(clientNetworkRead,	 &ClientNetworkRead::collectPlant,		clientLocalBroadcast,&ClientLocalBroadcast::collectPlant,   Qt::QueuedConnection);

    //connect for crafting
    connect(clientNetworkRead,	&ClientNetworkRead::useRecipe,	localClientHandler,&LocalClientHandler::useRecipe,Qt::QueuedConnection);

    //connect for trade
    connect(localClientHandler,	&LocalClientHandler::sendTradeRequest,          clientNetworkRead,&ClientNetworkRead::sendTradeRequest,     Qt::QueuedConnection);
    connect(localClientHandler,	&LocalClientHandler::sendBattleRequest,         clientNetworkRead,&ClientNetworkRead::sendBattleRequest,    Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::tradeAccepted,              localClientHandler,&LocalClientHandler::tradeAccepted,      Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::tradeCanceled,              localClientHandler,&LocalClientHandler::tradeCanceled,      Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::battleAccepted,             localClientHandler,&LocalClientHandler::battleAccepted,     Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::battleCanceled,             localClientHandler,&LocalClientHandler::battleCanceled,     Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::tradeFinished,              localClientHandler,&LocalClientHandler::tradeFinished,      Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::tradeAddTradeCash,          localClientHandler,&LocalClientHandler::tradeAddTradeCash,  Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::tradeAddTradeObject,        localClientHandler,&LocalClientHandler::tradeAddTradeObject,Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::tradeAddTradeMonster,       localClientHandler,&LocalClientHandler::tradeAddTradeMonster,Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::newQuestAction,             localClientHandler,&LocalClientHandler::newQuestAction,     Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::clanAction,                 localClientHandler,&LocalClientHandler::clanAction,         Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::clanInvite,                 localClientHandler,&LocalClientHandler::clanInvite,         Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::waitingForCityCaputre,      localClientHandler,&LocalClientHandler::waitingForCityCaputre,Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::moveMonster,                localClientHandler,&LocalClientHandler::moveMonster,        Qt::QueuedConnection);

    //connect the player information
    connect(clientHeavyLoad,	&ClientHeavyLoad::send_player_informations,		clientBroadCast,	&ClientBroadCast::send_player_informations, Qt::QueuedConnection);
    connect(clientHeavyLoad,	&ClientHeavyLoad::put_on_the_map,               localClientHandler,	&LocalClientHandler::put_on_the_map,        Qt::QueuedConnection);
    connect(clientHeavyLoad,	&ClientHeavyLoad::put_on_the_map,               clientLocalBroadcast,&ClientLocalBroadcast::put_on_the_map,     Qt::QueuedConnection);
    connect(clientHeavyLoad,	&ClientHeavyLoad::send_player_informations,		this,               &Client::send_player_informations,          Qt::QueuedConnection);
    connect(clientHeavyLoad,	&ClientHeavyLoad::newRandomNumber,              localClientHandler,	&LocalClientHandler::newRandomNumber,       Qt::QueuedConnection);
    connect(clientHeavyLoad,	&ClientHeavyLoad::haveClanInfo,                 localClientHandler,	&LocalClientHandler::haveClanInfo,          Qt::QueuedConnection);
    connect(localClientHandler,	&LocalClientHandler::askClan,                   clientHeavyLoad,	&ClientHeavyLoad::askClan,                  Qt::QueuedConnection);
    connect(localClientHandler,	&LocalClientHandler::clanChange,                clientBroadCast,	&ClientBroadCast::clanChange,               Qt::QueuedConnection);

    //packet parsed (heavy)
    connect(clientNetworkRead,&ClientNetworkRead::askLogin,clientHeavyLoad,&ClientHeavyLoad::askLogin,          Qt::QueuedConnection);
    connect(clientNetworkRead,&ClientNetworkRead::datapackList,clientHeavyLoad,&ClientHeavyLoad::datapackList,  Qt::QueuedConnection);

    //packet parsed (map management)
    connect(clientNetworkRead,	&ClientNetworkRead::moveThePlayer,			localClientHandler,	&LocalClientHandler::moveThePlayer,         Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::moveThePlayer,			clientLocalBroadcast,&MapBasicMove::moveThePlayer,              Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::teleportValidatedTo,	localClientHandler,	&LocalClientHandler::teleportValidatedTo,	Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::teleportValidatedTo,	clientLocalBroadcast,&ClientLocalBroadcast::teleportValidatedTo,Qt::QueuedConnection);
    connect(localClientHandler,	&LocalClientHandler::teleportTo,            clientNetworkRead,	&ClientNetworkRead::teleportTo,             Qt::QueuedConnection);

    //packet parsed (broadcast)
    connect(localClientHandler,	&LocalClientHandler::receiveSystemText,     clientBroadCast,	&ClientBroadCast::receiveSystemText,        Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::sendChatText,			clientBroadCast,	&ClientBroadCast::sendChatText,				Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::sendLocalChatText,      clientLocalBroadcast,&ClientLocalBroadcast::sendLocalChatText,	Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::sendPM,                 clientBroadCast,	&ClientBroadCast::sendPM,                   Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::sendBroadCastCommand,	clientBroadCast,	&ClientBroadCast::sendBroadCastCommand,		Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::sendHandlerCommand,		localClientHandler,	&LocalClientHandler::sendHandlerCommand,	Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::destroyObject,          localClientHandler,	&LocalClientHandler::destroyObject,         Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::useObject,              localClientHandler,	&LocalClientHandler::useObject,             Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::wareHouseStore,         localClientHandler,	&LocalClientHandler::wareHouseStore,        Qt::QueuedConnection);
    connect(clientBroadCast,	&ClientBroadCast::kicked,                   this,               &Client::kicked,                            Qt::QueuedConnection);

    //shops
    connect(clientNetworkRead,	&ClientNetworkRead::getShopList,            localClientHandler,	&LocalClientHandler::getShopList,           Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::buyObject,              localClientHandler,	&LocalClientHandler::buyObject,             Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::sellObject,             localClientHandler,	&LocalClientHandler::sellObject,            Qt::QueuedConnection);

    //factory
    connect(clientNetworkRead,	&ClientNetworkRead::getFactoryList,         localClientHandler,	&LocalClientHandler::getFactoryList,        Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::buyFactoryObject,       localClientHandler,	&LocalClientHandler::buyFactoryObject,      Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::sellFactoryObject,      localClientHandler,	&LocalClientHandler::sellFactoryObject,     Qt::QueuedConnection);

    //fight
    connect(clientNetworkRead,	&ClientNetworkRead::tryEscape,              localClientHandler,	&LocalClientHandler::tryEscape,             Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::useSkill,               localClientHandler,	&LocalClientHandler::useSkill,              Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::learnSkill,             localClientHandler,	&LocalClientHandler::learnSkill,            Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::heal,                   localClientHandler,	&LocalClientHandler::heal,                  Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::requestFight,           localClientHandler,	&LocalClientHandler::requestFight,          Qt::QueuedConnection);

    //connect the message
    connect(clientBroadCast,	&ClientBroadCast::error,					this,	&Client::errorOutput,Qt::QueuedConnection);
    connect(clientHeavyLoad,	&ClientHeavyLoad::error,					this,	&Client::errorOutput,Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::error,					this,	&Client::errorOutput,Qt::QueuedConnection);
    connect(clientNetworkWrite,	&ClientNetworkWrite::error,					this,	&Client::errorOutput,Qt::QueuedConnection);
    connect(localClientHandler,	&MapBasicMove::error,						this,	&Client::errorOutput,Qt::QueuedConnection);
    connect(clientLocalBroadcast,&ClientLocalBroadcast::error,				this,	&Client::errorOutput,Qt::QueuedConnection);
    connect(clientBroadCast,	&ClientBroadCast::message,					this,	&Client::normalOutput,Qt::QueuedConnection);
    connect(clientHeavyLoad,	&ClientHeavyLoad::message,					this,	&Client::normalOutput,Qt::QueuedConnection);
    connect(clientNetworkRead,	&ClientNetworkRead::message,				this,	&Client::normalOutput,Qt::QueuedConnection);
    connect(clientNetworkWrite,	&ClientNetworkWrite::message,				this,	&Client::normalOutput,Qt::QueuedConnection);
    connect(localClientHandler,	&MapBasicMove::message,                     this,	&Client::normalOutput,Qt::QueuedConnection);
    connect(clientLocalBroadcast,&ClientLocalBroadcast::message,			this,	&Client::normalOutput,Qt::QueuedConnection);

    //connect to quit
    connect(clientNetworkRead,	&ClientNetworkRead::isReadyToStop,          this,&Client::disconnectNextStep,Qt::QueuedConnection);
    connect(clientMapManagement,&ClientMapManagement::isReadyToStop,        this,&Client::disconnectNextStep,Qt::QueuedConnection);
    connect(clientBroadCast,	&ClientBroadCast::isReadyToStop,            this,&Client::disconnectNextStep,Qt::QueuedConnection);
    connect(clientHeavyLoad,	&ClientHeavyLoad::isReadyToStop,            this,&Client::disconnectNextStep,Qt::QueuedConnection);
    connect(clientNetworkWrite,	&ClientNetworkWrite::isReadyToStop,         this,&Client::disconnectNextStep,Qt::QueuedConnection);
    connect(localClientHandler,	&LocalClientHandler::isReadyToStop,         this,&Client::disconnectNextStep,Qt::QueuedConnection);
    connect(clientLocalBroadcast,&ClientLocalBroadcast::isReadyToStop,      this,&Client::disconnectNextStep,Qt::QueuedConnection);
    //stop
    connect(this,&Client::askIfIsReadyToStop,clientNetworkRead,         &ClientNetworkRead::askIfIsReadyToStop,     Qt::QueuedConnection);
    connect(this,&Client::askIfIsReadyToStop,clientMapManagement,       &ClientMapManagement::askIfIsReadyToStop,   Qt::QueuedConnection);
    connect(this,&Client::askIfIsReadyToStop,clientBroadCast,           &ClientBroadCast::askIfIsReadyToStop,       Qt::QueuedConnection);
    connect(this,&Client::askIfIsReadyToStop,clientHeavyLoad,           &ClientHeavyLoad::askIfIsReadyToStop,       Qt::QueuedConnection);
    connect(this,&Client::askIfIsReadyToStop,clientNetworkWrite,        &ClientNetworkWrite::askIfIsReadyToStop,    Qt::QueuedConnection);
    connect(this,&Client::askIfIsReadyToStop,localClientHandler,        &MapBasicMove::askIfIsReadyToStop,          Qt::QueuedConnection);
    connect(this,&Client::askIfIsReadyToStop,clientLocalBroadcast,      &MapBasicMove::askIfIsReadyToStop,          Qt::QueuedConnection);

    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(QString("connecting object time: %1").arg(time.elapsed()));
    #endif

    stopped_object=0;

    clientNetworkRead->purgeReadBuffer();
}

//need be call after isReadyToDelete() emited
Client::~Client()
{
    disconnectClient();
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("Destroyed client");
    #endif
    disconnect(this);
    if(socket!=NULL)
    {
        delete socket;
        socket=NULL;
    }

    if(clientBroadCast!=NULL)
        delete clientBroadCast;
    if(clientHeavyLoad!=NULL)
        delete clientHeavyLoad;
    if(clientNetworkRead!=NULL)
        delete clientNetworkRead;
    if(clientNetworkWrite!=NULL)
        delete clientNetworkWrite;
    if(localClientHandler!=NULL)
        delete localClientHandler;
    if(clientLocalBroadcast!=NULL)
        delete clientLocalBroadcast;
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
        normalOutput(QString("error detected for the client: %1").arg(error));
        socket->disconnectFromHost();
    }
}

/// \warning called in one other thread!!!
void Client::disconnectClient()
{
    player_informations.isConnected=false;
    if(ask_is_ready_to_stop)
        return;
    ask_is_ready_to_stop=true;
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("Disconnected client");
    #endif
    if(socket!=NULL)
    {
        socket->disconnectFromHost();
        if(socket->state()!=QAbstractSocket::UnconnectedState)
            socket->waitForDisconnected();
        socket=NULL;
    }
    clientNetworkRead->stopRead();

    emit askIfIsReadyToStop();

    BroadCastWithoutSender::broadCastWithoutSender.emit_player_is_disconnected(player_informations.public_and_private_informations.public_informations.pseudo);
}

void Client::disconnectNextStep()
{
    stopped_object++;
    if(stopped_object==OBJECTTOSTOP)
    {
        //remove the player
        if(player_informations.is_logged)
        {
            GlobalServerData::serverPrivateVariables.connected_players--;
            if(GlobalServerData::serverSettings.commmonServerSettings.sendPlayerNumber)
                GlobalServerData::serverPrivateVariables.player_updater.removeConnectedPlayer();
        }
        player_informations.is_logged=false;

        emit askIfIsReadyToStop();
        return;
    }
    if(stopped_object==(OBJECTTOSTOP*2))
    {
        clientBroadCast->deleteLater();
        clientHeavyLoad->deleteLater();
        clientMapManagement->deleteLater();
        clientNetworkRead->deleteLater();
        clientNetworkWrite->deleteLater();
        localClientHandler->deleteLater();
        clientLocalBroadcast->deleteLater();
        //now the object is not usable
        clientBroadCast=NULL;
        clientHeavyLoad=NULL;
        clientMapManagement=NULL;
        clientNetworkRead=NULL;
        clientNetworkWrite=NULL;
        localClientHandler=NULL;
        clientLocalBroadcast=NULL;

        emit isReadyToDelete();
        return;
    }
    if(stopped_object>(OBJECTTOSTOP*2))
    {
        DebugClass::debugConsole(QString("remove count error"));
        return;
    }
}

//* do the message by the general broadcast */
void Client::errorOutput(const QString &errorString)
{
    if(player_informations.is_logged)
        clientBroadCast->sendSystemMessage(player_informations.public_and_private_informations.public_informations.pseudo+" have been kicked from server, have try hack",false);

    normalOutput("Kicked by: "+errorString);
    disconnectClient();
}

void Client::kicked()
{
    normalOutput("kicked()");
    disconnectClient();
}

void Client::normalOutput(const QString &message)
{
    if(!player_informations.public_and_private_informations.public_informations.pseudo.isEmpty())
        DebugClass::debugConsole(QString("%1: %2").arg(player_informations.public_and_private_informations.public_informations.pseudo).arg(message));
    else
    {
        QString ip;
        if(socket==NULL)
            ip="unknown";
        else
        {
            QHostAddress hostAddress=socket->peerAddress();
            if(hostAddress==QHostAddress::LocalHost || hostAddress==QHostAddress::LocalHostIPv6)
                ip=QString("localhost:%1").arg(socket->peerPort());
            else if(hostAddress==QHostAddress::Null || hostAddress==QHostAddress::Any || hostAddress==QHostAddress::AnyIPv4 || hostAddress==QHostAddress::AnyIPv6 || hostAddress==QHostAddress::Broadcast)
                ip="internal";
            else
                ip=QString("%1:%2").arg(hostAddress.toString()).arg(socket->peerPort());
        }
        DebugClass::debugConsole(QString("%1: %2").arg(ip).arg(message));
    }
}

void Client::send_player_informations()
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(QString("load the normal player id: %1, simplified id: %2").arg(player_informations.id).arg(player_informations.public_and_private_informations.public_informations.simplifiedId));
    #endif
    BroadCastWithoutSender::broadCastWithoutSender.emit_new_player_is_connected(player_informations);
    this->player_informations=player_informations;
    GlobalServerData::serverPrivateVariables.connected_players++;
    if(GlobalServerData::serverSettings.commmonServerSettings.sendPlayerNumber)
        GlobalServerData::serverPrivateVariables.player_updater.addConnectedPlayer();

    //remove the useless connection
    /*disconnect(clientHeavyLoad,	SIGNAL(send_player_informations()),			clientBroadCast,	SLOT(send_player_informations()));
    disconnect(clientHeavyLoad,	SIGNAL(send_player_informations()),			clientNetworkRead,	SLOT(send_player_informations()));
    disconnect(clientHeavyLoad,	SIGNAL(put_on_the_map(quint32,Map_final*,quint16,quint16,Orientation,quint16)),	clientMapManagement,	SLOT(put_on_the_map(quint32,Map_final*,quint16,quint16,Orientation,quint16)));*/
}

QString Client::getPseudo()
{
    return player_informations.public_and_private_informations.public_informations.pseudo;
}

void Client::fake_receive_data(QByteArray data)
{
    emit fake_send_received_data(data);
}
