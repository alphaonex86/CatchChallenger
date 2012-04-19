#ifndef FakeBot_H
#define FakeBot_H

#include <QObject>
#include <QTimer>
#include <QSemaphore>
#include <QByteArray>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../pokecraft-general/DebugClass.h"
#include "Map_custom.h"
#include "../pokecraft-general/GeneralStructures.h"
#include "../pokecraft-general/MoveClient.h"

class FakeBot : public QObject, public MoveClient
{
	Q_OBJECT
public:
	explicit FakeBot(quint16 x,quint16 y,bool *bool_Walkable,quint16 width,quint16 height,Direction last_direction);
	~FakeBot();
	void start_step();
	void show_details();
	quint64 get_TX_size();
	quint64 get_RX_size();
private:
	bool *bool_Walkable;
	quint16 x,y,width,height;
	bool details;
	void send_player_move(quint8 moved_unit,Direction the_direction);
	QByteArray calculate_player_move(quint8 moved_unit,Direction the_direction);
	QString directionToString(Direction direction);
	quint64 TX_size,RX_size;
	void raw_trame(QByteArray data);
	void newDirection(Direction the_direction);
	int index_loop,loop_size;
	QSemaphore wait_to_stop;
	bool do_step;
signals:
	void fake_receive_data(QByteArray data);
	void disconnected();
public slots:
	void fake_send_data(QByteArray data);
	void stop_step();
	void stop();
	void doStep();
private slots:
	void random_new_step();
};

#endif // FakeBot_H
