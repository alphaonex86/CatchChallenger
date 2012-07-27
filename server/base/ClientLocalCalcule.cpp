#include "ClientLocalCalcule.h"
#include "../../general/base/ProtocolParsing.h"
#include "EventDispatcher.h"

/** \todo do client near list for the local player
  the list is limited to 50
  if is greater, then truncate to have the near player, truncate to have all near player grouped by distance where a group not do the list greater
  each Xs update the local player list
*/
/** Never reserve the list, because it have square memory usage, and use more cpu */

QString ClientLocalCalcule::temp_direction;

ClientLocalCalcule::ClientLocalCalcule()
{
}

ClientLocalCalcule::~ClientLocalCalcule()
{
}

bool ClientLocalCalcule::checkCollision()
{
	if(!current_map->parsed_layer.walkable[x+y*current_map->width])
	{
		emit error(QString("move at %1, can't wall at: %2,%3 on map: %4").arg(temp_direction).arg(x).arg(y).arg(current_map->map_file));
		return false;
	}
	else
		return true;
}

void ClientLocalCalcule::getRandomNumberIfNeeded()
{
	if(player_informations->public_and_private_informations.public_informations.randomNumber.size()<=POKECRAFT_SERVER_MIN_RANDOM_LIST_SIZE)
		emit askRandomNumber();
}

void ClientLocalCalcule::extraStop()
{
	//virtual stop the player
	if(last_direction>4)
		last_direction=Direction((quint8)last_direction-4);
	Orientation orientation=(Orientation)last_direction;
	if(current_map!=at_start_map_name || x!=at_start_x || y!=at_start_y || orientation!=at_start_orientation)
	{
		#ifdef DEBUG_MESSAGE_CLIENT_MOVE
		DebugClass::debugConsole(
					QString("current_map->map_file: %1,x: %2,y: %3, orientation: %4")
					.arg(current_map->map_file)
					.arg(x)
					.arg(y)
					.arg(orientation)
					);
		#endif
		if(!player_informations->is_logged || player_informations->isFake)
			return;
		QSqlQuery updateMapPositionQuery=EventDispatcher::generalData.serverPrivateVariables.updateMapPositionQuery;
		updateMapPositionQuery.bindValue(":map_name",current_map->map_file);
		updateMapPositionQuery.bindValue(":position_x",x);
		updateMapPositionQuery.bindValue(":position_y",y);
		updateMapPositionQuery.bindValue(":orientation",orientation);
		updateMapPositionQuery.bindValue(":id",player_informations->id);
		if(!updateMapPositionQuery.exec())
			DebugClass::debugConsole(QString("Sql query failed: %1, error: %2").arg(updateMapPositionQuery.lastQuery()).arg(updateMapPositionQuery.lastError().text()));
	}
}

/* why do that's here?
 * Because the ClientMapManagement can be totaly satured by the square complexity
 * that's allow to continue the player to connect and play
 * the overhead for the network it just at the connexion */
void ClientLocalCalcule::put_on_the_map(Map_server *map,const COORD_TYPE &x,const COORD_TYPE &y,const Orientation &orientation)
{
	#ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
	emit message(QString("ClientLocalCalcule put_on_the_map(): map: %1, x: %2, y: %3").arg(map->map_file).arg(x).arg(y));
	#endif
	MapBasicMove::put_on_the_map(map,x,y,orientation);
	at_start_orientation=orientation;
	at_start_map_name=map;
	at_start_x=x;
	at_start_y=y;

	loadOnTheMap();

	//send to the client the position of the player
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);

	outputData[0]=0x01;
	outputData+=map->rawMapFile;
	out.device()->seek(out.device()->size());
	if(EventDispatcher::generalData.serverSettings.max_players<=255)
	{
		out << (quint8)0x01;
		out << (quint8)player_informations->public_and_private_informations.public_informations.simplifiedId;
	}
	else
	{
		out << (quint16)0x0001;
		out << (quint16)player_informations->public_and_private_informations.public_informations.simplifiedId;
	}
	out << x;
	out << y;
	out << quint8((quint8)orientation|(quint8)player_informations->public_and_private_informations.public_informations.type);
	out << player_informations->public_and_private_informations.public_informations.speed;
	out << player_informations->public_and_private_informations.public_informations.clan;

	outputData+=player_informations->rawPseudo;
	out.device()->seek(out.device()->pos()+player_informations->rawPseudo.size());
	outputData+=player_informations->rawSkin;
	out.device()->seek(out.device()->pos()+player_informations->rawSkin.size());

	emit sendPacket(0xC0,outputData);

	//load the first time the random number list
	getRandomNumberIfNeeded();
}

bool ClientLocalCalcule::moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction)
{
	temp_direction=MapBasicMove::directionToString(direction);
	return MapBasicMove::moveThePlayer(previousMovedUnit,direction);
}

void ClientLocalCalcule::newRandomNumber(const QByteArray &randomData)
{
	player_informations->public_and_private_informations.public_informations.randomNumber+=randomData;
}
