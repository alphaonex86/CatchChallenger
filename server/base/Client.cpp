#include "Client.h"
#include "EventDispatcher.h"

/// \warning never cross the signals from and to the different client, complexity ^2
/// \todo drop instant player number notification, and before do the signal without signal/slot, check if the number have change
/// \todo change push position recording, from ClientMapManagement to ClientLocalCalcule, to disable ALL operation for MapVisibilityAlgorithm_None

Client::Client(QAbstractSocket *socket,bool isFake)
{
	player_informations.isFake=isFake;

	clientBroadCast=new ClientBroadCast();
	clientHeavyLoad=new ClientHeavyLoad();
	clientNetworkRead=new ClientNetworkRead(&player_informations,socket);
	clientNetworkWrite=new ClientNetworkWrite(socket);
	clientLocalCalcule=new ClientLocalCalcule();

	switch(EventDispatcher::generalData.serverSettings.mapVisibility.mapVisibilityAlgorithm)
	{
		default:
		case MapVisibilityAlgorithm_simple:
			clientMapManagement=new MapVisibilityAlgorithm_Simple();
		break;
		case MapVisibilityAlgorithm_none:
			clientMapManagement=new MapVisibilityAlgorithm_None();
		break;
	}
	if(EventDispatcher::generalData.serverSettings.mapVisibility.mapVisibilityAlgorithm!=MapVisibilityAlgorithm_none)
	{
		connect(clientMapManagement,	SIGNAL(sendPacket(quint8,quint16,QByteArray)),clientNetworkWrite,SLOT(sendPacket(quint8,quint16,QByteArray)),Qt::QueuedConnection);
		connect(clientMapManagement,	SIGNAL(sendPacket(quint8,QByteArray)),clientNetworkWrite,SLOT(sendPacket(quint8,QByteArray)),Qt::QueuedConnection);
		connect(clientHeavyLoad,	SIGNAL(put_on_the_map(quint32,Map_server*,quint16,quint16,Orientation,quint16)),	clientMapManagement,	SLOT(put_on_the_map(quint32,Map_server*,quint16,quint16,Orientation,quint16)),Qt::QueuedConnection);
		connect(clientNetworkRead,	SIGNAL(moveThePlayer(quint8,Direction)),			clientMapManagement,	SLOT(moveThePlayer(quint8,Direction)),				Qt::QueuedConnection);
		connect(clientMapManagement,	SIGNAL(error(QString)),						this,	SLOT(errorOutput(QString)),Qt::QueuedConnection);
		connect(clientMapManagement,	SIGNAL(message(QString)),					this,	SLOT(normalOutput(QString)),Qt::QueuedConnection);
	}

	player_informations.public_and_private_informations.public_informations.pseudo="";
	player_informations.public_and_private_informations.public_informations.id=0;

	if(!player_informations.isFake)
	{
		remote_ip=socket->peerAddress().toString();
		port=socket->peerPort();
		connect(socket,	SIGNAL(error(QAbstractSocket::SocketError)),	this, SLOT(connectionError(QAbstractSocket::SocketError)));
		normalOutput(QString("Connected client: %1, %2").arg(remote_ip).arg(port));
	}
	else
	{
		remote_ip="NA";
		port=9999;
	}
	connect(socket,	SIGNAL(disconnected()),				this, SLOT(disconnectClient()));
	this->socket			= socket;

	is_logged=false;
	is_ready_to_stop=false;
	ask_is_ready_to_stop=false;

	clientBroadCast->moveToThread(EventDispatcher::generalData.serverPrivateVariables.eventThreaderList.at(0));
	clientHeavyLoad->moveToThread(EventDispatcher::generalData.serverPrivateVariables.eventThreaderList.at(3));
	clientMapManagement->moveToThread(EventDispatcher::generalData.serverPrivateVariables.eventThreaderList.at(1));
	clientNetworkRead->moveToThread(EventDispatcher::generalData.serverPrivateVariables.eventThreaderList.at(2));
	clientLocalCalcule->moveToThread(EventDispatcher::generalData.serverPrivateVariables.eventThreaderList.at(6));

	//set variables
	clientBroadCast->setVariable(&player_informations);
	clientMapManagement->setVariable(&player_informations);
	clientHeavyLoad->setVariable(&player_informations);
	clientLocalCalcule->setVariable(&player_informations);

	//connect input/ouput
	connect(clientNetworkRead,	SIGNAL(newInputQuery(quint8,quint16,quint8)),	clientNetworkWrite,SLOT(newInputQuery(quint8,quint16,quint8)),Qt::QueuedConnection);
	connect(clientNetworkRead,	SIGNAL(newInputQuery(quint8,quint8)),		clientNetworkWrite,SLOT(newInputQuery(quint8,quint8)),Qt::QueuedConnection);
	connect(clientNetworkWrite,	SIGNAL(newOutputQuery(quint8,quint16,quint8)),	clientNetworkRead,SLOT(newOutputQuery(quint8,quint16,quint8)),Qt::QueuedConnection);
	connect(clientNetworkWrite,	SIGNAL(newOutputQuery(quint8,quint8)),		clientNetworkRead,SLOT(newOutputQuery(quint8,quint8)),Qt::QueuedConnection);

	//connect the write, to send packet on the network
	connect(clientNetworkRead,	SIGNAL(sendPacket(quint8,quint16,QByteArray)),clientNetworkWrite,SLOT(sendPacket(quint8,quint16,QByteArray)),Qt::QueuedConnection);
	connect(clientBroadCast,	SIGNAL(sendPacket(quint8,quint16,QByteArray)),clientNetworkWrite,SLOT(sendPacket(quint8,quint16,QByteArray)),Qt::QueuedConnection);
	connect(clientHeavyLoad,	SIGNAL(sendPacket(quint8,quint16,QByteArray)),clientNetworkWrite,SLOT(sendPacket(quint8,quint16,QByteArray)),Qt::QueuedConnection);
	connect(clientLocalCalcule,	SIGNAL(sendPacket(quint8,quint16,QByteArray)),clientNetworkWrite,SLOT(sendPacket(quint8,quint16,QByteArray)),Qt::QueuedConnection);

	connect(clientNetworkRead,	SIGNAL(sendPacket(quint8,QByteArray)),clientNetworkWrite,SLOT(sendPacket(quint8,QByteArray)),Qt::QueuedConnection);
	connect(clientBroadCast,	SIGNAL(sendPacket(quint8,QByteArray)),clientNetworkWrite,SLOT(sendPacket(quint8,QByteArray)),Qt::QueuedConnection);
	connect(clientHeavyLoad,	SIGNAL(sendPacket(quint8,QByteArray)),clientNetworkWrite,SLOT(sendPacket(quint8,QByteArray)),Qt::QueuedConnection);
	connect(clientLocalCalcule,	SIGNAL(sendPacket(quint8,QByteArray)),clientNetworkWrite,SLOT(sendPacket(quint8,QByteArray)),Qt::QueuedConnection);

	connect(clientNetworkRead,	SIGNAL(postReply(quint8,QByteArray)),clientNetworkWrite,SLOT(postReply(quint8,QByteArray)),Qt::QueuedConnection);
	connect(clientHeavyLoad,	SIGNAL(postReply(quint8,QByteArray)),clientNetworkWrite,SLOT(postReply(quint8,QByteArray)),Qt::QueuedConnection);

	//connect the player information
	connect(clientHeavyLoad,	SIGNAL(send_player_informations()),			clientBroadCast,	SLOT(send_player_informations()),Qt::QueuedConnection);
	connect(clientHeavyLoad,	SIGNAL(send_player_informations()),			clientNetworkRead,	SLOT(send_player_informations()),Qt::QueuedConnection);
	connect(clientHeavyLoad,	SIGNAL(put_on_the_map(quint32,Map_server*,quint16,quint16,Orientation,quint16)),	clientLocalCalcule,	SLOT(put_on_the_map(quint32,Map_server*,quint16,quint16,Orientation,quint16)),Qt::QueuedConnection);
	connect(clientHeavyLoad,	SIGNAL(send_player_informations()),			this,			SLOT(send_player_informations()),Qt::QueuedConnection);

	//packet parsed (heavy)
	connect(clientNetworkRead,SIGNAL(askLogin(quint8,QString,QByteArray)),
		clientHeavyLoad,SLOT(askLogin(quint8,QString,QByteArray)),Qt::QueuedConnection);
	connect(clientNetworkRead,SIGNAL(askRandomSeedList(quint8)),
		clientHeavyLoad,SLOT(askRandomSeedList(quint8)),Qt::QueuedConnection);
	connect(clientNetworkRead,SIGNAL(datapackList(quint8,QStringList,QList<quint32>)),
		clientHeavyLoad,SLOT(datapackList(quint8,QStringList,QList<quint32>)),Qt::QueuedConnection);

	//packet parsed (map management)
	connect(clientNetworkRead,	SIGNAL(moveThePlayer(quint8,Direction)),			clientLocalCalcule,	SLOT(moveThePlayer(quint8,Direction)),				Qt::QueuedConnection);
	//packet parsed (broadcast)
	connect(clientNetworkRead,	SIGNAL(sendChatText(Chat_type,QString)),			clientBroadCast,	SLOT(sendChatText(Chat_type,QString)),				Qt::QueuedConnection);
	connect(clientNetworkRead,	SIGNAL(sendPM(QString,QString)),				clientBroadCast,	SLOT(sendPM(QString,QString)),					Qt::QueuedConnection);
	connect(clientNetworkRead,	SIGNAL(sendChatText(Chat_type,QString)),			this,			SLOT(local_sendChatText(Chat_type,QString)),			Qt::QueuedConnection);
	connect(clientNetworkRead,	SIGNAL(sendPM(QString,QString)),				this,			SLOT(local_sendPM(QString,QString)),				Qt::QueuedConnection);
	connect(clientNetworkRead,	SIGNAL(addPlayersInformationToWatch(QList<quint32>,quint8)),	clientBroadCast,	SLOT(addPlayersInformationToWatch(QList<quint32>,quint8)),	Qt::QueuedConnection);
	connect(clientNetworkRead,	SIGNAL(sendBroadCastCommand(QString,QString)),			clientBroadCast,	SLOT(sendBroadCastCommand(QString,QString)),			Qt::QueuedConnection);
	connect(clientNetworkRead,	SIGNAL(removePlayersInformationToWatch(QList<quint32>)),	clientBroadCast,	SLOT(removePlayersInformationToWatch(QList<quint32>)),		Qt::QueuedConnection);
	connect(clientBroadCast,	SIGNAL(kicked()),						this,			SLOT(kicked()),							Qt::QueuedConnection);
	connect(clientNetworkRead,	SIGNAL(serverCommand(QString,QString)),				this,			SLOT(serverCommand(QString,QString)),			Qt::QueuedConnection);

	//connect the message
	connect(clientBroadCast,	SIGNAL(error(QString)),						this,	SLOT(errorOutput(QString)),Qt::QueuedConnection);
	connect(clientHeavyLoad,	SIGNAL(error(QString)),						this,	SLOT(errorOutput(QString)),Qt::QueuedConnection);
	connect(clientNetworkRead,	SIGNAL(error(QString)),						this,	SLOT(errorOutput(QString)),Qt::QueuedConnection);
	connect(clientNetworkWrite,	SIGNAL(error(QString)),						this,	SLOT(errorOutput(QString)),Qt::QueuedConnection);
	connect(clientLocalCalcule,	SIGNAL(error(QString)),						this,	SLOT(errorOutput(QString)),Qt::QueuedConnection);
	connect(clientBroadCast,	SIGNAL(message(QString)),					this,	SLOT(normalOutput(QString)),Qt::QueuedConnection);
	connect(clientHeavyLoad,	SIGNAL(message(QString)),					this,	SLOT(normalOutput(QString)),Qt::QueuedConnection);
	connect(clientNetworkRead,	SIGNAL(message(QString)),					this,	SLOT(normalOutput(QString)),Qt::QueuedConnection);
	connect(clientNetworkWrite,	SIGNAL(message(QString)),					this,	SLOT(normalOutput(QString)),Qt::QueuedConnection);
	connect(clientLocalCalcule,	SIGNAL(message(QString)),					this,	SLOT(normalOutput(QString)),Qt::QueuedConnection);

	stopped_object=0;
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
}

/// \brief new error at connexion
void Client::connectionError(QAbstractSocket::SocketError error)
{
	QTcpSocket *socket=qobject_cast<QTcpSocket *>(QObject::sender());
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
	if(ask_is_ready_to_stop)
		return;
	ask_is_ready_to_stop=true;
	if(is_ready_to_stop)
		return;
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
	clientNetworkRead->disconnect();
	clientBroadCast->disconnect();
	clientHeavyLoad->disconnect();
	clientMapManagement->disconnect();
	clientNetworkWrite->disconnect();

	//connect to save
	connect(clientMapManagement,SIGNAL(updatePlayerPosition(QString,quint16,quint16,Orientation)),
		clientHeavyLoad,SLOT(updatePlayerPosition(QString,quint16,quint16,Orientation)),Qt::QueuedConnection);

	//connect to quit
	connect(clientNetworkRead,	SIGNAL(isReadyToStop()),this,SLOT(disconnectNextStep()),Qt::QueuedConnection);
	connect(clientMapManagement,	SIGNAL(isReadyToStop()),this,SLOT(disconnectNextStep()),Qt::QueuedConnection);
	connect(clientBroadCast,	SIGNAL(isReadyToStop()),this,SLOT(disconnectNextStep()),Qt::QueuedConnection);
	connect(clientHeavyLoad,	SIGNAL(isReadyToStop()),this,SLOT(disconnectNextStep()),Qt::QueuedConnection);
	connect(clientNetworkWrite,	SIGNAL(isReadyToStop()),this,SLOT(disconnectNextStep()),Qt::QueuedConnection);
	connect(this,SIGNAL(askIfIsReadyToStop()),clientNetworkRead,SLOT(askIfIsReadyToStop()),Qt::QueuedConnection);
	connect(this,SIGNAL(askIfIsReadyToStop()),clientMapManagement,SLOT(askIfIsReadyToStop()),Qt::QueuedConnection);
	connect(this,SIGNAL(askIfIsReadyToStop()),clientBroadCast,SLOT(askIfIsReadyToStop()),Qt::QueuedConnection);
	connect(this,SIGNAL(askIfIsReadyToStop()),clientHeavyLoad,SLOT(askIfIsReadyToStop()),Qt::QueuedConnection);
	connect(this,SIGNAL(askIfIsReadyToStop()),clientNetworkWrite,SLOT(askIfIsReadyToStop()),Qt::QueuedConnection);
	emit askIfIsReadyToStop();

	emit player_is_disconnected(player_informations.public_and_private_informations.public_informations.pseudo);
}

void Client::disconnectNextStep()
{
	if(is_ready_to_stop)
		return;
	stopped_object++;
	if(stopped_object==5)
	{
		//remove the player
		EventDispatcher::generalData.serverPrivateVariables.connected_players--;
		EventDispatcher::generalData.serverPrivateVariables.player_updater.removeConnectedPlayer();
		is_logged=false;

		//reconnect to real stop
		clientNetworkRead->disconnect();
		clientBroadCast->disconnect();
		clientHeavyLoad->disconnect();
		clientMapManagement->disconnect();
		clientNetworkWrite->disconnect();
		connect(this,SIGNAL(askIfIsReadyToStop()),clientNetworkRead,SLOT(stop()),Qt::QueuedConnection);
		connect(this,SIGNAL(askIfIsReadyToStop()),clientMapManagement,SLOT(stop()),Qt::QueuedConnection);
		connect(this,SIGNAL(askIfIsReadyToStop()),clientBroadCast,SLOT(stop()),Qt::QueuedConnection);
		connect(this,SIGNAL(askIfIsReadyToStop()),clientHeavyLoad,SLOT(stop()),Qt::QueuedConnection);
		connect(this,SIGNAL(askIfIsReadyToStop()),clientNetworkWrite,SLOT(stop()),Qt::QueuedConnection);
		connect(this,SIGNAL(askIfIsReadyToStop()),clientLocalCalcule,SLOT(stop()),Qt::QueuedConnection);
		emit askIfIsReadyToStop();

		//now the object is not usable
		clientBroadCast=NULL;
		clientHeavyLoad=NULL;
		clientMapManagement=NULL;
		clientNetworkRead=NULL;
		clientNetworkWrite=NULL;

		emit isReadyToDelete();
	}

}

void Client::errorOutput(QString errorString)
{
	if(is_logged)
		clientBroadCast->sendSystemMessage(player_informations.public_and_private_informations.public_informations.pseudo+" have been kicked from server, have try hack");

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
	DebugClass::debugConsole(QString("%1:%2 %3").arg(remote_ip).arg(port).arg(message));
}

void Client::send_player_informations()
{
	#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
	normalOutput("load the normal player id: "+QString::number(player_informations.public_and_private_informations.public_informations.id));
	#endif
	emit new_player_is_connected(player_informations);
	this->player_informations=player_informations;
	this->id=player_informations.public_and_private_informations.public_informations.id;
	is_logged=true;
	EventDispatcher::generalData.serverPrivateVariables.connected_players++;
	EventDispatcher::generalData.serverPrivateVariables.player_updater.addConnectedPlayer();

	//remove the useless connection
	/*disconnect(clientHeavyLoad,	SIGNAL(send_player_informations()),			clientBroadCast,	SLOT(send_player_informations()));
	disconnect(clientHeavyLoad,	SIGNAL(send_player_informations()),			clientNetworkRead,	SLOT(send_player_informations()));
	disconnect(clientHeavyLoad,	SIGNAL(put_on_the_map(quint32,Map_final*,quint16,quint16,Orientation,quint16)),	clientMapManagement,	SLOT(put_on_the_map(quint32,Map_final*,quint16,quint16,Orientation,quint16)));*/
}

QString Client::getPseudo()
{
	return player_informations.public_and_private_informations.public_informations.pseudo;
}

void Client::serverCommand(QString command,QString extraText)
{
	//verified by previous code
	normalOutput(QString("command to do: %1 with args: %2").arg(command).arg(extraText));
	emit emit_serverCommand(command,extraText);
}

void Client::fake_receive_data(QByteArray data)
{
	emit fake_send_received_data(data);
}

void Client::local_sendPM(QString text,QString pseudo)
{
	emit new_chat_message(player_informations.public_and_private_informations.public_informations.pseudo,Chat_type_pm,QString("to: %1, %2").arg(pseudo).arg(text));
}

void Client::local_sendChatText(Chat_type chatType,QString text)
{
	emit new_chat_message(player_informations.public_and_private_informations.public_informations.pseudo,chatType,text);
}

Map_player_info Client::getMapPlayerInfo()
{
	Map_player_info temp=clientMapManagement->getMapPlayerInfo();
	temp.skin=player_informations.public_and_private_informations.public_informations.skin;
	return temp;
}
