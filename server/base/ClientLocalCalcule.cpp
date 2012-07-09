#include "ClientLocalCalcule.h"

/** \todo do client near list for the local player
  the list is limited to 50
  if is greater, then truncate to have the near player, truncate to have all near player grouped by distance where a group not do the list greater
  each Xs update the local player list
*/
/** Never reserve the list, because it have square memory usage, and use more cpu */

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
		emit error(QString("move at top, can't wall at: %1,%2 on map: %3").arg(x).arg(y).arg(current_map->map_file));
		return false;
	}
	else
		return true;
}

/* why do that's here?
 * Because the ClientMapManagement can be totaly satured by the square complexity
 * that's allow to continue the player to connect and play
 * the overhead for the network it just at the connexion */
void ClientLocalCalcule::put_on_the_map(Map_server *map,const quint16 &x,const quint16 &y,const Orientation &orientation,const quint16 &speed)
{
	emit message(QString("ClientLocalCalcule put_on_the_map(): map: %1, x: %2, y: %3").arg(map->map_file).arg(x).arg(y));
	MapBasicMove::put_on_the_map(map,x,y,orientation,speed);

	loadOnTheMap();

	//send to the client the position of the player
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	out << (quint8)0x01;
	out << player_id;
	out << map->map_file;
	out << x;
	out << y;
	out << (quint8)orientation;
	out << speed;
	out << 1;//only send the position of the local player
	emit message(QString("ClientLocalCalcule insert the local client: map: %1, x: %2, y: %3").arg(map->map_file).arg(x).arg(y));
	emit sendPacket(0xC0,outputData);
}
