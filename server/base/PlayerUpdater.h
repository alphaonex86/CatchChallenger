#ifndef CATCHCHALLENGER_PLAYERUPDATER_H
#define CATCHCHALLENGER_PLAYERUPDATER_H

#include <QObject>
#include <QTimer>

namespace CatchChallenger {
class PlayerUpdater : public QObject
{
    Q_OBJECT
public:
    explicit PlayerUpdater();
signals:
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
