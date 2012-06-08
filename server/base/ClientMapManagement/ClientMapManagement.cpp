#include "ClientMapManagement.h"
#include "../EventDispatcher.h"

/** \todo do client near list for the local player
  the list is limited to 50
  if is greater, then truncate to have the near player, truncate to have all near player grouped by distance where a group not do the list greater
  each Xs update the local player list
*/
/** Never reserve the list, because it have square memory usage, and use more cpu */

//temp variable for purge buffer
quint16 ClientMapManagement::purgeBuffer_player_affected;
QByteArray ClientMapManagement::purgeBuffer_outputData;
QByteArray ClientMapManagement::purgeBuffer_outputDataLoop;
int ClientMapManagement::purgeBuffer_index;
int ClientMapManagement::purgeBuffer_list_size;
int ClientMapManagement::purgeBuffer_list_size_internal;
int ClientMapManagement::purgeBuffer_indexMovement;
map_management_move ClientMapManagement::purgeBuffer_move;
QHash<quint32, QList<map_management_movement> >::const_iterator ClientMapManagement::i_move;
QHash<quint32, QList<map_management_movement> >::const_iterator ClientMapManagement::i_move_end;
QHash<quint32, map_management_insert>::const_iterator ClientMapManagement::i_insert;
QHash<quint32, map_management_insert>::const_iterator ClientMapManagement::i_insert_end;
QSet<quint32>::const_iterator ClientMapManagement::i_remove;
QSet<quint32>::const_iterator ClientMapManagement::i_remove_end;

//temp variable to move on the map
map_management_movement ClientMapManagement::moveClient_tempMov;
map_management_insert ClientMapManagement::insertClient_temp;//can have lot of due to over move

ClientMapManagement::ClientMapManagement()
{
}

ClientMapManagement::~ClientMapManagement()
{
}

Map_player_info ClientMapManagement::getMapPlayerInfo()
{
	Map_player_info temp;
	temp.map		= current_map;
	temp.x			= x;
	temp.y			= y;
	return temp;
}

void ClientMapManagement::setVariable(Player_private_and_public_informations *player_informations)
{
	MapBasicMove::setVariable(player_informations);
	connect(&EventDispatcher::generalData.timer_to_send_insert_move_remove,	SIGNAL(timeout()),this,SLOT(purgeBuffer()),Qt::QueuedConnection);
}

void ClientMapManagement::extraStop()
{
	stopIt=true;

	//call MapVisibilityAlgorithm to remove
	removeClient();

	to_send_map_management_insert.clear();
	to_send_map_management_move.clear();
	to_send_map_management_remove.clear();

	//virtual stop the player
	if(last_direction>4)
		last_direction=Direction((quint8)last_direction-4);
	Orientation orientation=(Orientation)last_direction;
	if(current_map->map_file!=at_start_map_name || x!=at_start_x || y!=at_start_y || orientation!=at_start_orientation)
	{
		emit updatePlayerPosition(current_map->map_file,x,y,orientation);
		#ifdef DEBUG_MESSAGE_CLIENT_MOVE
		DebugClass::debugConsole(
					QString("current_map->map_file: %1,x: %2,y: %3, orientation: %4")
					.arg(current_map->map_file)
					.arg(x)
					.arg(y)
					.arg(orientation)
					);
		#endif
	}
}

void ClientMapManagement::put_on_the_map(const quint32 &player_id,Map_final *map,const quint16 &x,const quint16 &y,const Orientation &orientation,const quint16 &speed)
{
	//store the starting informations
	at_start_x=x;
	at_start_y=y;
	at_start_orientation=orientation;
	at_start_map_name=map->map_file;

	MapBasicMove::put_on_the_map(player_id,map,x,y,orientation,speed);

	//call MapVisibilityAlgorithm to insert
	insertClient();

	loadOnTheMap();
}

/// \note The second heavy function
void ClientMapManagement::moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction)
{
	mapHaveChanged=false;
	MapBasicMove::moveThePlayer(previousMovedUnit,direction);
	if(stopCurrentMethod)
		return;
	moveClient(previousMovedUnit,direction,mapHaveChanged);
}

void ClientMapManagement::loadOnTheMap()
{
	current_map->clients << this;
}

void ClientMapManagement::unloadFromTheMap()
{
	mapHaveChanged=true;
	current_map->clients.removeOne(this);
	mapVisiblity_unloadFromTheMap();
}

void ClientMapManagement::insertAnotherClient(const quint32 &player_id,const Map_final *map,const quint16 &x,const quint16 &y,const Direction &direction,const quint16 &speed)
{
	#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
	emit message(QString("insertAnotherClient(%1,%2,%3)").arg(player_id).arg(x).arg(y));
	#endif
	insertClient_temp.fileName=map->map_file;
	insertClient_temp.direction=direction;
	insertClient_temp.x=x;
	insertClient_temp.y=y;
	insertClient_temp.speed=speed;
	to_send_map_management_insert[player_id]=insertClient_temp;
}

/// \note The first heavy function
/// \todo put extra check to check if into remove
void ClientMapManagement::moveAnotherClient(const quint32 &player_id,const quint8 &movedUnit,const Direction &direction)
{
	#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
	emit message(QString("moveClient(%1,%2,%3)").arg(player_id).arg(movedUnit).arg(directionToString((Direction)direction)));
	#endif

	moveClient_tempMov.movedUnit=movedUnit;
	moveClient_tempMov.direction=direction;
	to_send_map_management_move[player_id] << moveClient_tempMov;
}

void ClientMapManagement::removeAnotherClient(const quint32 &player_id)
{
	#ifdef POKECRAFT_SERVER_EXTRA_CHECK
	if(!stopIt && to_send_map_management_remove.contains(player_id))
	{
		emit message("try dual remove");
		return;
	}
	#endif
	#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
	if(!stopIt)
		emit message(QString("removeAnotherClient(%1,%2,%3)").arg(player_id));
	#endif

	to_send_map_management_move.remove(player_id);
	to_send_map_management_remove << player_id;
}

void ClientMapManagement::dropAllClients()
{
	to_send_map_management_insert.clear();
	to_send_map_management_move.clear();
	to_send_map_management_remove.clear();
	emit sendPacket(0xC3,0x0000,false,QByteArray());
}

void ClientMapManagement::purgeBuffer()
{
	/// \todo suspend when is into fighting
	if(to_send_map_management_insert.size()==0 && to_send_map_management_move.size()==0 && to_send_map_management_remove.size()==0)
		return;
	if(stopIt)
		return;

	#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
	emit message("purgeBuffer() runing....");
	#endif

	purgeBuffer_player_affected=0;

	purgeBuffer_outputData.clear();
	QDataStream out(&purgeBuffer_outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	purgeBuffer_outputDataLoop.clear();
	QDataStream outLoop(&purgeBuffer_outputDataLoop, QIODevice::WriteOnly);
	outLoop.setVersion(QDataStream::Qt_4_4);

	//////////////////////////// insert //////////////////////////
	i_insert = to_send_map_management_insert.constBegin();
	i_insert_end = to_send_map_management_insert.constEnd();
	while (i_insert != i_insert_end)
	{
		#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
		emit message(
			QString("insert player_id: %1, mapName %2, x: %3, y: %4,direction: %5, for player: %6")
			.arg(i_insert.key())
			.arg(i_insert.value().fileName)
			.arg(i_insert.value().x)
			.arg(i_insert.value().y)
			.arg(directionToString(i_insert.value().direction))
			.arg(player_id)
			 );
		#endif
		outLoop << (quint8)0x01;
		outLoop << i_insert.key();
		outLoop << i_insert.value().fileName;
		outLoop << i_insert.value().x;
		outLoop << i_insert.value().y;
		outLoop << (quint8)i_insert.value().direction;
		outLoop << i_insert.value().speed;
		++purgeBuffer_player_affected;
		++i_insert;
	}
	to_send_map_management_insert.clear();

	//////////////////////////// move //////////////////////////
	purgeBuffer_list_size_internal=0;
	purgeBuffer_indexMovement=0;
	i_move = to_send_map_management_move.constBegin();
	i_move_end = to_send_map_management_move.constEnd();
	while (i_move != i_move_end)
	{
		#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
		emit message(
			QString("move player_id: %1, for player: %2")
			.arg(i_move.key())
			.arg(player_id)
			 );
		#endif
		outLoop << (quint8)0x02;
		outLoop << i_move.key();
		purgeBuffer_list_size_internal=i_move.value().size();
/*		if(purgeBuffer_list_size_internal==0)
			DebugClass::debugConsole(QString("for player: %1, list_size_internal==0").arg(this->player_id));*/
		outLoop << (quint8)purgeBuffer_list_size_internal;
		purgeBuffer_indexMovement=0;
		while(purgeBuffer_indexMovement<purgeBuffer_list_size_internal)
		{
			outLoop << i_move.value().at(purgeBuffer_indexMovement).movedUnit;
			outLoop << (quint8)i_move.value().at(purgeBuffer_indexMovement).direction;
			purgeBuffer_indexMovement++;
		}
		++purgeBuffer_player_affected;
		++i_move;
	}
	to_send_map_management_move.clear();

	//////////////////////////// remove //////////////////////////
	i_remove = to_send_map_management_remove.constBegin();
	i_remove_end = to_send_map_management_remove.constEnd();
	while (i_remove != i_remove_end)
	{
		#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
		emit message(
			QString("player_id to remove: %1, for player: %2")
			.arg((quint32)(*i_remove))
			.arg(player_id)
			 );
		#endif
		outLoop << (quint8)0x03;
		outLoop << (quint32)(*i_remove);
		++purgeBuffer_player_affected;
		++i_remove;
	}
	to_send_map_management_remove.clear();
	
	#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_SQUARE
	emit message(QString("player_affected: %1").arg(purgeBuffer_player_affected));
	#endif
	out << purgeBuffer_player_affected;
	purgeBuffer_outputData+=purgeBuffer_outputDataLoop;
	emit sendPacket(0xC0,0x0000,true,purgeBuffer_outputData);
}
