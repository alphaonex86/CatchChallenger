#include "Client.h"
#include "GlobalServerData.h"
#include "../../general/base/QFakeSocket.h"

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

    clientBroadCast=new ClientBroadCast();
    clientHeavyLoad=new ClientHeavyLoad();
    clientNetworkRead=new ClientNetworkRead(&player_informations,socket);
    clientNetworkWrite=new ClientNetworkWrite(&player_informations,socket);
    localClientHandler=new LocalClientHandler();
    clientLocalBroadcast=new ClientLocalBroadcast();
    this->clientMapManagement=clientMapManagement;

    if(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm!=MapVisibilityAlgorithm_none)
    {
        connect(clientMapManagement,	SIGNAL(sendPacket(quint8,quint16,QByteArray)),clientNetworkWrite,SLOT(sendPacket(quint8,quint16,QByteArray)),Qt::QueuedConnection);
        connect(clientMapManagement,	SIGNAL(sendPacket(quint8,QByteArray)),clientNetworkWrite,SLOT(sendPacket(quint8,QByteArray)),Qt::QueuedConnection);
        connect(clientHeavyLoad,	SIGNAL(put_on_the_map(Map*,/*COORD_TYPE*/quint8,/*COORD_TYPE*/quint8,Orientation)),clientMapManagement,	SLOT(put_on_the_map(Map*,/*COORD_TYPE*/quint8,/*COORD_TYPE*/quint8,Orientation)),Qt::QueuedConnection);
        connect(clientNetworkRead,	SIGNAL(moveThePlayer(quint8,Direction)),			clientMapManagement,	SLOT(moveThePlayer(quint8,Direction)),				Qt::QueuedConnection);
        connect(clientNetworkRead,	SIGNAL(teleportValidatedTo(Map*,quint8,quint8,Orientation)),			clientMapManagement,	SLOT(teleportValidatedTo(Map*,quint8,quint8,Orientation)),				Qt::QueuedConnection);
        connect(clientMapManagement,	SIGNAL(error(QString)),						this,	SLOT(errorOutput(QString)),Qt::QueuedConnection);
        connect(clientMapManagement,	SIGNAL(message(QString)),					this,	SLOT(normalOutput(QString)),Qt::QueuedConnection);
        connect(&GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove,	SIGNAL(timeout()),clientMapManagement,SLOT(purgeBuffer()),Qt::QueuedConnection);
    }

    remote_ip=socket->peerAddress().toString();
    port=socket->peerPort();
    connect(socket,	SIGNAL(error(QAbstractSocket::SocketError)),	this, SLOT(connectionError(QAbstractSocket::SocketError)));
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(QString("Connected client: %1, %2").arg(remote_ip).arg(port));
    #endif
    connect(socket,	SIGNAL(disconnected()),				this, SLOT(disconnectClient()));

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
    connect(clientNetworkRead,	SIGNAL(newInputQuery(quint8,quint16,quint8)),	clientNetworkWrite,SLOT(newInputQuery(quint8,quint16,quint8)),Qt::QueuedConnection);
    connect(clientNetworkRead,	SIGNAL(newInputQuery(quint8,quint8)),		clientNetworkWrite,SLOT(newInputQuery(quint8,quint8)),Qt::QueuedConnection);
    connect(clientNetworkWrite,	SIGNAL(newOutputQuery(quint8,quint16,quint8)),	clientNetworkRead,SLOT(newOutputQuery(quint8,quint16,quint8)),Qt::QueuedConnection);
    connect(clientNetworkWrite,	SIGNAL(newOutputQuery(quint8,quint8)),		clientNetworkRead,SLOT(newOutputQuery(quint8,quint8)),Qt::QueuedConnection);

    //connect the write, to send packet on the network
    connect(clientNetworkRead,	SIGNAL(sendPacket(quint8,quint16,QByteArray)),clientNetworkWrite,SLOT(sendPacket(quint8,quint16,QByteArray)),Qt::QueuedConnection);
    connect(clientBroadCast,	SIGNAL(sendPacket(quint8,quint16,QByteArray)),clientNetworkWrite,SLOT(sendPacket(quint8,quint16,QByteArray)),Qt::QueuedConnection);
    connect(clientHeavyLoad,	SIGNAL(sendPacket(quint8,quint16,QByteArray)),clientNetworkWrite,SLOT(sendPacket(quint8,quint16,QByteArray)),Qt::QueuedConnection);
    connect(localClientHandler,	SIGNAL(sendPacket(quint8,quint16,QByteArray)),clientNetworkWrite,SLOT(sendPacket(quint8,quint16,QByteArray)),Qt::QueuedConnection);
    connect(clientLocalBroadcast,SIGNAL(sendPacket(quint8,quint16,QByteArray)),clientNetworkWrite,SLOT(sendPacket(quint8,quint16,QByteArray)),Qt::QueuedConnection);

    connect(clientNetworkRead,	SIGNAL(sendPacket(quint8,QByteArray)),clientNetworkWrite,SLOT(sendPacket(quint8,QByteArray)),Qt::QueuedConnection);
    connect(clientBroadCast,	SIGNAL(sendPacket(quint8,QByteArray)),clientNetworkWrite,SLOT(sendPacket(quint8,QByteArray)),Qt::QueuedConnection);
    connect(clientHeavyLoad,	SIGNAL(sendPacket(quint8,QByteArray)),clientNetworkWrite,SLOT(sendPacket(quint8,QByteArray)),Qt::QueuedConnection);
    connect(localClientHandler,	SIGNAL(sendPacket(quint8,QByteArray)),clientNetworkWrite,SLOT(sendPacket(quint8,QByteArray)),Qt::QueuedConnection);
    connect(clientLocalBroadcast,	SIGNAL(sendPacket(quint8,QByteArray)),clientNetworkWrite,SLOT(sendPacket(quint8,QByteArray)),Qt::QueuedConnection);

    connect(clientNetworkRead,	SIGNAL(sendQuery(quint8,quint16,quint8,QByteArray)),clientNetworkWrite,SLOT(sendQuery(quint8,quint16,quint8,QByteArray)),Qt::QueuedConnection);

    connect(clientNetworkRead,	SIGNAL(postReply(quint8,QByteArray)),clientNetworkWrite,SLOT(postReply(quint8,QByteArray)),Qt::QueuedConnection);
    connect(clientHeavyLoad,	SIGNAL(postReply(quint8,QByteArray)),clientNetworkWrite,SLOT(postReply(quint8,QByteArray)),Qt::QueuedConnection);
    connect(clientLocalBroadcast,SIGNAL(postReply(quint8,QByteArray)),clientNetworkWrite,SLOT(postReply(quint8,QByteArray)),Qt::QueuedConnection);
    connect(localClientHandler,SIGNAL(postReply(quint8,QByteArray)),clientNetworkWrite,SLOT(postReply(quint8,QByteArray)),Qt::QueuedConnection);

    connect(localClientHandler,SIGNAL(dbQuery(QString)),clientHeavyLoad,SLOT(dbQuery(QString)),Qt::QueuedConnection);
    connect(clientLocalBroadcast,SIGNAL(dbQuery(QString)),clientHeavyLoad,SLOT(dbQuery(QString)),Qt::QueuedConnection);
    connect(localClientHandler,SIGNAL(askRandomNumber()),clientHeavyLoad,SLOT(askedRandomNumber()),Qt::QueuedConnection);

    //connect for the seed
    connect(localClientHandler,SIGNAL(seedValidated()),clientLocalBroadcast,SLOT(seedValidated()),Qt::QueuedConnection);
    connect(clientLocalBroadcast,SIGNAL(useSeed(quint8)),localClientHandler,SLOT(useSeed(quint8)),Qt::QueuedConnection);
    connect(clientLocalBroadcast,SIGNAL(addObjectAndSend(quint32,quint32)),localClientHandler,SLOT(addObjectAndSend(quint32,quint32)),Qt::QueuedConnection);
    connect(clientNetworkRead,	SIGNAL(plantSeed(quint8,quint8)),	clientLocalBroadcast,SLOT(plantSeed(quint8,quint8)),Qt::QueuedConnection);
    connect(clientNetworkRead,	SIGNAL(collectPlant(quint8)),		clientLocalBroadcast,SLOT(collectPlant(quint8)),Qt::QueuedConnection);

    //connect for crafting
    connect(clientNetworkRead,	SIGNAL(useRecipe(quint8,quint32)),	localClientHandler,SLOT(useRecipe(quint8,quint32)),Qt::QueuedConnection);

    //connect for trade
    connect(localClientHandler,	SIGNAL(sendTradeRequest(QByteArray)),       clientNetworkRead,SLOT(sendTradeRequest(QByteArray)),Qt::QueuedConnection);
    connect(clientNetworkRead,	SIGNAL(tradeAccepted()),                    localClientHandler,SLOT(tradeAccepted()),Qt::QueuedConnection);
    connect(clientNetworkRead,	SIGNAL(tradeCanceled()),                    localClientHandler,SLOT(tradeCanceled()),Qt::QueuedConnection);
    connect(clientNetworkRead,	SIGNAL(tradeFinished()),                    localClientHandler,SLOT(tradeFinished()),Qt::QueuedConnection);
    connect(clientNetworkRead,	SIGNAL(tradeAddTradeCash(quint64)),         localClientHandler,SLOT(tradeAddTradeCash(quint64)),Qt::QueuedConnection);
    connect(clientNetworkRead,	SIGNAL(tradeAddTradeObject(quint32,quint32)),localClientHandler,SLOT(tradeAddTradeObject(quint32,quint32)),Qt::QueuedConnection);
    connect(clientNetworkRead,	SIGNAL(tradeAddTradeMonster(quint32)),      localClientHandler,SLOT(tradeAddTradeMonster(quint32)),Qt::QueuedConnection);

    //connect the player information
    connect(clientHeavyLoad,	SIGNAL(send_player_informations()),			clientBroadCast,	SLOT(send_player_informations()),Qt::QueuedConnection);
    connect(clientHeavyLoad,	SIGNAL(put_on_the_map(Map*,/*COORD_TYPE*/quint8,/*COORD_TYPE*/quint8,Orientation)),	localClientHandler,	SLOT(put_on_the_map(Map*,/*COORD_TYPE*/quint8,/*COORD_TYPE*/quint8,Orientation)),Qt::QueuedConnection);
    connect(clientHeavyLoad,	SIGNAL(put_on_the_map(Map*,/*COORD_TYPE*/quint8,/*COORD_TYPE*/quint8,Orientation)),	clientLocalBroadcast,	SLOT(put_on_the_map(Map*,/*COORD_TYPE*/quint8,/*COORD_TYPE*/quint8,Orientation)),Qt::QueuedConnection);
    connect(clientHeavyLoad,	SIGNAL(send_player_informations()),			this,			SLOT(send_player_informations()),Qt::QueuedConnection);
    connect(clientHeavyLoad,	SIGNAL(newRandomNumber(QByteArray)),			localClientHandler,	SLOT(newRandomNumber(QByteArray)),Qt::QueuedConnection);

    //packet parsed (heavy)
    connect(clientNetworkRead,SIGNAL(askLogin(quint8,QString,QByteArray)),
        clientHeavyLoad,SLOT(askLogin(quint8,QString,QByteArray)),Qt::QueuedConnection);
    connect(clientNetworkRead,SIGNAL(datapackList(quint8,QStringList,QList<quint64>)),
        clientHeavyLoad,SLOT(datapackList(quint8,QStringList,QList<quint64>)),Qt::QueuedConnection);

    //packet parsed (map management)
    connect(clientNetworkRead,	SIGNAL(moveThePlayer(quint8,Direction)),			localClientHandler,	SLOT(moveThePlayer(quint8,Direction)),                                          Qt::QueuedConnection);
    connect(clientNetworkRead,	SIGNAL(moveThePlayer(quint8,Direction)),			clientLocalBroadcast,	SLOT(moveThePlayer(quint8,Direction)),                                      Qt::QueuedConnection);
    connect(clientNetworkRead,	SIGNAL(teleportValidatedTo(Map*,quint8,quint8,Orientation)),			localClientHandler,	SLOT(teleportValidatedTo(Map*,quint8,quint8,Orientation)),	Qt::QueuedConnection);
    connect(clientNetworkRead,	SIGNAL(teleportValidatedTo(Map*,quint8,quint8,Orientation)),			clientLocalBroadcast,	SLOT(teleportValidatedTo(Map*,quint8,quint8,Orientation)),Qt::QueuedConnection);
    connect(localClientHandler,	SIGNAL(teleportTo(Map*,quint8,quint8,Orientation)),	clientNetworkRead,	SLOT(teleportTo(Map*,quint8,quint8,Orientation)),                               Qt::QueuedConnection);

    //packet parsed (broadcast)
    connect(localClientHandler,	SIGNAL(receiveSystemText(QString)),                 clientBroadCast,	SLOT(receiveSystemText(QString)),                   Qt::QueuedConnection);
    connect(clientNetworkRead,	SIGNAL(sendChatText(Chat_type,QString)),			clientBroadCast,	SLOT(sendChatText(Chat_type,QString)),				Qt::QueuedConnection);
    connect(clientNetworkRead,	SIGNAL(sendLocalChatText(QString)),                 clientLocalBroadcast,	SLOT(sendLocalChatText(QString)),				Qt::QueuedConnection);
    connect(clientNetworkRead,	SIGNAL(sendPM(QString,QString)),                    clientBroadCast,	SLOT(sendPM(QString,QString)),                      Qt::QueuedConnection);
    connect(clientNetworkRead,	SIGNAL(sendBroadCastCommand(QString,QString)),		clientBroadCast,	SLOT(sendBroadCastCommand(QString,QString)),		Qt::QueuedConnection);
    connect(clientNetworkRead,	SIGNAL(sendHandlerCommand(QString,QString)),		localClientHandler,	SLOT(sendHandlerCommand(QString,QString)),			Qt::QueuedConnection);
    connect(clientNetworkRead,	SIGNAL(destroyObject(quint32,quint32)),             localClientHandler,	SLOT(destroyObject(quint32,quint32)),               Qt::QueuedConnection);
    connect(clientNetworkRead,	SIGNAL(useObject(quint8,quint32)),                  localClientHandler,	SLOT(useObject(quint8,quint32)),                    Qt::QueuedConnection);
    connect(clientBroadCast,	SIGNAL(kicked()),                                   this,               SLOT(kicked()),                                     Qt::QueuedConnection);

    //shops
    connect(clientNetworkRead,	SIGNAL(getShopList(quint32,quint32)),                           localClientHandler,	SLOT(getShopList(quint32,quint32)),                         Qt::QueuedConnection);
    connect(clientNetworkRead,	SIGNAL(buyObject(quint32,quint32,quint32,quint32,quint32)),		localClientHandler,	SLOT(buyObject(quint32,quint32,quint32,quint32,quint32)),	Qt::QueuedConnection);
    connect(clientNetworkRead,	SIGNAL(sellObject(quint32,quint32,quint32,quint32,quint32)),    localClientHandler,	SLOT(sellObject(quint32,quint32,quint32,quint32,quint32)),	Qt::QueuedConnection);

    //fight
    connect(clientNetworkRead,	SIGNAL(tryEscape()),                                localClientHandler,	SLOT(tryEscape()),                         Qt::QueuedConnection);
    connect(clientNetworkRead,	SIGNAL(useSkill(quint32)),                          localClientHandler,	SLOT(useSkill(quint32)),                   Qt::QueuedConnection);
    connect(clientNetworkRead,	SIGNAL(learnSkill(quint32,quint32)),                localClientHandler,	SLOT(learnSkill(quint32,quint32)),         Qt::QueuedConnection);

    //connect the message
    connect(clientBroadCast,	SIGNAL(error(QString)),						this,	SLOT(errorOutput(QString)),Qt::QueuedConnection);
    connect(clientHeavyLoad,	SIGNAL(error(QString)),						this,	SLOT(errorOutput(QString)),Qt::QueuedConnection);
    connect(clientNetworkRead,	SIGNAL(error(QString)),						this,	SLOT(errorOutput(QString)),Qt::QueuedConnection);
    connect(clientNetworkWrite,	SIGNAL(error(QString)),						this,	SLOT(errorOutput(QString)),Qt::QueuedConnection);
    connect(localClientHandler,	SIGNAL(error(QString)),						this,	SLOT(errorOutput(QString)),Qt::QueuedConnection);
    connect(clientLocalBroadcast,SIGNAL(error(QString)),					this,	SLOT(errorOutput(QString)),Qt::QueuedConnection);
    connect(clientBroadCast,	SIGNAL(message(QString)),					this,	SLOT(normalOutput(QString)),Qt::QueuedConnection);
    connect(clientHeavyLoad,	SIGNAL(message(QString)),					this,	SLOT(normalOutput(QString)),Qt::QueuedConnection);
    connect(clientNetworkRead,	SIGNAL(message(QString)),					this,	SLOT(normalOutput(QString)),Qt::QueuedConnection);
    connect(clientNetworkWrite,	SIGNAL(message(QString)),					this,	SLOT(normalOutput(QString)),Qt::QueuedConnection);
    connect(localClientHandler,	SIGNAL(message(QString)),					this,	SLOT(normalOutput(QString)),Qt::QueuedConnection);
    connect(clientLocalBroadcast,SIGNAL(message(QString)),					this,	SLOT(normalOutput(QString)),Qt::QueuedConnection);

    //connect to quit
    connect(clientNetworkRead,	SIGNAL(isReadyToStop()),this,SLOT(disconnectNextStep()),Qt::QueuedConnection);
    connect(clientMapManagement,	SIGNAL(isReadyToStop()),this,SLOT(disconnectNextStep()),Qt::QueuedConnection);
    connect(clientBroadCast,	SIGNAL(isReadyToStop()),this,SLOT(disconnectNextStep()),Qt::QueuedConnection);
    connect(clientHeavyLoad,	SIGNAL(isReadyToStop()),this,SLOT(disconnectNextStep()),Qt::QueuedConnection);
    connect(clientNetworkWrite,	SIGNAL(isReadyToStop()),this,SLOT(disconnectNextStep()),Qt::QueuedConnection);
    connect(localClientHandler,	SIGNAL(isReadyToStop()),this,SLOT(disconnectNextStep()),Qt::QueuedConnection);
    connect(clientLocalBroadcast,	SIGNAL(isReadyToStop()),this,SLOT(disconnectNextStep()),Qt::QueuedConnection);
    //stop
    connect(this,SIGNAL(askIfIsReadyToStop()),clientNetworkRead,SLOT(askIfIsReadyToStop()),Qt::QueuedConnection);
    connect(this,SIGNAL(askIfIsReadyToStop()),clientMapManagement,SLOT(askIfIsReadyToStop()),Qt::QueuedConnection);
    connect(this,SIGNAL(askIfIsReadyToStop()),clientBroadCast,SLOT(askIfIsReadyToStop()),Qt::QueuedConnection);
    connect(this,SIGNAL(askIfIsReadyToStop()),clientHeavyLoad,SLOT(askIfIsReadyToStop()),Qt::QueuedConnection);
    connect(this,SIGNAL(askIfIsReadyToStop()),clientNetworkWrite,SLOT(askIfIsReadyToStop()),Qt::QueuedConnection);
    connect(this,SIGNAL(askIfIsReadyToStop()),localClientHandler,SLOT(askIfIsReadyToStop()),Qt::QueuedConnection);
    connect(this,SIGNAL(askIfIsReadyToStop()),clientLocalBroadcast,SLOT(askIfIsReadyToStop()),Qt::QueuedConnection);

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
void Client::errorOutput(QString errorString)
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

void Client::normalOutput(QString message)
{
    if(!player_informations.is_logged)
        DebugClass::debugConsole(QString("%1:%2 %3").arg(remote_ip).arg(port).arg(message));
    else
        DebugClass::debugConsole(QString("%1: %2").arg(player_informations.public_and_private_informations.public_informations.pseudo).arg(message));
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
