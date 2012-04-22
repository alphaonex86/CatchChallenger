#include "FakeBot.h"

FakeBot::FakeBot(const quint16 &x,const quint16 &y,Map_final *map,const Direction &last_direction)
{
	details=false;
	this->x=x;
	this->y=y;
	this->walkable=map->parsed_layer.walkable;
	this->width=map->width;
	this->height=map->height;
	this->last_direction=last_direction;
	TX_size=0;
	RX_size=0;
	do_step=false;
}

FakeBot::~FakeBot()
{
	wait_to_stop.acquire();
}

void FakeBot::doStep()
{
	if(do_step)
		random_new_step();
}

void FakeBot::start_step()
{
	do_step=true;
}

void FakeBot::random_new_step()
{
	QList<Direction> directions_allowed;
	if(walkable[x-1+y*width] && x>0)
		directions_allowed << Direction_move_at_left;
	if(walkable[x+1+y*width] && x<width)
		directions_allowed << Direction_move_at_right;
	if(walkable[x+(y-1)*width] && y>0)
		directions_allowed << Direction_move_at_top;
	if(walkable[x+(y+1)*width] && y<height)
		directions_allowed << Direction_move_at_bottom;
	loop_size=directions_allowed.size();
	if(loop_size<=0)
		return;
	if(details)
	{
		QStringList directions_allowed_string;
		index_loop=0;
		while(index_loop<loop_size)
		{
			directions_allowed_string << directionToString(directions_allowed.at(index_loop));
			index_loop++;
		}
		DebugClass::debugConsole(QString("FakeBot::random_new_step(), x: %1, y:%2, directions_allowed_string: %3").arg(x).arg(y).arg(directions_allowed_string.join(", ")));
	}
	int random = rand()%loop_size;
	Direction final_direction=directions_allowed.at(random);
	newDirection(final_direction);
}

void FakeBot::stop_step()
{
	do_step=false;
}

void FakeBot::stop()
{
	stop_step();
	emit disconnected();
	wait_to_stop.release();
}

void FakeBot::show_details()
{
	details=true;
	DebugClass::debugConsole(QString("FakeBot::show_details(), x: %1, y:%2").arg(x).arg(y));
}

void FakeBot::fake_send_data(const QByteArray &data)
{
	RX_size+=data.size();
}

void FakeBot::newDirection(const Direction &the_direction)
{
	MoveClient::newDirection(the_direction);
	switch(the_direction)
	{
		case Direction_move_at_top:
			y-=1;
		break;
		case Direction_move_at_right:
			x+=1;
		break;
		case Direction_move_at_bottom:
			y+=1;
		break;
		case Direction_move_at_left:
			x-=1;
		break;
		default:
		break;
	}
	if(details)
		DebugClass::debugConsole(QString("FakeBot::newDirection(), after %3, x: %1, y:%2, last_step: %4").arg(x).arg(y).arg(directionToString(the_direction)).arg(last_step));
}

QByteArray FakeBot::calculate_player_move(const quint8 &moved_unit,const Direction &the_direction)
{
	if(details)
		DebugClass::debugConsole(QString("FakeBot::calculate_player_move(), moved_unit: %1, the_direction: %2").arg(moved_unit).arg(directionToString(the_direction)));
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	out << (quint8)0x40;
	out << moved_unit;
	out << (quint8)the_direction;
	return outputData;
}

void FakeBot::send_player_move(const quint8 &moved_unit,const Direction &the_direction)
{
	raw_trame(calculate_player_move(moved_unit,the_direction));
}

void FakeBot::raw_trame(const QByteArray &data)
{
	TX_size+=data.size();
	emit fake_receive_data(data);
}

quint64 FakeBot::get_TX_size()
{
	return TX_size;
}

quint64 FakeBot::get_RX_size()
{
	return RX_size;
}

QString FakeBot::directionToString(const Direction &direction)
{
	switch(direction)
	{
		case Direction_look_at_top:
			return "look at top";
		break;
		case Direction_look_at_right:
			return "look at right";
		break;
		case Direction_look_at_bottom:
			return "look at bottom";
		break;
		case Direction_look_at_left:
			return "look at left";
		break;
		case Direction_move_at_top:
			return "move at top";
		break;
		case Direction_move_at_right:
			return "move at right";
		break;
		case Direction_move_at_bottom:
			return "move at bottom";
		break;
		case Direction_move_at_left:
			return "move at left";
		break;
		default:
		break;
	}
	return "???";
}
