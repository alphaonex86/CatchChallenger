#ifndef CATCHCHALLENGER_QTSERVER_H
#define CATCHCHALLENGER_QTSERVER_H

#include <QObject>
#include "BaseServer.h"

namespace CatchChallenger {
class QtServer : public QObject, public BaseServer
{
    Q_OBJECT
public:
    QtServer();
    virtual ~QtServer();
    void preload_the_city_capture();
    void removeOneClient();
    void newConnection();
    void connect_the_last_client(Client * client, QIODevice *socket);
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
    void preload_finish();
    void quitForCriticalDatabaseQueryFailed();
signals:
    void try_initAll() const;
    void try_stop_server() const;
    void need_be_started() const;
    //stat
    void is_started(const bool &) const;
    void error(const std::string &error) const;

    void haveQuitForCriticalDatabaseQueryFailed();
private:
    std::unordered_set<Client *> client_list;
};
}

#endif // CATCHCHALLENGER_QTSERVER_H
