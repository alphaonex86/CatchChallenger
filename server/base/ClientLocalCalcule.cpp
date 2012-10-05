#include "ClientLocalCalcule.h"
#include "../../general/base/ProtocolParsing.h"
#include "GlobalData.h"

/** \todo do client near list for the local player
  the list is limited to 50
  if is greater, then truncate to have the near player, truncate to have all near player grouped by distance where a group not do the list greater
  each Xs update the local player list
*/
/** Never reserve the list, because it have square memory usage, and use more cpu */

using namespace Pokecraft;

QString ClientLocalCalcule::temp_direction;

ClientLocalCalcule::ClientLocalCalcule()
{
}

ClientLocalCalcule::~ClientLocalCalcule()
{
}

bool ClientLocalCalcule::checkCollision()
{
    if(map->parsed_layer.walkable==NULL)
        return false;
    if(!map->parsed_layer.walkable[x+y*map->width])
    {
        emit error(QString("move at %1, can't wall at: %2,%3 on map: %4").arg(temp_direction).arg(x).arg(y).arg(map->map_file));
        return false;
    }
    else
        return true;
}

void ClientLocalCalcule::getRandomNumberIfNeeded()
{
    if(player_informations->public_and_private_informations.randomNumber.size()<=POKECRAFT_SERVER_MIN_RANDOM_LIST_SIZE)
        emit askRandomNumber();
}

void ClientLocalCalcule::extraStop()
{
    //virtual stop the player
    Orientation orientation;
    QString orientationString;
    switch(last_direction)
    {
        case Direction_look_at_bottom:
        case Direction_move_at_bottom:
            orientationString="bottom";
            orientation=Orientation_bottom;
        break;
        case Direction_look_at_top:
        case Direction_move_at_top:
            orientationString="top";
            orientation=Orientation_top;
        break;
        case Direction_look_at_left:
        case Direction_move_at_left:
            orientationString="left";
            orientation=Orientation_left;
        break;
        case Direction_look_at_right:
        case Direction_move_at_right:
            orientationString="right";
            orientation=Orientation_right;
        break;
        default:
            #ifdef DEBUG_MESSAGE_CLIENT_MOVE
            DebugClass::debugConsole("direction wrong and fixed before save");
            #endif
            orientationString="bottom";
            orientation=Orientation_bottom;
        break;
    }
    if(map!=at_start_map_name || x!=at_start_x || y!=at_start_y || orientation!=at_start_orientation)
    {
        #ifdef DEBUG_MESSAGE_CLIENT_MOVE
        DebugClass::debugConsole(
                    QString("map->map_file: %1,x: %2,y: %3, orientation: %4")
                    .arg(map->map_file)
                    .arg(x)
                    .arg(y)
                    .arg(orientationString)
                    );
        #endif
        if(!player_informations->is_logged || player_informations->isFake)
            return;
        QSqlQuery updateMapPositionQuery;
        switch(GlobalData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                updateMapPositionQuery=QString("UPDATE player SET map_name=%1,position_x=%2,position_y=%3,orientation=%4 WHERE id=%5")
                    .arg(SqlFunction::quoteSqlVariable(map->map_file))
                    .arg(x)
                    .arg(y)
                    .arg(orientationString)
                    .arg(player_informations->id);
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                updateMapPositionQuery=QString("UPDATE player SET map_name=%1,position_x=%2,position_y=%3,orientation=%4 WHERE id=%5")
                    .arg(SqlFunction::quoteSqlVariable(map->map_file))
                    .arg(x)
                    .arg(y)
                    .arg(orientationString)
                    .arg(player_informations->id);
            break;
        }
        if(!updateMapPositionQuery.exec())
            DebugClass::debugConsole(QString("Sql query failed: %1, error: %2").arg(updateMapPositionQuery.lastQuery()).arg(updateMapPositionQuery.lastError().text()));
    }
}

/* why do that's here?
 * Because the ClientMapManagement can be totaly satured by the square complexity
 * that's allow to continue the player to connect and play
 * the overhead for the network it just at the connexion */
void ClientLocalCalcule::put_on_the_map(Map *map,const COORD_TYPE &x,const COORD_TYPE &y,const Orientation &orientation)
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message(QString("ClientLocalCalcule put_on_the_map(): map: %1, x: %2, y: %3").arg(map->map_file).arg(x).arg(y));
    #endif
    MapBasicMove::put_on_the_map(map,x,y,orientation);
    at_start_orientation=orientation;
    at_start_map_name=map;
    at_start_x=x;
    at_start_y=y;

    //send to the client the position of the player
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);

    outputData[0]=0x01;
    outputData+=map->rawMapFile;
    out.device()->seek(out.device()->size());
    if(GlobalData::serverSettings.max_players<=255)
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
    qDebug() << "put_on_the_map merge" << quint8((quint8)orientation|(quint8)player_informations->public_and_private_informations.public_informations.type) << "=" << (quint8)orientation << "|" << (quint8)player_informations->public_and_private_informations.public_informations.type;
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
    temp_direction=MoveOnTheMap::directionToString(direction);
    return MapBasicMove::moveThePlayer(previousMovedUnit,direction);
}

void ClientLocalCalcule::newRandomNumber(const QByteArray &randomData)
{
    player_informations->public_and_private_informations.randomNumber+=randomData;
}

bool ClientLocalCalcule::singleMove(const Direction &direction)
{
    Map* map=this->map;
    if(!MoveOnTheMap::canGoTo(direction,*map,x,y,true))
    {
        emit error(QString("ClientLocalCalcule::singleMove(), can go into this direction: %1 with map: %2(%3,%4)").arg(MoveOnTheMap::directionToString(direction)).arg(map->map_file).arg(x).arg(y));
        return false;
    }
    MoveOnTheMap::move(direction,&map,&x,&y);
    this->map=static_cast<Map_server_MapVisibility_simple*>(map);
    return true;
}
