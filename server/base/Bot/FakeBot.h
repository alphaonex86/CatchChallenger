#include <QObject>
#include <QTimer>
#include <QSemaphore>
#include <QByteArray>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../general/base/MoveOnTheMap.h"
#include "../general/base/QFakeSocket.h"
#include "../Map_server.h"
#include "../client/base/Api_client_virtual.h"

#ifndef POKECRAFT_FakeBot_H
#define POKECRAFT_FakeBot_H

namespace Pokecraft {
class FakeBot : public QObject, public MoveOnTheMap
{
	Q_OBJECT
public:
	explicit FakeBot();
	~FakeBot();
	void start_step();
	void show_details();
	quint64 get_TX_size();
	quint64 get_RX_size();
	QFakeSocket socket;
	void tryLink();
private:
	Api_client_virtual api;
	//debug
	bool details;
	//virtual action
	void send_player_move(const quint8 &moved_unit,const Direction &the_direction);

	static int index_loop,loop_size;
	static QSemaphore wait_to_stop;
	bool do_step;
	Map *map;
	COORD_TYPE x,y;
	QList<Direction> predefinied_step;
public slots:
	void stop_step();
	void stop();
	void doStep();
private slots:
	void random_new_step();
	void insert_player(Player_public_informations player,QString mapName,quint16 x,quint16 y,Direction direction);
	void have_current_player_info(Player_private_and_public_informations info,QString pseudo);
	void newError(QString error,QString detailedError);
	void newSocketError(QAbstractSocket::SocketError error);
	void disconnected();
};
}

#endif // FakeBot_H
