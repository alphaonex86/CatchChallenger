#include "LocalClientHandler.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/FacilityLib.h"
#include "GlobalServerData.h"

#include <QStringList>

using namespace CatchChallenger;

Direction LocalClientHandler::temp_direction;
QHash<QString,LocalClientHandler *> LocalClientHandler::playerByPseudo;
QHash<quint32,LocalClientHandler::Clan> LocalClientHandler::playerByClan;
QHash<QString,QMultiHash<quint32,LocalClientHandler *> > LocalClientHandler::captureCity;
QHash<QString,QMultiHash<quint32,LocalClientHandler *> > LocalClientHandler::captureCityValidated;
QHash<quint32,LocalClientHandler::Clan *> LocalClientHandler::clanList;

LocalClientHandler::LocalClientHandler()
{
    otherPlayerTrade=NULL;
    clan=NULL;
    tradeIsValidated=false;

    connect(&localClientHandlerFight,&LocalClientHandlerFight::message,             this,&LocalClientHandler::message);
    connect(&localClientHandlerFight,&LocalClientHandlerFight::error,               this,&LocalClientHandler::error);
    connect(&localClientHandlerFight,&LocalClientHandlerFight::dbQuery,             this,&LocalClientHandler::dbQuery);
    connect(&localClientHandlerFight,&LocalClientHandlerFight::askRandomNumber,     this,&LocalClientHandler::askRandomNumber);
    connect(&localClientHandlerFight,&LocalClientHandlerFight::receiveSystemText,   this,&LocalClientHandler::receiveSystemText);
    connect(&localClientHandlerFight,&LocalClientHandlerFight::postReply,           this,&LocalClientHandler::postReply);
    connect(&localClientHandlerFight,&LocalClientHandlerFight::sendBattleRequest,   this,&LocalClientHandler::sendBattleRequest);
    connect(&localClientHandlerFight,&LocalClientHandlerFight::sendPacket,          this,&LocalClientHandler::sendPacket);
    connect(&localClientHandlerFight,&LocalClientHandlerFight::sendFullPacket,      this,&LocalClientHandler::sendFullPacket);
    connect(&localClientHandlerFight,&LocalClientHandlerFight::teleportTo,          this,&LocalClientHandler::teleportTo);
    connect(&localClientHandlerFight,&LocalClientHandlerFight::addObjectAndSend,    this,&LocalClientHandler::addObjectAndSend);
    connect(&localClientHandlerFight,&LocalClientHandlerFight::addCash,             this,&LocalClientHandler::addCash);
}

LocalClientHandler::~LocalClientHandler()
{
}

void LocalClientHandler::setVariable(Player_internal_informations *player_informations)
{
    MapBasicMove::setVariable(player_informations);
    localClientHandlerFight.setVariable(player_informations);
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
    localClientHandlerFight.battleCanceled();
    if(player_informations->is_logged)
    {
        playerByPseudo.remove(player_informations->public_and_private_informations.public_informations.pseudo);
        if(!playerByClan.contains(player_informations->public_and_private_informations.clan))
        {
            playerByClan[player_informations->public_and_private_informations.clan].players.removeOne(this);
            if(playerByClan[player_informations->public_and_private_informations.clan].players.isEmpty())
                playerByClan.remove(player_informations->public_and_private_informations.clan);
        }
    }
    if(clan!=NULL)
    {
        clan->playercount--;
        if(clan->playercount==0)
        {
            delete clan;
            clanList.remove(player_informations->public_and_private_informations.clan);
        }
    }

    if(!player_informations->is_logged || player_informations->isFake)
        return;
    //save the monster
    if(GlobalServerData::serverSettings.database.fightSync==ServerSettings::Database::FightSync_AtTheEndOfBattle && !localClientHandlerFight.isInFight())
        localClientHandlerFight.saveCurrentMonsterStat();
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

QString LocalClientHandler::directionToStringToSave(const Direction &direction)
{
    switch(direction)
    {
        case Direction_look_at_top:
        case Direction_move_at_top:
            return "top";
        break;
        case Direction_look_at_right:
        case Direction_move_at_right:
            return "right";
        break;
        case Direction_look_at_bottom:
        case Direction_move_at_bottom:
            return "bottom";
        break;
        case Direction_look_at_left:
        case Direction_move_at_left:
            return "left";
        break;
        default:
        break;
    }
    return "bottom";
}

QString LocalClientHandler::orientationToStringToSave(const Orientation &orientation)
{
    switch(orientation)
    {
        case Orientation_top:
            return "top";
        break;
        case Orientation_bottom:
            return "bottom";
        break;
        case Orientation_right:
            return "right";
        break;
        case Orientation_left:
            return "left";
        break;
        default:
        break;
    }
    return "bottom";
}

void LocalClientHandler::savePosition()
{
    if(player_informations->isFake)
        return;
    //virtual stop the player
    //Orientation orientation;
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
            updateMapPositionQuery=QString("UPDATE player SET map_name=\"%1\",position_x=%2,position_y=%3,orientation=\"%4\",%5 WHERE id=%6")
                .arg(SqlFunction::quoteSqlVariable(map->map_file))
                .arg(x)
                .arg(y)
                .arg(directionToStringToSave(getLastDirection()))
                .arg(
                        QString("rescue_map=\"%1\",rescue_x=%2,rescue_y=%3,rescue_orientation=\"%4\",unvalidated_rescue_map=\"%5\",unvalidated_rescue_x=%6,unvalidated_rescue_y=%7,unvalidated_rescue_orientation=\"%8\"")
                        .arg(player_informations->rescue.map->map_file)
                        .arg(player_informations->rescue.x)
                        .arg(player_informations->rescue.y)
                        .arg(orientationToStringToSave(player_informations->rescue.orientation))
                        .arg(player_informations->unvalidated_rescue.map->map_file)
                        .arg(player_informations->unvalidated_rescue.x)
                        .arg(player_informations->unvalidated_rescue.y)
                        .arg(orientationToStringToSave(player_informations->unvalidated_rescue.orientation))
                )
                .arg(player_informations->id);
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            updateMapPositionQuery=QString("UPDATE player SET map_name=\"%1\",position_x=%2,position_y=%3,orientation=\"%4\",%5 WHERE id=%6")
                .arg(SqlFunction::quoteSqlVariable(map->map_file))
                .arg(x)
                .arg(y)
                .arg(directionToStringToSave(getLastDirection()))
                .arg(
                        QString("rescue_map=\"%1\",rescue_x=%2,rescue_y=%3,rescue_orientation=\"%4\",unvalidated_rescue_map=\"%5\",unvalidated_rescue_x=%6,unvalidated_rescue_y=%7,unvalidated_rescue_orientation=\"%8\"")
                        .arg(player_informations->rescue.map->map_file)
                        .arg(player_informations->rescue.x)
                        .arg(player_informations->rescue.y)
                        .arg(orientationToStringToSave(player_informations->rescue.orientation))
                        .arg(player_informations->unvalidated_rescue.map->map_file)
                        .arg(player_informations->unvalidated_rescue.x)
                        .arg(player_informations->unvalidated_rescue.y)
                        .arg(orientationToStringToSave(player_informations->unvalidated_rescue.orientation))
                )
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

    outputData+=player_informations->rawPseudo;
    out.device()->seek(out.device()->pos()+player_informations->rawPseudo.size());
    out << player_informations->public_and_private_informations.public_informations.skinId;

    emit sendPacket(0xC0,outputData);

    //load the first time the random number list
    localClientHandlerFight.getRandomNumberIfNeeded();

    playerByPseudo[player_informations->public_and_private_informations.public_informations.pseudo]=this;
    if(player_informations->public_and_private_informations.clan>0)
    {
        if(!playerByClan.contains(player_informations->public_and_private_informations.clan))
            emit askClan(player_informations->public_and_private_informations.clan);
        else
            sendClanInfo();
        playerByClan[player_informations->public_and_private_informations.clan].players << this;
        if(!clanList.contains(player_informations->public_and_private_informations.clan))
        {
            clan=new Clan;
            clan->clanId=player_informations->public_and_private_informations.clan;
            clan->playercount=0;
            clanList[player_informations->public_and_private_informations.clan]=clan;
        }
        clan->playercount++;
    }
    if(GlobalServerData::serverSettings.database.secondToPositionSync>0 && !player_informations->isFake)
        QObject::connect(&GlobalServerData::serverPrivateVariables.positionSync,SIGNAL(timeout()),this,SLOT(savePosition()),Qt::QueuedConnection);

    localClientHandlerFight.updateCanDoFight();
    if(localClientHandlerFight.getAbleToFight())
        localClientHandlerFight.botFightCollision(map,x,y);
    else if(localClientHandlerFight.haveMonsters())
        localClientHandlerFight.checkLoose();
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

bool LocalClientHandler::captureCityInProgress() const
{
    if(clan==NULL)
        return false;
    if(clan->captureCityInProgress.isEmpty())
        return false;
    return captureCity.count(clan->captureCityInProgress)>0;
}

bool LocalClientHandler::singleMove(const Direction &direction)
{
    if(localClientHandlerFight.isInFight())//check if is in fight
    {
        emit error(QString("error: try move when is in fight"));
        return false;
    }
    if(captureCityInProgress())
    {
        emit error("Try move when is in capture city");
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
    if(static_cast<Map_server_MapVisibility_simple*>(map)->rescue.contains(QPair<quint8,quint8>(x,y)))
    {
        player_informations->unvalidated_rescue.map=map;
        player_informations->unvalidated_rescue.x=x;
        player_informations->unvalidated_rescue.y=y;
        player_informations->unvalidated_rescue.orientation=static_cast<Map_server_MapVisibility_simple*>(map)->rescue[QPair<quint8,quint8>(x,y)];
    }
    if(localClientHandlerFight.botFightCollision(map,x,y))
        return true;
    if(localClientHandlerFight.generateWildFightIfCollision(map,x,y))
    {
        emit message(QString("LocalClientHandler::singleMove(), now is in front of wild monster with map: %1(%2,%3)").arg(map->map_file).arg(x).arg(y));
        return true;
    }
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
    emit sendFullPacket(0xD0,0x0002,outputData);
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
                emit dbQuery(QString("UPDATE item SET quantity=%1 WHERE item_id=%2 AND player_id=%3 AND warehouse!=1;")
                             .arg(player_informations->public_and_private_informations.items[item])
                             .arg(item)
                             .arg(player_informations->id)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QString("UPDATE item SET quantity=%1 WHERE item_id=%2 AND player_id=%3 AND warehouse!=1;")
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
                emit dbQuery(QString("INSERT INTO item(item_id,player_id,quantity,warehouse) VALUES(%1,%2,%3,0);")
                             .arg(item)
                             .arg(player_informations->id)
                             .arg(quantity)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QString("INSERT INTO item(item_id,player_id,quantity,warehouse) VALUES(%1,%2,%3,0);")
                         .arg(item)
                         .arg(player_informations->id)
                         .arg(quantity)
                         );
            break;
        }
        player_informations->public_and_private_informations.items[item]=quantity;
    }
}

void LocalClientHandler::addWarehouseObject(const quint32 &item,const quint32 &quantity)
{
    if(player_informations->public_and_private_informations.warehouse_items.contains(item))
    {
        player_informations->public_and_private_informations.warehouse_items[item]+=quantity;
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                emit dbQuery(QString("UPDATE item SET quantity=%1 WHERE item_id=%2 AND player_id=%3 AND warehouse=1;")
                             .arg(player_informations->public_and_private_informations.warehouse_items[item])
                             .arg(item)
                             .arg(player_informations->id)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QString("UPDATE item SET quantity=%1 WHERE item_id=%2 AND player_id=%3 AND warehouse=1;")
                         .arg(player_informations->public_and_private_informations.warehouse_items[item])
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
                emit dbQuery(QString("INSERT INTO item(item_id,player_id,quantity,warehouse) VALUES(%1,%2,%3,1);")
                             .arg(item)
                             .arg(player_informations->id)
                             .arg(quantity)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QString("INSERT INTO item(item_id,player_id,quantity,warehouse) VALUES(%1,%2,%3,1);")
                         .arg(item)
                         .arg(player_informations->id)
                         .arg(quantity)
                         );
            break;
        }
        player_informations->public_and_private_informations.warehouse_items[item]=quantity;
    }
}

quint32 LocalClientHandler::removeWarehouseObject(const quint32 &item,const quint32 &quantity)
{
    if(player_informations->public_and_private_informations.warehouse_items.contains(item))
    {
        if(player_informations->public_and_private_informations.warehouse_items[item]>quantity)
        {
            player_informations->public_and_private_informations.warehouse_items[item]-=quantity;
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    emit dbQuery(QString("UPDATE item SET quantity=%1 WHERE item_id=%2 AND player_id=%3 AND warehouse=1;")
                                 .arg(player_informations->public_and_private_informations.warehouse_items[item])
                                 .arg(item)
                                 .arg(player_informations->id)
                                 );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    emit dbQuery(QString("UPDATE item SET quantity=%1 WHERE item_id=%2 AND player_id=%3 AND warehouse=1;")
                                 .arg(player_informations->public_and_private_informations.warehouse_items[item])
                                 .arg(item)
                                 .arg(player_informations->id)
                             );
                break;
            }
            return quantity;
        }
        else
        {
            quint32 removed_quantity=player_informations->public_and_private_informations.warehouse_items[item];
            player_informations->public_and_private_informations.warehouse_items.remove(item);
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    emit dbQuery(QString("DELETE FROM item WHERE item_id=%1 AND player_id=%2 AND warehouse=1")
                                 .arg(item)
                                 .arg(player_informations->id)
                                 );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    emit dbQuery(QString("DELETE FROM item WHERE item_id=%1 AND player_id=%2 AND warehouse=1")
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

void LocalClientHandler::saveObjectRetention(const quint32 &item)
{
    if(player_informations->public_and_private_informations.items.contains(item))
    {
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                emit dbQuery(QString("UPDATE item SET quantity=%1 WHERE item_id=%2 AND player_id=%3 AND warehouse!=1;")
                             .arg(player_informations->public_and_private_informations.items[item])
                             .arg(item)
                             .arg(player_informations->id)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QString("UPDATE item SET quantity=%1 WHERE item_id=%2 AND player_id=%3 AND warehouse!=1;")
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
                emit dbQuery(QString("DELETE FROM item WHERE item_id=%1 AND player_id=%2 AND warehouse!=1")
                             .arg(item)
                             .arg(player_informations->id)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QString("DELETE FROM item WHERE item_id=%1 AND player_id=%2 AND warehouse!=1")
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
                    emit dbQuery(QString("UPDATE item SET quantity=%1 WHERE item_id=%2 AND player_id=%3 AND warehouse!=1;")
                                 .arg(player_informations->public_and_private_informations.items[item])
                                 .arg(item)
                                 .arg(player_informations->id)
                                 );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    emit dbQuery(QString("UPDATE item SET quantity=%1 WHERE item_id=%2 AND player_id=%3 AND warehouse!=1;")
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
                    emit dbQuery(QString("DELETE FROM item WHERE item_id=%1 AND player_id=%2 AND warehouse!=1")
                                 .arg(item)
                                 .arg(player_informations->id)
                                 );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    emit dbQuery(QString("DELETE FROM item WHERE item_id=%1 AND player_id=%2 AND warehouse!=1")
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
    emit sendFullPacket(0xD0,0x0003,outputData);
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

void LocalClientHandler::addWarehouseCash(const quint64 &cash, const bool &forceSave)
{
    if(cash==0 && !forceSave)
        return;
    player_informations->public_and_private_informations.warehouse_cash+=cash;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            emit dbQuery(QString("UPDATE player SET warehouse_cash=%1 WHERE id=%2;")
                         .arg(player_informations->public_and_private_informations.warehouse_cash)
                         .arg(player_informations->id)
                         );
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            emit dbQuery(QString("UPDATE player SET warehouse_cash=%1 WHERE id=%2;")
                     .arg(player_informations->public_and_private_informations.warehouse_cash)
                     .arg(player_informations->id)
                     );
        break;
    }
}

void LocalClientHandler::removeWarehouseCash(const quint64 &cash)
{
    if(cash==0)
        return;
    player_informations->public_and_private_informations.warehouse_cash-=cash;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            emit dbQuery(QString("UPDATE player SET warehouse_cash=%1 WHERE id=%2;")
                         .arg(player_informations->public_and_private_informations.warehouse_cash)
                         .arg(player_informations->id)
                         );
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            emit dbQuery(QString("UPDATE player SET warehouse_cash=%1 WHERE id=%2;")
                     .arg(player_informations->public_and_private_informations.warehouse_cash)
                     .arg(player_informations->id)
                     );
        break;
    }
}

void LocalClientHandler::wareHouseStore(const qint64 &cash, const QList<QPair<quint32, qint32> > &items, const QList<quint32> &withdrawMonsters, const QList<quint32> &depositeMonsters)
{
    if(!wareHouseStoreCheck(cash,items,withdrawMonsters,depositeMonsters))
        return;
    {
        int index=0;
        while(index<items.size())
        {
            const QPair<quint32, qint32> &item=items.at(index);
            if(item.second>0)
            {
                removeWarehouseObject(item.first,item.second);
                addObject(item.first,item.second);
            }
            if(item.second<0)
            {
                removeObject(item.first,-item.second);
                addWarehouseObject(item.first,-item.second);
            }
            index++;
        }
    }
    //validate the change here
    if(cash>0)
    {
        removeWarehouseCash(cash);
        addCash(cash);
    }
    if(cash<0)
    {
        removeCash(-cash);
        addWarehouseCash(-cash);
    }
    {
        int index=0;
        while(index<withdrawMonsters.size())
        {
            int sub_index=0;
            while(sub_index<player_informations->public_and_private_informations.warehouse_playerMonster.size())
            {
                if(player_informations->public_and_private_informations.warehouse_playerMonster.at(sub_index).id==withdrawMonsters.at(index))
                {
                    switch(GlobalServerData::serverSettings.database.type)
                    {
                        default:
                        case ServerSettings::Database::DatabaseType_Mysql:
                            emit dbQuery(QString("UPDATE monster SET warehouse!=1 WHERE id=%1;")
                                         .arg(withdrawMonsters.at(index))
                                         );
                        break;
                        case ServerSettings::Database::DatabaseType_SQLite:
                            emit dbQuery(QString("UPDATE monster SET warehouse!=1 WHERE id=%1;")
                                         .arg(withdrawMonsters.at(index))
                                         );
                        break;
                    }
                    player_informations->public_and_private_informations.playerMonster << player_informations->public_and_private_informations.warehouse_playerMonster.at(sub_index);
                    player_informations->public_and_private_informations.warehouse_playerMonster.removeAt(sub_index);
                    break;
                }
                sub_index++;
            }
            index++;
        }
    }
    {
        int index=0;
        while(index<depositeMonsters.size())
        {
            int sub_index=0;
            while(sub_index<player_informations->public_and_private_informations.playerMonster.size())
            {
                if(player_informations->public_and_private_informations.playerMonster.at(sub_index).id==depositeMonsters.at(index))
                {
                    switch(GlobalServerData::serverSettings.database.type)
                    {
                        default:
                        case ServerSettings::Database::DatabaseType_Mysql:
                            emit dbQuery(QString("UPDATE monster SET warehouse=1 WHERE id=%1;")
                                         .arg(withdrawMonsters.at(index))
                                         );
                        break;
                        case ServerSettings::Database::DatabaseType_SQLite:
                            emit dbQuery(QString("UPDATE monster SET warehouse=1 WHERE id=%1;")
                                         .arg(withdrawMonsters.at(index))
                                         );
                        break;
                    }
                    player_informations->public_and_private_informations.warehouse_playerMonster << player_informations->public_and_private_informations.playerMonster.at(sub_index);
                    player_informations->public_and_private_informations.playerMonster.removeAt(sub_index);
                    break;
                }
                sub_index++;
            }
            index++;
        }
    }

}

bool LocalClientHandler::wareHouseStoreCheck(const qint64 &cash, const QList<QPair<quint32, qint32> > &items, const QList<quint32> &withdrawMonsters, const QList<quint32> &depositeMonsters) const
{
    //check all
    if((cash>0 && (qint64)player_informations->public_and_private_informations.warehouse_cash<cash) || (cash<0 && (qint64)player_informations->public_and_private_informations.cash<-cash))
    {
        emit error("cash transfer is wrong");
        return false;
    }
    {
        int index=0;
        while(index<items.size())
        {
            const QPair<quint32, qint32> &item=items.at(index);
            if(item.second>0)
            {
                if(!player_informations->public_and_private_informations.warehouse_items.contains(item.first))
                {
                    emit error("warehouse item transfer is wrong due to missing");
                    return false;
                }
                if((qint64)player_informations->public_and_private_informations.warehouse_items[item.first]<item.second)
                {
                    emit error("warehouse item transfer is wrong due to wrong quantity");
                    return false;
                }
            }
            if(item.second<0)
            {
                if(!player_informations->public_and_private_informations.items.contains(item.first))
                {
                    emit error("item transfer is wrong due to missing");
                    return false;
                }
                if((qint64)player_informations->public_and_private_informations.items[item.first]<-item.second)
                {
                    emit error("item transfer is wrong due to wrong quantity");
                    return false;
                }
            }
            index++;
        }
    }
    int count_change=0;
    {
        int index=0;
        while(index<withdrawMonsters.size())
        {
            int sub_index=0;
            while(sub_index<player_informations->public_and_private_informations.warehouse_playerMonster.size())
            {
                if(player_informations->public_and_private_informations.warehouse_playerMonster.at(sub_index).id==withdrawMonsters.at(index))
                {
                    count_change++;
                    break;
                }
                sub_index++;
            }
            if(sub_index==player_informations->public_and_private_informations.warehouse_playerMonster.size())
            {
                emit error("no monster to withdraw");
                return false;
            }
            index++;
        }
    }
    {
        int index=0;
        while(index<depositeMonsters.size())
        {
            int sub_index=0;
            while(sub_index<player_informations->public_and_private_informations.playerMonster.size())
            {
                if(player_informations->public_and_private_informations.playerMonster.at(sub_index).id==depositeMonsters.at(index))
                {
                    count_change--;
                    break;
                }
                sub_index++;
            }
            if(sub_index==player_informations->public_and_private_informations.playerMonster.size())
            {
                emit error("no monster to deposite");
                return false;
            }
            index++;
        }
    }
    if((player_informations->public_and_private_informations.playerMonster.size()+count_change)>CATCHCHALLENGER_MONSTER_MAX_WEAR_ON_PLAYER)
    {
        emit error("have more monster to withdraw than the allowed");
        return false;
    }
    return true;
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
        if(!CommonDatapack::commonDatapack.items.item.contains(objectId))
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
        if(!CommonDatapack::commonDatapack.items.item.contains(objectId))
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
            emit receiveSystemText(QString("You are already in trade"));
            return;
        }
        if(localClientHandlerFight.isInBattle())
        {
            emit receiveSystemText(QString("you are already in battle"));
            return;
        }
        if(playerByPseudo[extraText]->getInTrade())
        {
            emit receiveSystemText(QString("%1 is already in trade").arg(extraText));
            return;
        }
        if(playerByPseudo[extraText]->localClientHandlerFight.isInBattle())
        {
            emit receiveSystemText(QString("%1 is already in battle").arg(extraText));
            return;
        }
        if(!otherPlayerIsInRange(playerByPseudo[extraText]))
        {
            emit receiveSystemText(QString("%1 is not in range").arg(extraText));
            return;
        }
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        emit message("Trade requested");
        #endif
        otherPlayerTrade=playerByPseudo[extraText];
        otherPlayerTrade->registerTradeRequest(this);
    }
    else if(command=="battle")
    {
        if(extraText.isEmpty())
        {
            emit receiveSystemText(QString("no player given, syntaxe: /battle player").arg(extraText));
            return;
        }
        if(!playerByPseudo.contains(extraText))
        {
            emit receiveSystemText(QString("%1 is not connected").arg(extraText));
            return;
        }
        if(player_informations->public_and_private_informations.public_informations.pseudo==extraText)
        {
            emit receiveSystemText(QString("You can't battle with yourself").arg(extraText));
            return;
        }
        if(localClientHandlerFight.isInBattle())
        {
            emit receiveSystemText(QString("you are already in battle"));
            return;
        }
        if(getInTrade())
        {
            emit receiveSystemText(QString("you are already in trade"));
            return;
        }
        if(playerByPseudo[extraText]->localClientHandlerFight.isInBattle())
        {
            emit receiveSystemText(QString("%1 is already in battle").arg(extraText));
            return;
        }
        if(playerByPseudo[extraText]->getInTrade())
        {
            emit receiveSystemText(QString("%1 is already in battle").arg(extraText));
            return;
        }
        if(!otherPlayerIsInRange(playerByPseudo[extraText]))
        {
            emit receiveSystemText(QString("%1 is not in range").arg(extraText));
            return;
        }
        if(!playerByPseudo[extraText]->localClientHandlerFight.getAbleToFight())
        {
            emit receiveSystemText("The other player can't fight");
            return;
        }
        if(!localClientHandlerFight.getAbleToFight())
        {
            emit receiveSystemText("You can't fight");
            return;
        }
        if(playerByPseudo[extraText]->localClientHandlerFight.isInFight())
        {
            emit receiveSystemText("The other player is in fight");
            return;
        }
        if(localClientHandlerFight.isInFight())
        {
            emit receiveSystemText("You are in fight");
            return;
        }
        if(captureCityInProgress())
        {
            emit error("Try battle when is in capture city");
            return;
        }
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        emit message("Battle requested");
        #endif
        playerByPseudo[extraText]->localClientHandlerFight.registerBattleRequest(&localClientHandlerFight);
    }
}

bool LocalClientHandler::learnSkill(const quint32 &monsterId,const quint32 &skill)
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message(QString("learnSkill(%1,%2)").arg(monsterId).arg(skill));
    #endif
    Map *map=this->map;
    quint8 x=this->x;
    quint8 y=this->y;
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
                    return false;
                }
            }
            else
            {
                emit error("No valid map in this direction");
                return false;
            }
        break;
        default:
        emit error("Wrong direction to use a learn skill");
        return false;
    }
    if(!static_cast<MapServer*>(this->map)->learn.contains(QPair<quint8,quint8>(x,y)))
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
                        return false;
                    }
                }
                else
                {
                    emit error("No valid map in this direction");
                    return false;
                }
            break;
            default:
            break;
        }
        if(!static_cast<MapServer*>(this->map)->learn.contains(QPair<quint8,quint8>(x,y)))
        {
            emit error("not learn skill into this direction");
            return false;
        }
    }
    return localClientHandlerFight.learnSkillInternal(monsterId,skill);
}

bool LocalClientHandler::otherPlayerIsInRange(LocalClientHandler * otherPlayer)
{
    if(getMap()==NULL)
        return false;
    return getMap()==otherPlayer->getMap();
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
    if(!player_informations->public_and_private_informations.items.contains(itemId))
    {
        emit error(QString("can't use the object: %1 because don't have into the inventory").arg(itemId));
        return;
    }
    if(objectQuantity(itemId)<1)
    {
        emit error(QString("have not quantity to use this object: %1 because recipe already registred").arg(itemId));
        return;
    }
    removeObject(itemId);
    //if is crafting recipe
    if(CommonDatapack::commonDatapack.itemToCrafingRecipes.contains(itemId))
    {
        quint32 recipeId=CommonDatapack::commonDatapack.itemToCrafingRecipes[itemId];
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
    //use trap into fight
    else if(CommonDatapack::commonDatapack.items.trap.contains(itemId))
    {
        if(!localClientHandlerFight.isInFight())
        {
            emit error(QString("is not in fight to use trap").arg(itemId));
            return;
        }
        if(!localClientHandlerFight.isInFightWithWild())
        {
            emit error(QString("is not in fight with wild to use trap").arg(itemId));
            return;
        }
        localClientHandlerFight.tryCapture(itemId);
        //send the network reply
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);
        out << (quint8)ObjectUsage_correctlyUsed;
        out << (quint32)GlobalServerData::serverPrivateVariables.maxMonsterId;
        emit postReply(query_id,outputData);
    }
    else
    {
        emit error(QString("can't use the object: %1 because don't have an usage").arg(itemId));
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
    switch(direction)
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
                    emit error(QString("plantSeed() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
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
                            emit error(QString("plantSeed() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
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
        if(CommonDatapack::commonDatapack.items.item[items.at(index)].price>0)
        {
            out2 << (quint32)items.at(index);
            out2 << (quint32)CommonDatapack::commonDatapack.items.item[items.at(index)].price;
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
    Direction direction=getLastDirection();
    switch(direction)
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
                    emit error(QString("plantSeed() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
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
            Direction direction=getLastDirection();
            switch(direction)
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
                            emit error(QString("plantSeed() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
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
    if(CommonDatapack::commonDatapack.items.item[objectId].price==0)
    {
        out << (quint8)BuyStat_HaveNotQuantity;
        emit postReply(query_id,outputData);
        return;
    }
    if(CommonDatapack::commonDatapack.items.item[objectId].price>price)
    {
        out << (quint8)BuyStat_PriceHaveChanged;
        emit postReply(query_id,outputData);
        return;
    }
    if(CommonDatapack::commonDatapack.items.item[objectId].price<price)
    {
        out << (quint8)BuyStat_BetterPrice;
        out << (quint32)CommonDatapack::commonDatapack.items.item[objectId].price;
    }
    else
        out << (quint8)BuyStat_Done;
    if(player_informations->public_and_private_informations.cash>=(CommonDatapack::commonDatapack.items.item[objectId].price*quantity))
        removeCash(CommonDatapack::commonDatapack.items.item[objectId].price*quantity);
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
    Direction direction=getLastDirection();
    switch(direction)
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
                    emit error(QString("plantSeed() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
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
            Direction direction=getLastDirection();
            switch(direction)
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
                            emit error(QString("plantSeed() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
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
    if(!CommonDatapack::commonDatapack.items.item.contains(objectId))
    {
        emit error("this item don't exists");
        return;
    }
    if(objectQuantity(objectId)<quantity)
    {
        emit error("you have not this quantity to sell");
        return;
    }
    quint32 realPrice=CommonDatapack::commonDatapack.items.item[objectId].price/2;
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

bool operator==(const CatchChallenger::MonsterDrops &monsterDrops1,const CatchChallenger::MonsterDrops &monsterDrops2)
{
    if(monsterDrops1.item!=monsterDrops2.item)
        return false;
    if(monsterDrops1.luck!=monsterDrops2.luck)
        return false;
    if(monsterDrops1.quantity_min!=monsterDrops2.quantity_min)
        return false;
    if(monsterDrops1.quantity_max!=monsterDrops2.quantity_max)
        return false;
    return true;
}

void LocalClientHandler::appendAllow(const ActionAllow &allow)
{
    if(player_informations->public_and_private_informations.allow.contains(allow))
        return;
    player_informations->public_and_private_informations.allow << allow;
    updateAllow();
}

void LocalClientHandler::removeAllow(const ActionAllow &allow)
{
    if(!player_informations->public_and_private_informations.allow.contains(allow))
        return;
    player_informations->public_and_private_informations.allow.remove(allow);
    updateAllow();
}

void LocalClientHandler::updateAllow()
{
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            emit dbQuery(QString("UPDATE player SET allow='%1' WHERE id=%2;")
                         .arg(FacilityLib::allowToQString(player_informations->public_and_private_informations.allow))
                         .arg(player_informations->id)
                         );
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            emit dbQuery(QString("UPDATE player SET allow='%1' WHERE id=%2;")
                         .arg(FacilityLib::allowToQString(player_informations->public_and_private_informations.allow))
                         .arg(player_informations->id)
                         );
        break;
    }
}

//reputation
void LocalClientHandler::appendReputationPoint(const QString &type,const qint32 &point)
{
    if(point==0)
        return;
    if(!CommonDatapack::commonDatapack.reputation.contains(type))
    {
        emit error(QString("Unknow reputation: %1").arg(type));
        return;
    }
    PlayerReputation playerReputation;
    playerReputation.point=0;
    playerReputation.level=0;
    if(player_informations->public_and_private_informations.reputation.contains(type))
        playerReputation=player_informations->public_and_private_informations.reputation[type];
    #ifdef DEBUG_MESSAGE_CLIENT_REPUTATION
    emit message(QString("Reputation %1 at level: %2 with point: %3").arg(type).arg(playerReputation.level).arg(playerReputation.point));
    #endif
    playerReputation.point+=point;
    do
    {
        if(playerReputation.level<0 && playerReputation.point>0)
        {
            playerReputation.level++;
            playerReputation.point+=CommonDatapack::commonDatapack.reputation[type].reputation_negative.at(-playerReputation.level);
            continue;
        }
        if(playerReputation.level>0 && playerReputation.point<0)
        {
            playerReputation.level--;
            playerReputation.point+=CommonDatapack::commonDatapack.reputation[type].reputation_negative.at(playerReputation.level);
            continue;
        }
        if(playerReputation.level<=0 && playerReputation.point<CommonDatapack::commonDatapack.reputation[type].reputation_negative.at(-playerReputation.level))
        {
            if((-playerReputation.level)<CommonDatapack::commonDatapack.reputation[type].reputation_negative.size())
            {
                playerReputation.point-=CommonDatapack::commonDatapack.reputation[type].reputation_negative.at(-playerReputation.level);
                playerReputation.level--;
            }
            else
            {
                #ifdef DEBUG_MESSAGE_CLIENT_REPUTATION
                emit message(QString("Reputation %1 at level max: %2").arg(type).arg(playerReputation.level));
                #endif
                playerReputation.point=CommonDatapack::commonDatapack.reputation[type].reputation_negative.at(-playerReputation.level);
            }
            continue;
        }
        if(playerReputation.level>=0 && playerReputation.point<CommonDatapack::commonDatapack.reputation[type].reputation_positive.at(playerReputation.level))
        {
            if(playerReputation.level<CommonDatapack::commonDatapack.reputation[type].reputation_negative.size())
            {
                playerReputation.point-=CommonDatapack::commonDatapack.reputation[type].reputation_negative.at(playerReputation.level);
                playerReputation.level++;
            }
            else
            {
                #ifdef DEBUG_MESSAGE_CLIENT_REPUTATION
                emit message(QString("Reputation %1 at level max: %2").arg(type).arg(playerReputation.level));
                #endif
                playerReputation.point=CommonDatapack::commonDatapack.reputation[type].reputation_negative.at(playerReputation.level);
            }
            continue;
        }
    } while(false);
    if(!player_informations->public_and_private_informations.reputation.contains(type))
    {
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
            emit dbQuery(QString("INSERT INTO reputation(player,type,point,level) VALUES(%1,\"%2\",%3,%4);")
                             .arg(player_informations->id)
                             .arg(SqlFunction::quoteSqlVariable(type))
                             .arg(playerReputation.point)
                             .arg(playerReputation.level)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QString("INSERT INTO reputation(player,type,point,level) VALUES(%1,\"%2\",%3,%4);")
                             .arg(player_informations->id)
                             .arg(SqlFunction::quoteSqlVariable(type))
                             .arg(playerReputation.point)
                             .arg(playerReputation.level)
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
                emit dbQuery(QString("UPDATE reputation SET point=%3,level=%4 WHERE player=%1 AND type=\"%2\";")
                             .arg(player_informations->id)
                             .arg(SqlFunction::quoteSqlVariable(type))
                             .arg(playerReputation.point)
                             .arg(playerReputation.level)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QString("UPDATE reputation SET point=%3,level=%4 WHERE player=%1 AND type=\"%2\";")
                             .arg(player_informations->id)
                             .arg(SqlFunction::quoteSqlVariable(type))
                             .arg(playerReputation.point)
                             .arg(playerReputation.level)
                             );
            break;
        }
    }
    player_informations->public_and_private_informations.reputation[type]=playerReputation;
    #ifdef DEBUG_MESSAGE_CLIENT_REPUTATION
    emit message(QString("New reputation %1 at level: %2 with point: %3").arg(type).arg(playerReputation.level).arg(playerReputation.point));
    #endif
}

void LocalClientHandler::battleCanceled()
{
    localClientHandlerFight.battleCanceled();
}

void LocalClientHandler::battleAccepted()
{
    localClientHandlerFight.battleAccepted();
}

void LocalClientHandler::newRandomNumber(const QByteArray &randomData)
{
    localClientHandlerFight.newRandomNumber(randomData);
}

bool LocalClientHandler::tryEscape()
{
    if(localClientHandlerFight.canEscape())
        return localClientHandlerFight.tryEscape();
    else
    {
        emit error("Try escape when not allowed");
        return false;
    }
}

void LocalClientHandler::heal()
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message(QString("ask heal at %1 (%2,%3)").arg(this->map->map_file).arg(this->x).arg(this->y));
    #endif
    Map *map=this->map;
    quint8 x=this->x;
    quint8 y=this->y;
    //resolv the object
    Direction direction=getLastDirection();
    switch(direction)
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
                    emit error(QString("plantSeed() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
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
        emit error("Wrong direction to use a heal");
        return;
    }
    //check if is shop
    if(!static_cast<MapServer*>(this->map)->heal.contains(QPair<quint8,quint8>(x,y)))
    {
        Direction direction=getLastDirection();
        switch(direction)
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
                        emit error(QString("plantSeed() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
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
            emit error("Wrong direction to use a heal");
            return;
        }
        if(!static_cast<MapServer*>(this->map)->heal.contains(QPair<quint8,quint8>(x,y)))
        {
            emit error("no heal point in this direction");
            return;
        }
    }
    //send the shop items (no taxes from now)
    localClientHandlerFight.healAllMonsters();
    player_informations->rescue=player_informations->unvalidated_rescue;
}

void LocalClientHandler::useSkill(const quint32 &skill)
{
    localClientHandlerFight.useSkill(skill);
}

void LocalClientHandler::requestFight(const quint32 &fightId)
{
    if(localClientHandlerFight.isInFight())
    {
        emit error(QString("error: map: %1 (%2,%3), is in fight").arg(static_cast<MapServer *>(map)->map_file).arg(x).arg(y));
        return;
    }
    if(captureCityInProgress())
    {
        emit error("Try requestFight when is in capture city");
        return;
    }
    if(player_informations->isFake)
        return;
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message(QString("request fight at %1 (%2,%3)").arg(this->map->map_file).arg(this->x).arg(this->y));
    #endif
    Map *map=this->map;
    quint8 x=this->x;
    quint8 y=this->y;
    //resolv the object
    Direction direction=getLastDirection();
    switch(direction)
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
                    emit error(QString("plantSeed() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
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
    bool found=false;
    if(static_cast<MapServer*>(this->map)->botsFight.contains(QPair<quint8,quint8>(x,y)))
    {
        const QList<quint32> &botsFightList=static_cast<MapServer*>(this->map)->botsFight.values(QPair<quint8,quint8>(x,y));
        if(botsFightList.contains(fightId))
            found=true;
    }
    if(!found)
    {
        Direction direction=getLastDirection();
        switch(direction)
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
                        emit error(QString("plantSeed() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
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
        if(static_cast<MapServer*>(this->map)->botsFight.contains(QPair<quint8,quint8>(x,y)))
        {
            const QList<quint32> &botsFightList=static_cast<MapServer*>(this->map)->botsFight.values(QPair<quint8,quint8>(x,y));
            if(botsFightList.contains(fightId))
                found=true;
        }
        if(!found)
        {
            emit error(QString("no fight with id %1 in this direction").arg(fightId));
            return;
        }
    }
    emit message(QString("is now in fight (after a request) on map %1 (%2,%3) with the bot %4").arg(map->map_file).arg(x).arg(y).arg(fightId));
    localClientHandlerFight.requestFight(fightId);
}

void LocalClientHandler::clanAction(const quint8 &query_id,const quint8 &action,const QString &text)
{
    switch(action)
    {
        case 0x01:
        {
            if(player_informations->public_and_private_informations.clan>0)
            {
                emit error("You are already in clan");
                return;
            }
            if(text.isEmpty())
            {
                emit error("You can't create clan with empty name");
                return;
            }
            if(!player_informations->public_and_private_informations.allow.contains(ActionAllow_Clan))
            {
                emit error("You have not the right to create clan");
                return;
            }
            GlobalServerData::serverPrivateVariables.maxClanId++;
            playerByClan[player_informations->public_and_private_informations.clan].name=text;
            player_informations->public_and_private_informations.clan_leader=true;
            //send the network reply
            QByteArray outputData;
            QDataStream out(&outputData, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_4_4);
            out << (quint8)0x01;
            out << (quint32)GlobalServerData::serverPrivateVariables.maxClanId;
            emit postReply(query_id,outputData);
            //add into db
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    emit dbQuery(QString("INSERT INTO clan(id,name) VALUES(%1,\"%2\");")
                             .arg(GlobalServerData::serverPrivateVariables.maxClanId)
                             .arg(SqlFunction::quoteSqlVariable(text))
                             );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    emit dbQuery(QString("INSERT INTO clan(id,name) VALUES(%1,\"%2\");")
                             .arg(GlobalServerData::serverPrivateVariables.maxClanId)
                             .arg(SqlFunction::quoteSqlVariable(text))
                             );
                break;
            }
            insertIntoAClan(GlobalServerData::serverPrivateVariables.maxClanId);
        }
        break;
        case 0x02:
        {
            if(player_informations->public_and_private_informations.clan==0)
            {
                emit error("You have not a clan");
                return;
            }
            if(player_informations->public_and_private_informations.clan_leader)
            {
                emit error("You can't leave if you are the leader");
                return;
            }
            playerByClan[player_informations->public_and_private_informations.clan].players.removeOne(this);
            if(playerByClan[player_informations->public_and_private_informations.clan].players.isEmpty())
                playerByClan.remove(player_informations->public_and_private_informations.clan);
            player_informations->public_and_private_informations.clan=0;
            emit clanChange(player_informations->public_and_private_informations.clan);
            //send the network reply
            QByteArray outputData;
            QDataStream out(&outputData, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_4_4);
            out << (quint8)0x01;
            emit postReply(query_id,outputData);
            //update the db
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    emit dbQuery(QString("UPDATE player SET clan=0 WHERE id=%1;")
                             .arg(player_informations->id)
                             );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    emit dbQuery(QString("UPDATE player SET clan=0 WHERE id=%1;")
                             .arg(player_informations->id)
                             );
                break;
            }
        }
        break;
        case 0x03:
        {
            if(player_informations->public_and_private_informations.clan==0)
            {
                emit error("You have not a clan");
                return;
            }
            if(!player_informations->public_and_private_informations.clan_leader)
            {
                emit error("You are not a leader to dissolve the clan");
                return;
            }
            const QList<LocalClientHandler *> &players=playerByClan[player_informations->public_and_private_informations.clan].players;
            //send the network reply
            QByteArray outputData;
            QDataStream out(&outputData, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_4_4);
            out << (quint8)0x01;
            emit postReply(query_id,outputData);
            //update the db
            int index=0;
            while(index<players.size())
            {
                switch(GlobalServerData::serverSettings.database.type)
                {
                    default:
                    case ServerSettings::Database::DatabaseType_Mysql:
                        emit dbQuery(QString("UPDATE player SET clan=0 WHERE id=%1;")
                                 .arg(players.at(index)->getPlayerId())
                                 );
                    break;
                    case ServerSettings::Database::DatabaseType_SQLite:
                        emit dbQuery(QString("UPDATE player SET clan=0 WHERE id=%1;")
                                 .arg(players.at(index)->getPlayerId())
                                 );
                    break;
                }
                index++;
            }
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    emit dbQuery(QString("DELETE FROM clan WHERE id=%1")
                                 .arg(player_informations->public_and_private_informations.clan)
                                 );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    emit dbQuery(QString("DELETE FROM clan WHERE id=%1")
                                 .arg(player_informations->public_and_private_informations.clan)
                                 );
                break;
            }
            //update the object
            playerByClan.remove(player_informations->public_and_private_informations.clan);
            index=0;
            while(index<players.size())
            {
                if(players.at(index)==this)
                {
                    player_informations->public_and_private_informations.clan=0;
                    emit clanChange(player_informations->public_and_private_informations.clan);
                }
                else
                    players.at(index)->dissolvedClan();
                index++;
            }
        }
        break;
        case 0x04:
        {
            if(player_informations->public_and_private_informations.clan==0)
            {
                emit error("You have not a clan");
                return;
            }
            if(!player_informations->public_and_private_informations.clan_leader)
            {
                emit error("You are not a leader to invite into the clan");
                return;
            }
            bool haveAClan=true;
            if(playerByPseudo.contains(text))
                if(!playerByPseudo[text]->haveAClan())
                    haveAClan=false;
            bool isFound=playerByPseudo.contains(text);
            //send the network reply
            QByteArray outputData;
            QDataStream out(&outputData, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_4_4);
            if(isFound && !haveAClan)
            {
                if(playerByPseudo[text]->inviteToClan(player_informations->public_and_private_informations.clan))
                    out << (quint8)0x01;
                else
                    out << (quint8)0x02;
            }
            else
            {
                if(!isFound)
                    emit message(QString("Clan invite: Player %1 not found, is connected?").arg(text));
                if(haveAClan)
                    emit message(QString("Clan invite: Player %1 is already into a clan").arg(text));
                out << (quint8)0x02;
            }
            emit postReply(query_id,outputData);
        }
        break;
        case 0x05:
        {
            if(player_informations->public_and_private_informations.clan==0)
            {
                emit error("You have not a clan");
                return;
            }
            if(!player_informations->public_and_private_informations.clan_leader)
            {
                emit error("You are not a leader to invite into the clan");
                return;
            }
            if(player_informations->public_and_private_informations.public_informations.pseudo==text)
            {
                emit error("You can't eject your self");
                return;
            }
            bool isIntoTheClan=false;
            if(playerByPseudo.contains(text))
                if(playerByPseudo[text]->getClanId()==player_informations->public_and_private_informations.clan)
                    isIntoTheClan=true;
            bool isFound=playerByPseudo.contains(text);
            //send the network reply
            QByteArray outputData;
            QDataStream out(&outputData, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_4_4);
            if(isFound && isIntoTheClan)
                out << (quint8)0x01;
            else
            {
                if(!isFound)
                    emit message(QString("Clan invite: Player %1 not found, is connected?").arg(text));
                if(!isIntoTheClan)
                    emit message(QString("Clan invite: Player %1 is not into your clan").arg(text));
                out << (quint8)0x02;
            }
            emit postReply(query_id,outputData);
            if(!isFound)
            {
                switch(GlobalServerData::serverSettings.database.type)
                {
                    default:
                    case ServerSettings::Database::DatabaseType_Mysql:
                        emit dbQuery(QString("UPDATE player SET clan=0 WHERE pseudo=%1 AND clan=%2;")
                                 .arg(text)
                                 .arg(player_informations->public_and_private_informations.clan)
                                 );
                    break;
                    case ServerSettings::Database::DatabaseType_SQLite:
                        emit dbQuery(QString("UPDATE player SET clan=0 WHERE pseudo=%1 AND clan=%2;")
                                 .arg(text)
                                 .arg(player_informations->public_and_private_informations.clan)
                                 );
                    break;
                }
                return;
            }
            else if(isIntoTheClan)
                playerByPseudo[text]->ejectToClan();
        }
        break;
        default:
            emit error("Action on the clan not found");
        return;
    }
}

quint32 LocalClientHandler::getPlayerId() const
{
    if(player_informations->is_logged)
        return player_informations->id;
    else
        return 0;
}

void LocalClientHandler::haveClanInfo(const QString &clanName)
{
    if(playerByClan.contains(player_informations->public_and_private_informations.clan))
        playerByClan[player_informations->public_and_private_informations.clan].name=clanName;
    sendClanInfo();
}

void LocalClientHandler::sendClanInfo()
{
    if(player_informations->public_and_private_informations.clan==0)
        return;
    if(!playerByClan.contains(player_informations->public_and_private_informations.clan))
        return;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << playerByClan[player_informations->public_and_private_informations.clan].name;
    emit sendFullPacket(0xC2,0x000A,outputData);
}

void LocalClientHandler::dissolvedClan()
{
    player_informations->public_and_private_informations.clan=0;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    emit sendFullPacket(0xC2,0x0009,QByteArray());
    emit clanChange(player_informations->public_and_private_informations.clan);
}

bool LocalClientHandler::inviteToClan(const quint32 &clanId)
{
    if(!inviteToClanList.isEmpty())
        return false;
    inviteToClanList << clanId;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint32)clanId;
    out << playerByClan[clanId].name;
    emit sendFullPacket(0xC2,0x000B,outputData);
    return false;
}

void LocalClientHandler::clanInvite(const bool &accept)
{
    if(!accept)
    {
        emit message("You have refused the clan invitation");
        inviteToClanList.removeFirst();
        return;
    }
    emit message("You have accepted the clan invitation");
    if(inviteToClanList.isEmpty())
    {
        emit error("Can't responde to clan invite, because no in suspend");
        return;
    }
    player_informations->public_and_private_informations.clan_leader=false;
    insertIntoAClan(inviteToClanList.first());
    inviteToClanList.removeFirst();
}

quint32 LocalClientHandler::clanId() const
{
    return player_informations->public_and_private_informations.clan;
}

void LocalClientHandler::insertIntoAClan(const quint32 &clanId)
{
    player_informations->public_and_private_informations.clan=clanId;
    playerByClan[clanId].players << this;
    //add into db
    QString clan_leader;
    if(player_informations->public_and_private_informations.clan_leader)
        clan_leader="1";
    else
        clan_leader="0";
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            emit dbQuery(QString("UPDATE player SET clan=%1,clan_leader=%2 WHERE id=%3;")
                     .arg(clanId)
                     .arg(clan_leader)
                     .arg(player_informations->id)
                     );
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            emit dbQuery(QString("UPDATE player SET clan=%1,clan_leader=%2 WHERE id=%3;")
                     .arg(clanId)
                     .arg(clan_leader)
                     .arg(player_informations->id)
                     );
        break;
    }
    sendClanInfo();
    emit clanChange(player_informations->public_and_private_informations.clan);
}

void LocalClientHandler::ejectToClan()
{
    dissolvedClan();
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            emit dbQuery(QString("UPDATE player SET clan=0 WHERE id=%1;")
                     .arg(player_informations->id)
                     );
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            emit dbQuery(QString("UPDATE player SET clan=0 WHERE id=%1;")
                     .arg(player_informations->id)
                     );
        break;
    }
}

quint32 LocalClientHandler::getClanId() const
{
    return player_informations->public_and_private_informations.clan;
}

bool LocalClientHandler::haveAClan() const
{
    return player_informations->public_and_private_informations.clan>0;
}

void LocalClientHandler::waitingForCityCaputre(const bool &cancel)
{
    if(clan==NULL)
    {
        emit error("Try capture city when is not in clan");
        return;
    }
    if(!cancel)
    {
        if(captureCityInProgress())
        {
            emit error("Try capture city when is already into that's");
            return;
        }
        if(!localClientHandlerFight.isInFight())
        {
            emit error("Try capture city when is in fight");
            return;
        }
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        emit message(QString("ask zonecapture at %1 (%2,%3)").arg(this->map->map_file).arg(this->x).arg(this->y));
        #endif
        Map *map=this->map;
        quint8 x=this->x;
        quint8 y=this->y;
        //resolv the object
        Direction direction=getLastDirection();
        switch(direction)
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
                        emit error(QString("plantSeed() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
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
            emit error("Wrong direction to use a zonecapture");
            return;
        }
        //check if is shop
        if(!static_cast<MapServer*>(this->map)->zonecapture.contains(QPair<quint8,quint8>(x,y)))
        {
            Direction direction=getLastDirection();
            switch(direction)
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
                            emit error(QString("plantSeed() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
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
                emit error("Wrong direction to use a zonecapture");
                return;
            }
            if(!static_cast<MapServer*>(this->map)->zonecapture.contains(QPair<quint8,quint8>(x,y)))
            {
                emit error("no zonecapture point in this direction");
                return;
            }
        }
        //send the shop items (no taxes from now)
        const QString &zoneName=static_cast<MapServer*>(this->map)->zonecapture[QPair<quint8,quint8>(x,y)];
        if(!player_informations->public_and_private_informations.clan_leader)
        {
            if(clan->captureCityInProgress.isEmpty())
            {
                QByteArray outputData;
                QDataStream out(&outputData, QIODevice::WriteOnly);
                out.setVersion(QDataStream::Qt_4_4);
                out << (quint8)0x01;
                emit sendFullPacket(0xF0,0x0001,outputData);
                return;
            }
        }
        else
        {
            if(clan->captureCityInProgress.isEmpty())
                clan->captureCityInProgress=zoneName;
        }
        if(clan->captureCityInProgress!=zoneName)
        {
            QByteArray outputData;
            QDataStream out(&outputData, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_4_4);
            out << (quint8)0x02;
            out << clan->captureCityInProgress;
            emit sendFullPacket(0xF0,0x0002,outputData);
            return;
        }
        if(captureCity.count(zoneName)>0)
        {
            emit error("already in capture city");
            return;
        }
        captureCity[zoneName].insert(clan->clanId,this);
    }
    else
    {
        if(clan->captureCityInProgress.isEmpty())
        {
            emit error("your clan is not in capture city");
            return;
        }
        int number_removed=captureCity[clan->captureCityInProgress].remove(clan->clanId,this);
        if(number_removed==0)
        {
            emit error("not in capture city");
            return;
        }
        if(captureCity[clan->captureCityInProgress].count(clan->clanId)==0)
        {
            captureCity[clan->captureCityInProgress].remove(clan->clanId);
            if(captureCity[clan->captureCityInProgress].count()==0)
                captureCity.remove(clan->captureCityInProgress);
        }
    }
}

void LocalClientHandler::starttheCityCapture()
{
}
