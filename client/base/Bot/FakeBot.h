#ifndef EPOLLCATCHCHALLENGERSERVER

#ifndef CATCHCHALLENGER_FakeBot_H
#define CATCHCHALLENGER_FakeBot_H

#include <QObject>
#include <QTimer>
#include <QSemaphore>
#include <QByteArray>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../../../general/base/MoveOnTheMap.h"
#include "../../../general/base/QFakeSocket.h"
#include "../../../general/base/ConnectedSocket.h"
#include "../Api_client_virtual.h"

namespace CatchChallenger {
class FakeBot : public QObject, public MoveOnTheMap
{
    Q_OBJECT
public:
    explicit FakeBot();
    ~FakeBot();
    void start_step();
    void show_details();
    uint64_t get_TX_size();
    uint64_t get_RX_size();
    QFakeSocket fakeSocket;
    ConnectedSocket socket;
private:
    Api_client_virtual api;
    //debug
    bool details;
    //virtual action
    void send_player_move(const uint8_t &moved_unit,const Direction &the_direction);

    static int index_loop,loop_size;
    static QSemaphore wait_to_stop;
    bool do_step;
    CommonMap *map;
    COORD_TYPE x,y;
    std::vector<Direction> predefinied_step;
public slots:
    void stop_step();
    void stop();
    void doStep();
private slots:
    void random_new_step();
    void insert_player(const CatchChallenger::Player_public_informations &player,const uint32_t &mapId,const uint16_t &x,const uint16_t &y,const CatchChallenger::Direction &direction);
    void have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations);
    void newError(std::string error, std::string detailedError);
    void newSocketError(QAbstractSocket::SocketError error);
    void disconnected();
    void tryLink();
signals:
    void isDisconnected();
};
}

#endif // FakeBot_H
#endif
