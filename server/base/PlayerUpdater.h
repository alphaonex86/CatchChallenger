#ifndef CATCHCHALLENGER_PLAYERUPDATER_H
#define CATCHCHALLENGER_PLAYERUPDATER_H

#include <QObject>
#include <QTimer>

namespace CatchChallenger {
class PlayerUpdater
        #ifndef EPOLLCATCHCHALLENGERSERVER
        : public QObject
        #endif
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    Q_OBJECT
    #endif
public:
    explicit PlayerUpdater();
#ifndef EPOLLCATCHCHALLENGERSERVER
signals:
#else
public:
#endif
    void newConnectedPlayer(quint16 connected_players) const;
    void send_addConnectedPlayer() const;
    void send_removeConnectedPlayer() const;
    void try_initAll() const;
public slots:
    void addConnectedPlayer();
    void removeConnectedPlayer();
private slots:
    void internal_addConnectedPlayer();
    void internal_removeConnectedPlayer();
    void send_timer();
    void initAll();
private:
    quint16 connected_players,sended_connected_players;
    QTimer *next_send_timer;
};
}

#endif // PLAYERUPDATER_H
