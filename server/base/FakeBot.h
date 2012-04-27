#ifndef FakeBot_H
#define FakeBot_H

#include <QObject>
#include <QTimer>
#include <QSemaphore>
#include <QByteArray>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../general/base/DebugClass.h"
#include "../general/base/GeneralStructures.h"
#include "../base/ServerStructures.h"
#include "../general/base/MoveClient.h"

class FakeBot : public QObject, public MoveClient
{
	Q_OBJECT
public:
	explicit FakeBot(const quint16 &x,const quint16 &y,Map_final *map,const Direction &last_direction);
	~FakeBot();
	void start_step();
	void show_details();
	quint64 get_TX_size();
	quint64 get_RX_size();
private:
	bool *walkable;
	quint16 width,height;
	quint16 x,y;
	bool details;
	void send_player_move(const quint8 &moved_unit,const Direction &the_direction);
	QByteArray calculate_player_move(const quint8 &moved_unit,const Direction &the_direction);
	QString directionToString(const Direction &direction);
	quint64 TX_size,RX_size;
	void raw_trame(const QByteArray &data);
	void newDirection(const Direction &the_direction);
	int index_loop,loop_size;
	QSemaphore wait_to_stop;
	bool do_step;
signals:
	void fake_receive_data(const QByteArray &data);
	void disconnected();
public slots:
	void fake_send_data(const QByteArray &data);
	void stop_step();
	void stop();
	void doStep();
private slots:
	void random_new_step();
};

#endif // FakeBot_H
