#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <QByteArray>
#include <QMutex>
#include <QMutexLocker>
#include <QSemaphore>
#include <QTimer>

#include "../general/base/DebugClass.h"
#include "ServerStructures.h"
#include "ClientBroadCast.h"
#include "ClientHeavyLoad.h"
#include "ClientMapManagement/ClientMapManagement.h"
#include "ClientNetworkRead.h"
#include "ClientNetworkWrite.h"
#include "ClientLocalCalcule.h"
#include "EventThreader.h"
#include "../general/base/GeneralStructures.h"
#include "../VariableServer.h"
#include "ClientMapManagement/MapVisibilityAlgorithm_Simple.h"
#include "ClientMapManagement/MapVisibilityAlgorithm_None.h"

#ifndef CLIENT_H
#define CLIENT_H

namespace Pokecraft {
class Client : public QObject
{
	Q_OBJECT
public:
	explicit Client(QAbstractSocket *socket,bool isFake);
	~Client();
	//to get some info
	QString getPseudo();
	Map_player_info getMapPlayerInfo();
	//to prevent remove before the right moment
	bool is_ready_to_stop;
	//do a fake login
	void setInternal();
private:
	bool ask_is_ready_to_stop;
	//-------------------
	Player_internal_informations player_informations;
	ClientBroadCast *clientBroadCast;
	ClientHeavyLoad *clientHeavyLoad;
	ClientMapManagement *clientMapManagement;
	ClientNetworkRead *clientNetworkRead;
	ClientNetworkWrite *clientNetworkWrite;
	ClientLocalCalcule *clientLocalCalcule;

	//socket related
	QAbstractSocket *socket;
	QString remote_ip;
	quint16 port;

	quint8 stopped_object;
private slots:
	//socket related
	void connectionError(QAbstractSocket::SocketError);
	//normal management related
	void errorOutput(QString errorString);
	void kicked();
	void normalOutput(QString message);
	//internal management related
	void send_player_informations();
	//remove and stop related
	void disconnectNextStep();

	//??? in private slot?
	void serverCommand(QString command,QString extraText);
	void local_sendPM(QString text,QString pseudo);
	void local_sendChatText(Chat_type chatType,QString text);
signals:
	//remove and stop related
	void askIfIsReadyToStop();
	void isReadyToDelete();

	//to pass to the event manager to display the information
	void new_player_is_connected(const Player_internal_informations &player);
	void player_is_disconnected(const QString &pseudo);
	void new_chat_message(const QString &pseudo,const Chat_type &type,const QString &text);

	//to async the message
	void send_fakeLogin(quint32 last_fake_player_id,quint16 x,quint16 y,Map_server_MapVisibility_simple *map,Orientation orientation,QString skin);
	void fake_send_data(const QByteArray &data);
	void fake_send_received_data(const QByteArray &data);
	void try_ask_stop();

	//to send to event manger to have command
	void emit_serverCommand(const QString&,const QString&);
public slots:
	void disconnectClient();
	/// \warning it need be complete protocol trame
	void fake_receive_data(QByteArray data);
};
}

#endif // CLIENT_H
