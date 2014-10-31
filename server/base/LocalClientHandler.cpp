#include "Client.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/FacilityLib.h"
#include "SqlFunction.h"
#include "MapServer.h"

#include "GlobalServerData.h"

#include <QStringList>

using namespace CatchChallenger;

bool Client::checkCollision()
{
    if(map->parsed_layer.walkable==NULL)
        return false;
    if(!map->parsed_layer.walkable[x+y*map->width])
    {
        errorOutput(QStringLiteral("move at %1, can't wall at: %2,%3 on map: %4").arg(temp_direction).arg(x).arg(y).arg(map->map_file));
        return false;
    }
    else
        return true;
}

void Client::removeFromClan()
{
    clanChangeWithoutDb(0);
}

QString Client::directionToStringToSave(const Direction &direction)
{
    switch(direction)
    {
        case Direction_look_at_top:
        case Direction_move_at_top:
            return Client::text_top;
        break;
        case Direction_look_at_right:
        case Direction_move_at_right:
            return Client::text_right;
        break;
        case Direction_look_at_bottom:
        case Direction_move_at_bottom:
            return Client::text_bottom;
        break;
        case Direction_look_at_left:
        case Direction_move_at_left:
            return Client::text_left;
        break;
        default:
        break;
    }
    return Client::text_bottom;
}

QString Client::orientationToStringToSave(const Orientation &orientation)
{
    switch(orientation)
    {
        case Orientation_top:
            return Client::text_top;
        break;
        case Orientation_bottom:
            return Client::text_bottom;
        break;
        case Orientation_right:
            return Client::text_right;
        break;
        case Orientation_left:
            return Client::text_left;
        break;
        default:
        break;
    }
    return Client::text_bottom;
}

void Client::savePosition()
{
    //virtual stop the player
    //Orientation orientation;
    #ifdef DEBUG_MESSAGE_CLIENT_MOVE
    normalOutput(
                QLatin1String("map->map_file: %1,x: %2,y: %3, orientation: %4")
                .arg(map->map_file)
                .arg(x)
                .arg(y)
                .arg(orientationString)
                );
    #endif
    /* disable because use memory, but useful only into less than < 0.1% of case
     * if(map!=at_start_map_name || x!=at_start_x || y!=at_start_y || orientation!=at_start_orientation) */
    QString updateMapPositionQuery;
    const quint32 &map_file_database_id=static_cast<MapServer *>(map)->reverse_db_id;
    const quint32 &rescue_map_file_database_id=static_cast<MapServer *>(rescue.map)->reverse_db_id;
    const quint32 &unvalidated_rescue_map_file_database_id=static_cast<MapServer *>(unvalidated_rescue.map)->reverse_db_id;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            updateMapPositionQuery=QStringLiteral("UPDATE `character` SET `map`=%1,`x`=%2,`y`=%3,`orientation`=%4,%5 WHERE `id`=%6")
                .arg(map_file_database_id)
                .arg(x)
                .arg(y)
                .arg((quint8)getLastDirection())
                .arg(
                        QStringLiteral("`rescue_map`=%1,`rescue_x`=%2,`rescue_y`=%3,`rescue_orientation`=%4,`unvalidated_rescue_map`=%5,`unvalidated_rescue_x`=%6,`unvalidated_rescue_y`=%7,`unvalidated_rescue_orientation`=%8")
                        .arg(rescue_map_file_database_id)
                        .arg(rescue.x)
                        .arg(rescue.y)
                        .arg((quint8)rescue.orientation)
                        .arg(unvalidated_rescue_map_file_database_id)
                        .arg(unvalidated_rescue.x)
                        .arg(unvalidated_rescue.y)
                        .arg((quint8)unvalidated_rescue.orientation)
                )
                .arg(character_id);
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            updateMapPositionQuery=QStringLiteral("UPDATE character SET map=%1,x=%2,y=%3,orientation=%4,%5 WHERE id=%6")
                .arg(map_file_database_id)
                .arg(x)
                .arg(y)
                .arg((quint8)getLastDirection())
                .arg(
                        QStringLiteral("rescue_map=%1,rescue_x=%2,rescue_y=%3,rescue_orientation=%4,unvalidated_rescue_map=%5,unvalidated_rescue_x=%6,unvalidated_rescue_y=%7,unvalidated_rescue_orientation=%8")
                        .arg(rescue_map_file_database_id)
                        .arg(rescue.x)
                        .arg(rescue.y)
                        .arg((quint8)rescue.orientation)
                        .arg(unvalidated_rescue_map_file_database_id)
                        .arg(unvalidated_rescue.x)
                        .arg(unvalidated_rescue.y)
                        .arg((quint8)unvalidated_rescue.orientation)
                )
                .arg(character_id);
        break;
        case ServerSettings::Database::DatabaseType_PostgreSQL:
            updateMapPositionQuery=QStringLiteral("UPDATE character SET map=%1,x=%2,y=%3,orientation=%4,%5 WHERE id=%6")
                .arg(map_file_database_id)
                .arg(x)
                .arg(y)
                .arg((quint8)getLastDirection())
                .arg(
                        QStringLiteral("rescue_map=%1,rescue_x=%2,rescue_y=%3,rescue_orientation=%4,unvalidated_rescue_map=%5,unvalidated_rescue_x=%6,unvalidated_rescue_y=%7,unvalidated_rescue_orientation=%8")
                        .arg(rescue_map_file_database_id)
                        .arg(rescue.x)
                        .arg(rescue.y)
                        .arg((quint8)rescue.orientation)
                        .arg(unvalidated_rescue_map_file_database_id)
                        .arg(unvalidated_rescue.x)
                        .arg(unvalidated_rescue.y)
                        .arg((quint8)unvalidated_rescue.orientation)
                )
                .arg(character_id);
        break;
    }
    dbQueryWrite(updateMapPositionQuery);
/* do at moveDownMonster and moveUpMonster
 *     const QList<PlayerMonster> &playerMonsterList=getPlayerMonster();
    int index=0;
    while(index<playerMonsterList.size())
    {
        const PlayerMonster &playerMonster=playerMonsterList.at(index);
        saveMonsterPosition(playerMonster.id,index+1);
        index++;
    }*/
}

/* why do that's here?
 * Because the ClientMapManagement can be totaly satured by the square complexity
 * that's allow to continue the player to connect and play
 * the overhead for the network it just at the connexion */
void Client::put_on_the_map(CommonMap *map,const COORD_TYPE &x,const COORD_TYPE &y,const Orientation &orientation)
{
    MapBasicMove::put_on_the_map(map,x,y,orientation);
    insertClientOnMap(map);

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
        out << (quint8)public_and_private_informations.public_informations.simplifiedId;
    }
    else
    {
        out << (quint16)0x0001;
        out << (quint16)public_and_private_informations.public_informations.simplifiedId;
    }
    out << x;
    out << y;
    if(GlobalServerData::serverSettings.dontSendPlayerType)
        out << quint8((quint8)orientation | (quint8)Player_type_normal);
    else
        out << quint8((quint8)orientation | (quint8)public_and_private_informations.public_informations.type);
    if(CommonSettings::commonSettings.forcedSpeed==0)
        out << public_and_private_informations.public_informations.speed;

    if(!CommonSettings::commonSettings.dontSendPseudo)
    {
        outputData+=rawPseudo;
        out.device()->seek(out.device()->pos()+rawPseudo.size());
    }
    out << public_and_private_informations.public_informations.skinId;

    sendPacket(0xC0,outputData);

    //load the first time the random number list
    generateRandomNumber();

    playerByPseudo[public_and_private_informations.public_informations.pseudo]=this;
    playerById[character_id]=this;
    if(public_and_private_informations.clan>0)
        sendClanInfo();

    updateCanDoFight();
    if(getAbleToFight())
        botFightCollision(map,x,y);
    else if(haveMonsters())
        checkLoose();

    int index=0;
    while(index<GlobalServerData::serverPrivateVariables.server_message.size())
    {
        receiveSystemText(GlobalServerData::serverPrivateVariables.server_message.at(index));
        index++;
    }
}

void Client::createMemoryClan()
{
    if(!clanList.contains(public_and_private_informations.clan))
    {
        clan=new Clan;
        clan->cash=0;
        clan->clanId=public_and_private_informations.clan;
        clanList[public_and_private_informations.clan]=clan;
        if(GlobalServerData::serverPrivateVariables.cityStatusListReverse.contains(clan->clanId))
            clan->capturedCity=GlobalServerData::serverPrivateVariables.cityStatusListReverse.value(clan->clanId);
    }
    else
        clan=clanList.value(public_and_private_informations.clan);
    clan->players << this;
}

bool Client::moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(this->x>=this->map->width)
    {
        qDebug() << QStringLiteral("x to out of map: %1 > %2 (%3)").arg(this->x).arg(this->map->width).arg(this->map->map_file);
        abort();
        return false;
    }
    if(this->y>=this->map->height)
    {
        qDebug() << QStringLiteral("y to out of map: %1 > %2 (%3)").arg(this->y).arg(this->map->height).arg(this->map->map_file);
        abort();
        return false;
    }
    #endif

    #ifdef DEBUG_MESSAGE_CLIENT_MOVE
    normalOutput(QStringLiteral("moveThePlayer(): for player (%1,%2): %3, previousMovedUnit: %4 (%5), next direction: %6")
                 .arg(x)
                 .arg(y)
                 .arg(public_and_private_informations.public_informations.simplifiedId)
                 .arg(previousMovedUnit)
                 .arg(MoveOnTheMap::directionToString(getLastDirection()))
                 .arg(MoveOnTheMap::directionToString(direction))
                 );
    #endif
    const bool &returnValue=MapBasicMove::moveThePlayer(previousMovedUnit,direction);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(returnValue)
    {
        if(this->x>=this->map->width)
        {
            qDebug() << QStringLiteral("x to out of map: %1 > %2 (%3)").arg(this->x).arg(this->map->width).arg(this->map->map_file);
            abort();
            return false;
        }
        if(this->y>=this->map->height)
        {
            qDebug() << QStringLiteral("y to out of map: %1 > %2 (%3)").arg(this->y).arg(this->map->height).arg(this->map->map_file);
            abort();
            return false;
        }
    }
    #endif
    return returnValue;
}

bool Client::captureCityInProgress()
{
    if(clan==NULL)
        return false;
    if(clan->captureCityInProgress.isEmpty())
        return false;
    //search in capture not validated
    if(captureCity.contains(clan->captureCityInProgress))
        if(captureCity.value(clan->captureCityInProgress).contains(this))
            return true;
    //search in capture validated
    if(captureCityValidatedList.contains(clan->captureCityInProgress))
    {
        const CaptureCityValidated &captureCityValidated=captureCityValidatedList.value(clan->captureCityInProgress);
        //check if this player is into the capture city with the other player of the team
        if(captureCityValidated.players.contains(this))
            return true;
    }
    return false;
}

bool Client::singleMove(const Direction &direction)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(this->x>=this->map->width)
    {
        qDebug() << QStringLiteral("x to out of map: %1 > %2 (%3)").arg(this->x).arg(this->map->width).arg(this->map->map_file);
        abort();
        return false;
    }
    if(this->y>=this->map->height)
    {
        qDebug() << QStringLiteral("y to out of map: %1 > %2 (%3)").arg(this->y).arg(this->map->height).arg(this->map->map_file);
        abort();
        return false;
    }
    #endif

    if(isInFight())//check if is in fight
    {
        errorOutput(QStringLiteral("error: try move when is in fight"));
        return false;
    }
    if(captureCityInProgress())
    {
        errorOutput(QStringLiteral("Try move when is in capture city"));
        return false;
    }
    if(!oldEvents.oldEventList.isEmpty() && (QDateTime::currentDateTime().toTime_t()-oldEvents.time.toTime_t())>30/*30s*/)
    {
        errorOutput(QStringLiteral("Try move but lost of event sync"));
        return false;
    }
    COORD_TYPE x=this->x,y=this->y;
    temp_direction=direction;
    CommonMap* map=this->map;
    #ifdef DEBUG_MESSAGE_CLIENT_MOVE
    normalOutput(QStringLiteral("Client::singleMove(), go in this direction: %1 with map: %2(%3,%4)").arg(MoveOnTheMap::directionToString(direction)).arg(map->map_file).arg(x).arg(y));
    #endif
    if(!MoveOnTheMap::canGoTo(direction,*map,x,y,true))
    {
        errorOutput(QStringLiteral("Client::singleMove(), can't go into this direction: %1 with map: %2(%3,%4)").arg(MoveOnTheMap::directionToString(direction)).arg(map->map_file).arg(x).arg(y));
        return false;
    }
    if(!MoveOnTheMap::moveWithoutTeleport(direction,&map,&x,&y,false,true))
    {
        errorOutput(QStringLiteral("Client::singleMove(), can go but move failed into this direction: %1 with map: %2(%3,%4)").arg(MoveOnTheMap::directionToString(direction)).arg(map->map_file).arg(x).arg(y));
        return false;
    }

    if(map->teleporter.contains(x+y*map->width))
    {
        const CommonMap::Teleporter &teleporter=map->teleporter.value(x+y*map->width);
        switch(teleporter.condition.type)
        {
            case CatchChallenger::MapConditionType_None:
            case CatchChallenger::MapConditionType_Clan://not do for now
            break;
            case CatchChallenger::MapConditionType_FightBot:
                if(!public_and_private_informations.bot_already_beaten.contains(teleporter.condition.value))
                {
                    errorOutput(QStringLiteral("Need have FightBot win to use this teleporter: %1 with map: %2(%3,%4)").arg(teleporter.condition.value).arg(map->map_file).arg(x).arg(y));
                    return false;
                }
            break;
            case CatchChallenger::MapConditionType_Item:
                if(!public_and_private_informations.items.contains(teleporter.condition.value))
                {
                    errorOutput(QStringLiteral("Need have item to use this teleporter: %1 with map: %2(%3,%4)").arg(teleporter.condition.value).arg(map->map_file).arg(x).arg(y));
                    return false;
                }
            break;
            case CatchChallenger::MapConditionType_Quest:
                if(!public_and_private_informations.quests.contains(teleporter.condition.value))
                {
                    errorOutput(QStringLiteral("Need have quest to use this teleporter: %1 with map: %2(%3,%4)").arg(teleporter.condition.value).arg(map->map_file).arg(x).arg(y));
                    return false;
                }
                if(!public_and_private_informations.quests.value(teleporter.condition.value).finish_one_time)
                {
                    errorOutput(QStringLiteral("Need have finish the quest to use this teleporter: %1 with map: %2(%3,%4)").arg(teleporter.condition.value).arg(map->map_file).arg(x).arg(y));
                    return false;
                }
            break;
            default:
            break;
        }
        x=teleporter.x;
        y=teleporter.y;
        map=teleporter.map;
    }

    if(this->map!=map)
    {
        CommonMap *new_map=map;
        CommonMap *old_map=this->map;
        this->map=old_map;
        removeClientOnMap(old_map);
        this->map=new_map;
        insertClientOnMap(map);
    }

    this->map=map;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(this->map!=NULL)
        if(this->map->width==999)
            this->map->width=9999;
    #endif
    this->x=x;
    this->y=y;
    if(static_cast<MapServer*>(map)->rescue.contains(QPair<quint8,quint8>(x,y)))
    {
        unvalidated_rescue.map=map;
        unvalidated_rescue.x=x;
        unvalidated_rescue.y=y;
        unvalidated_rescue.orientation=static_cast<MapServer*>(map)->rescue.value(QPair<quint8,quint8>(x,y));
    }
    if(botFightCollision(map,x,y))
        return true;
    if(public_and_private_informations.repel_step<=0)
    {
        //merge the result event:
        QList<quint8> mergedEvents(GlobalServerData::serverPrivateVariables.events);
        if(!oldEvents.oldEventList.isEmpty())
        {
            int index=0;
            while(index<oldEvents.oldEventList.size())
            {
                mergedEvents[oldEvents.oldEventList.at(index).event]=oldEvents.oldEventList.at(index).eventValue;
                index++;
            }
        }
        if(generateWildFightIfCollision(map,x,y,public_and_private_informations.items,mergedEvents))
        {
            normalOutput(QStringLiteral("Client::singleMove(), now is in front of wild monster with map: %1(%2,%3)").arg(map->map_file).arg(x).arg(y));
            return true;
        }
    }
    else
        public_and_private_informations.repel_step--;

    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(this->map!=NULL)
    {
        if(this->x>=this->map->width)
        {
            qDebug() << QStringLiteral("x to out of map: %1 > %2 (%3)").arg(this->x).arg(this->map->width).arg(this->map->map_file);
            abort();
            return false;
        }
        if(this->y>=this->map->height)
        {
            qDebug() << QStringLiteral("y to out of map: %1 > %2 (%3)").arg(this->y).arg(this->map->height).arg(this->map->map_file);
            abort();
            return false;
        }
    }
    #endif

    return true;
}

void Client::addObjectAndSend(const quint16 &item,const quint32 &quantity)
{
    addObject(item,quantity);
    //add into the inventory
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint16)1;
    out << (quint16)item;
    out << (quint32)quantity;
    sendFullPacket(0xD0,0x0002,outputData);
}

void Client::addObject(const quint16 &item,const quint32 &quantity)
{
    if(!CommonDatapack::commonDatapack.items.item.contains(item))
    {
        errorOutput("Object is not found into the item list");
        return;
    }
    if(public_and_private_informations.items.contains(item))
    {
        public_and_private_informations.items[item]+=quantity;
        dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_update_item
                 .arg(public_and_private_informations.items.value(item))
                 .arg(item)
                 .arg(character_id)
                 );
    }
    else
    {
        dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_insert_item
                     .arg(item)
                     .arg(character_id)
                     .arg(quantity)
                     );
        public_and_private_informations.items[item]=quantity;
    }
}

void Client::addWarehouseObject(const quint16 &item,const quint32 &quantity)
{
    if(public_and_private_informations.warehouse_items.contains(item))
    {
        public_and_private_informations.warehouse_items[item]+=quantity;
        dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_update_item_warehouse
                 .arg(public_and_private_informations.items.value(item))
                 .arg(item)
                 .arg(character_id)
                 );
    }
    else
    {
        dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_insert_item_warehouse
                     .arg(item)
                     .arg(character_id)
                     .arg(quantity)
                     );
        public_and_private_informations.warehouse_items[item]=quantity;
    }
}

quint32 Client::removeWarehouseObject(const quint16 &item,const quint32 &quantity)
{
    if(public_and_private_informations.warehouse_items.contains(item))
    {
        if(public_and_private_informations.warehouse_items.value(item)>quantity)
        {
            public_and_private_informations.warehouse_items[item]-=quantity;
            dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_update_item_warehouse
                     .arg(public_and_private_informations.items.value(item))
                     .arg(item)
                     .arg(character_id)
                     );
            return quantity;
        }
        else
        {
            quint32 removed_quantity=public_and_private_informations.warehouse_items.value(item);
            public_and_private_informations.warehouse_items.remove(item);
            dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_delete_item_warehouse
                         .arg(item)
                         .arg(character_id)
                         );
            return removed_quantity;
        }
    }
    else
        return 0;
}

void Client::saveObjectRetention(const quint16 &item)
{
    if(public_and_private_informations.items.contains(item))
    {
        dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_update_item
                 .arg(public_and_private_informations.items.value(item))
                 .arg(item)
                 .arg(character_id)
                 );
    }
    else
    {
        dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_delete_item
                     .arg(item)
                     .arg(character_id)
                     );
    }
}

quint32 Client::removeObject(const quint16 &item, const quint32 &quantity)
{
    if(public_and_private_informations.items.contains(item))
    {
        if(public_and_private_informations.items.value(item)>quantity)
        {
            public_and_private_informations.items[item]-=quantity;
            dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_update_item
                     .arg(public_and_private_informations.items.value(item))
                     .arg(item)
                     .arg(character_id)
                     );
            return quantity;
        }
        else
        {
            quint32 removed_quantity=public_and_private_informations.items.value(item);
            public_and_private_informations.items.remove(item);
            dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_delete_item
                         .arg(item)
                         .arg(character_id)
                         );
            return removed_quantity;
        }
    }
    else
        return 0;
}

void Client::sendRemoveObject(const quint16 &item,const quint32 &quantity)
{
    //add into the inventory
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint32)1;
    out << (quint16)item;
    out << (quint32)quantity;
    sendFullPacket(0xD0,0x0003,outputData);
}

quint32 Client::objectQuantity(const quint16 &item)
{
    if(public_and_private_informations.items.contains(item))
        return public_and_private_informations.items.value(item);
    else
        return 0;
}

bool Client::addMarketCashWithoutSave(const quint64 &cash)
{
    market_cash+=cash;
    return true;
}

void Client::addCash(const quint64 &cash, const bool &forceSave)
{
    if(cash==0 && !forceSave)
        return;
    public_and_private_informations.cash+=cash;
    dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_update_cash
                 .arg(public_and_private_informations.cash)
                 .arg(character_id)
                 );
}

void Client::removeCash(const quint64 &cash)
{
    if(cash==0)
        return;
    public_and_private_informations.cash-=cash;
    dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_update_cash
                 .arg(public_and_private_informations.cash)
                 .arg(character_id)
                 );
}

void Client::addWarehouseCash(const quint64 &cash, const bool &forceSave)
{
    if(cash==0 && !forceSave)
        return;
    public_and_private_informations.warehouse_cash+=cash;
    dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_update_warehouse_cash
                 .arg(public_and_private_informations.warehouse_cash)
                 .arg(character_id)
                 );
}

void Client::removeWarehouseCash(const quint64 &cash)
{
    if(cash==0)
        return;
    public_and_private_informations.warehouse_cash-=cash;
    dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_update_warehouse_cash
                 .arg(public_and_private_informations.warehouse_cash)
                 .arg(character_id)
                 );
}

void Client::wareHouseStore(const qint64 &cash, const QList<QPair<quint16, qint32> > &items, const QList<quint32> &withdrawMonsters, const QList<quint32> &depositeMonsters)
{
    if(!wareHouseStoreCheck(cash,items,withdrawMonsters,depositeMonsters))
        return;
    {
        int index=0;
        while(index<items.size())
        {
            const QPair<quint16, qint32> &item=items.at(index);
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
            while(sub_index<public_and_private_informations.warehouse_playerMonster.size())
            {
                const PlayerMonster &playerMonsterinWarehouse=public_and_private_informations.warehouse_playerMonster.at(sub_index);
                if(playerMonsterinWarehouse.id==withdrawMonsters.at(index))
                {
                        dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_delete_monster_warehouse_by_id.arg(playerMonsterinWarehouse.id));
                        if(CommonSettings::commonSettings.useSP)
                            dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_insert_monster_full
                                     .arg(
                                         QStringLiteral("%1,%2,%3,%4,%5,%6,%7,%8,%9")
                                         .arg(playerMonsterinWarehouse.id)
                                         .arg(playerMonsterinWarehouse.hp)
                                         .arg(character_id)
                                         .arg(playerMonsterinWarehouse.monster)
                                         .arg(playerMonsterinWarehouse.level)
                                         .arg(playerMonsterinWarehouse.remaining_xp)
                                         .arg(playerMonsterinWarehouse.sp)
                                         .arg(playerMonsterinWarehouse.catched_with)
                                         .arg((quint8)playerMonsterinWarehouse.gender)
                                         )
                                     .arg(
                                         QStringLiteral("%1,%2,%3")
                                         .arg(playerMonsterinWarehouse.egg_step)
                                         .arg(playerMonsterinWarehouse.character_origin)
                                         .arg(public_and_private_informations.playerMonster.size()+2)
                                         )
                                     );
                        else
                            dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_insert_monster_full
                                     .arg(
                                         QStringLiteral("%1,%2,%3,%4,%5,%6,%7,%8")
                                         .arg(playerMonsterinWarehouse.id)
                                         .arg(playerMonsterinWarehouse.hp)
                                         .arg(character_id)
                                         .arg(playerMonsterinWarehouse.monster)
                                         .arg(playerMonsterinWarehouse.level)
                                         .arg(playerMonsterinWarehouse.remaining_xp)
                                         .arg(playerMonsterinWarehouse.catched_with)
                                         .arg((quint8)playerMonsterinWarehouse.gender)
                                         )
                                     .arg(
                                         QStringLiteral("%1,%2,%3")
                                         .arg(playerMonsterinWarehouse.egg_step)
                                         .arg(playerMonsterinWarehouse.character_origin)
                                         .arg(public_and_private_informations.playerMonster.size()+2)
                                         )
                                     );
                    public_and_private_informations.playerMonster << public_and_private_informations.warehouse_playerMonster.at(sub_index);
                    public_and_private_informations.warehouse_playerMonster.removeAt(sub_index);
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
            while(sub_index<public_and_private_informations.playerMonster.size())
            {
                const PlayerMonster &playerMonsterOnPlayer=public_and_private_informations.playerMonster.at(sub_index);
                if(playerMonsterOnPlayer.id==depositeMonsters.at(index))
                {
                        dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_delete_monster_by_id.arg(playerMonsterOnPlayer.id));
                        if(CommonSettings::commonSettings.useSP)
                            dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_insert_warehouse_monster_full
                                         .arg(
                                             QStringLiteral("%1,%2,%3,%4,%5,%6,%7,%8,%9")
                                             .arg(playerMonsterOnPlayer.id)
                                             .arg(playerMonsterOnPlayer.hp)
                                             .arg(character_id)
                                             .arg(playerMonsterOnPlayer.monster)
                                             .arg(playerMonsterOnPlayer.level)
                                             .arg(playerMonsterOnPlayer.remaining_xp)
                                             .arg(playerMonsterOnPlayer.sp)
                                             .arg(playerMonsterOnPlayer.catched_with)
                                             .arg((quint8)playerMonsterOnPlayer.gender)
                                             )
                                         .arg(
                                             QStringLiteral("%1,%2,%3")
                                             .arg(playerMonsterOnPlayer.egg_step)
                                             .arg(playerMonsterOnPlayer.character_origin)
                                             .arg(public_and_private_informations.warehouse_playerMonster.size()+2)
                                             )
                                         );
                        else
                            dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_insert_warehouse_monster_full
                                         .arg(
                                             QStringLiteral("%1,%2,%3,%4,%5,%6,%7,%8")
                                             .arg(playerMonsterOnPlayer.id)
                                             .arg(playerMonsterOnPlayer.hp)
                                             .arg(character_id)
                                             .arg(playerMonsterOnPlayer.monster)
                                             .arg(playerMonsterOnPlayer.level)
                                             .arg(playerMonsterOnPlayer.remaining_xp)
                                             .arg(playerMonsterOnPlayer.catched_with)
                                             .arg((quint8)playerMonsterOnPlayer.gender)
                                             )
                                         .arg(
                                             QStringLiteral("%1,%2,%3")
                                             .arg(playerMonsterOnPlayer.egg_step)
                                             .arg(playerMonsterOnPlayer.character_origin)
                                             .arg(public_and_private_informations.warehouse_playerMonster.size()+2)
                                             )
                                         );
                    public_and_private_informations.warehouse_playerMonster << public_and_private_informations.playerMonster.at(sub_index);
                    public_and_private_informations.playerMonster.removeAt(sub_index);
                    break;
                }
                sub_index++;
            }
            index++;
        }
    }
    if(!depositeMonsters.isEmpty() || !withdrawMonsters.isEmpty())
        if(GlobalServerData::serverSettings.database.fightSync==ServerSettings::Database::FightSync_AtTheDisconnexion)
            saveMonsterStat(public_and_private_informations.playerMonster.last());
}

bool Client::wareHouseStoreCheck(const qint64 &cash, const QList<QPair<quint16, qint32> > &items, const QList<quint32> &withdrawMonsters, const QList<quint32> &depositeMonsters)
{
    //check all
    if((cash>0 && (qint64)public_and_private_informations.warehouse_cash<cash) || (cash<0 && (qint64)public_and_private_informations.cash<-cash))
    {
        errorOutput("cash transfer is wrong");
        return false;
    }
    {
        int index=0;
        while(index<items.size())
        {
            const QPair<quint16, qint32> &item=items.at(index);
            if(item.second>0)
            {
                if(!public_and_private_informations.warehouse_items.contains(item.first))
                {
                    errorOutput("warehouse item transfer is wrong due to missing");
                    return false;
                }
                if((qint64)public_and_private_informations.warehouse_items.value(item.first)<item.second)
                {
                    errorOutput("warehouse item transfer is wrong due to wrong quantity");
                    return false;
                }
            }
            if(item.second<0)
            {
                if(!public_and_private_informations.items.contains(item.first))
                {
                    errorOutput("item transfer is wrong due to missing");
                    return false;
                }
                if((qint64)public_and_private_informations.items.value(item.first)<-item.second)
                {
                    errorOutput("item transfer is wrong due to wrong quantity");
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
            while(sub_index<public_and_private_informations.warehouse_playerMonster.size())
            {
                if(public_and_private_informations.warehouse_playerMonster.at(sub_index).id==withdrawMonsters.at(index))
                {
                    count_change++;
                    break;
                }
                sub_index++;
            }
            if(sub_index==public_and_private_informations.warehouse_playerMonster.size())
            {
                errorOutput("no monster to withdraw");
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
            while(sub_index<public_and_private_informations.playerMonster.size())
            {
                if(public_and_private_informations.playerMonster.at(sub_index).id==depositeMonsters.at(index))
                {
                    count_change--;
                    break;
                }
                sub_index++;
            }
            if(sub_index==public_and_private_informations.playerMonster.size())
            {
                errorOutput("no monster to deposite");
                return false;
            }
            index++;
        }
    }
    if((public_and_private_informations.playerMonster.size()+count_change)>CommonSettings::commonSettings.maxPlayerMonsters)
    {
        errorOutput("have more monster to withdraw than the allowed");
        return false;
    }
    return true;
}

void Client::sendHandlerCommand(const QString &command,const QString &extraText)
{
    if(command==Client::text_give)
    {
        bool ok;
        QStringList arguments=extraText.split(Client::text_space,QString::SkipEmptyParts);
        if(arguments.size()==2)
            arguments << Client::text_1;
        if(arguments.size()!=3)
        {
            receiveSystemText(QStringLiteral("Wrong arguments number for the command, usage: /give objectId player [quantity=1]"));
            return;
        }
        const quint32 &objectId=arguments.first().toUInt(&ok);
        if(!ok)
        {
            receiveSystemText(QStringLiteral("objectId is not a number, usage: /give objectId player [quantity=1]"));
            return;
        }
        if(!CommonDatapack::commonDatapack.items.item.contains(objectId))
        {
            receiveSystemText(QStringLiteral("objectId is not a valid item, usage: /give objectId player [quantity=1]"));
            return;
        }
        const quint32 &quantity=arguments.last().toUInt(&ok);
        if(!ok)
        {
            receiveSystemText(QStringLiteral("quantity is not a number, usage: /give objectId player [quantity=1]"));
            return;
        }
        if(!playerByPseudo.contains(arguments.at(1)))
        {
            receiveSystemText(QStringLiteral("player is not connected, usage: /give objectId player [quantity=1]"));
            return;
        }
        normalOutput(QStringLiteral("%1 have give to %2 the item with id: %3 in quantity: %4").arg(public_and_private_informations.public_informations.pseudo).arg(arguments.at(1)).arg(objectId).arg(quantity));
        playerByPseudo.value(arguments.at(1))->addObjectAndSend(objectId,quantity);
    }
    else if(command==Client::text_setevent)
    {
        const QStringList &arguments=extraText.split(Client::text_space,QString::SkipEmptyParts);
        if(arguments.size()!=2)
        {
            receiveSystemText(QStringLiteral("Wrong arguments number for the command, usage: /give setevent [event] [value]"));
            return;
        }
        int index=0,sub_index;
        while(index<CommonDatapack::commonDatapack.events.size())
        {
            const Event &event=CommonDatapack::commonDatapack.events.at(index);
            if(event.name==arguments.at(0))
            {
                sub_index=0;
                while(sub_index<event.values.size())
                {
                    if(event.values.at(sub_index)==arguments.at(1))
                    {
                        if(GlobalServerData::serverPrivateVariables.events.at(index)==sub_index)
                        {
                            receiveSystemText(QStringLiteral("The event have aready this value"));
                            return;
                        }
                        else
                        {
                            setEvent(index,sub_index);
                            GlobalServerData::serverPrivateVariables.events[index]=sub_index;
                        }
                        break;
                    }
                    sub_index++;
                }
                if(sub_index==event.values.size())
                {
                    receiveSystemText(QStringLiteral("The event value is not found"));
                    return;
                }
                break;
            }
            index++;
        }
        if(index==CommonDatapack::commonDatapack.events.size())
        {
            receiveSystemText(QStringLiteral("The event is not found"));
            return;
        }
    }
    else if(command==Client::text_take)
    {
        bool ok;
        QStringList arguments=extraText.split(Client::text_space,QString::SkipEmptyParts);
        if(arguments.size()==2)
            arguments << Client::text_1;
        if(arguments.size()!=3)
        {
            receiveSystemText(QStringLiteral("Wrong arguments number for the command, usage: /take objectId player [quantity=1]"));
            return;
        }
        quint32 objectId=arguments.first().toUInt(&ok);
        if(!ok)
        {
            receiveSystemText(QStringLiteral("objectId is not a number, usage: /take objectId player [quantity=1]"));
            return;
        }
        if(!CommonDatapack::commonDatapack.items.item.contains(objectId))
        {
            receiveSystemText(QStringLiteral("objectId is not a valid item, usage: /take objectId player [quantity=1]"));
            return;
        }
        quint32 quantity=arguments.last().toUInt(&ok);
        if(!ok)
        {
            receiveSystemText(QStringLiteral("quantity is not a number, usage: /take objectId player [quantity=1]"));
            return;
        }
        if(!playerByPseudo.contains(arguments.at(1)))
        {
            receiveSystemText(QStringLiteral("player is not connected, usage: /take objectId player [quantity=1]"));
            return;
        }
        normalOutput(QStringLiteral("%1 have take to %2 the item with id: %3 in quantity: %4").arg(public_and_private_informations.public_informations.pseudo).arg(arguments.at(1)).arg(objectId).arg(quantity));
        playerByPseudo.value(arguments.at(1))->sendRemoveObject(objectId,playerByPseudo.value(arguments.at(1))->removeObject(objectId,quantity));
    }
    else if(command==Client::text_tp)
    {
        QStringList arguments=extraText.split(Client::text_space,QString::SkipEmptyParts);
        if(arguments.size()==3)
        {
            if(arguments.at(1)!=Client::text_to)
            {
                receiveSystemText(QStringLiteral("wrong second arguement: %1, usage: /tp player1 to player2").arg(arguments.at(1)));
                return;
            }
            if(!playerByPseudo.contains(arguments.first()))
            {
                receiveSystemText(QStringLiteral("%1 is not connected, usage: /tp player1 to player2").arg(arguments.first()));
                return;
            }
            if(!playerByPseudo.contains(arguments.last()))
            {
                receiveSystemText(QStringLiteral("%1 is not connected, usage: /tp player1 to player2").arg(arguments.last()));
                return;
            }
            playerByPseudo.value(arguments.first())->receiveTeleportTo(playerByPseudo.value(arguments.last())->map,playerByPseudo.value(arguments.last())->x,playerByPseudo.value(arguments.last())->y,MoveOnTheMap::directionToOrientation(playerByPseudo.value(arguments.last())->getLastDirection()));
        }
        else
        {
            receiveSystemText(QStringLiteral("Wrong arguments number for the command, usage: /tp player1 to player2"));
            return;
        }
    }
    else if(command==Client::text_trade)
    {
        if(extraText.isEmpty())
        {
            receiveSystemText(QStringLiteral("no player given, syntaxe: /trade player").arg(extraText));
            return;
        }
        if(!playerByPseudo.contains(extraText))
        {
            receiveSystemText(QStringLiteral("%1 is not connected").arg(extraText));
            return;
        }
        if(public_and_private_informations.public_informations.pseudo==extraText)
        {
            receiveSystemText(QStringLiteral("You can't trade with yourself").arg(extraText));
            return;
        }
        if(getInTrade())
        {
            receiveSystemText(QStringLiteral("You are already in trade"));
            return;
        }
        if(isInBattle())
        {
            receiveSystemText(QStringLiteral("You are already in battle"));
            return;
        }
        if(playerByPseudo.value(extraText)->getInTrade())
        {
            receiveSystemText(QStringLiteral("%1 is already in trade").arg(extraText));
            return;
        }
        if(playerByPseudo.value(extraText)->isInBattle())
        {
            receiveSystemText(QStringLiteral("%1 is already in battle").arg(extraText));
            return;
        }
        if(!otherPlayerIsInRange(playerByPseudo.value(extraText)))
        {
            receiveSystemText(QStringLiteral("%1 is not in range").arg(extraText));
            return;
        }
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        normalOutput(QStringLiteral("Trade requested"));
        #endif
        otherPlayerTrade=playerByPseudo.value(extraText);
        otherPlayerTrade->registerTradeRequest(this);
    }
    else if(command==Client::text_battle)
    {
        if(extraText.isEmpty())
        {
            receiveSystemText(QStringLiteral("no player given, syntaxe: /battle player").arg(extraText));
            return;
        }
        if(!playerByPseudo.contains(extraText))
        {
            receiveSystemText(QStringLiteral("%1 is not connected").arg(extraText));
            return;
        }
        if(public_and_private_informations.public_informations.pseudo==extraText)
        {
            receiveSystemText(QStringLiteral("You can't battle with yourself").arg(extraText));
            return;
        }
        if(isInBattle())
        {
            receiveSystemText(QStringLiteral("you are already in battle"));
            return;
        }
        if(getInTrade())
        {
            receiveSystemText(QStringLiteral("you are already in trade"));
            return;
        }
        if(playerByPseudo.value(extraText)->isInBattle())
        {
            receiveSystemText(QStringLiteral("%1 is already in battle").arg(extraText));
            return;
        }
        if(playerByPseudo.value(extraText)->getInTrade())
        {
            receiveSystemText(QStringLiteral("%1 is already in battle").arg(extraText));
            return;
        }
        if(!otherPlayerIsInRange(playerByPseudo.value(extraText)))
        {
            receiveSystemText(QStringLiteral("%1 is not in range").arg(extraText));
            return;
        }
        if(!playerByPseudo.value(extraText)->getAbleToFight())
        {
            receiveSystemText(QStringLiteral("The other player can't fight"));
            return;
        }
        if(!getAbleToFight())
        {
            receiveSystemText(QStringLiteral("You can't fight"));
            return;
        }
        if(playerByPseudo.value(extraText)->isInFight())
        {
            receiveSystemText(QStringLiteral("The other player is in fight"));
            return;
        }
        if(isInFight())
        {
            receiveSystemText(QStringLiteral("You are in fight"));
            return;
        }
        if(captureCityInProgress())
        {
            errorOutput(QStringLiteral("Try battle when is in capture city"));
            return;
        }
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        normalOutput(QStringLiteral("Battle requested"));
        #endif
        playerByPseudo.value(extraText)->registerBattleRequest(this);
    }
}

void Client::setEvent(const quint8 &event, const quint8 &new_value)
{
    const quint8 &event_value=GlobalServerData::serverPrivateVariables.events.at(event);
    QDateTime currentDateTime=QDateTime::currentDateTime();
    QList<Client *> playerList;
    QHashIterator<QString,Client *> i(playerByPseudo);
    while (i.hasNext()) {
        i.next();
        i.value()->addEventInQueue(event,event_value,currentDateTime);
        playerList << i.value();
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << event;
    out << new_value;
    int index=0;
    while(index<playerList.size())
    {
        playerList.at(index)->sendNewEvent(outputData);
        index++;
    }
    GlobalServerData::serverPrivateVariables.events[event]=new_value;
}

void Client::addEventInQueue(const quint8 &event,const quint8 &event_value,const QDateTime &currentDateTime)
{
    if(oldEvents.oldEventList.isEmpty())
        oldEvents.time=currentDateTime;
    OldEvents::OldEventEntry entry;
    entry.event=event;
    entry.eventValue=event_value;
    oldEvents.oldEventList << entry;
}

void Client::removeFirstEventInQueue()
{
    if(oldEvents.oldEventList.isEmpty())
    {
        errorOutput(QLatin1Literal("Not event in queue to remove"));
        return;
    }
    oldEvents.oldEventList.removeFirst();
    if(!oldEvents.oldEventList.isEmpty())
        oldEvents.time=QDateTime::currentDateTime();
}

bool Client::learnSkill(const quint32 &monsterId, const quint16 &skill)
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(QStringLiteral("learnSkill(%1,%2)").arg(monsterId).arg(skill));
    #endif
    CommonMap *map=this->map;
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
                    errorOutput(QStringLiteral("learnSkill() Can't move at top from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                    return false;
                }
            }
            else
            {
                errorOutput(QStringLiteral("No valid map in this direction"));
                return false;
            }
        break;
        default:
        errorOutput(QStringLiteral("Wrong direction to use a learn skill"));
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
                        errorOutput(QStringLiteral("learnSkill() Can't move at top from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                        return false;
                    }
                }
                else
                {
                    errorOutput(QStringLiteral("No valid map in this direction"));
                    return false;
                }
            break;
            default:
            break;
        }
        if(!static_cast<MapServer*>(this->map)->learn.contains(QPair<quint8,quint8>(x,y)))
        {
            errorOutput(QStringLiteral("not learn skill into this direction"));
            return false;
        }
    }
    return learnSkillInternal(monsterId,skill);
}

bool Client::otherPlayerIsInRange(Client * otherPlayer)
{
    if(getMap()==NULL)
        return false;
    return getMap()==otherPlayer->getMap();
}

void Client::destroyObject(const quint16 &itemId,const quint32 &quantity)
{
    normalOutput(QStringLiteral("The player have destroy them self %1 item(s) with id: %2").arg(quantity).arg(itemId));
    removeObject(itemId,quantity);
}

bool Client::useObjectOnMonster(const quint16 &object,const quint32 &monster)
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(QStringLiteral("use the object: %1 on monster %2").arg(object).arg(monster));
    #endif
    if(!public_and_private_informations.items.contains(object))
    {
        errorOutput(QStringLiteral("can't use the object: %1 because don't have into the inventory").arg(object));
        return false;
    }
    if(objectQuantity(object)<1)
    {
        errorOutput(QStringLiteral("have not quantity to use this object: %1").arg(object));
        return false;
    }
    if(CommonFightEngine::useObjectOnMonster(object,monster))
    {
        if(CommonDatapack::commonDatapack.items.item.value(object).consumeAtUse)
            removeObject(object);
    }
    return true;
}

void Client::useObject(const quint8 &query_id,const quint16 &itemId)
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(QStringLiteral("use the object: %1").arg(itemId));
    #endif
    if(!public_and_private_informations.items.contains(itemId))
    {
        errorOutput(QStringLiteral("can't use the object: %1 because don't have into the inventory").arg(itemId));
        return;
    }
    if(objectQuantity(itemId)<1)
    {
        errorOutput(QStringLiteral("have not quantity to use this object: %1 because recipe already registred").arg(itemId));
        return;
    }
    if(CommonDatapack::commonDatapack.items.item.value(itemId).consumeAtUse)
        removeObject(itemId);
    //if is crafting recipe
    if(CommonDatapack::commonDatapack.itemToCrafingRecipes.contains(itemId))
    {
        const quint32 &recipeId=CommonDatapack::commonDatapack.itemToCrafingRecipes.value(itemId);
        if(public_and_private_informations.recipes.contains(recipeId))
        {
            errorOutput(QStringLiteral("can't use the object: %1").arg(itemId));
            return;
        }
        if(!haveReputationRequirements(CommonDatapack::commonDatapack.crafingRecipes.value(recipeId).requirements.reputation))
        {
            errorOutput(QStringLiteral("The player have not the requirement: %1 to to learn crafting recipe").arg(recipeId));
            return;
        }
        public_and_private_informations.recipes << recipeId;
        //send the network reply
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);
        out << (quint8)ObjectUsage_correctlyUsed;
        postReply(query_id,outputData);
        //add into db
        dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_insert_recipe
                     .arg(character_id)
                     .arg(recipeId)
                     );
    }
    //use trap into fight
    else if(CommonDatapack::commonDatapack.items.trap.contains(itemId))
    {
        if(!isInFight())
        {
            errorOutput(QStringLiteral("is not in fight to use trap: %1").arg(itemId));
            return;
        }
        if(!isInFightWithWild())
        {
            errorOutput(QStringLiteral("is not in fight with wild to use trap: %1").arg(itemId));
            return;
        }
        const quint32 &maxMonsterId=tryCapture(itemId);
        //send the network reply
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);
        out << (quint8)ObjectUsage_correctlyUsed;
        if(maxMonsterId>0)
            out << (quint32)maxMonsterId;
        else
            out << (quint32)0x00000000;
        postReply(query_id,outputData);
    }
    //use repel into fight
    else if(CommonDatapack::commonDatapack.items.repel.contains(itemId))
    {
        public_and_private_informations.repel_step+=CommonDatapack::commonDatapack.items.repel.value(itemId);
        //send the network reply
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);
        out << (quint8)ObjectUsage_correctlyUsed;
        postReply(query_id,outputData);
    }
    else
    {
        errorOutput(QStringLiteral("can't use the object: %1 because don't have an usage").arg(itemId));
        return;
    }
}

void Client::receiveTeleportTo(CommonMap *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation)
{
    teleportTo(map,x,y,orientation);
}

void Client::teleportValidatedTo(CommonMap *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation)
{
    normalOutput(QStringLiteral("teleportValidatedTo(%1,%2,%3,%4)").arg(map->map_file).arg(x).arg(y).arg((quint8)orientation));
    bool mapChange=this->map!=map;
    if(mapChange)
        removeNearPlant();
    MapBasicMove::teleportValidatedTo(map,x,y,orientation);
    if(GlobalServerData::serverSettings.database.positionTeleportSync)
        savePosition();
    if(mapChange)
        sendNearPlant();
}

Direction Client::lookToMove(const Direction &direction)
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

void Client::getShopList(const quint8 &query_id,const quint16 &shopId)
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(QStringLiteral("getShopList(%1,%2)").arg(query_id).arg(shopId));
    #endif
    if(!CommonDatapack::commonDatapack.shops.contains(shopId))
    {
        errorOutput(QStringLiteral("shopId not found: %1").arg(shopId));
        return;
    }
    CommonMap *map=this->map;
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
                    errorOutput(QStringLiteral("getShopList() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                    return;
                }
            }
            else
            {
                errorOutput(QStringLiteral("No valid map in this direction"));
                return;
            }
        break;
        default:
        errorOutput(QStringLiteral("Wrong direction to use a shop"));
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
                            errorOutput(QStringLiteral("getShopList() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                            return;
                        }
                    }
                    else
                    {
                        errorOutput(QStringLiteral("No valid map in this direction"));
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
                    errorOutput(QStringLiteral("not shop into this direction"));
                    return;
                }
            }
        }
    }
    //send the shop items (no taxes from now)
    const Shop &shop=CommonDatapack::commonDatapack.shops.value(shopId);
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    QByteArray outputData2;
    QDataStream out2(&outputData2, QIODevice::WriteOnly);
    out2.setVersion(QDataStream::Qt_4_4);
    int index=0;
    int objectCount=0;
    while(index<shop.items.size())
    {
        if(shop.prices.at(index)>0)
        {
            out2 << (quint16)shop.items.at(index);
            out2 << (quint32)shop.prices.at(index);
            out2 << (quint32)0;
            objectCount++;
        }
        index++;
    }
    out << (quint16)objectCount;
    postReply(query_id,outputData+outputData2);
}

void Client::buyObject(const quint8 &query_id,const quint16 &shopId,const quint16 &objectId,const quint32 &quantity,const quint32 &price)
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(QStringLiteral("getShopList(%1,%2)").arg(query_id).arg(shopId));
    #endif
    if(!CommonDatapack::commonDatapack.shops.contains(shopId))
    {
        errorOutput(QStringLiteral("shopId not found: %1").arg(shopId));
        return;
    }
    if(quantity<=0)
    {
        errorOutput(QStringLiteral("quantity wrong: %1").arg(quantity));
        return;
    }
    CommonMap *map=this->map;
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
                    errorOutput(QStringLiteral("buyObject() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                    return;
                }
            }
            else
            {
                errorOutput(QStringLiteral("No valid map in this direction"));
                return;
            }
        break;
        default:
        errorOutput(QStringLiteral("Wrong direction to use a shop"));
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
                            errorOutput(QStringLiteral("buyObject() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                            return;
                        }
                    }
                    else
                    {
                        errorOutput(QStringLiteral("No valid map in this direction"));
                        return;
                    }
                break;
                default:
                errorOutput(QStringLiteral("Wrong direction to use a shop"));
                return;
            }
            if(static_cast<MapServer*>(this->map)->shops.contains(QPair<quint8,quint8>(x,y)))
            {
                QList<quint32> shops=static_cast<MapServer*>(this->map)->shops.values(QPair<quint8,quint8>(x,y));
                if(!shops.contains(shopId))
                {
                    errorOutput(QStringLiteral("not shop into this direction"));
                    return;
                }
            }
        }
    }
    //send the shop items (no taxes from now)
    const int &priceIndex=CommonDatapack::commonDatapack.shops.value(shopId).items.indexOf(objectId);
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    if(priceIndex==-1)
    {
        out << (quint8)BuyStat_HaveNotQuantity;
        postReply(query_id,outputData);
        return;
    }
    const quint32 &realprice=CommonDatapack::commonDatapack.shops.value(shopId).prices.at(priceIndex);
    if(realprice==0)
    {
        out << (quint8)BuyStat_HaveNotQuantity;
        postReply(query_id,outputData);
        return;
    }
    if(realprice>price)
    {
        out << (quint8)BuyStat_PriceHaveChanged;
        postReply(query_id,outputData);
        return;
    }
    if(realprice<price)
    {
        out << (quint8)BuyStat_BetterPrice;
        out << (quint32)price;
    }
    else
        out << (quint8)BuyStat_Done;
    if(public_and_private_informations.cash>=(realprice*quantity))
        removeCash(realprice*quantity);
    else
    {
        errorOutput(QStringLiteral("The player have not the cash to buy %1 item of id: %2").arg(quantity).arg(objectId));
        return;
    }
    addObject(objectId,quantity);
    postReply(query_id,outputData);
}

void Client::sellObject(const quint8 &query_id,const quint16 &shopId,const quint16 &objectId,const quint32 &quantity,const quint32 &price)
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(QStringLiteral("getShopList(%1,%2)").arg(query_id).arg(shopId));
    #endif
    if(!CommonDatapack::commonDatapack.shops.contains(shopId))
    {
        errorOutput(QStringLiteral("shopId not found: %1").arg(shopId));
        return;
    }
    if(quantity<=0)
    {
        errorOutput(QStringLiteral("quantity wrong: %1").arg(quantity));
        return;
    }
    CommonMap *map=this->map;
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
                    errorOutput(QStringLiteral("sellObject() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                    return;
                }
            }
            else
            {
                errorOutput(QStringLiteral("No valid map in this direction"));
                return;
            }
        break;
        default:
        errorOutput(QStringLiteral("Wrong direction to use a shop"));
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
                            errorOutput(QStringLiteral("sellObject() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                            return;
                        }
                    }
                    else
                    {
                        errorOutput(QStringLiteral("No valid map in this direction"));
                        return;
                    }
                break;
                default:
                errorOutput(QStringLiteral("Wrong direction to use a shop"));
                return;
            }
            if(static_cast<MapServer*>(this->map)->shops.contains(QPair<quint8,quint8>(x,y)))
            {
                QList<quint32> shops=static_cast<MapServer*>(this->map)->shops.values(QPair<quint8,quint8>(x,y));
                if(!shops.contains(shopId))
                {
                    errorOutput(QStringLiteral("not shop into this direction"));
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
        errorOutput(QStringLiteral("this item don't exists"));
        return;
    }
    if(objectQuantity(objectId)<quantity)
    {
        errorOutput(QStringLiteral("you have not this quantity to sell"));
        return;
    }
    const quint32 &realPrice=CommonDatapack::commonDatapack.items.item.value(objectId).price/2;
    if(realPrice<price)
    {
        out << (quint8)SoldStat_PriceHaveChanged;
        postReply(query_id,outputData);
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
    postReply(query_id,outputData);
}

void Client::saveIndustryStatus(const quint32 &factoryId,const IndustryStatus &industryStatus,const Industry &industry)
{
    QStringList resourcesStringList,productsStringList;
    int index;
    //send the resource
    index=0;
    while(index<industry.resources.size())
    {
        const Industry::Resource &resource=industry.resources.at(index);
        const quint32 &quantityInStock=industryStatus.resources.value(resource.item);
        resourcesStringList << QStringLiteral("%1->%2").arg(resource.item).arg(quantityInStock);
        index++;
    }
    //send the product
    index=0;
    while(index<industry.products.size())
    {
        const Industry::Product &product=industry.products.at(index);
        const quint32 &quantityInStock=industryStatus.products.value(product.item);
        productsStringList << QStringLiteral("%1->%2").arg(product.item).arg(quantityInStock);
        index++;
    }

    //save in db
    if(!GlobalServerData::serverPrivateVariables.industriesStatus.contains(factoryId))
    {
        dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_insert_factory
                     .arg(factoryId)
                     .arg(resourcesStringList.join(Client::text_dotcomma))
                     .arg(productsStringList.join(Client::text_dotcomma))
                     .arg(industryStatus.last_update)
                     );
    }
    else
    {
        dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_update_factory
                     .arg(factoryId)
                     .arg(resourcesStringList.join(Client::text_dotcomma))
                     .arg(productsStringList.join(Client::text_dotcomma))
                     .arg(industryStatus.last_update)
                     );
    }
    GlobalServerData::serverPrivateVariables.industriesStatus[factoryId]=industryStatus;
}

void Client::getFactoryList(const quint8 &query_id, const quint16 &factoryId)
{
    if(isInFight())
    {
        errorOutput(QStringLiteral("Try do inventory action when is in fight"));
        return;
    }
    if(captureCityInProgress())
    {
        errorOutput(QStringLiteral("Try do inventory action when is in capture city"));
        return;
    }
    if(!CommonDatapack::commonDatapack.industriesLink.contains(factoryId))
    {
        errorOutput(QStringLiteral("factory id not found"));
        return;
    }
    const Industry &industry=CommonDatapack::commonDatapack.industries.value(CommonDatapack::commonDatapack.industriesLink.value(factoryId).industry);
    //send the shop items (no taxes from now)
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    if(!GlobalServerData::serverPrivateVariables.industriesStatus.contains(factoryId))
    {
        out << (quint32)0;
        out << (quint16)industry.resources.size();
        int index=0;
        while(index<industry.resources.size())
        {
            const Industry::Resource &resource=industry.resources.at(index);
            out << (quint16)resource.item;
            out << (quint32)CommonDatapack::commonDatapack.items.item.value(resource.item).price*(100+CATCHCHALLENGER_SERVER_FACTORY_PRICE_CHANGE)/100;
            out << (quint32)resource.quantity*industry.cycletobefull;
            index++;
        }
        out << (quint16)0x0000;//no product do
    }
    else
    {
        int index,count_item;
        const IndustryStatus &industryStatus=FacilityLib::industryStatusWithCurrentTime(GlobalServerData::serverPrivateVariables.industriesStatus.value(factoryId),industry);
        quint64 currentTime=QDateTime::currentMSecsSinceEpoch()/1000;
        if(industryStatus.last_update>currentTime)
            out << (quint32)0;
        else if((currentTime-industryStatus.last_update)>industry.time)
            out << (quint32)0;
        else
            out << (quint32)(industry.time-(currentTime-industryStatus.last_update));
        //send the resource
        count_item=0;
        index=0;
        while(index<industry.resources.size())
        {
            const Industry::Resource &resource=industry.resources.at(index);
            const quint32 &quantityInStock=industryStatus.resources.value(resource.item);
            if(quantityInStock<resource.quantity*industry.cycletobefull)
                count_item++;
            index++;
        }
        out << (quint16)count_item;
        index=0;
        while(index<industry.resources.size())
        {
            const Industry::Resource &resource=industry.resources.at(index);
            const quint32 &quantityInStock=industryStatus.resources.value(resource.item);
            if(quantityInStock<resource.quantity*industry.cycletobefull)
            {
                out << (quint16)resource.item;
                out << (quint32)FacilityLib::getFactoryResourcePrice(quantityInStock,resource,industry);
                out << (quint32)resource.quantity*industry.cycletobefull-quantityInStock;
            }
            index++;
        }
        //send the product
        count_item=0;
        index=0;
        while(index<industry.products.size())
        {
            const Industry::Product &product=industry.products.at(index);
            const quint32 &quantityInStock=industryStatus.products.value(product.item);
            if(quantityInStock>0)
                count_item++;
            index++;
        }
        out << (quint16)count_item;
        index=0;
        while(index<industry.products.size())
        {
            const Industry::Product &product=industry.products.at(index);
            const quint32 &quantityInStock=industryStatus.products.value(product.item);
            if(quantityInStock>0)
            {
                out << (quint16)product.item;
                out << (quint32)FacilityLib::getFactoryProductPrice(quantityInStock,product,industry);
                out << (quint32)quantityInStock;
            }
            index++;
        }
    }
    postReply(query_id,outputData);
}

void Client::buyFactoryProduct(const quint8 &query_id,const quint16 &factoryId,const quint16 &objectId,const quint32 &quantity,const quint32 &price)
{
    if(isInFight())
    {
        errorOutput(QStringLiteral("Try do inventory action when is in fight"));
        return;
    }
    if(captureCityInProgress())
    {
        errorOutput(QStringLiteral("Try do inventory action when is in capture city"));
        return;
    }
    if(!CommonDatapack::commonDatapack.industriesLink.contains(factoryId))
    {
        errorOutput(QStringLiteral("factory id not found"));
        return;
    }
    if(!CommonDatapack::commonDatapack.items.item.contains(objectId))
    {
        errorOutput(QStringLiteral("object id not found into the factory product list"));
        return;
    }
    if(!GlobalServerData::serverPrivateVariables.industriesStatus.contains(factoryId))
    {
        errorOutput(QStringLiteral("factory id not found in active list"));
        return;
    }
    if(!haveReputationRequirements(CommonDatapack::commonDatapack.industriesLink.value(factoryId).requirements.reputation))
    {
        errorOutput(QStringLiteral("The player have not the requirement: %1 to use the factory").arg(factoryId));
        return;
    }
    const Industry &industry=CommonDatapack::commonDatapack.industries.value(CommonDatapack::commonDatapack.industriesLink.value(factoryId).industry);
    IndustryStatus industryStatus=FacilityLib::industryStatusWithCurrentTime(GlobalServerData::serverPrivateVariables.industriesStatus.value(factoryId),industry);
    quint32 quantityInStock=0;
    quint32 actualPrice=0;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    Industry::Product product;
    //get the right product
    {
        int index=0;
        while(index<industry.products.size())
        {
            product=industry.products.at(index);
            if(product.item==objectId)
            {
                quantityInStock=industryStatus.products.value(product.item);
                actualPrice=FacilityLib::getFactoryProductPrice(quantityInStock,product,industry);
                break;
            }
            index++;
        }
        if(index==industry.products.size())
        {
            errorOutput(QStringLiteral("internal bug, product for the factory not found"));
            return;
        }
    }
    if(public_and_private_informations.cash<(actualPrice*quantity))
    {
        errorOutput(QStringLiteral("have not the cash to buy into this factory"));
        return;
    }
    if(quantity>quantityInStock)
    {
        out << (quint8)0x03;
        postReply(query_id,outputData);
        return;
    }
    if(actualPrice>price)
    {
        out << (quint8)0x04;
        postReply(query_id,outputData);
        return;
    }
    if(actualPrice==price)
        out << (quint8)0x01;
    else
    {
        out << (quint8)0x02;
        out << (quint32)actualPrice;
    }
    quantityInStock-=quantity;
    if(quantityInStock==(product.item*industry.cycletobefull))
    {
        industryStatus.products[product.item]=quantityInStock;
        industryStatus=FacilityLib::factoryCheckProductionStart(industryStatus,industry);
    }
    else
        industryStatus.products[product.item]=quantityInStock;
    removeCash(actualPrice*quantity);
    saveIndustryStatus(factoryId,industryStatus,industry);
    addObject(objectId,quantity);
    appendReputationRewards(CommonDatapack::commonDatapack.industriesLink.value(factoryId).rewards.reputation);
    postReply(query_id,outputData);
}

void Client::sellFactoryResource(const quint8 &query_id,const quint16 &factoryId,const quint16 &objectId,const quint32 &quantity,const quint32 &price)
{
    if(isInFight())
    {
        errorOutput(QStringLiteral("Try do inventory action when is in fight"));
        return;
    }
    if(captureCityInProgress())
    {
        errorOutput(QStringLiteral("Try do inventory action when is in capture city"));
        return;
    }
    if(!CommonDatapack::commonDatapack.industriesLink.contains(factoryId))
    {
        errorOutput(QStringLiteral("factory id not found"));
        return;
    }
    if(!CommonDatapack::commonDatapack.items.item.contains(objectId))
    {
        errorOutput(QStringLiteral("object id not found"));
        return;
    }
    if(objectQuantity(objectId)<quantity)
    {
        errorOutput(QStringLiteral("you have not the object quantity to sell at this factory"));
        return;
    }
    if(!haveReputationRequirements(CommonDatapack::commonDatapack.industriesLink.value(factoryId).requirements.reputation))
    {
        errorOutput(QStringLiteral("The player have not the requirement: %1 to use the factory").arg(factoryId));
        return;
    }
    const Industry &industry=CommonDatapack::commonDatapack.industries.value(CommonDatapack::commonDatapack.industriesLink.value(factoryId).industry);
    IndustryStatus industryStatus;
    if(!GlobalServerData::serverPrivateVariables.industriesStatus.contains(factoryId))
    {
        industryStatus.last_update=(QDateTime::currentMSecsSinceEpoch()/1000);
        int index;
        //send the resource
        index=0;
        while(index<industry.resources.size())
        {
            const Industry::Resource &resource=industry.resources.at(index);
            industryStatus.resources[resource.item]=0;
            index++;
        }
        //send the product
        index=0;
        while(index<industry.products.size())
        {
            const Industry::Product &product=industry.products.at(index);
            industryStatus.products[product.item]=0;
            index++;
        }
    }
    else
        industryStatus=FacilityLib::industryStatusWithCurrentTime(GlobalServerData::serverPrivateVariables.industriesStatus.value(factoryId),industry);
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    quint32 resourcePrice;
    //check if not overfull
    {
        int index=0;
        while(index<industry.resources.size())
        {
            const Industry::Resource &resource=industry.resources.at(index);
            if(resource.item==objectId)
            {
                if((resource.quantity*industry.cycletobefull-industryStatus.resources.value(resource.item))<quantity)
                {
                    out << (quint8)0x03;
                    postReply(query_id,outputData);
                    return;
                }
                resourcePrice=FacilityLib::getFactoryResourcePrice(industryStatus.resources.value(resource.item),resource,industry);
                if(price>resourcePrice)
                {
                    out << (quint8)0x04;
                    postReply(query_id,outputData);
                    return;
                }
                if((industryStatus.resources.value(resource.item)+quantity)==resource.quantity)
                {
                    industryStatus.resources[resource.item]+=quantity;
                    industryStatus=FacilityLib::factoryCheckProductionStart(industryStatus,industry);
                }
                else
                    industryStatus.resources[resource.item]+=quantity;
                break;
            }
            index++;
        }
        if(index==industry.resources.size())
        {
            errorOutput(QStringLiteral("internal bug, resource for the factory not found"));
            return;
        }
    }
    if(price==resourcePrice)
        out << (quint8)0x01;
    else
    {
        out << (quint8)0x02;
        out << (quint32)resourcePrice;
    }
    removeObject(objectId,quantity);
    addCash(resourcePrice*quantity);
    saveIndustryStatus(factoryId,industryStatus,industry);
    appendReputationRewards(CommonDatapack::commonDatapack.industriesLink.value(factoryId).rewards.reputation);
    postReply(query_id,outputData);
}

bool CatchChallenger::operator==(const CatchChallenger::MonsterDrops &monsterDrops1,const CatchChallenger::MonsterDrops &monsterDrops2)
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

void Client::appendAllow(const ActionAllow &allow)
{
    if(public_and_private_informations.allow.contains(allow))
        return;
    public_and_private_informations.allow << allow;
    dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_insert_character_allow
                 .arg(character_id)
                 .arg(GlobalServerData::serverPrivateVariables.dictionary_allow_reverse.at(allow))
                 );
}

void Client::removeAllow(const ActionAllow &allow)
{
    if(!public_and_private_informations.allow.contains(allow))
        return;
    public_and_private_informations.allow.remove(allow);
    dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_delete_character_allow
                 .arg(character_id)
                 .arg(GlobalServerData::serverPrivateVariables.dictionary_allow_reverse.at(allow))
                 );
}

void Client::appendReputationRewards(const QList<ReputationRewards> &reputationList)
{
    int index=0;
    while(index<reputationList.size())
    {
        const ReputationRewards &reputationRewards=reputationList.at(index);
        appendReputationPoint(reputationRewards.reputationId,reputationRewards.point);
        index++;
    }
}

//reputation
void Client::appendReputationPoint(const quint8 &reputationId, const qint32 &point)
{
    if(point==0)
        return;
    const Reputation &reputation=CommonDatapack::commonDatapack.reputation.value(reputationId);
    bool isNewReputation=false;
    PlayerReputation *playerReputation=NULL;
    //search
    {
        QMapIterator<quint8,PlayerReputation> i(public_and_private_informations.reputation);
        while (i.hasNext()) {
            i.next();
            if(i.key()==reputationId)
            {
                playerReputation=const_cast<PlayerReputation *>(&i.value());
                break;
            }
        }
        if(playerReputation==NULL)
        {
            PlayerReputation temp;
            temp.point=0;
            temp.level=0;
            isNewReputation=true;
            public_and_private_informations.reputation.insert(reputationId,temp);
            playerReputation=&public_and_private_informations.reputation[reputationId];
        }
    }

    #ifdef DEBUG_MESSAGE_CLIENT_REPUTATION
    normalOutput(QStringLiteral("Reputation %1 at level: %2 with point: %3").arg(type).arg(playerReputation.level).arg(playerReputation.point));
    #endif
    FacilityLib::appendReputationPoint(playerReputation,point,reputation);
    if(!isNewReputation)
    {
        dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_insert_reputation
                         .arg(character_id)
                         .arg(reputation.reverse_database_id)
                         .arg(playerReputation->point)
                         .arg(playerReputation->level)
                         );
    }
    else
    {
        dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_update_reputation
                     .arg(character_id)
                     .arg(reputation.reverse_database_id)
                     .arg(playerReputation->point)
                     .arg(playerReputation->level)
                     );
    }
    #ifdef DEBUG_MESSAGE_CLIENT_REPUTATION
    normalOutput(QStringLiteral("New reputation %1 at level: %2 with point: %3").arg(type).arg(playerReputation.level).arg(playerReputation.point));
    #endif
}

void Client::heal()
{
    if(isInFight())
    {
        errorOutput(QStringLiteral("Try do heal action when is in fight"));
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(QStringLiteral("ask heal at %1 (%2,%3)").arg(this->map->map_file).arg(this->x).arg(this->y));
    #endif
    CommonMap *map=this->map;
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
                    errorOutput(QStringLiteral("heal() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                    return;
                }
            }
            else
            {
                errorOutput(QStringLiteral("No valid map in this direction"));
                return;
            }
        break;
        default:
        errorOutput(QStringLiteral("Wrong direction to use a heal"));
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
                        errorOutput(QStringLiteral("heal() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                        return;
                    }
                }
                else
                {
                    errorOutput(QStringLiteral("No valid map in this direction"));
                    return;
                }
            break;
            default:
            errorOutput(QStringLiteral("Wrong direction to use a heal"));
            return;
        }
        if(!static_cast<MapServer*>(this->map)->heal.contains(QPair<quint8,quint8>(x,y)))
        {
            errorOutput(QStringLiteral("no heal point in this direction"));
            return;
        }
    }
    //send the shop items (no taxes from now)
    healAllMonsters();
    rescue=unvalidated_rescue;
}

void Client::requestFight(const quint16 &fightId)
{
    if(isInFight())
    {
        errorOutput(QStringLiteral("error: map: %1 (%2,%3), is in fight").arg(static_cast<MapServer *>(map)->map_file).arg(x).arg(y));
        return;
    }
    if(captureCityInProgress())
    {
        errorOutput("Try requestFight when is in capture city");
        return;
    }
    if(public_and_private_informations.bot_already_beaten.contains(fightId))
    {
        errorOutput("You can't rebeat this fighter");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput(QStringLiteral("request fight at %1 (%2,%3)").arg(this->map->map_file).arg(this->x).arg(this->y));
    #endif
    CommonMap *map=this->map;
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
                    errorOutput(QStringLiteral("requestFight() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                    return;
                }
            }
            else
            {
                errorOutput(QStringLiteral("No valid map in this direction"));
                return;
            }
        break;
        default:
        errorOutput(QStringLiteral("Wrong direction to use a shop"));
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
                        errorOutput(QStringLiteral("requestFight() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                        return;
                    }
                }
                else
                {
                    errorOutput(QStringLiteral("No valid map in this direction"));
                    return;
                }
            break;
            default:
            errorOutput(QStringLiteral("Wrong direction to use a shop"));
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
            errorOutput(QStringLiteral("no fight with id %1 in this direction").arg(fightId));
            return;
        }
    }
    normalOutput(QStringLiteral("is now in fight (after a request) on map %1 (%2,%3) with the bot %4").arg(map->map_file).arg(x).arg(y).arg(fightId));
    botFightStart(fightId);
}

void Client::clanAction(const quint8 &query_id,const quint8 &action,const QString &text)
{
    switch(action)
    {
        //create
        case 0x01:
        {
            if(public_and_private_informations.clan>0)
            {
                errorOutput(QStringLiteral("You are already in clan"));
                return;
            }
            if(text.isEmpty())
            {
                errorOutput(QStringLiteral("You can't create clan with empty name"));
                return;
            }
            if(!public_and_private_informations.allow.contains(ActionAllow_Clan))
            {
                errorOutput(QStringLiteral("You have not the right to create clan"));
                return;
            }
            ClanActionParam *clanActionParam=new ClanActionParam();
            clanActionParam->query_id=query_id;
            clanActionParam->action=action;
            clanActionParam->text=text;

            const QString &queryText=GlobalServerData::serverPrivateVariables.db_query_select_clan_by_name.arg(SqlFunction::quoteSqlVariable(text));
            CatchChallenger::DatabaseBase::CallBack *callback=GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&Client::addClan_static);
            if(callback==NULL)
            {
                qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());

                QByteArray outputData;
                QDataStream out(&outputData, QIODevice::WriteOnly);
                out.setVersion(QDataStream::Qt_4_4);
                out << (quint8)0x02;
                postReply(query_id,outputData);
                delete clanActionParam;
                return;
            }
            else
            {
                paramToPassToCallBack << clanActionParam;
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                paramToPassToCallBackType << QStringLiteral("ClanActionParam");
                #endif
                callbackRegistred << callback;
            }
            return;
        }
        break;
        //leave
        case 0x02:
        {
            if(public_and_private_informations.clan==0)
            {
                errorOutput(QStringLiteral("You have not a clan"));
                return;
            }
            if(public_and_private_informations.clan_leader)
            {
                errorOutput(QStringLiteral("You can't leave if you are the leader"));
                return;
            }
            removeFromClan();
            clanChangeWithoutDb(public_and_private_informations.clan);
            //send the network reply
            QByteArray outputData;
            QDataStream out(&outputData, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_4_4);
            out << (quint8)0x01;
            postReply(query_id,outputData);
            //update the db
            dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_update_character_clan.arg(character_id));
        }
        break;
        //dissolve
        case 0x03:
        {
            if(public_and_private_informations.clan==0)
            {
                errorOutput(QStringLiteral("You have not a clan"));
                return;
            }
            if(!public_and_private_informations.clan_leader)
            {
                errorOutput(QStringLiteral("You are not a leader to dissolve the clan"));
                return;
            }
            if(!clan->captureCityInProgress.isEmpty())
            {
                errorOutput(QStringLiteral("You can't disolv the clan if is in city capture"));
                return;
            }
            const QList<Client *> &players=clanList.value(public_and_private_informations.clan)->players;
            //send the network reply
            QByteArray outputData;
            QDataStream out(&outputData, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_4_4);
            out << (quint8)0x01;
            postReply(query_id,outputData);
            //update the db
            int index=0;
            while(index<players.size())
            {
                dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_update_character_clan.arg(players.at(index)->getPlayerId()));
                index++;
            }
            dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_delete_clan.arg(public_and_private_informations.clan));
            dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_delete_city.arg(clan->capturedCity));
            //update the object
            clanList.remove(public_and_private_informations.clan);
            GlobalServerData::serverPrivateVariables.cityStatusListReverse.remove(clan->clanId);
            GlobalServerData::serverPrivateVariables.cityStatusList[clan->captureCityInProgress].clan=0;
            delete clan;
            index=0;
            while(index<players.size())
            {
                if(players.at(index)==this)
                {
                    public_and_private_informations.clan=0;
                    clan=NULL;
                    clanChangeWithoutDb(public_and_private_informations.clan);//to send to another thread the clan change, 0 to remove
                }
                else
                    players.at(index)->dissolvedClan();
                index++;
            }
        }
        break;
        //invite
        case 0x04:
        {
            if(public_and_private_informations.clan==0)
            {
                errorOutput(QStringLiteral("You have not a clan"));
                return;
            }
            if(!public_and_private_informations.clan_leader)
            {
                errorOutput(QStringLiteral("You are not a leader to invite into the clan"));
                return;
            }
            bool haveAClan=true;
            if(playerByPseudo.contains(text))
                if(!playerByPseudo.value(text)->haveAClan())
                    haveAClan=false;
            bool isFound=playerByPseudo.contains(text);
            //send the network reply
            QByteArray outputData;
            QDataStream out(&outputData, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_4_4);
            if(isFound && !haveAClan)
            {
                if(playerByPseudo.value(text)->inviteToClan(public_and_private_informations.clan))
                    out << (quint8)0x01;
                else
                    out << (quint8)0x02;
            }
            else
            {
                if(!isFound)
                    normalOutput(QStringLiteral("Clan invite: Player %1 not found, is connected?").arg(text));
                if(haveAClan)
                    normalOutput(QStringLiteral("Clan invite: Player %1 is already into a clan").arg(text));
                out << (quint8)0x02;
            }
            postReply(query_id,outputData);
        }
        break;
        //eject
        case 0x05:
        {
            if(public_and_private_informations.clan==0)
            {
                errorOutput(QStringLiteral("You have not a clan"));
                return;
            }
            if(!public_and_private_informations.clan_leader)
            {
                errorOutput(QStringLiteral("You are not a leader to invite into the clan"));
                return;
            }
            if(public_and_private_informations.public_informations.pseudo==text)
            {
                errorOutput(QStringLiteral("You can't eject your self"));
                return;
            }
            bool isIntoTheClan=false;
            if(playerByPseudo.contains(text))
                if(playerByPseudo.value(text)->getClanId()==public_and_private_informations.clan)
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
                    normalOutput(QStringLiteral("Clan invite: Player %1 not found, is connected?").arg(text));
                if(!isIntoTheClan)
                    normalOutput(QStringLiteral("Clan invite: Player %1 is not into your clan").arg(text));
                out << (quint8)0x02;
            }
            postReply(query_id,outputData);
            if(!isFound)
            {
                dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_update_character_clan_by_pseudo.arg(text).arg(public_and_private_informations.clan));
                return;
            }
            else if(isIntoTheClan)
                playerByPseudo[text]->ejectToClan();
        }
        break;
        default:
            errorOutput(QStringLiteral("Action on the clan not found"));
        return;
    }
}

void Client::addClan_static(void *object)
{
    if(object!=NULL)
        static_cast<Client *>(object)->addClan_object();
    GlobalServerData::serverPrivateVariables.db.clear();
}

void Client::addClan_object()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBack.isEmpty())
    {
        qDebug() << "paramToPassToCallBack.isEmpty()" << __FILE__ << __LINE__;
        abort();
    }
    #endif
    ClanActionParam *clanActionParam=static_cast<ClanActionParam *>(paramToPassToCallBack.takeFirst());
    addClan_return(clanActionParam->query_id,clanActionParam->action,clanActionParam->text);
    delete clanActionParam;
}

void Client::addClan_return(const quint8 &query_id,const quint8 &action,const QString &text)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(paramToPassToCallBackType.takeFirst()!=QStringLiteral("ClanActionParam"))
    {
        qDebug() << "is not ClanActionParam" << paramToPassToCallBackType.join(";") << __FILE__ << __LINE__;
        abort();
    }
    #endif
    callbackRegistred.removeFirst();
    Q_UNUSED(action);
    if(GlobalServerData::serverPrivateVariables.db.next())
    {
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);
        out << (quint8)0x02;
        postReply(query_id,outputData);
        return;
    }
    GlobalServerData::serverPrivateVariables.maxClanId++;
    public_and_private_informations.clan=GlobalServerData::serverPrivateVariables.maxClanId;
    createMemoryClan();
    clan->name=text;
    public_and_private_informations.clan_leader=true;
    //send the network reply
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x01;
    out << (quint32)GlobalServerData::serverPrivateVariables.maxClanId;
    postReply(query_id,outputData);
    //add into db
    dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_insert_clan
             .arg(GlobalServerData::serverPrivateVariables.maxClanId)
             .arg(SqlFunction::quoteSqlVariable(text))
             .arg(QDateTime::currentMSecsSinceEpoch()/1000)
             );
    insertIntoAClan(GlobalServerData::serverPrivateVariables.maxClanId);
}

quint32 Client::getPlayerId() const
{
    if(character_loaded)
        return character_id;
    return 0;
}

void Client::haveClanInfo(const quint32 &clanId,const QString &clanName,const quint64 &cash)
{
    normalOutput(QStringLiteral("First client of the clan: %1, clanId: %2 to connect").arg(clanName).arg(clanId));
    createMemoryClan();
    clanList[clanId]->name=clanName;
    clanList[clanId]->cash=cash;
}

void Client::sendClanInfo()
{
    if(public_and_private_informations.clan==0)
        return;
    if(clan==NULL)
        return;
    normalOutput(QStringLiteral("Send the clan info: %1, clanId: %2, get the info").arg(clan->name).arg(public_and_private_informations.clan));
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << clan->name;
    sendFullPacket(0xC2,0x000A,outputData);
}

void Client::dissolvedClan()
{
    public_and_private_informations.clan=0;
    clan=NULL;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    sendFullPacket(0xC2,0x0009,QByteArray());
    clanChangeWithoutDb(public_and_private_informations.clan);
}

bool Client::inviteToClan(const quint32 &clanId)
{
    if(!inviteToClanList.isEmpty())
        return false;
    if(clan==NULL)
        return false;
    inviteToClanList << clanId;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint32)clanId;
    out << clan->name;
    sendFullPacket(0xC2,0x000B,outputData);
    return false;
}

void Client::clanInvite(const bool &accept)
{
    if(!accept)
    {
        normalOutput(QStringLiteral("You have refused the clan invitation"));
        inviteToClanList.removeFirst();
        return;
    }
    normalOutput(QStringLiteral("You have accepted the clan invitation"));
    if(inviteToClanList.isEmpty())
    {
        errorOutput(QStringLiteral("Can't responde to clan invite, because no in suspend"));
        return;
    }
    public_and_private_informations.clan_leader=false;
    public_and_private_informations.clan=inviteToClanList.first();
    createMemoryClan();
    insertIntoAClan(inviteToClanList.first());
    inviteToClanList.removeFirst();
}

quint32 Client::clanId() const
{
    return public_and_private_informations.clan;
}

void Client::insertIntoAClan(const quint32 &clanId)
{
    //add into db
    QString clan_leader;
    if(GlobalServerData::serverSettings.database.type!=ServerSettings::Database::DatabaseType_PostgreSQL)
    {
        if(public_and_private_informations.clan_leader)
            clan_leader=Client::text_1;
        else
            clan_leader=Client::text_0;
    }
    else
    {
        if(public_and_private_informations.clan_leader)
            clan_leader=Client::text_true;
        else
            clan_leader=Client::text_false;
    }
    dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_update_character_clan_and_leader
             .arg(clanId)
             .arg(clan_leader)
             .arg(character_id)
             );
    sendClanInfo();
    clanChangeWithoutDb(public_and_private_informations.clan);
}

void Client::ejectToClan()
{
    dissolvedClan();
    dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_update_character_clan.arg(character_id));
}

quint32 Client::getClanId() const
{
    return public_and_private_informations.clan;
}

bool Client::haveAClan() const
{
    return public_and_private_informations.clan>0;
}

void Client::waitingForCityCaputre(const bool &cancel)
{
    if(clan==NULL)
    {
        errorOutput(QStringLiteral("Try capture city when is not in clan"));
        return;
    }
    if(!cancel)
    {
        if(captureCityInProgress())
        {
            errorOutput(QStringLiteral("Try capture city when is already into that's"));
            return;
        }
        if(isInFight())
        {
            errorOutput(QStringLiteral("Try capture city when is in fight"));
            return;
        }
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        normalOutput(QStringLiteral("ask zonecapture at %1 (%2,%3)").arg(this->map->map_file).arg(this->x).arg(this->y));
        #endif
        CommonMap *map=this->map;
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
                        errorOutput(QStringLiteral("waitingForCityCaputre() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                        return;
                    }
                }
                else
                {
                    errorOutput(QStringLiteral("No valid map in this direction"));
                    return;
                }
            break;
            default:
            errorOutput("Wrong direction to use a zonecapture");
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
                            errorOutput(QStringLiteral("waitingForCityCaputre() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                            return;
                        }
                    }
                    else
                    {
                        errorOutput(QStringLiteral("No valid map in this direction"));
                        return;
                    }
                break;
                default:
                errorOutput(QStringLiteral("Wrong direction to use a zonecapture"));
                return;
            }
            if(!static_cast<MapServer*>(this->map)->zonecapture.contains(QPair<quint8,quint8>(x,y)))
            {
                errorOutput(QStringLiteral("no zonecapture point in this direction"));
                return;
            }
        }
        //send the zone capture
        const QString &zoneName=static_cast<MapServer*>(this->map)->zonecapture.value(QPair<quint8,quint8>(x,y));
        if(!public_and_private_informations.clan_leader)
        {
            if(clan->captureCityInProgress.isEmpty())
            {
                QByteArray outputData;
                QDataStream out(&outputData, QIODevice::WriteOnly);
                out.setVersion(QDataStream::Qt_4_4);
                out << (quint8)0x01;
                sendFullPacket(0xF0,0x0001,outputData);
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
            sendFullPacket(0xF0,0x0001,outputData);
            return;
        }
        if(captureCity.count(zoneName)>0)
        {
            errorOutput(QStringLiteral("already in capture city"));
            return;
        }
        captureCity[zoneName] << this;
        setInCityCapture(true);
    }
    else
    {
        if(clan->captureCityInProgress.isEmpty())
        {
            errorOutput(QStringLiteral("your clan is not in capture city"));
            return;
        }
        if(!captureCity[clan->captureCityInProgress].removeOne(this))
        {
            errorOutput(QStringLiteral("not in capture city"));
            return;
        }
        leaveTheCityCapture();
    }
}

void Client::leaveTheCityCapture()
{
    if(clan==NULL)
        return;
    if(clan->captureCityInProgress.isEmpty())
        return;
    if(captureCity[clan->captureCityInProgress].removeOne(this))
    {
        //drop all the capture because no body clam it
        if(captureCity.value(clan->captureCityInProgress).isEmpty())
        {
            captureCity.remove(clan->captureCityInProgress);
            clan->captureCityInProgress.clear();
        }
        else
        {
            //drop the clan capture in no other player of the same clan is into it
            int index=0;
            const int &list_size=captureCity.value(clan->captureCityInProgress).size();
            while(index<list_size)
            {
                if(captureCity.value(clan->captureCityInProgress).at(index)->clanId()==clanId())
                    break;
                index++;
            }
            if(index==captureCity.value(clan->captureCityInProgress).size())
                clan->captureCityInProgress.clear();
        }
    }
    setInCityCapture(false);
    otherCityPlayerBattle=NULL;
}

void Client::startTheCityCapture()
{
    QHashIterator<QString,QList<Client *> > i(captureCity);
    while (i.hasNext()) {
        i.next();
        //the city is not free to capture
        if(captureCityValidatedList.contains(i.key()))
        {
            int index=0;
            while(index<i.value().size())
            {
                i.value().at(index)->previousCityCaptureNotFinished();
                index++;
            }
        }
        //the city is ready to be captured
        else
        {
            CaptureCityValidated tempCaptureCityValidated;
            if(!GlobalServerData::serverPrivateVariables.cityStatusList.contains(i.key()))
                GlobalServerData::serverPrivateVariables.cityStatusList[i.key()].clan=0;
            if(GlobalServerData::serverPrivateVariables.cityStatusList.value(i.key()).clan==0)
                if(GlobalServerData::serverPrivateVariables.captureFightIdList.contains(i.key()))
                    tempCaptureCityValidated.bots=GlobalServerData::serverPrivateVariables.captureFightIdList.value(i.key());
            tempCaptureCityValidated.players=i.value();
            int index;
            int sub_index;
            //do the clan count
            int player_count=tempCaptureCityValidated.players.size()+tempCaptureCityValidated.bots.size();
            int clan_count=0;
            if(!tempCaptureCityValidated.bots.isEmpty())
                clan_count++;
            if(!tempCaptureCityValidated.players.isEmpty())
            {
                index=0;
                while(index<tempCaptureCityValidated.players.size())
                {
                    const quint32 &clanId=tempCaptureCityValidated.players.at(index)->clanId();
                    if(tempCaptureCityValidated.clanSize.contains(clanId))
                        tempCaptureCityValidated.clanSize[clanId]++;
                    else
                        tempCaptureCityValidated.clanSize[clanId]=1;
                    index++;
                }
                clan_count+=tempCaptureCityValidated.clanSize.size();
            }
            //do the PvP
            index=0;
            while(index<tempCaptureCityValidated.players.size())
            {
                sub_index=index+1;
                while(sub_index<tempCaptureCityValidated.players.size())
                {
                    if(tempCaptureCityValidated.players.at(index)->clanId()!=tempCaptureCityValidated.players.at(sub_index)->clanId())
                    {
                        tempCaptureCityValidated.players.at(index)->otherCityPlayerBattle=tempCaptureCityValidated.players.at(sub_index);
                        tempCaptureCityValidated.players.at(sub_index)->otherCityPlayerBattle=tempCaptureCityValidated.players.at(index);
                        tempCaptureCityValidated.players.at(index)->battleFakeAccepted(tempCaptureCityValidated.players.at(sub_index));
                        tempCaptureCityValidated.playersInFight << tempCaptureCityValidated.players.at(index);
                        tempCaptureCityValidated.playersInFight.last()->cityCaptureBattle(player_count,clan_count);
                        tempCaptureCityValidated.playersInFight << tempCaptureCityValidated.players.at(sub_index);
                        tempCaptureCityValidated.playersInFight.last()->cityCaptureBattle(player_count,clan_count);
                        tempCaptureCityValidated.players.removeAt(index);
                        index--;
                        tempCaptureCityValidated.players.removeAt(sub_index-1);
                        break;
                    }
                    sub_index++;
                }
                index++;
            }
            //bot the bot fight
            while(!tempCaptureCityValidated.players.isEmpty() && !tempCaptureCityValidated.bots.isEmpty())
            {
                tempCaptureCityValidated.playersInFight << tempCaptureCityValidated.players.first();
                tempCaptureCityValidated.playersInFight.last()->cityCaptureBotFight(player_count,clan_count,tempCaptureCityValidated.bots.first());
                tempCaptureCityValidated.botsInFight << tempCaptureCityValidated.bots.first();
                tempCaptureCityValidated.players.first()->botFightStart(tempCaptureCityValidated.bots.first());
                tempCaptureCityValidated.players.removeFirst();
                tempCaptureCityValidated.bots.removeFirst();
            }
            //send the wait to the rest
            cityCaptureSendInWait(tempCaptureCityValidated,player_count,clan_count);

            captureCityValidatedList[i.key()]=tempCaptureCityValidated;
        }
    }
    captureCity.clear();
}

//fightId == 0 if is in battle
void Client::fightOrBattleFinish(const bool &win, const quint32 &fightId)
{
    if(clan!=NULL)
    {
        if(!clan->captureCityInProgress.isEmpty() && captureCityValidatedList.contains(clan->captureCityInProgress))
        {
            CaptureCityValidated &captureCityValidated=captureCityValidatedList[clan->captureCityInProgress];
            //check if this player is into the capture city with the other player of the team
            if(captureCityValidated.playersInFight.contains(this))
            {
                if(win)
                {
                    if(fightId!=0)
                        captureCityValidated.botsInFight.removeOne(fightId);
                    else
                    {
                        if(otherCityPlayerBattle!=NULL)
                        {
                            captureCityValidated.playersInFight.removeOne(otherCityPlayerBattle);
                            otherCityPlayerBattle=NULL;
                        }
                    }
                    quint16 player_count=cityCapturePlayerCount(captureCityValidated);
                    quint16 clan_count=cityCaptureClanCount(captureCityValidated);
                    bool newFightFound=false;
                    int index=0;
                    while(index<captureCityValidated.players.size())
                    {
                        if(clanId()!=captureCityValidated.players.at(index)->clanId())
                        {
                            battleFakeAccepted(captureCityValidated.players.at(index));
                            captureCityValidated.playersInFight << captureCityValidated.players.at(index);
                            captureCityValidated.playersInFight.last()->cityCaptureBattle(player_count,clan_count);
                            cityCaptureBattle(player_count,clan_count);
                            captureCityValidated.players.removeAt(index);
                            newFightFound=true;
                            break;
                        }
                        index++;
                    }
                    if(!newFightFound && !captureCityValidated.bots.isEmpty())
                    {
                        cityCaptureBotFight(player_count,clan_count,captureCityValidated.bots.first());
                        captureCityValidated.botsInFight << captureCityValidated.bots.first();
                        botFightStart(captureCityValidated.bots.first());
                        captureCityValidated.bots.removeFirst();
                        newFightFound=true;
                    }
                    if(!newFightFound)
                    {
                        captureCityValidated.playersInFight.removeOne(this);
                        captureCityValidated.players << this;
                        otherCityPlayerBattle=NULL;
                    }
                }
                else
                {
                    if(fightId!=0)
                    {
                        captureCityValidated.botsInFight.removeOne(fightId);
                        captureCityValidated.bots.removeOne(fightId);
                    }
                    else
                    {
                        captureCityValidated.playersInFight.removeOne(this);
                        otherCityPlayerBattle=NULL;
                    }
                    captureCityValidated.clanSize[clanId()]--;
                    if(captureCityValidated.clanSize.value(clanId())==0)
                        captureCityValidated.clanSize.remove(clanId());
                }
                quint16 player_count=cityCapturePlayerCount(captureCityValidated);
                quint16 clan_count=cityCaptureClanCount(captureCityValidated);
                //city capture
                if(captureCityValidated.bots.isEmpty() && captureCityValidated.botsInFight.isEmpty() && captureCityValidated.playersInFight.isEmpty())
                {
                    if(clan->capturedCity==clan->captureCityInProgress)
                        clan->captureCityInProgress.clear();
                    else
                    {
                        if(GlobalServerData::serverPrivateVariables.cityStatusList.contains(clan->capturedCity))
                        {
                            GlobalServerData::serverPrivateVariables.cityStatusListReverse.remove(clan->clanId);
                            GlobalServerData::serverPrivateVariables.cityStatusList[clan->capturedCity].clan=0;
                        }
                        dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_delete_city.arg(clan->capturedCity));
                        if(!GlobalServerData::serverPrivateVariables.cityStatusList.contains(clan->captureCityInProgress))
                            GlobalServerData::serverPrivateVariables.cityStatusList[clan->captureCityInProgress].clan=0;
                        if(GlobalServerData::serverPrivateVariables.cityStatusList.value(clan->captureCityInProgress).clan!=0)
                            dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_update_city_clan.arg(clan->clanId).arg(clan->captureCityInProgress));
                        else
                            dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_insert_city.arg(clan->clanId).arg(clan->captureCityInProgress));
                        GlobalServerData::serverPrivateVariables.cityStatusListReverse[clan->clanId]=clan->captureCityInProgress;
                        GlobalServerData::serverPrivateVariables.cityStatusList[clan->captureCityInProgress].clan=clan->clanId;
                        clan->capturedCity=clan->captureCityInProgress;
                        clan->captureCityInProgress.clear();
                        int index=0;
                        while(index<captureCityValidated.players.size())
                        {
                            captureCityValidated.players.last()->cityCaptureWin();
                            index++;
                        }
                    }
                }
                else
                    cityCaptureSendInWait(captureCityValidated,player_count,clan_count);
                return;
            }
        }
    }
}

void Client::cityCaptureSendInWait(const CaptureCityValidated &captureCityValidated, const quint16 &number_of_player, const quint16 &number_of_clan)
{
    int index=0;
    while(index<captureCityValidated.players.size())
    {
        captureCityValidated.playersInFight.last()->cityCaptureInWait(number_of_player,number_of_clan);
        index++;
    }
}

quint16 Client::cityCapturePlayerCount(const CaptureCityValidated &captureCityValidated)
{
    return captureCityValidated.bots.size()+captureCityValidated.botsInFight.size()+captureCityValidated.players.size()+captureCityValidated.playersInFight.size();
}

quint16 Client::cityCaptureClanCount(const CaptureCityValidated &captureCityValidated)
{
    if(captureCityValidated.bots.isEmpty() && captureCityValidated.botsInFight.isEmpty())
        return captureCityValidated.clanSize.size();
    else
        return captureCityValidated.clanSize.size()+1;
}

void Client::cityCaptureBattle(const quint16 &number_of_player,const quint16 &number_of_clan)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x04;
    out << (quint16)number_of_player;
    out << (quint16)number_of_clan;
    sendFullPacket(0xF0,0x0001,outputData);
}

void Client::cityCaptureBotFight(const quint16 &number_of_player,const quint16 &number_of_clan,const quint32 &fightId)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x04;
    out << (quint16)number_of_player;
    out << (quint16)number_of_clan;
    out << (quint32)fightId;
    sendFullPacket(0xF0,0x0001,outputData);
}

void Client::cityCaptureInWait(const quint16 &number_of_player,const quint16 &number_of_clan)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x05;
    out << (quint16)number_of_player;
    out << (quint16)number_of_clan;
    sendFullPacket(0xF0,0x0001,outputData);
}

void Client::cityCaptureWin()
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x06;
    sendFullPacket(0xF0,0x0001,outputData);
}

void Client::previousCityCaptureNotFinished()
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x02;
    sendFullPacket(0xF0,0x0003,outputData);
}

void Client::moveMonster(const bool &up,const quint8 &number)
{
    if(up)
        moveUpMonster(number-1);
    else
        moveDownMonster(number-1);
}

void Client::getMarketList(const quint32 &query_id)
{
    if(getInTrade() || isInFight())
    {
        errorOutput(QStringLiteral("You can't use the market in trade/fight"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint64)market_cash;
    int index;
    QList<MarketItem> marketItemList,marketOwnItemList;
    QList<MarketPlayerMonster> marketPlayerMonsterList,marketOwnPlayerMonsterList;
    //object filter
    index=0;
    while(index<GlobalServerData::serverPrivateVariables.marketItemList.size())
    {
        const MarketItem &marketObject=GlobalServerData::serverPrivateVariables.marketItemList.at(index);
        if(marketObject.player==character_id)
            marketOwnItemList << marketObject;
        else
            marketItemList << marketObject;
        index++;
    }
    //monster filter
    index=0;
    while(index<GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.size())
    {
        const MarketPlayerMonster &marketPlayerMonster=GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.at(index);
        if(marketPlayerMonster.player==character_id)
            marketOwnPlayerMonsterList << marketPlayerMonster;
        else
            marketPlayerMonsterList << marketPlayerMonster;
        index++;
    }
    //object
    out << (quint32)marketItemList.size();
    index=0;
    while(index<marketItemList.size())
    {
        const MarketItem &marketObject=marketItemList.at(index);
        out << marketObject.marketObjectId;
        out << marketObject.item;
        out << marketObject.quantity;
        out << marketObject.cash;
        index++;
    }
    //monster
    out << (quint32)marketPlayerMonsterList.size();
    index=0;
    while(index<marketPlayerMonsterList.size())
    {
        const MarketPlayerMonster &marketPlayerMonster=marketPlayerMonsterList.at(index);
        out << marketPlayerMonster.monster.id;
        out << marketPlayerMonster.monster.monster;
        out << marketPlayerMonster.monster.level;
        out << marketPlayerMonster.cash;
        index++;
    }
    //own object
    out << (quint32)marketOwnItemList.size();
    index=0;
    while(index<marketOwnItemList.size())
    {
        const MarketItem &marketObject=marketOwnItemList.at(index);
        out << marketObject.marketObjectId;
        out << marketObject.item;
        out << marketObject.quantity;
        out << marketObject.cash;
        index++;
    }
    //own monster
    out << (quint32)marketPlayerMonsterList.size();
    index=0;
    while(index<marketPlayerMonsterList.size())
    {
        const MarketPlayerMonster &marketPlayerMonster=marketPlayerMonsterList.at(index);
        out << marketPlayerMonster.monster.id;
        out << marketPlayerMonster.monster.monster;
        out << marketPlayerMonster.monster.level;
        out << marketPlayerMonster.cash;
        index++;
    }

    postReply(query_id,outputData);
}

void Client::buyMarketObject(const quint32 &query_id,const quint32 &marketObjectId,const quint32 &quantity)
{
    if(getInTrade() || isInFight())
    {
        errorOutput(QStringLiteral("You can't use the market in trade/fight"));
        return;
    }
    if(quantity<=0)
    {
        errorOutput(QStringLiteral("You can't use the market with null quantity"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    //search into the market
    int index=0;
    while(index<GlobalServerData::serverPrivateVariables.marketItemList.size())
    {
        const MarketItem marketItem=GlobalServerData::serverPrivateVariables.marketItemList.at(index);
        if(marketItem.marketObjectId==marketObjectId)
        {
            if(marketItem.quantity<quantity)
            {
                out << (quint8)0x02;
                postReply(query_id,outputData);
                return;
            }
            //check if have the price
            if((quantity*marketItem.cash)>public_and_private_informations.cash)
            {
                out << (quint8)0x03;
                postReply(query_id,outputData);
                return;
            }
            //apply the buy
            if(marketItem.quantity==quantity)
            {
                dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_delete_item_market.arg(marketItem.item).arg(marketItem.player));
                GlobalServerData::serverPrivateVariables.marketItemList.removeAt(index);
            }
            else
            {
                GlobalServerData::serverPrivateVariables.marketItemList[index].quantity=marketItem.quantity-quantity;
                dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_update_item_market
                             .arg(marketItem.quantity-quantity)
                             .arg(marketItem.item)
                             .arg(marketItem.player)
                             );
            }
            removeCash(quantity*marketItem.cash);
            if(playerById.contains(marketItem.player))
            {
                if(!playerById.value(marketItem.player)->addMarketCashWithoutSave(quantity*marketItem.cash))
                    normalOutput(QStringLiteral("Problem at market cash adding"));
            }
            dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_update_charaters_market_cash
                         .arg(quantity*marketItem.cash)
                         .arg(marketItem.player)
                         );
            addObject(marketItem.item,quantity);
            out << (quint8)0x01;
            postReply(query_id,outputData);
            return;
        }
        index++;
    }
    out << (quint8)0x03;
    postReply(query_id,outputData);
}

void Client::buyMarketMonster(const quint32 &query_id,const quint32 &monsterId)
{
    if(getInTrade() || isInFight())
    {
        errorOutput(QStringLiteral("You can't use the market in trade/fight"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    if(public_and_private_informations.playerMonster.size()>=CommonSettings::commonSettings.maxPlayerMonsters)
    {
        out << (quint8)0x02;
        postReply(query_id,outputData);
        return;
    }
    //search into the market
    int index=0;
    while(index<GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.size())
    {
        const MarketPlayerMonster marketPlayerMonster=GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.at(index);
        if(marketPlayerMonster.monster.id==monsterId)
        {
            //check if have the price
            if(marketPlayerMonster.cash>public_and_private_informations.cash)
            {
                out << (quint8)0x03;
                postReply(query_id,outputData);
                return;
            }
            //apply the buy
            GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.removeAt(index);
            removeCash(marketPlayerMonster.cash);
            dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_update_charaters_market_cash
                         .arg(marketPlayerMonster.cash)
                         .arg(marketPlayerMonster.player)
                         );
            addPlayerMonster(marketPlayerMonster.monster);
            dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_delete_monster_market_by_id.arg(marketPlayerMonster.monster.id));
            if(CommonSettings::commonSettings.useSP)
                dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_insert_monster_full
                         .arg(
                             QStringLiteral("%1,%2,%3,%4,%5,%6,%7,%8,%9")
                             .arg(marketPlayerMonster.monster.id)
                             .arg(marketPlayerMonster.monster.hp)
                             .arg(character_id)
                             .arg(marketPlayerMonster.monster.monster)
                             .arg(marketPlayerMonster.monster.level)
                             .arg(marketPlayerMonster.monster.remaining_xp)
                             .arg(marketPlayerMonster.monster.sp)
                             .arg(marketPlayerMonster.monster.catched_with)
                             .arg((quint8)marketPlayerMonster.monster.gender)
                             )
                         .arg(
                             QStringLiteral("%1,%2,%3")
                             .arg(marketPlayerMonster.monster.egg_step)
                             .arg(marketPlayerMonster.monster.character_origin)
                             .arg(getPlayerMonster().size())
                             )
                         );
            else
                dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_insert_monster_full
                         .arg(
                             QStringLiteral("%1,%2,%3,%4,%5,%6,%7,%8")
                             .arg(marketPlayerMonster.monster.id)
                             .arg(marketPlayerMonster.monster.hp)
                             .arg(character_id)
                             .arg(marketPlayerMonster.monster.monster)
                             .arg(marketPlayerMonster.monster.level)
                             .arg(marketPlayerMonster.monster.remaining_xp)
                             .arg(marketPlayerMonster.monster.catched_with)
                             .arg((quint8)marketPlayerMonster.monster.gender)
                             )
                         .arg(
                             QStringLiteral("%1,%2,%3")
                             .arg(marketPlayerMonster.monster.egg_step)
                             .arg(marketPlayerMonster.monster.character_origin)
                             .arg(getPlayerMonster().size())
                             )
                         );
            out << (quint8)0x01;
            postReply(query_id,outputData);
            return;
        }
        index++;
    }
    out << (quint8)0x03;
    postReply(query_id,outputData);
}

void Client::putMarketObject(const quint32 &query_id,const quint32 &objectId,const quint32 &quantity,const quint32 &price)
{
    if(getInTrade() || isInFight())
    {
        errorOutput(QStringLiteral("You can't use the market in trade/fight"));
        return;
    }
    if(quantity<=0)
    {
        errorOutput(QStringLiteral("You can't use the market with null quantity"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    if(objectQuantity(objectId)<quantity)
    {
        out << (quint8)0x02;
        postReply(query_id,outputData);
        return;
    }
    //search into the market
    int index=0;
    while(index<GlobalServerData::serverPrivateVariables.marketItemList.size())
    {
        const MarketItem &marketItem=GlobalServerData::serverPrivateVariables.marketItemList.at(index);
        if(marketItem.player==character_id && marketItem.item==objectId)
        {
            removeObject(objectId,quantity);
            GlobalServerData::serverPrivateVariables.marketItemList[index].cash=price;
            GlobalServerData::serverPrivateVariables.marketItemList[index].quantity+=quantity;
            out << (quint8)0x01;
            postReply(query_id,outputData);
            dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_update_item_market_and_price
                .arg(GlobalServerData::serverPrivateVariables.marketItemList[index].quantity)
                .arg(price)
                .arg(objectId)
                .arg(character_id)
                );
            return;
        }
        index++;
    }
    if(marketObjectIdList.isEmpty())
    {
        out << (quint8)0x02;
        postReply(query_id,outputData);
        normalOutput(QStringLiteral("No more id into marketObjectIdList"));
        return;
    }
    //append to the market
    removeObject(objectId,quantity);
    dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_insert_item_market
                 .arg(objectId)
                 .arg(character_id)
                 .arg(quantity)
                 .arg(price)
                 );
    MarketItem marketItem;
    marketItem.cash=price;
    marketItem.item=objectId;
    marketItem.marketObjectId=marketObjectIdList.first();
    marketItem.player=character_id;
    marketItem.quantity=quantity;
    marketObjectIdList.removeFirst();
    GlobalServerData::serverPrivateVariables.marketItemList << marketItem;
    out << (quint8)0x01;
    postReply(query_id,outputData);
}

void Client::putMarketMonster(const quint32 &query_id,const quint32 &monsterId,const quint32 &price)
{
    if(getInTrade() || isInFight())
    {
        errorOutput(QStringLiteral("You can't use the market in trade/fight"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    int index=0;
    while(index<public_and_private_informations.playerMonster.size())
    {
        const PlayerMonster &playerMonster=public_and_private_informations.playerMonster.at(index);
        if(playerMonster.id==monsterId)
        {
            if(!remainMonstersToFight(monsterId))
            {
                normalOutput(QStringLiteral("You can't put in market this msonter because you will be without monster to fight"));
                out << (quint8)0x02;
                postReply(query_id,outputData);
                return;
            }
            MarketPlayerMonster marketPlayerMonster;
            marketPlayerMonster.cash=price;
            marketPlayerMonster.monster=playerMonster;
            marketPlayerMonster.player=character_id;
            public_and_private_informations.playerMonster.removeAt(index);
            GlobalServerData::serverPrivateVariables.marketPlayerMonsterList << marketPlayerMonster;
            dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_delete_monster_by_id.arg(marketPlayerMonster.monster.id));
            if(CommonSettings::commonSettings.useSP)
                dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_insert_monster_market
                         .arg(
                             QStringLiteral("%1,%2,%3,%4,%5,%6,%7,%8,%9")
                             .arg(marketPlayerMonster.monster.id)
                             .arg(marketPlayerMonster.monster.hp)
                             .arg(character_id)
                             .arg(marketPlayerMonster.monster.monster)
                             .arg(marketPlayerMonster.monster.level)
                             .arg(marketPlayerMonster.monster.remaining_xp)
                             .arg(marketPlayerMonster.monster.sp)
                             .arg(marketPlayerMonster.monster.catched_with)
                             .arg((quint8)marketPlayerMonster.monster.gender)
                             )
                         .arg(
                             QStringLiteral("%1,%2,%3")
                             .arg(marketPlayerMonster.monster.egg_step)
                             .arg(marketPlayerMonster.monster.character_origin)
                             .arg(price)
                             )
                         );
            else
                dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_insert_monster_market
                         .arg(
                             QStringLiteral("%1,%2,%3,%4,%5,%6,%7,%8,%9")
                             .arg(marketPlayerMonster.monster.id)
                             .arg(marketPlayerMonster.monster.hp)
                             .arg(character_id)
                             .arg(marketPlayerMonster.monster.monster)
                             .arg(marketPlayerMonster.monster.level)
                             .arg(marketPlayerMonster.monster.remaining_xp)
                             .arg(marketPlayerMonster.monster.catched_with)
                             .arg((quint8)marketPlayerMonster.monster.gender)
                             )
                         .arg(
                             QStringLiteral("%1,%2,%3")
                             .arg(marketPlayerMonster.monster.egg_step)
                             .arg(marketPlayerMonster.monster.character_origin)
                             .arg(price)
                             )
                         );
            while(index<public_and_private_informations.playerMonster.size())
            {
                const PlayerMonster &playerMonster=public_and_private_informations.playerMonster.at(index);
                dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_update_monster_position
                             .arg(index+1)
                             .arg(playerMonster.id)
                             );
                index++;
            }
            out << (quint8)0x01;
            postReply(query_id,outputData);
            return;
        }
        index++;
    }
    out << (quint8)0x02;
    postReply(query_id,outputData);
}

void Client::recoverMarketCash(const quint32 &query_id)
{
    if(getInTrade() || isInFight())
    {
        errorOutput(QStringLiteral("You can't use the market in trade/fight"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint64)market_cash;
    public_and_private_informations.cash+=market_cash;
    market_cash=0;
    dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_get_market_cash
                 .arg(public_and_private_informations.cash)
                 .arg(character_id)
                 );
    postReply(query_id,outputData);
}

void Client::withdrawMarketObject(const quint32 &query_id,const quint32 &objectId,const quint32 &quantity)
{
    if(getInTrade() || isInFight())
    {
        errorOutput(QStringLiteral("You can't use the market in trade/fight"));
        return;
    }
    if(quantity<=0)
    {
        errorOutput(QStringLiteral("You can't use the market with null quantity"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    int index=0;
    while(index<GlobalServerData::serverPrivateVariables.marketItemList.size())
    {
        const MarketItem &marketItem=GlobalServerData::serverPrivateVariables.marketItemList.at(index);
        if(marketItem.item==objectId)
        {
            if(marketItem.player!=character_id)
            {
                out << (quint8)0x02;
                postReply(query_id,outputData);
                return;
            }
            if(marketItem.quantity<quantity)
            {
                out << (quint8)0x02;
                postReply(query_id,outputData);
                return;
            }
            out << (quint8)0x01;
            out << (quint8)0x01;
            out << marketItem.item;
            out << marketItem.quantity;
            GlobalServerData::serverPrivateVariables.marketItemList[index].quantity=marketItem.quantity-quantity;
            if(GlobalServerData::serverPrivateVariables.marketItemList.value(index).quantity==0)
            {
                marketObjectIdList << marketItem.marketObjectId;
                GlobalServerData::serverPrivateVariables.marketItemList.removeAt(index);
                dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_delete_item_market
                             .arg(objectId)
                             .arg(character_id)
                             );
            }
            else
                dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_update_item_market
                             .arg(GlobalServerData::serverPrivateVariables.marketItemList.value(index).quantity)
                             .arg(objectId)
                             .arg(character_id)
                             );
            addObject(objectId,quantity);
            postReply(query_id,outputData);
            return;
        }
        index++;
    }
    out << (quint8)0x02;
    postReply(query_id,outputData);
}

void Client::withdrawMarketMonster(const quint32 &query_id,const quint32 &monsterId)
{
    if(getInTrade() || isInFight())
    {
        errorOutput(QStringLiteral("You can't use the market in trade/fight"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    int index=0;
    while(index<GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.size())
    {
        const MarketPlayerMonster &marketPlayerMonster=GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.at(index);
        if(marketPlayerMonster.monster.id==monsterId)
        {
            if(marketPlayerMonster.player!=character_id)
            {
                out << (quint8)0x02;
                postReply(query_id,outputData);
                return;
            }
            if(public_and_private_informations.playerMonster.size()>=CommonSettings::commonSettings.maxPlayerMonsters)
            {
                out << (quint8)0x02;
                postReply(query_id,outputData);
                return;
            }
            GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.removeAt(index);
            public_and_private_informations.playerMonster << marketPlayerMonster.monster;
            dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_delete_monster_by_id.arg(marketPlayerMonster.monster.id));
            if(CommonSettings::commonSettings.useSP)
                dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_insert_monster_market
                         .arg(
                             QStringLiteral("%1,%2,%3,%4,%5,%6,%7,%8,%9")
                             .arg(marketPlayerMonster.monster.id)
                             .arg(marketPlayerMonster.monster.hp)
                             .arg(character_id)
                             .arg(marketPlayerMonster.monster.monster)
                             .arg(marketPlayerMonster.monster.level)
                             .arg(marketPlayerMonster.monster.remaining_xp)
                             .arg(marketPlayerMonster.monster.sp)
                             .arg(marketPlayerMonster.monster.catched_with)
                             .arg((quint8)marketPlayerMonster.monster.gender)
                             )
                         .arg(
                             QStringLiteral("%1,%2,%3")
                             .arg(marketPlayerMonster.monster.egg_step)
                             .arg(marketPlayerMonster.monster.character_origin)
                             .arg(public_and_private_informations.playerMonster.size())
                             )
                         );
            else
                dbQueryWrite(GlobalServerData::serverPrivateVariables.db_query_insert_monster_market
                         .arg(
                             QStringLiteral("%1,%2,%3,%4,%5,%6,%7,%8")
                             .arg(marketPlayerMonster.monster.id)
                             .arg(marketPlayerMonster.monster.hp)
                             .arg(character_id)
                             .arg(marketPlayerMonster.monster.monster)
                             .arg(marketPlayerMonster.monster.level)
                             .arg(marketPlayerMonster.monster.remaining_xp)
                             .arg(marketPlayerMonster.monster.catched_with)
                             .arg((quint8)marketPlayerMonster.monster.gender)
                             )
                         .arg(
                             QStringLiteral("%1,%2,%3")
                             .arg(marketPlayerMonster.monster.egg_step)
                             .arg(marketPlayerMonster.monster.character_origin)
                             .arg(public_and_private_informations.playerMonster.size())
                             )
                         );
            out << (quint8)0x01;
            out << (quint8)0x02;
            postReply(query_id,outputData+FacilityLib::privateMonsterToBinary(public_and_private_informations.playerMonster.last()));
            return;
        }
        index++;
    }
    out << (quint8)0x02;
    postReply(query_id,outputData);
}

bool Client::haveReputationRequirements(const QList<ReputationRequirements> &reputationList) const
{
    int index=0;
    while(index<reputationList.size())
    {
        const CatchChallenger::ReputationRequirements &reputationRequierement=reputationList.at(index);
        if(public_and_private_informations.reputation.contains(reputationRequierement.reputationId))
        {
            const PlayerReputation &playerReputation=public_and_private_informations.reputation.value(reputationRequierement.reputationId);
            if(!reputationRequierement.positif)
            {
                if(-reputationRequierement.level<playerReputation.level)
                {
                    normalOutput(QStringLiteral("reputation.level(%1)<playerReputation.level(%2)").arg(reputationRequierement.level).arg(playerReputation.level));
                    return false;
                }
            }
            else
            {
                if(reputationRequierement.level>playerReputation.level || playerReputation.point<0)
                {
                    normalOutput(QStringLiteral("reputation.level(%1)>playerReputation.level(%2) || playerReputation.point(%3)<0").arg(reputationRequierement.level).arg(playerReputation.level).arg(playerReputation.point));
                    return false;
                }
            }
        }
        else
            if(!reputationRequierement.positif)//default level is 0, but required level is negative
            {
                normalOutput(QStringLiteral("reputation.level(%1)<0 and no reputation.type=%2").arg(reputationRequierement.level).arg(CommonDatapack::commonDatapack.reputation.at(reputationRequierement.reputationId).name));
                return false;
            }
        index++;
    }
    return true;
}
