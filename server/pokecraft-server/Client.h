#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <QByteArray>
#include <QMutex>
#include <QMutexLocker>
#include <QSemaphore>
#include <QTimer>

#include "../pokecraft-general/DebugClass.h"
#include "ServerStructures.h"
#include "ClientBroadCast.h"
#include "ClientHeavyLoad.h"
#include "ClientMapManagement.h"
#include "ClientNetworkRead.h"
#include "ClientNetworkWrite.h"
#include "EventThreader.h"
#include "Map_custom.h"
#include "../pokecraft-general/GeneralStructures.h"
#include "../VariableServer.h"

#ifndef CLIENT_H
#define CLIENT_H

class Client : public QObject
{
	Q_OBJECT
public:
	struct GeneralData
	{
		//connection
		quint16 max_players;
		quint16 connected_players;
		QList<quint32> connected_players_id_list;
		bool instant_player_number;
		// files
		QStringList cached_files_name;
		QList<QByteArray> cached_files_data;
		QList<quint32> cached_files_mtime;
		//bd
		QSqlDatabase *db;//use pointer here to init in correct thread
		//instant variable
		qint64 cache_max_file_size;
		qint64 cache_max_size;
		qint64 cache_size;
		//general data
		QTimer *timer_update_number_connected;
		QList<EventThreader *> eventThreaderList;
		QList<Map_custom *> map_list;
		QTimer *timer_player_map;
		//interconnected thread
		QList<ClientBroadCast *> clientBroadCastList;
		QMutex clientBroadCastListMutex;
	};
	explicit Client(QTcpSocket *socket,GeneralData *generalData);
	~Client();
	quint32 id;
	void externalPacketSend(QByteArray data);
	void externalAddLookupPlayer(quint32 id);
	void externalRemoveLookupPlayer(quint32 id);
	QString getPseudo();
	bool is_ready_to_stop;
	void fakeLogin(quint32 last_fake_player_id,quint16 x,quint16 y,QString map,Orientation orientation,bool benchmark,QString skin);
	Map_player_info getMapPlayerInfo();
private:
	bool ask_is_ready_to_stop;
	QByteArray random_seed;
	QString code_map_layer;
	bool is_logged;
	//Map object pointer where you are registered
	QString map_name;
	//-------------------
	GeneralData *generalData;
	Player_private_and_public_informations player_informations;
	ClientBroadCast *clientBroadCast;
	ClientHeavyLoad *clientHeavyLoad;
	ClientMapManagement *clientMapManagement;
	ClientNetworkRead *clientNetworkRead;
	ClientNetworkWrite *clientNetworkWrite;
	enum disconnectNextStepValue
	{
		disconnectNextStepValue_before_stop_the_thread_1,
		disconnectNextStepValue_before_stop_the_thread_2,
		disconnectNextStepValue_before_stop_the_thread_3,
		disconnectNextStepValue_before_stop_the_thread_4,
		disconnectNextStepValue_before_stop_the_thread_5
	};
	disconnectNextStepValue disconnectStep;
	void connectToBroadCast();
	void disconnectToBroadCast();
	//socket related
	QTcpSocket *socket;
	QString remote_ip;
	quint16 port;
	bool stopIt,pass_in_destructor;
	QSemaphore wait_the_end;
private slots:
	void connectionError(QAbstractSocket::SocketError);
	void errorOutput(QString errorString);
	void kicked();
	void normalOutput(QString message);
	void send_player_informations(Player_private_and_public_informations player_informations);
	void disconnectNextStep();
	void serverCommand(QString command,QString extraText);
	void local_sendPM(QString text,QString pseudo);
	void local_sendChatText(Chat_type chatType,QString text);
	void initAll();
signals:
	void askIfIsReadyToStop();
	void isReadyToDelete();
	void updatePlayerNumber();
	void stop_server();
	void restart_server();
	void new_player_is_connected(Player_private_and_public_informations player);
	void player_is_disconnected(QString pseudo);
	void new_chat_message(QString pseudo,Chat_type type,QString text);
	void send_fakeLogin(quint32 last_fake_player_id,quint16 x,quint16 y,QString map,Orientation orientation,QString skin);
	void fake_send_data(QByteArray data);
	void fake_send_received_data(QByteArray data);
	void emit_serverCommand(QString,QString);
	void try_initAll();
public slots:
	void disconnectClient();
	void askUpdatePlayerNumber();
	/// \warning it need be complete protocol trame
	void fake_receive_data(QByteArray data);
};

#endif // CLIENT_H
