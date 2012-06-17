#include "FakeBot.h"

int FakeBot::index_loop;
int FakeBot::loop_size;
QSemaphore FakeBot::wait_to_stop;

/// \todo ask player information at the insert
FakeBot::FakeBot()
{
	connect(&api,SIGNAL(insert_player(quint32,QString,quint16,quint16,quint8,quint16)),this,SLOT(insert_player(quint32,QString,quint16,quint16,quint8,quint16)));
	api.sendProtocol();
	api.tryLogin("Fake","Fake");

	canLeaveTheMap=false;
	details=false;
	this->map=NULL;

	do_step=false;
	socket.connectToHost();
}

FakeBot::~FakeBot()
{
	wait_to_stop.acquire();
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
	if(map->parsed_layer.walkable[x-1+y*map->width] && (canLeaveTheMap || x>0))
		directions_allowed << Direction_move_at_left;
	if(map->parsed_layer.walkable[x+1+y*map->width] && (canLeaveTheMap || x<map->width))
		directions_allowed << Direction_move_at_right;
	if(map->parsed_layer.walkable[x+(y-1)*map->width] && (canLeaveTheMap || y>0))
		directions_allowed << Direction_move_at_top;
	if(map->parsed_layer.walkable[x+(y+1)*map->width] && (canLeaveTheMap || y<map->height))
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
	newDirection(final_direction);
}

void FakeBot::insert_player(quint32 id,QString mapName,quint16 x,quint16 y,quint8 direction,quint16 speed)
{
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
		this->last_direction=direction;
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
	emit disconnected();
}

void FakeBot::show_details()
{
	details=true;
	DebugClass::debugConsole(QString("FakeBot::show_details(), x: %1, y:%2").arg(x).arg(y));
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
		DebugClass::debugConsole(QString("FakeBot::newDirection(), after %3, x: %1, y:%2, last_step: %4").arg(x).arg(y).arg(MapBasicMove::directionToString(the_direction)).arg(last_step));
	if(details)
		DebugClass::debugConsole(QString("FakeBot::calculate_player_move(), moved_unit: %1, the_direction: %2").arg(moved_unit).arg(MapBasicMove::directionToString(the_direction)));
}

