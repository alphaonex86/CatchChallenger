#include "LocalClientHandler.h"
#include "../../general/base/ProtocolParsing.h"
#include "GlobalServerData.h"

/** \todo do client near list for the local player
  the list is limited to 50
  if is greater, then truncate to have the near player, truncate to have all near player grouped by distance where a group not do the list greater
  each Xs update the local player list
*/
/** Never reserve the list, because it have square memory usage, and use more cpu */
/// \todo have teleportation list to save the last teleportation at close

using namespace CatchChallenger;

Direction LocalClientHandler::temp_direction;
QHash<QString,LocalClientHandler *> LocalClientHandler::playerByPseudo;

LocalClientHandler::LocalClientHandler()
{
    stepFight_Grass=0;
    stepFight_Water=0;
    stepFight_Cave=0;
    otherPlayerTrade=NULL;
    tradeIsValidated=false;
}

LocalClientHandler::~LocalClientHandler()
{
}

bool LocalClientHandler::getInTrade()
{
    return (otherPlayerTrade!=NULL);
}

void LocalClientHandler::registerTradeRequest(LocalClientHandler * otherPlayerTrade)
{
    if(getInTrade())
    {
        emit message("Already in trade, internal error");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message(QString("%1 have requested trade with you").arg(otherPlayerTrade->player_informations->public_and_private_informations.public_informations.pseudo));
    #endif
    this->otherPlayerTrade=otherPlayerTrade;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << otherPlayerTrade->player_informations->public_and_private_informations.public_informations.skinId;
    emit sendTradeRequest(otherPlayerTrade->player_informations->rawPseudo+outputData);
}

bool LocalClientHandler::getIsFreezed()
{
    return tradeIsFreezed;
}

quint64 LocalClientHandler::getTradeCash()
{
    return tradeCash;
}

QHash<quint32,quint32> LocalClientHandler::getTradeObjects()
{
    return tradeObjects;
}

QList<PlayerMonster> LocalClientHandler::getTradeMonster()
{
    return tradeMonster;
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

void LocalClientHandler::extraStop()
{
    tradeCanceled();
    playerByPseudo.remove(player_informations->public_and_private_informations.public_informations.pseudo);

    if(!player_informations->is_logged || player_informations->isFake)
        return;
    //save the monster
    if(GlobalServerData::serverSettings.database.fightSync==ServerSettings::Database::FightSync_AtTheEndOfBattle && !wildMonsters.empty())
        saveCurrentMonsterStat();
    if(GlobalServerData::serverSettings.database.fightSync==ServerSettings::Database::FightSync_AtTheDisconnexion)
    {
        int index=0;
        int size=player_informations->public_and_private_informations.playerMonster.size();
        while(index<size)
        {
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    emit dbQuery(QString("UPDATE monster SET hp=%3,xp=%4,level=%5,sp=%6 WHERE id=%1;")
                                 .arg(player_informations->public_and_private_informations.playerMonster[index].id)
                                 .arg(player_informations->id)
                                 .arg(player_informations->public_and_private_informations.playerMonster[index].hp)
                                 .arg(player_informations->public_and_private_informations.playerMonster[index].remaining_xp)
                                 .arg(player_informations->public_and_private_informations.playerMonster[index].level)
                                 .arg(player_informations->public_and_private_informations.playerMonster[index].sp)
                                 );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    emit dbQuery(QString("UPDATE monster SET hp=%3,xp=%4,level=%5,sp=%6 WHERE id=%1;")
                                 .arg(player_informations->public_and_private_informations.playerMonster[index].id)
                                 .arg(player_informations->id)
                                 .arg(player_informations->public_and_private_informations.playerMonster[index].hp)
                                 .arg(player_informations->public_and_private_informations.playerMonster[index].remaining_xp)
                                 .arg(player_informations->public_and_private_informations.playerMonster[index].level)
                                 .arg(player_informations->public_and_private_informations.playerMonster[index].sp)
                                 );
                break;
            }
            index++;
        }
    }
    savePosition();
}

void LocalClientHandler::savePosition()
{
    if(player_informations->isFake)
        return;
    //virtual stop the player
    //Orientation orientation;
    QString orientationString;
    switch(getLastDirection())
    {
        case Direction_look_at_bottom:
        case Direction_move_at_bottom:
            orientationString="bottom";
            //orientation=Orientation_bottom;
        break;
        case Direction_look_at_top:
        case Direction_move_at_top:
            orientationString="top";
            //orientation=Orientation_top;
        break;
        case Direction_look_at_left:
        case Direction_move_at_left:
            orientationString="left";
            //orientation=Orientation_left;
        break;
        case Direction_look_at_right:
        case Direction_move_at_right:
            orientationString="right";
            //orientation=Orientation_right;
        break;
        default:
            #ifdef DEBUG_MESSAGE_CLIENT_MOVE
            emit message("direction wrong and fixed before save");
            #endif
            orientationString="bottom";
            //orientation=Orientation_bottom;
        break;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_MOVE
    emit message(
                QString("map->map_file: %1,x: %2,y: %3, orientation: %4")
                .arg(map->map_file)
                .arg(x)
                .arg(y)
                .arg(orientationString)
                );
    #endif
    /* disable because use memory, but useful only into less than < 0.1% of case
     * if(map!=at_start_map_name || x!=at_start_x || y!=at_start_y || orientation!=at_start_orientation) */
    QString updateMapPositionQuery;
    switch(GlobalServerData::serverSettings.database.type)
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
    if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
        out << (quint8)map->id;
    else if(GlobalServerData::serverPrivateVariables.map_list.size()<=65535)
        out << (quint16)map->id;
    else
        out << (quint32)map->id;
    if(GlobalServerData::serverSettings.max_players<=255)
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
    out << player_informations->public_and_private_informations.public_informations.skinId;

    emit sendPacket(0xC0,outputData);

    //load the first time the random number list
    getRandomNumberIfNeeded();

    playerByPseudo[player_informations->public_and_private_informations.public_informations.pseudo]=this;
    if(GlobalServerData::serverSettings.database.secondToPositionSync>0 && !player_informations->isFake)
        connect(&GlobalServerData::serverPrivateVariables.positionSync,SIGNAL(timeout()),this,SLOT(savePosition()),Qt::QueuedConnection);

    updateCanDoFight();
}

bool LocalClientHandler::moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction)
{
    #ifdef DEBUG_MESSAGE_CLIENT_MOVE
    emit message(QString("moveThePlayer(): for player (%1,%2): %3, previousMovedUnit: %4 (%5), next direction: %6")
                 .arg(x)
                 .arg(y)
                 .arg(player_informations->public_and_private_informations.public_informations.simplifiedId)
                 .arg(previousMovedUnit)
                 .arg(MoveOnTheMap::directionToString(getLastDirection()))
                 .arg(MoveOnTheMap::directionToString(direction))
                 );
    #endif
    return MapBasicMove::moveThePlayer(previousMovedUnit,direction);
}

bool LocalClientHandler::singleMove(const Direction &direction)
{
    if(!wildMonsters.empty())//check if is in fight
    {
        emit error(QString("error: try move when is in fight"));
        return false;
    }
    COORD_TYPE x=this->x,y=this->y;
    temp_direction=direction;
    Map* map=this->map;
    #ifdef DEBUG_MESSAGE_CLIENT_MOVE
    emit message(QString("LocalClientHandler::singleMove(), go in this direction: %1 with map: %2(%3,%4)").arg(MoveOnTheMap::directionToString(direction)).arg(map->map_file).arg(x).arg(y));
    #endif
    if(!MoveOnTheMap::canGoTo(direction,*map,x,y,true))
    {
        emit error(QString("LocalClientHandler::singleMove(), can't go into this direction: %1 with map: %2(%3,%4)").arg(MoveOnTheMap::directionToString(direction)).arg(map->map_file).arg(x).arg(y));
        return false;
    }
    MoveOnTheMap::move(direction,&map,&x,&y);
    this->map=static_cast<Map_server_MapVisibility_simple*>(map);
    this->x=x;
    this->y=y;
    checkFightCollision(map,x,y);
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
        switch(GlobalServerData::serverSettings.database.type)
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
        switch(GlobalServerData::serverSettings.database.type)
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

void LocalClientHandler::saveObjectRetention(const quint32 &item)
{
    if(player_informations->public_and_private_informations.items.contains(item))
    {
        switch(GlobalServerData::serverSettings.database.type)
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
        switch(GlobalServerData::serverSettings.database.type)
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
    }
}

quint32 LocalClientHandler::removeObject(const quint32 &item,const quint32 &quantity)
{
    if(player_informations->public_and_private_informations.items.contains(item))
    {
        if(player_informations->public_and_private_informations.items[item]>quantity)
        {
            player_informations->public_and_private_informations.items[item]-=quantity;
            switch(GlobalServerData::serverSettings.database.type)
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
            switch(GlobalServerData::serverSettings.database.type)
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

void LocalClientHandler::addCash(const quint64 &cash, const bool &forceSave)
{
    if(cash==0 && !forceSave)
        return;
    player_informations->public_and_private_informations.cash+=cash;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            emit dbQuery(QString("UPDATE player SET cash=%1 WHERE id=%2;")
                         .arg(player_informations->public_and_private_informations.cash)
                         .arg(player_informations->id)
                         );
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            emit dbQuery(QString("UPDATE player SET cash=%1 WHERE id=%2;")
                     .arg(player_informations->public_and_private_informations.cash)
                     .arg(player_informations->id)
                     );
        break;
    }
}

void LocalClientHandler::removeCash(const quint64 &cash)
{
    if(cash==0)
        return;
    player_informations->public_and_private_informations.cash-=cash;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            emit dbQuery(QString("UPDATE player SET cash=%1 WHERE id=%2;")
                         .arg(player_informations->public_and_private_informations.cash)
                         .arg(player_informations->id)
                         );
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            emit dbQuery(QString("UPDATE player SET cash=%1 WHERE id=%2;")
                     .arg(player_informations->public_and_private_informations.cash)
                     .arg(player_informations->id)
                     );
        break;
    }
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
        if(!GlobalServerData::serverPrivateVariables.items.contains(objectId))
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
        if(!GlobalServerData::serverPrivateVariables.items.contains(objectId))
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
            playerByPseudo[arguments.first()]->receiveTeleportTo(playerByPseudo[arguments.last()]->map,playerByPseudo[arguments.last()]->x,playerByPseudo[arguments.last()]->y,MoveOnTheMap::directionToOrientation(playerByPseudo[arguments.last()]->getLastDirection()));
        }
        else
        {
            emit receiveSystemText("Wrong arguments number for the command, usage: /tp player1 to player2");
            return;
        }
    }
    else if(command=="trade")
    {
        if(extraText.isEmpty())
        {
            emit receiveSystemText(QString("no player given, syntaxe: /trade player").arg(extraText));
            return;
        }
        if(!playerByPseudo.contains(extraText))
        {
            emit receiveSystemText(QString("%1 is not connected").arg(extraText));
            return;
        }
        if(player_informations->public_and_private_informations.public_informations.pseudo==extraText)
        {
            emit receiveSystemText(QString("You can't trade with yourself").arg(extraText));
            return;
        }
        if(getInTrade())
        {
            emit receiveSystemText(QString("you are already in trade"));
            return;
        }
        if(playerByPseudo[extraText]->getInTrade())
        {
            emit receiveSystemText(QString("%1 is already in trade").arg(extraText));
            return;
        }
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        emit message("Trade requested");
        #endif
        otherPlayerTrade=playerByPseudo[extraText];
        otherPlayerTrade->registerTradeRequest(this);
    }
}

void LocalClientHandler::destroyObject(const quint32 &itemId,const quint32 &quantity)
{
    emit message(QString("The player have destroy them self %1 item(s) with id: %2").arg(quantity).arg(itemId));
    removeObject(itemId,quantity);
}

void LocalClientHandler::useObject(const quint8 &query_id,const quint32 &itemId)
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message(QString("use the object: %1").arg(itemId));
    #endif
    if(player_informations->public_and_private_informations.items.contains(itemId))
    {
        //if is crafting recipe
        if(GlobalServerData::serverPrivateVariables.itemToCrafingRecipes.contains(itemId))
        {
            quint32 recipeId=GlobalServerData::serverPrivateVariables.itemToCrafingRecipes[itemId];
            if(player_informations->public_and_private_informations.recipes.contains(recipeId))
            {
                emit error(QString("can't use the object: %1 because recipe already registred").arg(itemId));
                return;
            }
            player_informations->public_and_private_informations.recipes << recipeId;
            //send the network reply
            QByteArray outputData;
            QDataStream out(&outputData, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_4_4);
            out << (quint8)ObjectUsage_correctlyUsed;
            emit postReply(query_id,outputData);
            //add into db
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    emit dbQuery(QString("INSERT INTO recipes(player,recipe) VALUES(%1,%2);")
                             .arg(player_informations->id)
                             .arg(recipeId)
                             );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    emit dbQuery(QString("INSERT INTO recipes(player,recipe) VALUES(%1,%2);")
                             .arg(player_informations->id)
                             .arg(recipeId)
                             );
                break;
            }
        }
        else
        {
            emit error(QString("can't use the object: %1 because don't have an usage").arg(itemId));
            return;
        }
    }
    else
    {
        emit error(QString("can't use the object: %1 because don't have into the inventory").arg(itemId));
        return;
    }
}

void LocalClientHandler::receiveTeleportTo(Map *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation)
{
    emit teleportTo(map,x,y,orientation);
}

void LocalClientHandler::teleportValidatedTo(Map *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation)
{
    emit message(QString("teleportValidatedTo(%1,%2,%3,%4)").arg(map->map_file).arg(x).arg(y).arg((quint8)orientation));
    MapBasicMove::teleportValidatedTo(map,x,y,orientation);
    if(GlobalServerData::serverSettings.database.positionTeleportSync)
        savePosition();
}

Direction LocalClientHandler::lookToMove(const Direction &direction)
{
    switch(direction)
    {
        case Direction_look_at_top:
            return Direction_move_at_top;
        break;
        case Direction_look_at_right:
            return Direction_move_at_right;
        break;
        case Direction_look_at_bottom:
            return Direction_move_at_bottom;
        break;
        case Direction_look_at_left:
            return Direction_move_at_left;
        break;
        default:
        return direction;
    }
}

void LocalClientHandler::getShopList(const quint32 &query_id,const quint32 &shopId)
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message(QString("getShopList(%1,%2)").arg(query_id).arg(shopId));
    #endif
    if(!GlobalServerData::serverPrivateVariables.shops.contains(shopId))
    {
        emit error(QString("shopId not found: %1").arg(shopId));
        return;
    }
    Map *map=this->map;
    quint8 x=this->x;
    quint8 y=this->y;
    //resolv the object
    Direction direction=getLastDirection();
    switch(getLastDirection())
    {
        case Direction_look_at_top:
        case Direction_look_at_right:
        case Direction_look_at_bottom:
        case Direction_look_at_left:
            direction=lookToMove(direction);
            if(MoveOnTheMap::canGoTo(direction,*map,x,y,false))
            {
                if(!MoveOnTheMap::move(direction,&map,&x,&y,false))
                {
                    emit error(QString("plantSeed() Can't move at top from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                    return;
                }
            }
            else
            {
                emit error("No valid map in this direction");
                return;
            }
        break;
        default:
        emit error("Wrong direction to use a shop");
        return;
    }
    //check if is shop
    if(static_cast<MapServer*>(this->map)->shops.contains(QPair<quint8,quint8>(x,y)))
    {
        QList<quint32> shops=static_cast<MapServer*>(this->map)->shops.values(QPair<quint8,quint8>(x,y));
        if(!shops.contains(shopId))
        {
            switch(direction)
            {
                case Direction_look_at_top:
                case Direction_look_at_right:
                case Direction_look_at_bottom:
                case Direction_look_at_left:
                    if(MoveOnTheMap::canGoTo(direction,*map,x,y,false))
                    {
                        if(!MoveOnTheMap::move(direction,&map,&x,&y,false))
                        {
                            emit error(QString("plantSeed() Can't move at top from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                            return;
                        }
                    }
                    else
                    {
                        emit error("No valid map in this direction");
                        return;
                    }
                break;
                default:
                break;
            }
            if(static_cast<MapServer*>(this->map)->shops.contains(QPair<quint8,quint8>(x,y)))
            {
                QList<quint32> shops=static_cast<MapServer*>(this->map)->shops.values(QPair<quint8,quint8>(x,y));
                if(!shops.contains(shopId))
                {
                    emit error("not shop into this direction");
                    return;
                }
            }
        }
    }
    //send the shop items (no taxes from now)
    QList<quint32> items=GlobalServerData::serverPrivateVariables.shops[shopId].items;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    QByteArray outputData2;
    QDataStream out2(&outputData2, QIODevice::WriteOnly);
    out2.setVersion(QDataStream::Qt_4_4);
    int index=0;
    int objectCount=0;
    while(index<items.size())
    {
        if(GlobalServerData::serverPrivateVariables.items[items.at(index)].price>0)
        {
            out2 << (quint32)items.at(index);
            out2 << (quint32)GlobalServerData::serverPrivateVariables.items[items.at(index)].price;
            out2 << (quint32)0;
            objectCount++;
        }
        index++;
    }
    out << objectCount;
    emit postReply(query_id,outputData+outputData2);
}

void LocalClientHandler::buyObject(const quint32 &query_id,const quint32 &shopId,const quint32 &objectId,const quint32 &quantity,const quint32 &price)
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message(QString("getShopList(%1,%2)").arg(query_id).arg(shopId));
    #endif
    if(!GlobalServerData::serverPrivateVariables.shops.contains(shopId))
    {
        emit error(QString("shopId not found: %1").arg(shopId));
        return;
    }
    if(quantity<=0)
    {
        emit error(QString("quantity wrong: %1").arg(quantity));
        return;
    }
    Map *map=this->map;
    quint8 x=this->x;
    quint8 y=this->y;
    //resolv the object
    switch(getLastDirection())
    {
        case Direction_look_at_top:
            if(MoveOnTheMap::canGoTo(Direction_move_at_top,*map,x,y,false))
            {
                if(!MoveOnTheMap::move(Direction_move_at_top,&map,&x,&y,false))
                {
                    emit error(QString("plantSeed() Can't move at top from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                    return;
                }
            }
            else
            {
                emit error("No valid map in this direction");
                return;
            }
        break;
        case Direction_look_at_right:
            if(MoveOnTheMap::canGoTo(Direction_move_at_right,*map,x,y,false))
            {
                if(!MoveOnTheMap::move(Direction_move_at_right,&map,&x,&y,false))
                {
                    emit error(QString("plantSeed() Can't move at right from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                    return;
                }
            }
            else
            {
                emit error("No valid map in this direction");
                return;
            }
        break;
        case Direction_look_at_bottom:
            if(MoveOnTheMap::canGoTo(Direction_move_at_bottom,*map,x,y,false))
            {
                if(!MoveOnTheMap::move(Direction_move_at_bottom,&map,&x,&y,false))
                {
                    emit error(QString("plantSeed() Can't move at bottom from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                    return;
                }
            }
            else
            {
                emit error("No valid map in this direction");
                return;
            }
        break;
        case Direction_look_at_left:
            if(MoveOnTheMap::canGoTo(Direction_move_at_left,*map,x,y,false))
            {
                if(!MoveOnTheMap::move(Direction_move_at_left,&map,&x,&y,false))
                {
                    emit error(QString("plantSeed() Can't move at left from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                    return;
                }
            }
            else
            {
                emit error("No valid map in this direction");
                return;
            }
        break;
        default:
        emit error("Wrong direction to plant a seed");
        return;
    }
    //check if is shop
    if(static_cast<MapServer*>(this->map)->shops.contains(QPair<quint8,quint8>(x,y)))
    {
        QList<quint32> shops=static_cast<MapServer*>(this->map)->shops.values(QPair<quint8,quint8>(x,y));
        if(!shops.contains(shopId))
        {
            switch(getLastDirection())
            {
                case Direction_look_at_top:
                    if(MoveOnTheMap::canGoTo(Direction_move_at_top,*map,x,y,false))
                    {
                        if(!MoveOnTheMap::move(Direction_move_at_top,&map,&x,&y,false))
                        {
                            emit error(QString("plantSeed() Can't move at top from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                            return;
                        }
                    }
                    else
                    {
                        emit error("No valid map in this direction");
                        return;
                    }
                break;
                case Direction_look_at_right:
                    if(MoveOnTheMap::canGoTo(Direction_move_at_right,*map,x,y,false))
                    {
                        if(!MoveOnTheMap::move(Direction_move_at_right,&map,&x,&y,false))
                        {
                            emit error(QString("plantSeed() Can't move at right from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                            return;
                        }
                    }
                    else
                    {
                        emit error("No valid map in this direction");
                        return;
                    }
                break;
                case Direction_look_at_bottom:
                    if(MoveOnTheMap::canGoTo(Direction_move_at_bottom,*map,x,y,false))
                    {
                        if(!MoveOnTheMap::move(Direction_move_at_bottom,&map,&x,&y,false))
                        {
                            emit error(QString("plantSeed() Can't move at bottom from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                            return;
                        }
                    }
                    else
                    {
                        emit error("No valid map in this direction");
                        return;
                    }
                break;
                case Direction_look_at_left:
                    if(MoveOnTheMap::canGoTo(Direction_move_at_left,*map,x,y,false))
                    {
                        if(!MoveOnTheMap::move(Direction_move_at_left,&map,&x,&y,false))
                        {
                            emit error(QString("plantSeed() Can't move at left from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                            return;
                        }
                    }
                    else
                    {
                        emit error("No valid map in this direction");
                        return;
                    }
                break;
                default:
                emit error("Wrong direction to plant a seed");
                return;
            }
            if(static_cast<MapServer*>(this->map)->shops.contains(QPair<quint8,quint8>(x,y)))
            {
                QList<quint32> shops=static_cast<MapServer*>(this->map)->shops.values(QPair<quint8,quint8>(x,y));
                if(!shops.contains(shopId))
                {
                    emit error("not shop into this direction");
                    return;
                }
            }
        }
    }
    //send the shop items (no taxes from now)
    QList<quint32> items=GlobalServerData::serverPrivateVariables.shops[shopId].items;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    if(!items.contains(objectId))
    {
        out << (quint8)BuyStat_HaveNotQuantity;
        emit postReply(query_id,outputData);
        return;
    }
    if(GlobalServerData::serverPrivateVariables.items[objectId].price==0)
    {
        out << (quint8)BuyStat_HaveNotQuantity;
        emit postReply(query_id,outputData);
        return;
    }
    if(GlobalServerData::serverPrivateVariables.items[objectId].price>price)
    {
        out << (quint8)BuyStat_PriceHaveChanged;
        emit postReply(query_id,outputData);
        return;
    }
    if(GlobalServerData::serverPrivateVariables.items[objectId].price<price)
    {
        out << (quint8)BuyStat_BetterPrice;
        out << (quint32)GlobalServerData::serverPrivateVariables.items[objectId].price;
    }
    else
        out << (quint8)BuyStat_Done;
    if(player_informations->public_and_private_informations.cash>=(GlobalServerData::serverPrivateVariables.items[objectId].price*quantity))
        removeCash(GlobalServerData::serverPrivateVariables.items[objectId].price*quantity);
    else
    {
        emit error(QString("The player have not the cash to buy %1 item of id: %2").arg(quantity).arg(objectId));
        return;
    }
    addObject(objectId,quantity);
    emit postReply(query_id,outputData);
}

void LocalClientHandler::sellObject(const quint32 &query_id,const quint32 &shopId,const quint32 &objectId,const quint32 &quantity,const quint32 &price)
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message(QString("getShopList(%1,%2)").arg(query_id).arg(shopId));
    #endif
    if(!GlobalServerData::serverPrivateVariables.shops.contains(shopId))
    {
        emit error(QString("shopId not found: %1").arg(shopId));
        return;
    }
    if(quantity<=0)
    {
        emit error(QString("quantity wrong: %1").arg(quantity));
        return;
    }
    Map *map=this->map;
    quint8 x=this->x;
    quint8 y=this->y;
    //resolv the object
    switch(getLastDirection())
    {
        case Direction_look_at_top:
            if(MoveOnTheMap::canGoTo(Direction_move_at_top,*map,x,y,false))
            {
                if(!MoveOnTheMap::move(Direction_move_at_top,&map,&x,&y,false))
                {
                    emit error(QString("plantSeed() Can't move at top from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                    return;
                }
            }
            else
            {
                emit error("No valid map in this direction");
                return;
            }
        break;
        case Direction_look_at_right:
            if(MoveOnTheMap::canGoTo(Direction_move_at_right,*map,x,y,false))
            {
                if(!MoveOnTheMap::move(Direction_move_at_right,&map,&x,&y,false))
                {
                    emit error(QString("plantSeed() Can't move at right from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                    return;
                }
            }
            else
            {
                emit error("No valid map in this direction");
                return;
            }
        break;
        case Direction_look_at_bottom:
            if(MoveOnTheMap::canGoTo(Direction_move_at_bottom,*map,x,y,false))
            {
                if(!MoveOnTheMap::move(Direction_move_at_bottom,&map,&x,&y,false))
                {
                    emit error(QString("plantSeed() Can't move at bottom from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                    return;
                }
            }
            else
            {
                emit error("No valid map in this direction");
                return;
            }
        break;
        case Direction_look_at_left:
            if(MoveOnTheMap::canGoTo(Direction_move_at_left,*map,x,y,false))
            {
                if(!MoveOnTheMap::move(Direction_move_at_left,&map,&x,&y,false))
                {
                    emit error(QString("plantSeed() Can't move at left from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                    return;
                }
            }
            else
            {
                emit error("No valid map in this direction");
                return;
            }
        break;
        default:
        emit error("Wrong direction to plant a seed");
        return;
    }
    //check if is shop
    if(static_cast<MapServer*>(this->map)->shops.contains(QPair<quint8,quint8>(x,y)))
    {
        QList<quint32> shops=static_cast<MapServer*>(this->map)->shops.values(QPair<quint8,quint8>(x,y));
        if(!shops.contains(shopId))
        {
            switch(getLastDirection())
            {
                case Direction_look_at_top:
                    if(MoveOnTheMap::canGoTo(Direction_move_at_top,*map,x,y,false))
                    {
                        if(!MoveOnTheMap::move(Direction_move_at_top,&map,&x,&y,false))
                        {
                            emit error(QString("plantSeed() Can't move at top from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                            return;
                        }
                    }
                    else
                    {
                        emit error("No valid map in this direction");
                        return;
                    }
                break;
                case Direction_look_at_right:
                    if(MoveOnTheMap::canGoTo(Direction_move_at_right,*map,x,y,false))
                    {
                        if(!MoveOnTheMap::move(Direction_move_at_right,&map,&x,&y,false))
                        {
                            emit error(QString("plantSeed() Can't move at right from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                            return;
                        }
                    }
                    else
                    {
                        emit error("No valid map in this direction");
                        return;
                    }
                break;
                case Direction_look_at_bottom:
                    if(MoveOnTheMap::canGoTo(Direction_move_at_bottom,*map,x,y,false))
                    {
                        if(!MoveOnTheMap::move(Direction_move_at_bottom,&map,&x,&y,false))
                        {
                            emit error(QString("plantSeed() Can't move at bottom from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                            return;
                        }
                    }
                    else
                    {
                        emit error("No valid map in this direction");
                        return;
                    }
                break;
                case Direction_look_at_left:
                    if(MoveOnTheMap::canGoTo(Direction_move_at_left,*map,x,y,false))
                    {
                        if(!MoveOnTheMap::move(Direction_move_at_left,&map,&x,&y,false))
                        {
                            emit error(QString("plantSeed() Can't move at left from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                            return;
                        }
                    }
                    else
                    {
                        emit error("No valid map in this direction");
                        return;
                    }
                break;
                default:
                emit error("Wrong direction to plant a seed");
                return;
            }
            if(static_cast<MapServer*>(this->map)->shops.contains(QPair<quint8,quint8>(x,y)))
            {
                QList<quint32> shops=static_cast<MapServer*>(this->map)->shops.values(QPair<quint8,quint8>(x,y));
                if(!shops.contains(shopId))
                {
                    emit error("not shop into this direction");
                    return;
                }
            }
        }
    }
    //send the shop items (no taxes from now)
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    if(!GlobalServerData::serverPrivateVariables.items.contains(objectId))
    {
        emit error("this item don't exists");
        return;
    }
    if(objectQuantity(objectId)<quantity)
    {
        emit error("you have not this quantity to sell");
        return;
    }
    quint32 realPrice=GlobalServerData::serverPrivateVariables.items[objectId].price/2;
    if(realPrice<price)
    {
        out << (quint8)SoldStat_PriceHaveChanged;
        emit postReply(query_id,outputData);
        return;
    }
    if(realPrice>price)
    {
        out << (quint8)SoldStat_BetterPrice;
        out << (quint32)realPrice;
    }
    else
        out << (quint8)SoldStat_Done;
    removeObject(objectId,quantity);
    addCash(realPrice*quantity);
    emit postReply(query_id,outputData);
}

void LocalClientHandler::tradeCanceled()
{
    if(otherPlayerTrade!=NULL)
        otherPlayerTrade->internalTradeCanceled(true);
    internalTradeCanceled(false);
}

void LocalClientHandler::tradeAccepted()
{
    if(otherPlayerTrade!=NULL)
        otherPlayerTrade->internalTradeAccepted(true);
    internalTradeAccepted(false);
}

void LocalClientHandler::tradeFinished()
{
    if(!tradeIsValidated)
    {
        emit error("Trade not valid");
        return;
    }
    if(tradeIsFreezed)
    {
        emit error("Trade is freezed, unable to re-free");
        return;
    }
    tradeIsFreezed=true;
    if(getIsFreezed() && otherPlayerTrade->getIsFreezed())
    {
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        emit message("Trade finished");
        #endif
        //cash
        otherPlayerTrade->addCash(tradeCash,(otherPlayerTrade->getTradeCash()!=0));
        addCash(otherPlayerTrade->getTradeCash(),(tradeCash!=0));

        //object
        QHashIterator<quint32,quint32> i(tradeObjects);
        while (i.hasNext()) {
            i.next();
            otherPlayerTrade->addObject(i.key(),i.value());
            saveObjectRetention(i.key());
        }
        QHashIterator<quint32,quint32> j(otherPlayerTrade->getTradeObjects());
        while (j.hasNext()) {
            j.next();
            addObject(j.key(),j.value());
            otherPlayerTrade->saveObjectRetention(j.key());
        }

        //monster
        otherPlayerTrade->addExistingMonster(tradeMonster);
        addExistingMonster(otherPlayerTrade->tradeMonster);

        emit otherPlayerTrade->sendPacket(0xD0,0x0008);
        emit sendPacket(0xD0,0x0008);
        otherPlayerTrade->resetTheTrade();
        resetTheTrade();
    }
    else
    {
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        emit message("Trade freezed");
        #endif
        emit otherPlayerTrade->sendPacket(0xD0,0x0007);
    }
}

void LocalClientHandler::resetTheTrade()
{
    //reset out of trade
    tradeIsValidated=false;
    otherPlayerTrade=NULL;
    tradeCash=0;
    tradeObjects.clear();
    tradeMonster.clear();
    updateCanDoFight();
}

void LocalClientHandler::addExistingMonster(QList<PlayerMonster> tradeMonster)
{
    int index=0;
    while(index<tradeMonster.size())
    {
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                emit dbQuery(QString("UPDATE monster SET player=%2 WHERE id=%1;")
                             .arg(tradeMonster.at(index).id)
                             .arg(player_informations->id)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QString("UPDATE monster SET player=%2 WHERE id=%1;")
                             .arg(tradeMonster.at(index).id)
                             .arg(player_informations->id)
                             );
            break;
        }
        index++;
    }
    player_informations->public_and_private_informations.playerMonster << tradeMonster;
}

void LocalClientHandler::tradeAddTradeCash(const quint64 &cash)
{
    if(!tradeIsValidated)
    {
        emit error("Trade not valid");
        return;
    }
    if(tradeIsFreezed)
    {
        emit error("Trade is freezed, unable to change something");
        return;
    }
    if(cash==0)
    {
        emit error("Can't add 0 cash!");
        return;
    }
    if(cash>player_informations->public_and_private_informations.cash)
    {
        emit error("Trade cash superior to the actual cash");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message(QString("Add cash to trade: %1").arg(cash));
    #endif
    tradeCash+=cash;
    player_informations->public_and_private_informations.cash-=cash;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x01;
    out << cash;
    emit otherPlayerTrade->sendPacket(0xD0,0x0004,outputData);
}

void LocalClientHandler::tradeAddTradeObject(const quint32 &item,const quint32 &quantity)
{
    if(!tradeIsValidated)
    {
        emit error("Trade not valid");
        return;
    }
    if(tradeIsFreezed)
    {
        emit error("Trade is freezed, unable to change something");
        return;
    }
    if(quantity==0)
    {
        emit error("Can add 0 of quantity");
        return;
    }
    if(quantity>objectQuantity(item))
    {
        emit error(QString("Trade object %1 in quantity %2 superior to the actual quantity").arg(item).arg(quantity));
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message(QString("Add object to trade: %1 (quantity: %2)").arg(item).arg(quantity));
    #endif
    if(tradeObjects.contains(item))
        tradeObjects[item]+=quantity;
    else
        tradeObjects[item]=quantity;
    player_informations->public_and_private_informations.items[item]-=quantity;
    if(player_informations->public_and_private_informations.items[item]==0)
        player_informations->public_and_private_informations.items.remove(item);
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x02;
    out << item;
    out << quantity;
    emit otherPlayerTrade->sendPacket(0xD0,0x0004,outputData);
}

void LocalClientHandler::tradeAddTradeMonster(const quint32 &monsterId)
{
    if(!tradeIsValidated)
    {
        emit error("Trade not valid");
        return;
    }
    if(tradeIsFreezed)
    {
        emit error("Trade is freezed, unable to change something");
        return;
    }
    if(player_informations->public_and_private_informations.playerMonster.size()<=1)
    {
        emit error("Unable to trade your last monster");
        return;
    }
    if(isInFight())
    {
        emit error("You can't trade monster because you are in fight");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message(QString("Add monster to trade: %1").arg(monsterId));
    #endif
    int index=0;
    while(index<player_informations->public_and_private_informations.playerMonster.size())
    {
        if(player_informations->public_and_private_informations.playerMonster.at(index).id==monsterId)
        {
            if(!remainMonstersToFight(monsterId))
            {
                emit error("You can't trade monster because you are in fight");
                return;
            }
            tradeMonster << player_informations->public_and_private_informations.playerMonster.at(index);
            player_informations->public_and_private_informations.playerMonster.removeAt(index);
            updateCanDoFight();
            QByteArray outputData;
            QDataStream out(&outputData, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_4_4);
            out << (quint8)0x03;
            const PlayerMonster &monster=tradeMonster.last();
            out << (quint32)monster.id;
            out << (quint32)monster.monster;
            out << (quint8)monster.level;
            out << (quint32)monster.remaining_xp;
            out << (quint32)monster.hp;
            out << (quint32)monster.sp;
            out << (quint32)monster.captured_with;
            out << (quint8)monster.gender;
            out << (quint32)monster.egg_step;
            int sub_index=0;
            int sub_size=monster.buffs.size();
            out << (quint32)sub_size;
            while(sub_index<sub_size)
            {
                out << (quint32)monster.buffs.at(sub_index).buff;
                out << (quint8)monster.buffs.at(sub_index).level;
                sub_index++;
            }
            sub_index=0;
            sub_size=monster.skills.size();
            out << (quint32)sub_size;
            while(sub_index<sub_size)
            {
                out << (quint32)monster.skills.at(sub_index).skill;
                out << (quint8)monster.skills.at(sub_index).level;
                sub_index++;
            }
            emit otherPlayerTrade->sendPacket(0xD0,0x0004,outputData);
            return;
        }
        index++;
    }
    emit error("Trade monster not found");
}

void LocalClientHandler::internalTradeCanceled(const bool &send)
{
    if(otherPlayerTrade==NULL)
    {
        //emit message("Trade already canceled");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message("Trade canceled");
    #endif
    if(tradeIsValidated)
    {
        player_informations->public_and_private_informations.cash+=tradeCash;
        tradeCash=0;
        QHashIterator<quint32,quint32> i(tradeObjects);
        while (i.hasNext()) {
            i.next();
            if(player_informations->public_and_private_informations.items.contains(i.key()))
                player_informations->public_and_private_informations.items[i.key()]+=i.value();
            else
                player_informations->public_and_private_informations.items[i.key()]=i.value();
        }
        tradeObjects.clear();
        player_informations->public_and_private_informations.playerMonster << tradeMonster;
        tradeMonster.clear();
        updateCanDoFight();
    }
    otherPlayerTrade=NULL;
    if(send)
    {
        if(tradeIsValidated)
            emit sendPacket(0xD0,0x0006);
        else
            emit receiveSystemText(QString("Trade declined"));
    }
    tradeIsValidated=false;
}

void LocalClientHandler::internalTradeAccepted(const bool &send)
{
    if(otherPlayerTrade==NULL)
    {
        emit message("Can't accept trade if not in trade");
        return;
    }
    if(tradeIsValidated)
    {
        emit message("Trade already validated");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message("Trade accepted");
    #endif
    tradeIsValidated=true;
    tradeIsFreezed=false;
    tradeCash=0;
    if(send)
    {
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);
        out << otherPlayerTrade->player_informations->public_and_private_informations.public_informations.skinId;
        emit sendPacket(0xD0,0x0005,otherPlayerTrade->player_informations->rawPseudo+outputData);
    }
}
