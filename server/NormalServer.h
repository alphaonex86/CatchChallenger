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

#include "../general/base/DebugClass.h"
#include "base/ServerStructures.h"
#include "base/Client.h"
#include "../general/base/Map_loader.h"
#include "base/Bot/FakeBot.h"
#include "../general/base/ProtocolParsing.h"
#include "../general/base/QFakeServer.h"
#include "../general/base/QFakeSocket.h"
#include "base/Map_server.h"
#include "base/BaseServer.h"

#ifndef POKECRAFT_EVENTDISPATCHER_H
#define POKECRAFT_EVENTDISPATCHER_H

namespace Pokecraft {
class NormalServer : public BaseServer
{
    Q_OBJECT
public:
    explicit NormalServer();
    virtual ~NormalServer();
    //stat function
    bool isInBenchmark();
    quint16 player_current();
    quint16 player_max();
    //stat function
    bool isListen();
    bool isStopped();
public slots:
    //to manipulate the server for restart and stop
    void start_server();
    void stop_server();
    void start_benchmark(quint16 second,quint16 number_of_client,bool benchmark_map);
    //todo
    /*void send_system_message(QString text);
    void send_pm_message(QString pseudo,QString text);*/
private:
    //to load/unload the content
    void load_settings();
    //init, constructor, destructor
    void initAll();//call before all
    //internal usefull function
    QString listenIpAndPort(QString server_ip,quint16 server_port);
    //store about the network
    QString server_ip;
    QTcpServer *server;
    //store benchmark related
    bool in_benchmark_mode;
    int benchmark_latency;
    QTimer *timer_benchmark_stop;
    QTime time_benchmark_first_client;
    EventThreader * botThread;
    EventThreader * eventDispatcherThread;
    //to check double instance
    //shared into all the program
    static bool oneInstanceRunning;

    //bot related
    void removeBots();
    void addBot();
    QTimer nextStep;//all function call singal sync, then not pointer needed
private slots:
    //new connection
    void newConnection();
    //remove all finished client
    void removeOneClient();
    void removeOneBot();
    //void removeOneBot();
    //parse general order from the client
    void serverCommand(const QString &command,const QString &extraText);
    //starting function
    void stop_internal_server();
    void stop_benchmark();
    void check_if_now_stopped();
    void start_internal_benchmark(quint16 second,quint16 number_of_client,const bool &benchmark_map);
    void start_internal_server();
signals:
    //async the call
    void need_be_stopped();
    void need_be_restarted();
    void try_start_benchmark(const quint16 &second,const quint16 &number_of_client,const bool &benchmark_map);
    //stat player
    void new_player_is_connected(const Player_internal_informations &player);
    void player_is_disconnected(const QString &pseudo);
    void new_chat_message(const QString &pseudo,const Chat_type &type,const QString &text);
    //benchmark
    void benchmark_result(const int &latency,const double &TX_speed,const double &RX_speed,const double &TX_size,const double &RX_size,const double &second);
protected:
    virtual void parseJustLoadedMap(const Map_to_send &map_to_send,const QString &map_file);
};
}

#endif // EVENTDISPATCHER_H
