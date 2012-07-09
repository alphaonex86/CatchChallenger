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
	connect(&api,SIGNAL(have_current_player_info(Player_private_and_public_informations,QString)),this,SLOT(have_current_player_info(Player_private_and_public_informations,QString)));
	connect(&api,SIGNAL(newError(QString,QString)),this,SLOT(newError(QString,QString)));

	details=false;
	map=NULL;

	do_step=false;
	socket.connectToHost(QHostAddress::LocalHost,9999);
	socket.connectToHostImplementation();

	x=0;
	y=0;
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
	//DebugClass::debugConsole(QString("FakeBot::doStep(), do_step: %1, socket.isValid():%2, map!=NULL: %3").arg(do_step).arg(socket.isValid()).arg(map!=NULL));
	if(do_step && socket.isValid() && map!=NULL)
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
	if(canGoTo(Direction_move_at_left,map,x,y))
		directions_allowed << Direction_move_at_left;
	if(canGoTo(Direction_move_at_right,map,x,y))
		directions_allowed << Direction_move_at_right;
	if(canGoTo(Direction_move_at_top,map,x,y))
		directions_allowed << Direction_move_at_top;
	if(canGoTo(Direction_move_at_bottom,map,x,y))
		directions_allowed << Direction_move_at_bottom;
	loop_size=directions_allowed.size();
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
	if(loop_size<=0)
		return;
	int random = rand()%loop_size;
	Direction final_direction=directions_allowed.at(random);
	//to group the signle move into move line
	MoveOnTheMap::newDirection(final_direction);
	//to do the real move
	move(final_direction,(Map **)&map,x,y);
}

//quint32,QString,quint16,quint16,quint8,quint16
void FakeBot::insert_player(quint32 id,QString mapName,quint16 x,quint16 y,quint8 direction,quint16 speed)
{
	DebugClass::debugConsole(QString("FakeBot::insert_player() id: %1, mapName: %2, api.getId(): %3").arg(id).arg(mapName).arg(api.getId()));
	Q_UNUSED(speed);
	if(id==api.getId())
	{
		if(details)
			DebugClass::debugConsole(QString("FakeBot::insert_player() register id: %1, mapName: %2").arg(id).arg(mapName));
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

void FakeBot::have_current_player_info(Player_private_and_public_informations info,QString pseudo)
{
	Q_UNUSED(info);
	DebugClass::debugConsole(QString("FakeBot::have_current_player_info() pseudo: %1").arg(pseudo));
}

void FakeBot::newError(QString error,QString detailedError)
{
	DebugClass::debugConsole(QString("FakeBot::newError() error: %1, detailedError: %2").arg(error).arg(detailedError));
	socket.disconnectFromHost();
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
