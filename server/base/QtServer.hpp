#ifndef CATCHCHALLENGER_QTSERVER_H
#define CATCHCHALLENGER_QTSERVER_H

#include <QObject>
#include "BaseServer.hpp"

namespace CatchChallenger {
#ifndef NOTHREADS
class QtServer : public QThread, public BaseServer
#else
class QtServer : public QObject, public BaseServer
#endif
{
    Q_OBJECT
public:
    QtServer();
    virtual ~QtServer();
    void preload_the_city_capture();
    void removeOneClient();
    void newConnection();
    void connect_the_last_client(ClientWithSocket * client, QIODevice *socket);
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
    virtual void preload_finish();
    void quitForCriticalDatabaseQueryFailed();
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
    std::unordered_set<ClientWithSocket *> client_list;
private:
    void stop_internal_server_slot();
};
}

#endif // CATCHCHALLENGER_QTSERVER_H
