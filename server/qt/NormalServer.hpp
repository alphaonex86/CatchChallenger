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

#include "../base/ServerStructures.hpp"
#include "../base/Client.hpp"
#include "../../general/base/Map_loader.hpp"
#include "../../general/base/ProtocolParsing.hpp"
#include "../base/MapServer.hpp"
#include "QtServer.hpp"

namespace CatchChallenger {
class NormalServer : public QtServer
{
    #ifndef CATCHCHALLENGER_SERVER
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
    #ifndef CATCHCHALLENGER_SERVER
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
    //internal useful function
    std::string listenIpAndPort(std::string server_ip,uint16_t server_port);
    //store about the network. Plain QTcpServer — the in-band TLS
    //negotiation byte and the QSslServer wrapper that fronted it have
    //been removed; TLS now belongs to a separate listener (e.g. an
    //external reverse proxy).
    QTcpServer *sslServer;
    int number_of_client;
    //to check double instance
    //shared into all the program
    static bool oneInstanceRunning;
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
