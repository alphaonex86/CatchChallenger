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
	bool isListen();
	bool isStopped();
	bool isInBenchmark();
	quint16 player_current();
	quint16 player_max();
	qint64 cache_current();
	qint64 cache_max();
	QStringList getLatency();
	quint32 getTotalLatency();
	void start_benchmark(quint16 second,quint16 number_of_client);
public slots:
	//to manipulate the server for restart and stop
	void start_server();
	void stop_server();
	void load_settings();
	//todo
	/*void send_system_message(QString text);
	void send_pm_message(QString pseudo,QString text);*/
private:
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
	Client::GeneralData generalData;
	QList<Client *> client_list;
	QString listenIpAndPort(QString server_ip,quint16 server_port);
	void internal_stop();
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
	void addBot(quint16 x,quint16 y,bool *bool_Walkable,quint16 width,quint16 height,bool benchmark,QString map,QString skin="");
	QTimer nextStep;//all function call singal sync, then not pointer needed
	QList<FakeBot *> fake_clients;
private slots:
	void newConnection();
	void removeOneClient();
	void stop_benchmark();
	void start_internal_benchmark(quint16 second,quint16 number_of_client);
	void start_internal_server();
	void serverCommand(QString command,QString extraText);
	void initAll();
signals:
	void try_initAll();
	void need_be_stopped();
	void need_be_restarted();
	void need_be_started();
	void is_started(bool);
	void new_player_is_connected(Player_private_and_public_informations player);
	void player_is_disconnected(QString pseudo);
	void new_chat_message(QString pseudo,Chat_type type,QString text);
	void error(QString error);
	void benchmark_result(int latency,double TX_speed,double RX_speed,double TX_size,double RX_size,double second);
	void try_start_benchmark(quint16 second,quint16 number_of_client);
	void destroyAllBots();
};

#endif // EVENTDISPATCHER_H
