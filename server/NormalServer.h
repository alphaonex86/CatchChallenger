#ifndef CATCHCHALLENGER_EVENTDISPATCHER_H
#define CATCHCHALLENGER_EVENTDISPATCHER_H

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
#include <QSslKey>
#include <QSslCertificate>

#include "../general/base/DebugClass.h"
#include "base/ServerStructures.h"
#include "base/Client.h"
#include "../general/base/Map_loader.h"
#include "../general/base/ProtocolParsing.h"
#include "../general/base/QFakeServer.h"
#include "../general/base/QFakeSocket.h"
#include "base/MapServer.h"
#include "crafting/BaseServerCrafting.h"
#include "base/BaseServer.h"
#include "QSslServer.h"
#include "NormalServerGlobal.h"

namespace CatchChallenger {
class NormalServer : public BaseServer, public NormalServerGlobal
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
    quint16 player_current();
    quint16 player_max();
public:
    //to manipulate the server for restart and stop
    #ifndef EPOLLCATCHCHALLENGERSERVER
    bool isListen();
    bool isStopped();
    void start_server();
    void stop_server();
    #endif
    //todo
    /*void send_system_message(QString text);
    void send_pm_message(QString pseudo,QString text);*/
private:
    //to load/unload the content
    void load_settings();
    //init, constructor, destructor
    virtual void initAll();//call before all
    //internal usefull function
    QString listenIpAndPort(QString server_ip,quint16 server_port);
    //store about the network
    QSslServer *sslServer;
    int number_of_client;
    EventThreader * botThread;
    EventThreader * eventDispatcherThread;
    //to check double instance
    //shared into all the program
    static bool oneInstanceRunning;
    QSslCertificate *sslCertificate;
    QSslKey *sslKey;
    QHash<QHostAddress,QDateTime> kickedHosts;
    QTimer purgeKickedHostTimer;
    NormalServerSettings normalServerSettings;

    static QString text_restart;
    static QString text_stop;
private:
    //new connection
    #ifndef EPOLLCATCHCHALLENGERSERVER
    void newConnection();
    #endif
    void kicked(const QHostAddress &host);
    void purgeKickedHost();
    //remove all finished client
    void removeOneClient();
    //void removeOneBot();
    //parse general order from the client
    void serverCommand(const QString &command,const QString &extraText);
    //starting function
    #ifndef EPOLLCATCHCHALLENGERSERVER
    void stop_internal_server();
    bool check_if_now_stopped();
    void start_internal_server();
    void sslErrors(const QList<QSslError> &errors);
    #endif
    virtual void loadAndFixSettings();
#ifndef EPOLLCATCHCHALLENGERSERVER
signals:
    //async the call
    void need_be_stopped();
    void need_be_restarted();
    //stat player
    void new_player_is_connected(const Player_internal_informations &player);
    void player_is_disconnected(const QString &pseudo);
    void new_chat_message(const QString &pseudo,const Chat_type &type,const QString &text);
#endif
protected:
    virtual void parseJustLoadedMap(const Map_to_send &map_to_send,const QString &map_file);
};
}

#endif // EVENTDISPATCHER_H
