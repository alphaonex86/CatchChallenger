#include "LocalClientHandler.h"
#include "../../general/base/ProtocolParsing.h"
#include "GlobalData.h"

/** \todo do client near list for the local player
  the list is limited to 50
  if is greater, then truncate to have the near player, truncate to have all near player grouped by distance where a group not do the list greater
  each Xs update the local player list
*/
/** Never reserve the list, because it have square memory usage, and use more cpu */
/// \todo have teleportation list to save the last teleportation at close

using namespace Pokecraft;

Direction LocalClientHandler::temp_direction;
QHash<QString,LocalClientHandler *> LocalClientHandler::playerByPseudo;

LocalClientHandler::LocalClientHandler()
{
}

LocalClientHandler::~LocalClientHandler()
{
}

bool LocalClientHandler::checkCollision()
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

void LocalClientHandler::getRandomNumberIfNeeded()
{
    if(player_informations->public_and_private_informations.randomNumber.size()<=POKECRAFT_SERVER_MIN_RANDOM_LIST_SIZE)
        emit askRandomNumber();
}

void LocalClientHandler::extraStop()
{
    playerByPseudo.remove(player_informations->public_and_private_informations.public_informations.pseudo);
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
    /* disable because use memory, but useful only into less than < 0.1% of case
     * if(map!=at_start_map_name || x!=at_start_x || y!=at_start_y || orientation!=at_start_orientation) */
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
    QString updateMapPositionQuery;
    switch(GlobalData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            updateMapPositionQuery=QString("UPDATE player SET map_name=\"%1\",position_x=%2,position_y=%3,orientation=\"%4\" WHERE id=%5")
                .arg(SqlFunction::quoteSqlVariable(map->map_file))
                .arg(x)
                .arg(y)
                .arg(orientationString)
                .arg(player_informations->id);
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            updateMapPositionQuery=QString("UPDATE player SET map_name=\"%1\",position_x=%2,position_y=%3,orientation=\"%4\" WHERE id=%5")
                .arg(SqlFunction::quoteSqlVariable(map->map_file))
                .arg(x)
                .arg(y)
                .arg(orientationString)
                .arg(player_informations->id);
        break;
    }
    emit dbQuery(updateMapPositionQuery);
}

/* why do that's here?
 * Because the ClientMapManagement can be totaly satured by the square complexity
 * that's allow to continue the player to connect and play
 * the overhead for the network it just at the connexion */
void LocalClientHandler::put_on_the_map(Map *map,const COORD_TYPE &x,const COORD_TYPE &y,const Orientation &orientation)
{
    MapBasicMove::put_on_the_map(map,x,y,orientation);

    //send to the client the position of the player
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);

    out << (quint8)0x01;
    if(GlobalData::serverPrivateVariables.map_list.size()<=255)
        out << (quint8)map->id;
    else if(GlobalData::serverPrivateVariables.map_list.size()<=65535)
        out << (quint16)map->id;
    else
        out << (quint32)map->id;
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
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    qDebug() << "put_on_the_map merge" << quint8((quint8)orientation|(quint8)player_informations->public_and_private_informations.public_informations.type) << "=" << (quint8)orientation << "|" << (quint8)player_informations->public_and_private_informations.public_informations.type;
    #endif
    out << quint8((quint8)orientation|(quint8)player_informations->public_and_private_informations.public_informations.type);
    out << player_informations->public_and_private_informations.public_informations.speed;
    out << player_informations->public_and_private_informations.public_informations.clan;

    outputData+=player_informations->rawPseudo;
    out.device()->seek(out.device()->pos()+player_informations->rawPseudo.size());
    out << player_informations->public_and_private_informations.public_informations.skinId;

    emit sendPacket(0xC0,outputData);

    //load the first time the random number list
    getRandomNumberIfNeeded();

    playerByPseudo[player_informations->public_and_private_informations.public_informations.pseudo]=this;
}

bool LocalClientHandler::moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction)
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message(QString("moveThePlayer(): for player (%1,%2): %3, previousMovedUnit: %4 (%5), next direction: %6")
                 .arg(x)
                 .arg(y)
                 .arg(player_informations->public_and_private_informations.public_informations.simplifiedId)
                 .arg(previousMovedUnit)
                 .arg(MoveOnTheMap::directionToString(last_direction))
                 .arg(MoveOnTheMap::directionToString(direction))
                 );
    #endif
    return MapBasicMove::moveThePlayer(previousMovedUnit,direction);
}

void LocalClientHandler::newRandomNumber(const QByteArray &randomData)
{
    player_informations->public_and_private_informations.randomNumber+=randomData;
}

bool LocalClientHandler::singleMove(const Direction &direction)
{
    temp_direction=direction;
    Map* map=this->map;
    emit message(QString("LocalClientHandler::singleMove(), go in this direction: %1 with map: %2(%3,%4)").arg(MoveOnTheMap::directionToString(direction)).arg(map->map_file).arg(x).arg(y));
    if(!MoveOnTheMap::canGoTo(direction,*map,x,y,true))
    {
        emit error(QString("LocalClientHandler::singleMove(), can't' go into this direction: %1 with map: %2(%3,%4)").arg(MoveOnTheMap::directionToString(direction)).arg(map->map_file).arg(x).arg(y));
        return false;
    }
    MoveOnTheMap::move(direction,&map,&x,&y);
    this->map=static_cast<Map_server_MapVisibility_simple*>(map);
    return true;
}

void LocalClientHandler::addObjectAndSend(const quint32 &item,const quint32 &quantity)
{
    addObject(item,quantity);
    //add into the inventory
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint32)1;
    out << (quint32)item;
    out << (quint32)quantity;
    emit sendPacket(0xD0,0x0002,outputData);
}

void LocalClientHandler::addObject(const quint32 &item,const quint32 &quantity)
{
    if(player_informations->public_and_private_informations.items.contains(item))
    {
        player_informations->public_and_private_informations.items[item]+=quantity;
        switch(GlobalData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                emit dbQuery(QString("UPDATE item SET quantity=%1 WHERE item_id=%2 AND player_id=%3;")
                             .arg(player_informations->public_and_private_informations.items[item])
                             .arg(item)
                             .arg(player_informations->id)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QString("UPDATE item SET quantity=%1 WHERE item_id=%2 AND player_id=%3;")
                         .arg(player_informations->public_and_private_informations.items[item])
                         .arg(item)
                         .arg(player_informations->id)
                         );
            break;
        }
    }
    else
    {
        switch(GlobalData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                emit dbQuery(QString("INSERT INTO item(item_id,player_id,quantity) VALUES(%1,%2,%3);")
                             .arg(item)
                             .arg(player_informations->id)
                             .arg(quantity)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QString("INSERT INTO item(item_id,player_id,quantity) VALUES(%1,%2,%3);")
                         .arg(item)
                         .arg(player_informations->id)
                         .arg(quantity)
                         );
            break;
        }
        player_informations->public_and_private_informations.items[item]=quantity;
    }
}

quint32 LocalClientHandler::removeObject(const quint32 &item,const quint32 &quantity)
{
    if(player_informations->public_and_private_informations.items.contains(item))
    {
        if(player_informations->public_and_private_informations.items[item]>quantity)
        {
            player_informations->public_and_private_informations.items[item]-=quantity;
            switch(GlobalData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    emit dbQuery(QString("UPDATE item SET quantity=%1 WHERE item_id=%2 AND player_id=%3;")
                                 .arg(player_informations->public_and_private_informations.items[item])
                                 .arg(item)
                                 .arg(player_informations->id)
                                 );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    emit dbQuery(QString("UPDATE item SET quantity=%1 WHERE item_id=%2 AND player_id=%3;")
                                 .arg(player_informations->public_and_private_informations.items[item])
                                 .arg(item)
                                 .arg(player_informations->id)
                             );
                break;
            }
            return quantity;
        }
        else
        {
            quint32 removed_quantity=player_informations->public_and_private_informations.items[item];
            player_informations->public_and_private_informations.items.remove(item);
            switch(GlobalData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    emit dbQuery(QString("DELETE FROM item WHERE item_id=%1 AND player_id=%2")
                                 .arg(item)
                                 .arg(player_informations->id)
                                 );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    emit dbQuery(QString("DELETE FROM item WHERE item_id=%1 AND player_id=%2")
                             .arg(item)
                             .arg(player_informations->id)
                             );
                break;
            }
            return removed_quantity;
        }
    }
    else
        return 0;
}

void LocalClientHandler::sendRemoveObject(const quint32 &item,const quint32 &quantity)
{
    //add into the inventory
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint32)1;
    out << (quint32)item;
    out << (quint32)quantity;
    emit sendPacket(0xD0,0x0003,outputData);
}

quint32 LocalClientHandler::objectQuantity(const quint32 &item)
{
    if(player_informations->public_and_private_informations.items.contains(item))
        return player_informations->public_and_private_informations.items[item];
    else
        return 0;
}

void LocalClientHandler::sendHandlerCommand(const QString &command,const QString &extraText)
{
    if(command=="give")
    {
        bool ok;
        QStringList arguments=extraText.split(" ",QString::SkipEmptyParts);
        if(arguments.size()==2)
            arguments << "1";
        if(arguments.size()!=3)
        {
            emit receiveSystemText("Wrong arguments number for the command, usage: /give objectId player [quantity=1]");
            return;
        }
        quint32 objectId=arguments.first().toUInt(&ok);
        if(!ok)
        {
            emit receiveSystemText("objectId is not a number, usage: /give objectId player [quantity=1]");
            return;
        }
        if(!GlobalData::serverPrivateVariables.itemsId.contains(objectId))
        {
            emit receiveSystemText("objectId is not a valid item, usage: /give objectId player [quantity=1]");
            return;
        }
        quint32 quantity=arguments.last().toUInt(&ok);
        if(!ok)
        {
            emit receiveSystemText("quantity is not a number, usage: /give objectId player [quantity=1]");
            return;
        }
        if(!playerByPseudo.contains(arguments.at(1)))
        {
            emit receiveSystemText("player is not connected, usage: /give objectId player [quantity=1]");
            return;
        }
        emit message(QString("%1 have give to %2 the item with id: %3 in quantity: %4").arg(player_informations->public_and_private_informations.public_informations.pseudo).arg(arguments.at(1)).arg(objectId).arg(quantity));
        playerByPseudo[arguments.at(1)]->addObjectAndSend(objectId,quantity);
    }
    else if(command=="take")
    {
        bool ok;
        QStringList arguments=extraText.split(" ",QString::SkipEmptyParts);
        if(arguments.size()==2)
            arguments << "1";
        if(arguments.size()!=3)
        {
            emit receiveSystemText("Wrong arguments number for the command, usage: /take objectId player [quantity=1]");
            return;
        }
        quint32 objectId=arguments.first().toUInt(&ok);
        if(!ok)
        {
            emit receiveSystemText("objectId is not a number, usage: /take objectId player [quantity=1]");
            return;
        }
        if(!GlobalData::serverPrivateVariables.itemsId.contains(objectId))
        {
            emit receiveSystemText("objectId is not a valid item, usage: /take objectId player [quantity=1]");
            return;
        }
        quint32 quantity=arguments.last().toUInt(&ok);
        if(!ok)
        {
            emit receiveSystemText("quantity is not a number, usage: /take objectId player [quantity=1]");
            return;
        }
        if(!playerByPseudo.contains(arguments.at(1)))
        {
            emit receiveSystemText("player is not connected, usage: /take objectId player [quantity=1]");
            return;
        }
        emit message(QString("%1 have take to %2 the item with id: %3 in quantity: %4").arg(player_informations->public_and_private_informations.public_informations.pseudo).arg(arguments.at(1)).arg(objectId).arg(quantity));
        playerByPseudo[arguments.at(1)]->sendRemoveObject(objectId,playerByPseudo[arguments.at(1)]->removeObject(objectId,quantity));
    }
    else if(command=="tp")
    {
        QStringList arguments=extraText.split(" ",QString::SkipEmptyParts);
        if(arguments.size()==3)
        {
            if(arguments.at(1)!="to")
            {
                emit receiveSystemText(QString("wrong second arguement: %1, usage: /tp player1 to player2").arg(arguments.at(1)));
                return;
            }
            if(!playerByPseudo.contains(arguments.first()))
            {
                emit receiveSystemText(QString("%1 is not connected, usage: /tp player1 to player2").arg(arguments.first()));
                return;
            }
            if(!playerByPseudo.contains(arguments.last()))
            {
                emit receiveSystemText(QString("%1 is not connected, usage: /tp player1 to player2").arg(arguments.last()));
                return;
            }
            playerByPseudo[arguments.first()]->receiveTeleportTo(playerByPseudo[arguments.last()]->map,playerByPseudo[arguments.last()]->x,playerByPseudo[arguments.last()]->y,MoveOnTheMap::directionToOrientation(playerByPseudo[arguments.last()]->last_direction));
        }
        else
        {
            emit receiveSystemText("Wrong arguments number for the command, usage: /tp player1 to player2");
            return;
        }
    }
}

void LocalClientHandler::destroyObject(const quint32 &itemId,const quint32 &quantity)
{
    emit message(QString("The player have destroy them self %1 item(s) with id: %2").arg(quantity).arg(itemId));
    removeObject(itemId,quantity);
}

void LocalClientHandler::receiveTeleportTo(Map *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation)
{
    emit teleportTo(map,x,y,orientation);
}

void LocalClientHandler::teleportValidatedTo(Map *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation)
{
    emit message(QString("teleportValidatedTo(%1,%2,%3,%4)").arg(map->map_file).arg(x).arg(y).arg((quint8)orientation));
    MapBasicMove::teleportValidatedTo(map,x,y,orientation);
}
