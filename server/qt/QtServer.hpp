#ifndef CATCHCHALLENGER_QTSERVER_H
#define CATCHCHALLENGER_QTSERVER_H

#ifndef NOTHREADS
#include <QThread>
#else
#include <QObject>
#endif
#if ! defined(EPOLLCATCHCHALLENGERSERVER) && ! defined (ONLYMAPRENDER) && defined(CATCHCHALLENGER_SOLO)
#include <QTcpServer>
#include <QUdpSocket>
#include <QTimer>
#endif
#include "../base/BaseServer.hpp"
#include "QtServerStructures.hpp"
#include "QtClient.hpp"

#ifndef NOTHREADS
class QtServer : public QThread, public CatchChallenger::BaseServer
#else
class QtServer : public QObject, public CatchChallenger::BaseServer
#endif
{
    Q_OBJECT
public:
    QtServer();
    static QtServerPrivateVariables qtServerPrivateVariables;
    virtual ~QtServer();
    void preload_the_city_capture();
    void removeOneClient();
    void newConnection();
    void connect_the_last_client(CatchChallenger::Client * client, QIODevice *socket);
    void load_next_city_capture();
    void send_insert_move_remove();
    void positionSync();
    void ddosTimer();
    virtual void start();
    bool isListen();
    bool isStopped();
    void stop();
    bool check_if_now_stopped();//return true if can be stopped
    void stop_internal_server();
    virtual void preload_finish() override;
    void quitForCriticalDatabaseQueryFailed() override;
    void loadAndFixSettings() override;

    void setEventTimer(const uint8_t &event,const uint8_t &value,const unsigned int &time,const unsigned int &start) override;
    void preload_the_visibility_algorithm() override;
    void unload_the_visibility_algorithm() override;
    void unload_the_events() override;
    #if ! defined(EPOLLCATCHCHALLENGERSERVER) && ! defined (ONLYMAPRENDER) && defined(CATCHCHALLENGER_SOLO)
    bool openToLan(QString name,bool allowInternet=true);//for now internet filter not implemented
    #endif
signals:
    #if ! defined(EPOLLCATCHCHALLENGERSERVER) && ! defined (ONLYMAPRENDER) && defined(CATCHCHALLENGER_SOLO)
    void emitLanPort(uint16_t port);
    #endif
    void try_initAll() const;
    void try_stop_server() const;
    void need_be_started() const;
    //stat
    void is_started(const bool &) const;
    void error(const std::string &error) const;
    void stop_internal_server_signal();

    void haveQuitForCriticalDatabaseQueryFailed();
private:
    std::unordered_set<CatchChallenger::Client *> client_list;
    #if ! defined(EPOLLCATCHCHALLENGERSERVER) && ! defined (ONLYMAPRENDER) && defined(CATCHCHALLENGER_SOLO)
    QTcpServer server;
    QUdpSocket broadcastLan;
    QTimer broadcastLanTimer;
    QByteArray dataToSend;
    void sendBroadcastServer();
    #endif
private:
    void stop_internal_server_slot();
};

#endif // CATCHCHALLENGER_QTSERVER_H
