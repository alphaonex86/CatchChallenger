#include "FakeBot.h"
#include "../EventDispatcher.h"
#include "../ClientMapManagement/MapBasicMove.h"

int FakeBot::index_loop;
int FakeBot::loop_size;
QSemaphore FakeBot::wait_to_stop;

/// \todo ask player information at the insert
FakeBot::FakeBot() :
	api(&socket)
{
	connect(&api,SIGNAL(insert_player(quint32,QString,quint16,quint16,quint8,quint16)),this,SLOT(insert_player(quint32,QString,quint16,quint16,quint8,quint16)));

	details=false;

	do_step=false;
	socket.connectToHost();
}

FakeBot::~FakeBot()
{
	wait_to_stop.acquire();
}

void FakeBot::tryLink()
{
	api.sendProtocol();
	api.tryLogin("Fake","Fake");
}

void FakeBot::doStep()
{
	if(do_step && map!=NULL)
	{
		random_new_step();
/*		if(rand()%(EventDispatcher::generalData.botNumber*10)==0)
			api.sendChatText(Chat_type_local,"Hello world!");*/
	}
}

void FakeBot::start_step()
{
	do_step=true;
}

void FakeBot::random_new_step()
{
	QList<Direction> directions_allowed;
	if(canGoTo(Direction_move_at_left))
		directions_allowed << Direction_move_at_left;
	if(canGoTo(Direction_move_at_right))
		directions_allowed << Direction_move_at_right;
	if(canGoTo(Direction_move_at_top))
		directions_allowed << Direction_move_at_top;
	if(canGoTo(Direction_move_at_bottom))
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
			directions_allowed_string << MapBasicMove::directionToString(directions_allowed.at(index_loop));
			index_loop++;
		}
		DebugClass::debugConsole(QString("FakeBot::random_new_step(), x: %1, y:%2, directions_allowed_string: %3").arg(x).arg(y).arg(directions_allowed_string.join(", ")));
	}
	int random = rand()%loop_size;
	Direction final_direction=directions_allowed.at(random);
	//to group the signle move into move line
	MoveOnTheMap::newDirection(final_direction);
	//to do the real move
	move(final_direction);
}

void FakeBot::insert_player(quint32 id,QString mapName,quint16 x,quint16 y,Orientation direction,quint16 speed)
{
	Q_UNUSED(speed);
	if(id==api.getId())
	{
		if(!EventDispatcher::generalData.serverPrivateVariables.map_list.contains(mapName))
		{
			DebugClass::debugConsole(QString("FakeBot::insert_player(), map not found: %1").arg(mapName));
			return;
		}
		if(x>=EventDispatcher::generalData.serverPrivateVariables.map_list[mapName]->width)
		{
			DebugClass::debugConsole(QString("FakeBot::insert_player(), x>=EventDispatcher::generalData.serverPrivateVariables.map_list[mapName]->width: %1>=%2").arg(x).arg(EventDispatcher::generalData.serverPrivateVariables.map_list[mapName]->width));
			return;
		}
		if(y>=EventDispatcher::generalData.serverPrivateVariables.map_list[mapName]->height)
		{
			DebugClass::debugConsole(QString("FakeBot::insert_player(), x>=EventDispatcher::generalData.serverPrivateVariables.map_list[mapName]->width: %1>=%2").arg(y).arg(EventDispatcher::generalData.serverPrivateVariables.map_list[mapName]->height));
			return;
		}
		this->map=EventDispatcher::generalData.serverPrivateVariables.map_list[mapName];
		this->x=x;
		this->y=y;
		this->last_direction=(Direction)direction;
	}
}

void FakeBot::stop_step()
{
	do_step=false;
}

void FakeBot::stop()
{
	stop_step();
	wait_to_stop.release();
//	emit disconnected();
}

void FakeBot::show_details()
{
	details=true;
	DebugClass::debugConsole(QString("FakeBot::show_details(), x: %1, y:%2").arg(x).arg(y));
}

quint64 FakeBot::get_TX_size()
{
	return socket.getTXSize();
}

quint64 FakeBot::get_RX_size()
{
	return socket.getRXSize();
}

void FakeBot::send_player_move(const quint8 &moved_unit,const Direction &the_direction)
{
	api.send_player_move(moved_unit,the_direction);
}
