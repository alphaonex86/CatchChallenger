#ifndef CATCHCHALLENGER_QTSERVER_H
#define CATCHCHALLENGER_QTSERVER_H

#include <QObject>
#include "BaseServer.h"

namespace CatchChallenger {
class QtServer : public BaseServer, public QObject
{
    Q_OBJECT
public:
    void preload_the_city_capture();
    void removeOneClient();
    void newConnection();
    void connect_the_last_client(Client * client);
    void load_next_city_capture();
    void send_insert_move_remove();
    void positionSync();
    void ddosTimer();
};
}

#endif // CATCHCHALLENGER_QTSERVER_H
