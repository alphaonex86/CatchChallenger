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

#include "../pokecraft-general/DebugClass.h"
#include "ServerStructures.h"
#include "Client.h"
#include "Map_loader.h"
#include "FakeBot.h"
#include "LatencyChecker.h"

#ifndef EVENTDISPATCHER_H
#define EVENTDISPATCHER_H

class EventDispatcher : public QObject
{
	Q_OBJECT
public:
	explicit EventDispatcher();
	~EventDispatcher();
	//stat function
	bool isListen();
	bool isStopped();
	bool isInBenchmark();
	quint16 player_current();
	quint16 player_max();
	QStringList getLatency();
	quint32 getTotalLatency();

public slots:
	//to manipulate the server for restart and stop
	void start_server();
	void stop_server();
	void start_benchmark(quint16 second,quint16 number_of_client);
	//todo
	/*void send_system_message(QString text);
	void send_pm_message(QString pseudo,QString text);*/
private:
	void load_settings();
	void preload_the_data();
	void preload_the_map();
	void unload_the_data();
	void unload_the_map();
	QStringList listFolder(const QString& folder,const QString& suffix);
	QString server_ip;
	bool pvp;
	quint16 server_port;
	qreal rates_xp_normal;
	qreal rates_xp_premium;
	qreal rates_gold_normal;
	qreal rates_gold_premium;
	qreal rates_shiny_normal;
	qreal rates_shiny_premium;
	bool char_allow_all;
	bool char_allow_local;
	bool char_allow_private;
	bool char_allow_aliance;
	bool char_allow_clan;
	bool in_benchmark_mode;
	QTcpServer *server;
	/// \brief To lunch event only when the event loop is setup
	QTimer *lunchInitFunction;
	GeneralData generalData;
	QList<Client *> client_list;
	QString listenIpAndPort(QString server_ip,quint16 server_port);
	enum ServerStat
	{
		Down=0,
		InUp=1,
		Up=2,
		InDown=3
	};
	ServerStat stat;
	void connect_the_last_client();
	int benchmark_latency;
	QTimer *timer_benchmark_stop;
	QTime time_benchmark_first_client;
	LatencyChecker latencyChecker;
	bool *bool_Walkable;
	bool *bool_Water;
	QString mysql_host;
	QString mysql_db;
	QString mysql_login;
	QString mysql_pass;
	void removeBots();
	void addBot(quint16 x,quint16 y,Map_final *map,QString skin="");
	QTimer nextStep;//all function call singal sync, then not pointer needed
	QList<FakeBot *> fake_clients;
	struct Map_semi
	{
		//conversion x,y to position: x+y*width
		Map_final * map;
		Map_semi_border border;
	};
	bool stopIt;
	QSemaphore waitTheEnd;
private slots:
	//new connection
	void newConnection();
	//remove all finished client
	void removeOneClient();
	void removeOneBot();
	//parse general order from the client
	void serverCommand(QString command,QString extraText);
	//init, constructor, destructor
	void initAll();//call before all
	void destructor();//call after the server is closed
	//starting function
	void stop_internal_server();
	void stop_benchmark();
	void check_if_now_stopped();
	void start_internal_benchmark(quint16 second,quint16 number_of_client);
	void start_internal_server();
signals:
	void try_stop_server();
	void try_initAll();
	void need_be_stopped();
	void need_be_restarted();
	void need_be_started();
	void is_started(bool);
	void new_player_is_connected(const Player_private_and_public_informations &player);
	void player_is_disconnected(const QString &pseudo);
	void new_chat_message(const QString &pseudo,const Chat_type &type,const QString &text);
	void error(const QString &error);
	void benchmark_result(const int &latency,const double &TX_speed,const double &RX_speed,const double &TX_size,const double &RX_size,const double &second);
	void try_start_benchmark(const quint16 &second,const quint16 &number_of_client);
	void destroyAllBots();
	void call_destructor();
};

#endif // EVENTDISPATCHER_H
