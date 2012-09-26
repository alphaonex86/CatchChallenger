#include <QObject>
#include <QSettings>
#include <QTcpServer>
#include <QDebug>
#include <QTimer>
#include <QCoreApplication>
#include <QList>
#include <QByteArray>
#include <QSqlDatabase>
#include <QSqlError>
#include <QDir>
#include <QSemaphore>

#include "../../general/base/DebugClass.h"
#include "../../server/base/ServerStructures.h"
#include "../../server/base/Client.h"
#include "../../general/base/Map_loader.h"
#include "../../server/base/Bot/FakeBot.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/QFakeServer.h"
#include "../../general/base/QFakeSocket.h"
#include "../../server/base/Map_server.h"

#ifndef POKECRAFT_INTERNALSERVER_H
#define POKECRAFT_INTERNALSERVER_H

namespace Pokecraft {
class InternalServer : public QObject
{
	Q_OBJECT
public:
	explicit InternalServer();
	~InternalServer();
    void setSettings(ServerSettings settings);
	//stat function
	bool isListen();
	bool isStopped();
	bool isInBenchmark();
	quint16 player_current();
    quint16 player_max();
public slots:
	//to manipulate the server for restart and stop
	void start_server();
    void stop_server();
	//todo
	/*void send_system_message(QString text);
	void send_pm_message(QString pseudo,QString text);*/
private:
	//to load/unload the content
	struct Map_semi
	{
		//conversion x,y to position: x+y*width
		Map* map;
		Map_semi_border border;
		Map_to_send old_map;
    };
	void preload_the_data();
	void preload_the_map();
	void preload_the_visibility_algorithm();
	void unload_the_data();
	void unload_the_map();
	void unload_the_visibility_algorithm();
	//internal usefull function
    QStringList listFolder(const QString& folder,const QString& suffix);
	QString listenIpAndPort(QString server_ip,quint16 server_port);
	void connect_the_last_client();
	//to check double instance
	//shared into all the program
	static bool oneInstanceRunning;

	/// \brief To lunch event only when the event loop is setup
	QTimer *lunchInitFunction;

	//to keep client list
	QList<Client *> client_list;

	//stat
	enum ServerStat
	{
		Down=0,
		InUp=1,
		Up=2,
		InDown=3
	};
	ServerStat stat;

    bool initialize_the_database();
    QFakeServer server;
private slots:
	//new connection
	void newConnection();
	//remove all finished client
	void removeOneClient();
	//init, constructor, destructor
	void initAll();//call before all
	//starting function
	void stop_internal_server();
	void check_if_now_stopped();
	void start_internal_server();
signals:
	void error(const QString &error);
    void try_initAll();
    void need_be_started();
    void try_stop_server();
    void is_started(bool);
};
}

#endif
