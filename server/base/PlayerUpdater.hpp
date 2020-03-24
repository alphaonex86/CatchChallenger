#ifndef CATCHCHALLENGER_PLAYERUPDATER_H
#define CATCHCHALLENGER_PLAYERUPDATER_H

#ifdef EPOLLCATCHCHALLENGERSERVER
#include "../epoll/EpollTimer.hpp"
#else
#include <QObject>
#include <QTimer>
#endif

#include <stdint.h>

namespace CatchChallenger {
class PlayerUpdater
        #ifndef EPOLLCATCHCHALLENGERSERVER
        : public QObject
        #else
        : public EpollTimer
        #endif
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    Q_OBJECT
    #else
    friend class PlayerUpdaterToMaster;
    #endif
public:
    explicit PlayerUpdater();
#ifndef EPOLLCATCHCHALLENGERSERVER
signals:
    void newConnectedPlayer(uint16_t connected_players) const;
    void send_addConnectedPlayer() const;
    void send_removeConnectedPlayer() const;
    void try_initAll() const;
#endif
public:
    void addConnectedPlayer();
    void removeConnectedPlayer();
private:
    void internal_addConnectedPlayer();
    void internal_removeConnectedPlayer();
    void exec();
    void initAll();
private:
    static uint16_t connected_players;
    static uint16_t sended_connected_players;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    QTimer *next_send_timer;
    #endif
};
}

#endif // PLAYERUPDATER_H
