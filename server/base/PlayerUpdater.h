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
	void newConnectedPlayer(qint32 connected_players);
	void send_addConnectedPlayer();
	void send_removeConnectedPlayer();
public slots:
	void addConnectedPlayer();
	void removeConnectedPlayer();
private slots:
	void internal_addConnectedPlayer();
	void internal_removeConnectedPlayer();
	void send_timer();
private:
	quint16 connected_players,sended_connected_players;
	QTimer next_send_timer;
};
}

#endif // PLAYERUPDATER_H
