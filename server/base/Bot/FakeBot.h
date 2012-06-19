#include <QObject>
#include <QTimer>
#include <QSemaphore>
#include <QByteArray>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../general/base/MoveOnTheMap.h"
#include "MoveOnTheMap_Server.h"
#include "../general/base/QFakeSocket.h"
#include "../client/base/Api_client_virtual.h"

#ifndef FakeBot_H
#define FakeBot_H

class FakeBot : public QObject, public MoveOnTheMap_Server, public MoveOnTheMap
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
public slots:
	void stop_step();
	void stop();
	void doStep();
private slots:
	void random_new_step();
	void insert_player(quint32 id,QString mapName,quint16 x,quint16 y,Orientation direction,quint16 speed);
/*signals:
	void disconnected();*/
};

#endif // FakeBot_H
