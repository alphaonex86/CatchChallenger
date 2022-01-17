#ifndef CATCHCHALLENGER_QTSERVER_H
#define CATCHCHALLENGER_QTSERVER_H

#ifndef NOTHREADS
#include <QThread>
#else
#include <QObject>
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
signals:
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
private:
    void stop_internal_server_slot();
};

#endif // CATCHCHALLENGER_QTSERVER_H
