#ifndef CATCHCHALLENGER_PLAYERUPDATER_H
#define CATCHCHALLENGER_PLAYERUPDATER_H

#include <QObject>
#include <QTimer>

#ifdef EPOLLCATCHCHALLENGERSERVER
#include "../epoll/EpollTimer.h"
#endif

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
    void newConnectedPlayer(quint16 connected_players) const;
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
    static quint16 connected_players;
    static quint16 sended_connected_players;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    QTimer *next_send_timer;
    #else
    #endif
};
}

#endif // PLAYERUPDATER_H
