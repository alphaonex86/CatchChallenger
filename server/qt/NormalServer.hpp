#ifndef CATCHCHALLENGER_EVENTDISPATCHER_H
#define CATCHCHALLENGER_EVENTDISPATCHER_H

#include <QObject>
#include <QSettings>
#include <QTcpServer>
#include <QTimer>
#include <QCoreApplication>
#include <vector>
#include <QSqlDatabase>
#include <QSqlError>
#include <QDir>
#include <QSslKey>
#include <QSslCertificate>

#include "../base/ServerStructures.hpp"
#include "../base/Client.hpp"
#include "../../general/base/Map_loader.hpp"
#include "../../general/base/ProtocolParsing.hpp"
#include "../base/MapServer.hpp"
#include "QtServer.hpp"
#include "QSslServer.hpp"

namespace CatchChallenger {
class NormalServer : public QtServer
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    Q_OBJECT
    #endif
public:
    explicit NormalServer();
    virtual ~NormalServer();
    void setNormalSettings(const NormalServerSettings &settings);
    NormalServerSettings getNormalSettings() const;
    //stat function
    uint16_t player_current();
    uint16_t player_max();
public:
    //to manipulate the server for restart and stop
    #ifndef EPOLLCATCHCHALLENGERSERVER
    bool isListen();
    bool isStopped();
    void start_server();
    void stop_server();
    #endif
    //todo
    /*void send_system_message(std::string text);
    void send_pm_message(std::string pseudo,std::string text);*/
private:
    //to load/unload the content
    void load_settings();
    //init, constructor, destructor
    virtual void initAll();//call before all
    //internal usefull function
    std::string listenIpAndPort(std::string server_ip,uint16_t server_port);
    //store about the network
    QSslServer *sslServer;
    int number_of_client;
    //EventThreader * botThread;
    //EventThreader * eventDispatcherThread;
    //to check double instance
    //shared into all the program
    static bool oneInstanceRunning;
    QSslCertificate *sslCertificate;
    QSslKey *sslKey;
    QHash<QHostAddress,QDateTime> kickedHosts;
    QTimer purgeKickedHostTimer;
    QTimer timeRangeEventTimer;
    NormalServerSettings normalServerSettings;

    static std::string text_restart;
    static std::string text_stop;
private:
    //new connection
    void newConnection();
    void kicked(const QHostAddress &host);
    void purgeKickedHost();
    void timeRangeEvent();
    //remove all finished client
    void removeOneClient();
    //void removeOneBot();
    //parse general order from the client
    void serverCommand(const std::string &command,const std::string &extraText);
    //starting function
    void stop_internal_server();
    bool check_if_now_stopped();
    virtual void start_internal_server();
    void sslErrors(const QList<QSslError> &errors);
    virtual void loadAndFixSettings();
    void preload_finish();
signals:
    //async the call
    void need_be_stopped();
    void need_be_restarted();
    //stat player
    void new_player_is_connected(const Player_private_and_public_informations &player);
    void player_is_disconnected(const std::string &pseudo);
    void new_chat_message(const std::string &pseudo,const Chat_type &type,const std::string &text);
};
}

#endif // EVENTDISPATCHER_H
