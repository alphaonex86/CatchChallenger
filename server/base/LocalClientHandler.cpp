#include "LocalClientHandler.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/FacilityLib.h"
#include "GlobalServerData.h"
#include "LocalClientHandlerWithoutSender.h"

#include <QStringList>

using namespace CatchChallenger;

Direction LocalClientHandler::temp_direction;
QHash<QString,LocalClientHandler *> LocalClientHandler::playerByPseudo;
QHash<quint32,LocalClientHandler *> LocalClientHandler::playerById;
QHash<QString,QList<LocalClientHandler *> > LocalClientHandler::captureCity;
QHash<QString,LocalClientHandler::CaptureCityValidated> LocalClientHandler::captureCityValidatedList;
QHash<quint32,LocalClientHandler::Clan *> LocalClientHandler::clanList;

QList<quint16> LocalClientHandler::marketObjectIdList;
QRegularExpression LocalClientHandler::tmxRemove=QRegularExpression(QLatin1String("\\.tmx$"));

QString LocalClientHandler::text_top=QLatin1Literal("top");
QString LocalClientHandler::text_bottom=QLatin1Literal("bottom");
QString LocalClientHandler::text_left=QLatin1Literal("left");
QString LocalClientHandler::text_right=QLatin1Literal("right");
QString LocalClientHandler::text_give=QLatin1Literal("give");
QString LocalClientHandler::text_space=QLatin1Literal(" ");
QString LocalClientHandler::text_0=QLatin1Literal("0");
QString LocalClientHandler::text_1=QLatin1Literal("1");
QString LocalClientHandler::text_take=QLatin1Literal("take");
QString LocalClientHandler::text_tp=QLatin1Literal("tp");
QString LocalClientHandler::text_trade=QLatin1Literal("trade");
QString LocalClientHandler::text_battle=QLatin1Literal("battle");
QString LocalClientHandler::text_to=QLatin1Literal("to");
QString LocalClientHandler::text_dotcomma=QLatin1Literal(";");

LocalClientHandler::LocalClientHandler() :
    otherPlayerTrade(NULL),
    tradeIsValidated(false),
    clan(NULL),
    otherCityPlayerBattle(NULL)
{
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
    connect(&localClientHandlerFight,&LocalClientHandlerFight::fightOrBattleFinish, this,&LocalClientHandler::fightOrBattleFinish);
}

LocalClientHandler::~LocalClientHandler()
{
}

void LocalClientHandler::setVariable(Player_internal_informations *player_informations)
{
    MapBasicMove::setVariable(player_informations);
    localClientHandlerFight.setVariableInternal(player_informations);
}

bool LocalClientHandler::checkCollision()
{
    if(map->parsed_layer.walkable==NULL)
        return false;
    if(!map->parsed_layer.walkable[x+y*map->width])
    {
        emit error(QStringLiteral("move at %1, can't wall at: %2,%3 on map: %4").arg(temp_direction).arg(x).arg(y).arg(map->map_file));
        return false;
    }
    else
        return true;
}

void LocalClientHandler::removeFromClan()
{
    if(clan!=NULL)
    {
        if(!clan->captureCityInProgress.isEmpty())
        {
            if(captureCity[clan->captureCityInProgress].removeOne(this))
            {
                if(captureCity.value(clan->captureCityInProgress).isEmpty())
                    captureCity.remove(clan->captureCityInProgress);
            }
        }
        if(clan->players.removeOne(this))
        {
            delete clan;
            clanList.remove(player_informations->public_and_private_informations.clan);
        }
    }
    player_informations->public_and_private_informations.clan=0;
}

/// \todo battle with capture city canceled
void LocalClientHandler::extraStop()
{
    LocalClientHandlerWithoutSender::localClientHandlerWithoutSender.allClient.removeOne(this);

    tradeCanceled();
    localClientHandlerFight.battleCanceled();
    if(player_informations->character_loaded)
    {
        playerByPseudo.remove(player_informations->public_and_private_informations.public_informations.pseudo);
        playerById.remove(player_informations->character_id);
        leaveTheCityCapture();
    }
    removeFromClan();

    if(!player_informations->character_loaded)
        return;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            emit dbQuery(GlobalServerData::serverPrivateVariables.db_query_played_time.arg(player_informations->character_id).arg(QDateTime::currentDateTime().toMSecsSinceEpoch()/1000-player_informations->connectedSince.toMSecsSinceEpoch()/1000));
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            emit dbQuery(GlobalServerData::serverPrivateVariables.db_query_played_time.arg(player_informations->character_id).arg(QDateTime::currentDateTime().toMSecsSinceEpoch()/1000-player_informations->connectedSince.toMSecsSinceEpoch()/1000));
        break;
    }
    //save the monster
    if(GlobalServerData::serverSettings.database.fightSync==ServerSettings::Database::FightSync_AtTheEndOfBattle && localClientHandlerFight.isInFight())
        localClientHandlerFight.saveCurrentMonsterStat();
    if(GlobalServerData::serverSettings.database.fightSync==ServerSettings::Database::FightSync_AtTheDisconnexion)
    {
        int index=0;
        const int &size=player_informations->public_and_private_informations.playerMonster.size();
        while(index<size)
        {
            const PlayerMonster &playerMonster=player_informations->public_and_private_informations.playerMonster.at(index);
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    emit dbQuery(GlobalServerData::serverPrivateVariables.db_query_monster
                                 .arg(playerMonster.id)
                                 .arg(player_informations->character_id)
                                 .arg(playerMonster.hp)
                                 .arg(playerMonster.remaining_xp)
                                 .arg(playerMonster.level)
                                 .arg(playerMonster.sp)
                                 .arg(index+1)
                                 );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    emit dbQuery(GlobalServerData::serverPrivateVariables.db_query_monster
                                 .arg(playerMonster.id)
                                 .arg(player_informations->character_id)
                                 .arg(playerMonster.hp)
                                 .arg(playerMonster.remaining_xp)
                                 .arg(playerMonster.level)
                                 .arg(playerMonster.sp)
                                 .arg(index+1)
                                 );
                break;
            }
            int sub_index=0;
            const int &sub_size=playerMonster.skills.size();
            while(sub_index<sub_size)
            {
                const PlayerMonster::PlayerSkill &playerSkill=playerMonster.skills.at(sub_index);
                switch(GlobalServerData::serverSettings.database.type)
                {
                    default:
                    case ServerSettings::Database::DatabaseType_Mysql:
                        emit dbQuery(GlobalServerData::serverPrivateVariables.db_query_monster_skill
                                     .arg(playerSkill.endurance)
                                     .arg(playerMonster.id)
                                     .arg(playerSkill.skill)
                                     );
                    break;
                    case ServerSettings::Database::DatabaseType_SQLite:
                        emit dbQuery(GlobalServerData::serverPrivateVariables.db_query_monster_skill
                                     .arg(playerSkill.endurance)
                                     .arg(playerMonster.id)
                                     .arg(playerSkill.skill)
                                     );
                    break;
                }
                sub_index++;
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
            return LocalClientHandler::text_top;
        break;
        case Direction_look_at_right:
        case Direction_move_at_right:
            return LocalClientHandler::text_right;
        break;
        case Direction_look_at_bottom:
        case Direction_move_at_bottom:
            return LocalClientHandler::text_bottom;
        break;
        case Direction_look_at_left:
        case Direction_move_at_left:
            return LocalClientHandler::text_left;
        break;
        default:
        break;
    }
    return LocalClientHandler::text_bottom;
}

QString LocalClientHandler::orientationToStringToSave(const Orientation &orientation)
{
    switch(orientation)
    {
        case Orientation_top:
            return LocalClientHandler::text_top;
        break;
        case Orientation_bottom:
            return LocalClientHandler::text_bottom;
        break;
        case Orientation_right:
            return LocalClientHandler::text_right;
        break;
        case Orientation_left:
            return LocalClientHandler::text_left;
        break;
        default:
        break;
    }
    return LocalClientHandler::text_bottom;
}

void LocalClientHandler::savePosition()
{
    //virtual stop the player
    //Orientation orientation;
    #ifdef DEBUG_MESSAGE_CLIENT_MOVE
    emit message(
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
    QString map_file_clean=map->map_file;
    map_file_clean.remove(tmxRemove);
    QString rescue_map_file_clean=player_informations->rescue.map->map_file;
    rescue_map_file_clean.remove(tmxRemove);
    QString unvalidated_rescue_map_file_clean=player_informations->unvalidated_rescue.map->map_file;
    unvalidated_rescue_map_file_clean.remove(tmxRemove);
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            updateMapPositionQuery=QStringLiteral("UPDATE `character` SET `map`=\"%1\",`x`=%2,`y`=%3,`orientation`=\"%4\",%5 WHERE `id`=%6")
                .arg(SqlFunction::quoteSqlVariable(map_file_clean))
                .arg(x)
                .arg(y)
                .arg(directionToStringToSave(getLastDirection()))
                .arg(
                        QStringLiteral("`rescue_map`=\"%1\",`rescue_x`=%2,`rescue_y`=%3,`rescue_orientation`=\"%4\",`unvalidated_rescue_map`=\"%5\",`unvalidated_rescue_x`=%6,`unvalidated_rescue_y`=%7,`unvalidated_rescue_orientation`=\"%8\"")
                        .arg(rescue_map_file_clean)
                        .arg(player_informations->rescue.x)
                        .arg(player_informations->rescue.y)
                        .arg(orientationToStringToSave(player_informations->rescue.orientation))
                        .arg(unvalidated_rescue_map_file_clean)
                        .arg(player_informations->unvalidated_rescue.x)
                        .arg(player_informations->unvalidated_rescue.y)
                        .arg(orientationToStringToSave(player_informations->unvalidated_rescue.orientation))
                )
                .arg(player_informations->character_id);
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            updateMapPositionQuery=QStringLiteral("UPDATE character SET map=\"%1\",x=%2,y=%3,orientation=\"%4\",%5 WHERE id=%6")
                .arg(SqlFunction::quoteSqlVariable(map_file_clean))
                .arg(x)
                .arg(y)
                .arg(directionToStringToSave(getLastDirection()))
                .arg(
                        QStringLiteral("rescue_map=\"%1\",rescue_x=%2,rescue_y=%3,rescue_orientation=\"%4\",unvalidated_rescue_map=\"%5\",unvalidated_rescue_x=%6,unvalidated_rescue_y=%7,unvalidated_rescue_orientation=\"%8\"")
                        .arg(rescue_map_file_clean)
                        .arg(player_informations->rescue.x)
                        .arg(player_informations->rescue.y)
                        .arg(orientationToStringToSave(player_informations->rescue.orientation))
                        .arg(unvalidated_rescue_map_file_clean)
                        .arg(player_informations->unvalidated_rescue.x)
                        .arg(player_informations->unvalidated_rescue.y)
                        .arg(orientationToStringToSave(player_informations->unvalidated_rescue.orientation))
                )
                .arg(player_informations->character_id);
        break;
    }
    emit dbQuery(updateMapPositionQuery);
}

/* why do that's here?
 * Because the ClientMapManagement can be totaly satured by the square complexity
 * that's allow to continue the player to connect and play
 * the overhead for the network it just at the connexion */
void LocalClientHandler::put_on_the_map(CommonMap *map,const COORD_TYPE &x,const COORD_TYPE &y,const Orientation &orientation)
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
    if(GlobalServerData::serverSettings.dontSendPlayerType)
        out << quint8((quint8)orientation | (quint8)Player_type_normal);
    else
        out << quint8((quint8)orientation | (quint8)player_informations->public_and_private_informations.public_informations.type);
    if(CommonSettings::commonSettings.forcedSpeed==0)
        out << player_informations->public_and_private_informations.public_informations.speed;

    if(!CommonSettings::commonSettings.dontSendPseudo)
    {
        outputData+=player_informations->rawPseudo;
        out.device()->seek(out.device()->pos()+player_informations->rawPseudo.size());
    }
    out << player_informations->public_and_private_informations.public_informations.skinId;

    emit sendPacket(0xC0,outputData);

    //load the first time the random number list
    localClientHandlerFight.getRandomNumberIfNeeded();

    playerByPseudo[player_informations->public_and_private_informations.public_informations.pseudo]=this;
    playerById[player_informations->character_id]=this;
    if(player_informations->public_and_private_informations.clan>0)
        sendClanInfo();

    localClientHandlerFight.updateCanDoFight();
    if(localClientHandlerFight.getAbleToFight())
        localClientHandlerFight.botFightCollision(map,x,y);
    else if(localClientHandlerFight.haveMonsters())
        localClientHandlerFight.checkLoose();

    int index=0;
    while(index<GlobalServerData::serverPrivateVariables.server_message.size())
    {
        emit receiveSystemText(GlobalServerData::serverPrivateVariables.server_message.at(index));
        index++;
    }

    LocalClientHandlerWithoutSender::localClientHandlerWithoutSender.allClient << this;
}

void LocalClientHandler::createMemoryClan()
{
    if(!clanList.contains(player_informations->public_and_private_informations.clan))
    {
        clan=new Clan;
        clan->cash=0;
        clan->clanId=player_informations->public_and_private_informations.clan;
        clanList[player_informations->public_and_private_informations.clan]=clan;
        if(GlobalServerData::serverPrivateVariables.cityStatusListReverse.contains(clan->clanId))
            clan->capturedCity=GlobalServerData::serverPrivateVariables.cityStatusListReverse.value(clan->clanId);
    }
    else
        clan=clanList.value(player_informations->public_and_private_informations.clan);
    clan->players << this;
}

bool LocalClientHandler::moveThePlayer(const quint8 &previousMovedUnit,const Direction &direction)
{
    #ifdef DEBUG_MESSAGE_CLIENT_MOVE
    emit message(QStringLiteral("moveThePlayer(): for player (%1,%2): %3, previousMovedUnit: %4 (%5), next direction: %6")
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

bool LocalClientHandler::captureCityInProgress()
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

bool LocalClientHandler::singleMove(const Direction &direction)
{
    if(localClientHandlerFight.isInFight())//check if is in fight
    {
        emit error(QStringLiteral("error: try move when is in fight"));
        return false;
    }
    if(captureCityInProgress())
    {
        emit error(QStringLiteral("Try move when is in capture city"));
        return false;
    }
    COORD_TYPE x=this->x,y=this->y;
    temp_direction=direction;
    CommonMap* map=this->map;
    #ifdef DEBUG_MESSAGE_CLIENT_MOVE
    emit message(QStringLiteral("LocalClientHandler::singleMove(), go in this direction: %1 with map: %2(%3,%4)").arg(MoveOnTheMap::directionToString(direction)).arg(map->map_file).arg(x).arg(y));
    #endif
    if(!MoveOnTheMap::canGoTo(direction,*map,x,y,true))
    {
        emit error(QStringLiteral("LocalClientHandler::singleMove(), can't go into this direction: %1 with map: %2(%3,%4)").arg(MoveOnTheMap::directionToString(direction)).arg(map->map_file).arg(x).arg(y));
        return false;
    }
    if(!MoveOnTheMap::moveWithoutTeleport(direction,&map,&x,&y,false,true))
    {
        emit error(QStringLiteral("LocalClientHandler::singleMove(), can go but move failed into this direction: %1 with map: %2(%3,%4)").arg(MoveOnTheMap::directionToString(direction)).arg(map->map_file).arg(x).arg(y));
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
                if(!player_informations->public_and_private_informations.bot_already_beaten.contains(teleporter.condition.value))
                {
                    emit error(QStringLiteral("Need have FightBot win to use this teleporter: %1 with map: %2(%3,%4)").arg(teleporter.condition.value).arg(map->map_file).arg(x).arg(y));
                    return false;
                }
            break;
            case CatchChallenger::MapConditionType_Item:
                if(!player_informations->public_and_private_informations.items.contains(teleporter.condition.value))
                {
                    emit error(QStringLiteral("Need have item to use this teleporter: %1 with map: %2(%3,%4)").arg(teleporter.condition.value).arg(map->map_file).arg(x).arg(y));
                    return false;
                }
            break;
            case CatchChallenger::MapConditionType_Quest:
                if(!player_informations->public_and_private_informations.quests.contains(teleporter.condition.value))
                {
                    emit error(QStringLiteral("Need have quest to use this teleporter: %1 with map: %2(%3,%4)").arg(teleporter.condition.value).arg(map->map_file).arg(x).arg(y));
                    return false;
                }
                if(!player_informations->public_and_private_informations.quests.value(teleporter.condition.value).finish_one_time)
                {
                    emit error(QStringLiteral("Need have finish the quest to use this teleporter: %1 with map: %2(%3,%4)").arg(teleporter.condition.value).arg(map->map_file).arg(x).arg(y));
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

    this->map=static_cast<Map_server_MapVisibility_simple*>(map);
    this->x=x;
    this->y=y;
    if(static_cast<Map_server_MapVisibility_simple*>(map)->rescue.contains(QPair<quint8,quint8>(x,y)))
    {
        player_informations->unvalidated_rescue.map=map;
        player_informations->unvalidated_rescue.x=x;
        player_informations->unvalidated_rescue.y=y;
        player_informations->unvalidated_rescue.orientation=static_cast<Map_server_MapVisibility_simple*>(map)->rescue.value(QPair<quint8,quint8>(x,y));
    }
    if(localClientHandlerFight.botFightCollision(map,x,y))
        return true;
    if(player_informations->public_and_private_informations.repel_step<=0)
    {
        if(localClientHandlerFight.generateWildFightIfCollision(map,x,y,player_informations->public_and_private_informations.items))
        {
            emit message(QStringLiteral("LocalClientHandler::singleMove(), now is in front of wild monster with map: %1(%2,%3)").arg(map->map_file).arg(x).arg(y));
            return true;
        }
    }
    else
        player_informations->public_and_private_informations.repel_step--;
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
    if(!CommonDatapack::commonDatapack.items.item.contains(item))
    {
        emit error("Object is not found into the item list");
        return;
    }
    if(player_informations->public_and_private_informations.items.contains(item))
    {
        player_informations->public_and_private_informations.items[item]+=quantity;
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                emit dbQuery(QStringLiteral("UPDATE `item` SET `quantity`=%1 WHERE `item`=%2 AND `character`=%3 AND `place`='wear';")
                             .arg(player_informations->public_and_private_informations.items.value(item))
                             .arg(item)
                             .arg(player_informations->character_id)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QStringLiteral("UPDATE item SET quantity=%1 WHERE item=%2 AND character=%3 AND place='wear';")
                         .arg(player_informations->public_and_private_informations.items.value(item))
                         .arg(item)
                         .arg(player_informations->character_id)
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
                emit dbQuery(QStringLiteral("INSERT INTO `item`(`item`,`character`,`quantity`,`place`) VALUES(%1,%2,%3,'wear');")
                             .arg(item)
                             .arg(player_informations->character_id)
                             .arg(quantity)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QStringLiteral("INSERT INTO item(item,character,quantity,place) VALUES(%1,%2,%3,'wear');")
                         .arg(item)
                         .arg(player_informations->character_id)
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
                emit dbQuery(QStringLiteral("UPDATE `item` SET `quantity`=%1 WHERE `item`=%2 AND `character`=%3 AND place='warehouse';")
                             .arg(player_informations->public_and_private_informations.warehouse_items.value(item))
                             .arg(item)
                             .arg(player_informations->character_id)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QStringLiteral("UPDATE item SET quantity=%1 WHERE item=%2 AND character=%3 AND place='warehouse';")
                         .arg(player_informations->public_and_private_informations.warehouse_items.value(item))
                         .arg(item)
                         .arg(player_informations->character_id)
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
                emit dbQuery(QStringLiteral("INSERT INTO item(`item`,`character`,`quantity`,`place`) VALUES(%1,%2,%3,'warehouse');")
                             .arg(item)
                             .arg(player_informations->character_id)
                             .arg(quantity)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QStringLiteral("INSERT INTO item(item,character,quantity,place) VALUES(%1,%2,%3,'warehouse');")
                         .arg(item)
                         .arg(player_informations->character_id)
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
        if(player_informations->public_and_private_informations.warehouse_items.value(item)>quantity)
        {
            player_informations->public_and_private_informations.warehouse_items[item]-=quantity;
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    emit dbQuery(QStringLiteral("UPDATE `item` SET `quantity`=%1 WHERE `item`=%2 AND `character`=%3 AND `place`='warehouse';")
                                 .arg(player_informations->public_and_private_informations.warehouse_items.value(item))
                                 .arg(item)
                                 .arg(player_informations->character_id)
                                 );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    emit dbQuery(QStringLiteral("UPDATE item SET quantity=%1 WHERE item=%2 AND character=%3 AND place='warehouse';")
                                 .arg(player_informations->public_and_private_informations.warehouse_items.value(item))
                                 .arg(item)
                                 .arg(player_informations->character_id)
                             );
                break;
            }
            return quantity;
        }
        else
        {
            quint32 removed_quantity=player_informations->public_and_private_informations.warehouse_items.value(item);
            player_informations->public_and_private_informations.warehouse_items.remove(item);
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    emit dbQuery(QStringLiteral("DELETE FROM `item` WHERE `item`=%1 AND `character`=%2 AND `place`='warehouse'")
                                 .arg(item)
                                 .arg(player_informations->character_id)
                                 );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    emit dbQuery(QStringLiteral("DELETE FROM item WHERE item=%1 AND character=%2 AND place='warehouse'")
                             .arg(item)
                             .arg(player_informations->character_id)
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
                emit dbQuery(QStringLiteral("UPDATE `item` SET `quantity`=%1 WHERE `item`=%2 AND `character`=%3 AND `place`='wear';")
                             .arg(player_informations->public_and_private_informations.items.value(item))
                             .arg(item)
                             .arg(player_informations->character_id)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QStringLiteral("UPDATE item SET quantity=%1 WHERE item=%2 AND character=%3 AND place='wear';")
                             .arg(player_informations->public_and_private_informations.items.value(item))
                             .arg(item)
                             .arg(player_informations->character_id)
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
                emit dbQuery(QStringLiteral("DELETE FROM `item` WHERE `item`=%1 AND `character`=%2 AND `place`='wear'")
                             .arg(item)
                             .arg(player_informations->character_id)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QStringLiteral("DELETE FROM item WHERE item=%1 AND character=%2 AND place='wear'")
                         .arg(item)
                         .arg(player_informations->character_id)
                         );
            break;
        }
    }
}

quint32 LocalClientHandler::removeObject(const quint32 &item,const quint32 &quantity)
{
    if(player_informations->public_and_private_informations.items.contains(item))
    {
        if(player_informations->public_and_private_informations.items.value(item)>quantity)
        {
            player_informations->public_and_private_informations.items[item]-=quantity;
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    emit dbQuery(QStringLiteral("UPDATE `item` SET `quantity`=%1 WHERE `item`=%2 AND `character`=%3 AND `place`='wear';")
                                 .arg(player_informations->public_and_private_informations.items.value(item))
                                 .arg(item)
                                 .arg(player_informations->character_id)
                                 );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    emit dbQuery(QStringLiteral("UPDATE item SET quantity=%1 WHERE item=%2 AND character=%3 AND place='wear';")
                                 .arg(player_informations->public_and_private_informations.items.value(item))
                                 .arg(item)
                                 .arg(player_informations->character_id)
                             );
                break;
            }
            return quantity;
        }
        else
        {
            quint32 removed_quantity=player_informations->public_and_private_informations.items.value(item);
            player_informations->public_and_private_informations.items.remove(item);
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    emit dbQuery(QStringLiteral("DELETE FROM `item` WHERE `item`=%1 AND `character`=%2 AND `place`='wear'")
                                 .arg(item)
                                 .arg(player_informations->character_id)
                                 );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    emit dbQuery(QStringLiteral("DELETE FROM item WHERE item=%1 AND character=%2 AND place='wear'")
                             .arg(item)
                             .arg(player_informations->character_id)
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
        return player_informations->public_and_private_informations.items.value(item);
    else
        return 0;
}

bool LocalClientHandler::addMarketCashWithoutSave(const quint64 &cash,const double &bitcoin)
{
    if(bitcoin>0 && !bitcoinEnabled())
        return false;
    player_informations->market_cash+=cash;
    player_informations->market_bitcoin+=bitcoin;
    return true;
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
            emit dbQuery(QStringLiteral("UPDATE `character` SET `cash`=%1 WHERE `id`=%2;")
                         .arg(player_informations->public_and_private_informations.cash)
                         .arg(player_informations->character_id)
                         );
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            emit dbQuery(QStringLiteral("UPDATE character SET cash=%1 WHERE id=%2;")
                     .arg(player_informations->public_and_private_informations.cash)
                     .arg(player_informations->character_id)
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
            emit dbQuery(QStringLiteral("UPDATE `character` SET `cash`=%1 WHERE `id`=%2;")
                         .arg(player_informations->public_and_private_informations.cash)
                         .arg(player_informations->character_id)
                         );
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            emit dbQuery(QStringLiteral("UPDATE character SET cash=%1 WHERE id=%2;")
                     .arg(player_informations->public_and_private_informations.cash)
                     .arg(player_informations->character_id)
                     );
        break;
    }
}

void LocalClientHandler::addBitcoin(const double &bitcoin)
{
    if(bitcoin==0)
        return;
    player_informations->public_and_private_informations.bitcoin+=bitcoin;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            emit dbQuery(QStringLiteral("UPDATE `character` SET `bitcoin`=%1 WHERE `id`=%2;")
                         .arg(player_informations->public_and_private_informations.bitcoin)
                         .arg(player_informations->character_id)
                         );
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            emit dbQuery(QStringLiteral("UPDATE character SET bitcoin=%1 WHERE id=%2;")
                     .arg(player_informations->public_and_private_informations.bitcoin)
                     .arg(player_informations->character_id)
                     );
        break;
    }
}

void LocalClientHandler::removeBitcoin(const double &bitcoin)
{
    if(bitcoin==0)
        return;
    player_informations->public_and_private_informations.bitcoin-=bitcoin;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            emit dbQuery(QStringLiteral("UPDATE `character` SET `bitcoin`=%1 WHERE `id`=%2;")
                         .arg(player_informations->public_and_private_informations.bitcoin)
                         .arg(player_informations->character_id)
                         );
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            emit dbQuery(QStringLiteral("UPDATE character SET bitcoin=%1 WHERE id=%2;")
                     .arg(player_informations->public_and_private_informations.bitcoin)
                     .arg(player_informations->character_id)
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
            emit dbQuery(QStringLiteral("UPDATE `character` SET `warehouse_cash`=%1 WHERE `id`=%2;")
                         .arg(player_informations->public_and_private_informations.warehouse_cash)
                         .arg(player_informations->character_id)
                         );
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            emit dbQuery(QStringLiteral("UPDATE character SET warehouse_cash=%1 WHERE id=%2;")
                     .arg(player_informations->public_and_private_informations.warehouse_cash)
                     .arg(player_informations->character_id)
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
            emit dbQuery(QStringLiteral("UPDATE `character` SET `warehouse_cash`=%1 WHERE `id`=%2;")
                         .arg(player_informations->public_and_private_informations.warehouse_cash)
                         .arg(player_informations->character_id)
                         );
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            emit dbQuery(QStringLiteral("UPDATE character SET warehouse_cash=%1 WHERE id=%2;")
                     .arg(player_informations->public_and_private_informations.warehouse_cash)
                     .arg(player_informations->character_id)
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
                    player_informations->public_and_private_informations.playerMonster << player_informations->public_and_private_informations.warehouse_playerMonster.at(sub_index);
                    player_informations->public_and_private_informations.warehouse_playerMonster.removeAt(sub_index);
                    switch(GlobalServerData::serverSettings.database.type)
                    {
                        default:
                        case ServerSettings::Database::DatabaseType_Mysql:
                            emit dbQuery(QStringLiteral("UPDATE `monster` SET `place`='wear',`position`=%2 WHERE `id`=%1;")
                                        .arg(withdrawMonsters.at(index))
                                        .arg(player_informations->public_and_private_informations.playerMonster.size()+1)
                                        );
                        break;
                        case ServerSettings::Database::DatabaseType_SQLite:
                            emit dbQuery(QStringLiteral("UPDATE monster SET place='wear',position=%2 WHERE id=%1;")
                                        .arg(withdrawMonsters.at(index))
                                        .arg(player_informations->public_and_private_informations.playerMonster.size()+1)
                                        );
                        break;
                    }
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
                    player_informations->public_and_private_informations.warehouse_playerMonster << player_informations->public_and_private_informations.playerMonster.at(sub_index);
                    player_informations->public_and_private_informations.playerMonster.removeAt(sub_index);
                    if(GlobalServerData::serverSettings.database.fightSync==ServerSettings::Database::FightSync_AtTheDisconnexion)
                        localClientHandlerFight.saveMonsterStat(player_informations->public_and_private_informations.playerMonster.last());
                    switch(GlobalServerData::serverSettings.database.type)
                    {
                        default:
                        case ServerSettings::Database::DatabaseType_Mysql:
                            emit dbQuery(QStringLiteral("UPDATE `monster` SET `place`='warehouse',`position`=%2 WHERE `id`=%1;")
                                        .arg(withdrawMonsters.at(index))
                                        .arg(player_informations->public_and_private_informations.warehouse_playerMonster.size()+1)
                                        );
                        break;
                        case ServerSettings::Database::DatabaseType_SQLite:
                            emit dbQuery(QStringLiteral("UPDATE monster SET place='warehouse',position=%2 WHERE id=%1;")
                                        .arg(withdrawMonsters.at(index))
                                        .arg(player_informations->public_and_private_informations.warehouse_playerMonster.size()+1)
                                        );
                        break;
                    }
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
                if((qint64)player_informations->public_and_private_informations.warehouse_items.value(item.first)<item.second)
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
                if((qint64)player_informations->public_and_private_informations.items.value(item.first)<-item.second)
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
    if(command==LocalClientHandler::text_give)
    {
        bool ok;
        QStringList arguments=extraText.split(LocalClientHandler::text_space,QString::SkipEmptyParts);
        if(arguments.size()==2)
            arguments << LocalClientHandler::text_1;
        if(arguments.size()!=3)
        {
            emit receiveSystemText(QStringLiteral("Wrong arguments number for the command, usage: /give objectId player [quantity=1]"));
            return;
        }
        const quint32 &objectId=arguments.first().toUInt(&ok);
        if(!ok)
        {
            emit receiveSystemText(QStringLiteral("objectId is not a number, usage: /give objectId player [quantity=1]"));
            return;
        }
        if(!CommonDatapack::commonDatapack.items.item.contains(objectId))
        {
            emit receiveSystemText(QStringLiteral("objectId is not a valid item, usage: /give objectId player [quantity=1]"));
            return;
        }
        const quint32 &quantity=arguments.last().toUInt(&ok);
        if(!ok)
        {
            emit receiveSystemText(QStringLiteral("quantity is not a number, usage: /give objectId player [quantity=1]"));
            return;
        }
        if(!playerByPseudo.contains(arguments.at(1)))
        {
            emit receiveSystemText(QStringLiteral("player is not connected, usage: /give objectId player [quantity=1]"));
            return;
        }
        emit message(QStringLiteral("%1 have give to %2 the item with id: %3 in quantity: %4").arg(player_informations->public_and_private_informations.public_informations.pseudo).arg(arguments.at(1)).arg(objectId).arg(quantity));
        playerByPseudo.value(arguments.at(1))->addObjectAndSend(objectId,quantity);
    }
    else if(command==LocalClientHandler::text_take)
    {
        bool ok;
        QStringList arguments=extraText.split(LocalClientHandler::text_space,QString::SkipEmptyParts);
        if(arguments.size()==2)
            arguments << LocalClientHandler::text_1;
        if(arguments.size()!=3)
        {
            emit receiveSystemText(QStringLiteral("Wrong arguments number for the command, usage: /take objectId player [quantity=1]"));
            return;
        }
        quint32 objectId=arguments.first().toUInt(&ok);
        if(!ok)
        {
            emit receiveSystemText(QStringLiteral("objectId is not a number, usage: /take objectId player [quantity=1]"));
            return;
        }
        if(!CommonDatapack::commonDatapack.items.item.contains(objectId))
        {
            emit receiveSystemText(QStringLiteral("objectId is not a valid item, usage: /take objectId player [quantity=1]"));
            return;
        }
        quint32 quantity=arguments.last().toUInt(&ok);
        if(!ok)
        {
            emit receiveSystemText(QStringLiteral("quantity is not a number, usage: /take objectId player [quantity=1]"));
            return;
        }
        if(!playerByPseudo.contains(arguments.at(1)))
        {
            emit receiveSystemText(QStringLiteral("player is not connected, usage: /take objectId player [quantity=1]"));
            return;
        }
        emit message(QStringLiteral("%1 have take to %2 the item with id: %3 in quantity: %4").arg(player_informations->public_and_private_informations.public_informations.pseudo).arg(arguments.at(1)).arg(objectId).arg(quantity));
        playerByPseudo.value(arguments.at(1))->sendRemoveObject(objectId,playerByPseudo.value(arguments.at(1))->removeObject(objectId,quantity));
    }
    else if(command==LocalClientHandler::text_tp)
    {
        QStringList arguments=extraText.split(LocalClientHandler::text_space,QString::SkipEmptyParts);
        if(arguments.size()==3)
        {
            if(arguments.at(1)!=LocalClientHandler::text_to)
            {
                emit receiveSystemText(QStringLiteral("wrong second arguement: %1, usage: /tp player1 to player2").arg(arguments.at(1)));
                return;
            }
            if(!playerByPseudo.contains(arguments.first()))
            {
                emit receiveSystemText(QStringLiteral("%1 is not connected, usage: /tp player1 to player2").arg(arguments.first()));
                return;
            }
            if(!playerByPseudo.contains(arguments.last()))
            {
                emit receiveSystemText(QStringLiteral("%1 is not connected, usage: /tp player1 to player2").arg(arguments.last()));
                return;
            }
            playerByPseudo.value(arguments.first())->receiveTeleportTo(playerByPseudo.value(arguments.last())->map,playerByPseudo.value(arguments.last())->x,playerByPseudo.value(arguments.last())->y,MoveOnTheMap::directionToOrientation(playerByPseudo.value(arguments.last())->getLastDirection()));
        }
        else
        {
            emit receiveSystemText(QStringLiteral("Wrong arguments number for the command, usage: /tp player1 to player2"));
            return;
        }
    }
    else if(command==LocalClientHandler::text_trade)
    {
        if(extraText.isEmpty())
        {
            emit receiveSystemText(QStringLiteral("no player given, syntaxe: /trade player").arg(extraText));
            return;
        }
        if(!playerByPseudo.contains(extraText))
        {
            emit receiveSystemText(QStringLiteral("%1 is not connected").arg(extraText));
            return;
        }
        if(player_informations->public_and_private_informations.public_informations.pseudo==extraText)
        {
            emit receiveSystemText(QStringLiteral("You can't trade with yourself").arg(extraText));
            return;
        }
        if(getInTrade())
        {
            emit receiveSystemText(QStringLiteral("You are already in trade"));
            return;
        }
        if(localClientHandlerFight.isInBattle())
        {
            emit receiveSystemText(QStringLiteral("you are already in battle"));
            return;
        }
        if(playerByPseudo.value(extraText)->getInTrade())
        {
            emit receiveSystemText(QStringLiteral("%1 is already in trade").arg(extraText));
            return;
        }
        if(playerByPseudo.value(extraText)->localClientHandlerFight.isInBattle())
        {
            emit receiveSystemText(QStringLiteral("%1 is already in battle").arg(extraText));
            return;
        }
        if(!otherPlayerIsInRange(playerByPseudo.value(extraText)))
        {
            emit receiveSystemText(QStringLiteral("%1 is not in range").arg(extraText));
            return;
        }
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        emit message(QStringLiteral("Trade requested"));
        #endif
        otherPlayerTrade=playerByPseudo.value(extraText);
        otherPlayerTrade->registerTradeRequest(this);
    }
    else if(command==LocalClientHandler::text_battle)
    {
        if(extraText.isEmpty())
        {
            emit receiveSystemText(QStringLiteral("no player given, syntaxe: /battle player").arg(extraText));
            return;
        }
        if(!playerByPseudo.contains(extraText))
        {
            emit receiveSystemText(QStringLiteral("%1 is not connected").arg(extraText));
            return;
        }
        if(player_informations->public_and_private_informations.public_informations.pseudo==extraText)
        {
            emit receiveSystemText(QStringLiteral("You can't battle with yourself").arg(extraText));
            return;
        }
        if(localClientHandlerFight.isInBattle())
        {
            emit receiveSystemText(QStringLiteral("you are already in battle"));
            return;
        }
        if(getInTrade())
        {
            emit receiveSystemText(QStringLiteral("you are already in trade"));
            return;
        }
        if(playerByPseudo.value(extraText)->localClientHandlerFight.isInBattle())
        {
            emit receiveSystemText(QStringLiteral("%1 is already in battle").arg(extraText));
            return;
        }
        if(playerByPseudo.value(extraText)->getInTrade())
        {
            emit receiveSystemText(QStringLiteral("%1 is already in battle").arg(extraText));
            return;
        }
        if(!otherPlayerIsInRange(playerByPseudo.value(extraText)))
        {
            emit receiveSystemText(QStringLiteral("%1 is not in range").arg(extraText));
            return;
        }
        if(!playerByPseudo.value(extraText)->localClientHandlerFight.getAbleToFight())
        {
            emit receiveSystemText(QStringLiteral("The other player can't fight"));
            return;
        }
        if(!localClientHandlerFight.getAbleToFight())
        {
            emit receiveSystemText(QStringLiteral("You can't fight"));
            return;
        }
        if(playerByPseudo.value(extraText)->localClientHandlerFight.isInFight())
        {
            emit receiveSystemText(QStringLiteral("The other player is in fight"));
            return;
        }
        if(localClientHandlerFight.isInFight())
        {
            emit receiveSystemText(QStringLiteral("You are in fight"));
            return;
        }
        if(captureCityInProgress())
        {
            emit error(QStringLiteral("Try battle when is in capture city"));
            return;
        }
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        emit message(QStringLiteral("Battle requested"));
        #endif
        playerByPseudo.value(extraText)->localClientHandlerFight.registerBattleRequest(&localClientHandlerFight);
    }
}

bool LocalClientHandler::learnSkill(const quint32 &monsterId,const quint32 &skill)
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message(QStringLiteral("learnSkill(%1,%2)").arg(monsterId).arg(skill));
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
                    emit error(QStringLiteral("learnSkill() Can't move at top from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                    return false;
                }
            }
            else
            {
                emit error(QStringLiteral("No valid map in this direction"));
                return false;
            }
        break;
        default:
        emit error(QStringLiteral("Wrong direction to use a learn skill"));
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
                        emit error(QStringLiteral("learnSkill() Can't move at top from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                        return false;
                    }
                }
                else
                {
                    emit error(QStringLiteral("No valid map in this direction"));
                    return false;
                }
            break;
            default:
            break;
        }
        if(!static_cast<MapServer*>(this->map)->learn.contains(QPair<quint8,quint8>(x,y)))
        {
            emit error(QStringLiteral("not learn skill into this direction"));
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
    emit message(QStringLiteral("The player have destroy them self %1 item(s) with id: %2").arg(quantity).arg(itemId));
    removeObject(itemId,quantity);
}

void LocalClientHandler::useObjectOnMonster(const quint32 &object,const quint32 &monster)
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message(QStringLiteral("use the object: %1 on monster %2").arg(object).arg(monster));
    #endif
    if(!player_informations->public_and_private_informations.items.contains(object))
    {
        emit error(QStringLiteral("can't use the object: %1 because don't have into the inventory").arg(object));
        return;
    }
    if(objectQuantity(object)<1)
    {
        emit error(QStringLiteral("have not quantity to use this object: %1").arg(object));
        return;
    }
    if(localClientHandlerFight.useObjectOnMonster(object,monster))
    {
        if(CommonDatapack::commonDatapack.items.item.value(object).consumeAtUse)
            removeObject(object);
    }
}

void LocalClientHandler::useObject(const quint8 &query_id,const quint32 &itemId)
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message(QStringLiteral("use the object: %1").arg(itemId));
    #endif
    if(!player_informations->public_and_private_informations.items.contains(itemId))
    {
        emit error(QStringLiteral("can't use the object: %1 because don't have into the inventory").arg(itemId));
        return;
    }
    if(objectQuantity(itemId)<1)
    {
        emit error(QStringLiteral("have not quantity to use this object: %1 because recipe already registred").arg(itemId));
        return;
    }
    if(CommonDatapack::commonDatapack.items.item.value(itemId).consumeAtUse)
        removeObject(itemId);
    //if is crafting recipe
    if(CommonDatapack::commonDatapack.itemToCrafingRecipes.contains(itemId))
    {
        const quint32 &recipeId=CommonDatapack::commonDatapack.itemToCrafingRecipes.value(itemId);
        if(player_informations->public_and_private_informations.recipes.contains(recipeId))
        {
            emit error(QStringLiteral("can't use the object: %1").arg(itemId));
            return;
        }
        if(!haveReputationRequirements(CommonDatapack::commonDatapack.crafingRecipes.value(recipeId).requirements.reputation))
        {
            emit error(QStringLiteral("The player have not the requirement: %1 to to learn crafting recipe").arg(recipeId));
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
                emit dbQuery(QStringLiteral("INSERT INTO `recipes`(`character`,`recipe`) VALUES(%1,%2);")
                         .arg(player_informations->character_id)
                         .arg(recipeId)
                         );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QStringLiteral("INSERT INTO recipes(character,recipe) VALUES(%1,%2);")
                         .arg(player_informations->character_id)
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
            emit error(QStringLiteral("is not in fight to use trap: %1").arg(itemId));
            return;
        }
        if(!localClientHandlerFight.isInFightWithWild())
        {
            emit error(QStringLiteral("is not in fight with wild to use trap: %1").arg(itemId));
            return;
        }
        const quint32 &maxMonsterId=localClientHandlerFight.tryCapture(itemId);
        //send the network reply
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);
        out << (quint8)ObjectUsage_correctlyUsed;
        if(maxMonsterId>0)
            out << (quint32)maxMonsterId;
        else
            out << (quint32)0x00000000;
        emit postReply(query_id,outputData);
    }
    //use repel into fight
    else if(CommonDatapack::commonDatapack.items.repel.contains(itemId))
    {
        player_informations->public_and_private_informations.repel_step+=CommonDatapack::commonDatapack.items.repel.value(itemId);
        //send the network reply
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);
        out << (quint8)ObjectUsage_correctlyUsed;
        emit postReply(query_id,outputData);
    }
    else
    {
        emit error(QStringLiteral("can't use the object: %1 because don't have an usage").arg(itemId));
        return;
    }
}

void LocalClientHandler::receiveTeleportTo(CommonMap *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation)
{
    emit teleportTo(map,x,y,orientation);
}

void LocalClientHandler::teleportValidatedTo(CommonMap *map,const /*COORD_TYPE*/quint8 &x,const /*COORD_TYPE*/quint8 &y,const Orientation &orientation)
{
    emit message(QStringLiteral("teleportValidatedTo(%1,%2,%3,%4)").arg(map->map_file).arg(x).arg(y).arg((quint8)orientation));
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
    emit message(QStringLiteral("getShopList(%1,%2)").arg(query_id).arg(shopId));
    #endif
    if(!GlobalServerData::serverPrivateVariables.shops.contains(shopId))
    {
        emit error(QStringLiteral("shopId not found: %1").arg(shopId));
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
                    emit error(QStringLiteral("getShopList() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                    return;
                }
            }
            else
            {
                emit error(QStringLiteral("No valid map in this direction"));
                return;
            }
        break;
        default:
        emit error(QStringLiteral("Wrong direction to use a shop"));
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
                            emit error(QStringLiteral("getShopList() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                            return;
                        }
                    }
                    else
                    {
                        emit error(QStringLiteral("No valid map in this direction"));
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
                    emit error(QStringLiteral("not shop into this direction"));
                    return;
                }
            }
        }
    }
    //send the shop items (no taxes from now)
    QList<quint32> items=GlobalServerData::serverPrivateVariables.shops.value(shopId).items;
    QList<quint32> prices=GlobalServerData::serverPrivateVariables.shops.value(shopId).prices;
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
        if(prices.at(index)>0)
        {
            out2 << (quint32)items.at(index);
            out2 << (quint32)prices.at(index);
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
    emit message(QStringLiteral("getShopList(%1,%2)").arg(query_id).arg(shopId));
    #endif
    if(!GlobalServerData::serverPrivateVariables.shops.contains(shopId))
    {
        emit error(QStringLiteral("shopId not found: %1").arg(shopId));
        return;
    }
    if(quantity<=0)
    {
        emit error(QStringLiteral("quantity wrong: %1").arg(quantity));
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
                    emit error(QStringLiteral("buyObject() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                    return;
                }
            }
            else
            {
                emit error(QStringLiteral("No valid map in this direction"));
                return;
            }
        break;
        default:
        emit error(QStringLiteral("Wrong direction to use a shop"));
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
                            emit error(QStringLiteral("buyObject() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                            return;
                        }
                    }
                    else
                    {
                        emit error(QStringLiteral("No valid map in this direction"));
                        return;
                    }
                break;
                default:
                emit error(QStringLiteral("Wrong direction to use a shop"));
                return;
            }
            if(static_cast<MapServer*>(this->map)->shops.contains(QPair<quint8,quint8>(x,y)))
            {
                QList<quint32> shops=static_cast<MapServer*>(this->map)->shops.values(QPair<quint8,quint8>(x,y));
                if(!shops.contains(shopId))
                {
                    emit error(QStringLiteral("not shop into this direction"));
                    return;
                }
            }
        }
    }
    //send the shop items (no taxes from now)
    QList<quint32> items=GlobalServerData::serverPrivateVariables.shops.value(shopId).items;
    int priceIndex=items.indexOf(objectId);
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    if(priceIndex==-1)
    {
        out << (quint8)BuyStat_HaveNotQuantity;
        emit postReply(query_id,outputData);
        return;
    }
    const quint32 &realprice=GlobalServerData::serverPrivateVariables.shops.value(shopId).prices.at(priceIndex);
    if(realprice==0)
    {
        out << (quint8)BuyStat_HaveNotQuantity;
        emit postReply(query_id,outputData);
        return;
    }
    if(realprice>price)
    {
        out << (quint8)BuyStat_PriceHaveChanged;
        emit postReply(query_id,outputData);
        return;
    }
    if(realprice<price)
    {
        out << (quint8)BuyStat_BetterPrice;
        out << (quint32)price;
    }
    else
        out << (quint8)BuyStat_Done;
    if(player_informations->public_and_private_informations.cash>=(realprice*quantity))
        removeCash(realprice*quantity);
    else
    {
        emit error(QStringLiteral("The player have not the cash to buy %1 item of id: %2").arg(quantity).arg(objectId));
        return;
    }
    addObject(objectId,quantity);
    emit postReply(query_id,outputData);
}

void LocalClientHandler::sellObject(const quint32 &query_id,const quint32 &shopId,const quint32 &objectId,const quint32 &quantity,const quint32 &price)
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message(QStringLiteral("getShopList(%1,%2)").arg(query_id).arg(shopId));
    #endif
    if(!GlobalServerData::serverPrivateVariables.shops.contains(shopId))
    {
        emit error(QStringLiteral("shopId not found: %1").arg(shopId));
        return;
    }
    if(quantity<=0)
    {
        emit error(QStringLiteral("quantity wrong: %1").arg(quantity));
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
                    emit error(QStringLiteral("sellObject() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                    return;
                }
            }
            else
            {
                emit error(QStringLiteral("No valid map in this direction"));
                return;
            }
        break;
        default:
        emit error(QStringLiteral("Wrong direction to use a shop"));
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
                            emit error(QStringLiteral("sellObject() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                            return;
                        }
                    }
                    else
                    {
                        emit error(QStringLiteral("No valid map in this direction"));
                        return;
                    }
                break;
                default:
                emit error(QStringLiteral("Wrong direction to use a shop"));
                return;
            }
            if(static_cast<MapServer*>(this->map)->shops.contains(QPair<quint8,quint8>(x,y)))
            {
                QList<quint32> shops=static_cast<MapServer*>(this->map)->shops.values(QPair<quint8,quint8>(x,y));
                if(!shops.contains(shopId))
                {
                    emit error(QStringLiteral("not shop into this direction"));
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
        emit error(QStringLiteral("this item don't exists"));
        return;
    }
    if(objectQuantity(objectId)<quantity)
    {
        emit error(QStringLiteral("you have not this quantity to sell"));
        return;
    }
    const quint32 &realPrice=CommonDatapack::commonDatapack.items.item.value(objectId).price/2;
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

void LocalClientHandler::saveIndustryStatus(const quint32 &factoryId,const IndustryStatus &industryStatus,const Industry &industry)
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
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                emit dbQuery(QStringLiteral("INSERT INTO `factory`(`id`,`resources`,`products`,`last_update`) VALUES(%1,'%2','%3',%4);")
                             .arg(factoryId)
                             .arg(resourcesStringList.join(LocalClientHandler::text_dotcomma))
                             .arg(productsStringList.join(LocalClientHandler::text_dotcomma))
                             .arg(industryStatus.last_update)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QStringLiteral("INSERT INTO factory(id,resources,products,last_update) VALUES(%1,'%2','%3',%4);")
                             .arg(factoryId)
                             .arg(resourcesStringList.join(LocalClientHandler::text_dotcomma))
                             .arg(productsStringList.join(LocalClientHandler::text_dotcomma))
                             .arg(industryStatus.last_update)
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
                emit dbQuery(QStringLiteral("UPDATE `factory` SET `resources`='%2',`products`='%3',`last_update`=%4 WHERE `id`=%1")
                             .arg(factoryId)
                             .arg(resourcesStringList.join(LocalClientHandler::text_dotcomma))
                             .arg(productsStringList.join(LocalClientHandler::text_dotcomma))
                             .arg(industryStatus.last_update)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QStringLiteral("UPDATE factory SET resources='%2',products='%3',last_update=%4 WHERE id=%1")
                             .arg(factoryId)
                             .arg(resourcesStringList.join(LocalClientHandler::text_dotcomma))
                             .arg(productsStringList.join(LocalClientHandler::text_dotcomma))
                             .arg(industryStatus.last_update)
                             );
            break;
        }
    }
    GlobalServerData::serverPrivateVariables.industriesStatus[factoryId]=industryStatus;
}

void LocalClientHandler::getFactoryList(const quint32 &query_id, const quint32 &factoryId)
{
    if(localClientHandlerFight.isInFight())
    {
        emit error(QStringLiteral("Try do inventory action when is in fight"));
        return;
    }
    if(captureCityInProgress())
    {
        emit error(QStringLiteral("Try do inventory action when is in capture city"));
        return;
    }
    if(!CommonDatapack::commonDatapack.industriesLink.contains(factoryId))
    {
        emit error(QStringLiteral("factory id not found"));
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
        out << (quint32)industry.resources.size();
        int index=0;
        while(index<industry.resources.size())
        {
            const Industry::Resource &resource=industry.resources.at(index);
            out << (quint32)resource.item;
            out << (quint32)CommonDatapack::commonDatapack.items.item.value(resource.item).price*(100+CATCHCHALLENGER_SERVER_FACTORY_PRICE_CHANGE)/100;
            out << (quint32)resource.quantity*industry.cycletobefull;
            index++;
        }
        out << (quint32)0x00000000;//no product do
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
        out << (quint32)count_item;
        index=0;
        while(index<industry.resources.size())
        {
            const Industry::Resource &resource=industry.resources.at(index);
            const quint32 &quantityInStock=industryStatus.resources.value(resource.item);
            if(quantityInStock<resource.quantity*industry.cycletobefull)
            {
                out << (quint32)resource.item;
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
        out << (quint32)count_item;
        index=0;
        while(index<industry.products.size())
        {
            const Industry::Product &product=industry.products.at(index);
            const quint32 &quantityInStock=industryStatus.products.value(product.item);
            if(quantityInStock>0)
            {
                out << (quint32)product.item;
                out << (quint32)FacilityLib::getFactoryProductPrice(quantityInStock,product,industry);
                out << (quint32)quantityInStock;
            }
            index++;
        }
    }
    emit postReply(query_id,outputData);
}

void LocalClientHandler::buyFactoryProduct(const quint32 &query_id,const quint32 &factoryId,const quint32 &objectId,const quint32 &quantity,const quint32 &price)
{
    if(localClientHandlerFight.isInFight())
    {
        emit error(QStringLiteral("Try do inventory action when is in fight"));
        return;
    }
    if(captureCityInProgress())
    {
        emit error(QStringLiteral("Try do inventory action when is in capture city"));
        return;
    }
    if(!CommonDatapack::commonDatapack.industriesLink.contains(factoryId))
    {
        emit error(QStringLiteral("factory id not found"));
        return;
    }
    if(!CommonDatapack::commonDatapack.items.item.contains(objectId))
    {
        emit error(QStringLiteral("object id not found into the factory product list"));
        return;
    }
    if(!GlobalServerData::serverPrivateVariables.industriesStatus.contains(factoryId))
    {
        emit error(QStringLiteral("factory id not found in active list"));
        return;
    }
    if(!haveReputationRequirements(CommonDatapack::commonDatapack.industriesLink.value(factoryId).requirements.reputation))
    {
        emit error(QStringLiteral("The player have not the requirement: %1 to use the factory").arg(factoryId));
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
            emit error(QStringLiteral("internal bug, product for the factory not found"));
            return;
        }
    }
    if(player_informations->public_and_private_informations.cash<(actualPrice*quantity))
    {
        emit error(QStringLiteral("have not the cash to buy into this factory"));
        return;
    }
    if(quantity>quantityInStock)
    {
        out << (quint8)0x03;
        emit postReply(query_id,outputData);
        return;
    }
    if(actualPrice>price)
    {
        out << (quint8)0x04;
        emit postReply(query_id,outputData);
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
    emit postReply(query_id,outputData);
}

void LocalClientHandler::sellFactoryResource(const quint32 &query_id,const quint32 &factoryId,const quint32 &objectId,const quint32 &quantity,const quint32 &price)
{
    if(localClientHandlerFight.isInFight())
    {
        emit error(QStringLiteral("Try do inventory action when is in fight"));
        return;
    }
    if(captureCityInProgress())
    {
        emit error(QStringLiteral("Try do inventory action when is in capture city"));
        return;
    }
    if(!CommonDatapack::commonDatapack.industriesLink.contains(factoryId))
    {
        emit error(QStringLiteral("factory id not found"));
        return;
    }
    if(!CommonDatapack::commonDatapack.items.item.contains(objectId))
    {
        emit error(QStringLiteral("object id not found"));
        return;
    }
    if(objectQuantity(objectId)<quantity)
    {
        emit error(QStringLiteral("you have not the object quantity to sell at this factory"));
        return;
    }
    if(!haveReputationRequirements(CommonDatapack::commonDatapack.industriesLink.value(factoryId).requirements.reputation))
    {
        emit error(QStringLiteral("The player have not the requirement: %1 to use the factory").arg(factoryId));
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
                    emit postReply(query_id,outputData);
                    return;
                }
                resourcePrice=FacilityLib::getFactoryResourcePrice(industryStatus.resources.value(resource.item),resource,industry);
                if(price>resourcePrice)
                {
                    out << (quint8)0x04;
                    emit postReply(query_id,outputData);
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
            emit error(QStringLiteral("internal bug, resource for the factory not found"));
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
    emit postReply(query_id,outputData);
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
            emit dbQuery(QStringLiteral("UPDATE `character` SET `allow`='%1' WHERE `id`=%2;")
                         .arg(FacilityLib::allowToString(player_informations->public_and_private_informations.allow))
                         .arg(player_informations->character_id)
                         );
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            emit dbQuery(QStringLiteral("UPDATE character SET allow='%1' WHERE id=%2;")
                         .arg(FacilityLib::allowToString(player_informations->public_and_private_informations.allow))
                         .arg(player_informations->character_id)
                         );
        break;
    }
}

void LocalClientHandler::appendReputationRewards(const QList<ReputationRewards> &reputationList)
{
    int index=0;
    while(index<reputationList.size())
    {
        const ReputationRewards &reputationRewards=reputationList.at(index);
        appendReputationPoint(reputationRewards.type,reputationRewards.point);
        index++;
    }
}

//reputation
void LocalClientHandler::appendReputationPoint(const QString &type,const qint32 &point)
{
    if(point==0)
        return;
    if(!CommonDatapack::commonDatapack.reputation.contains(type))
    {
        emit error(QStringLiteral("Unknow reputation: %1").arg(type));
        return;
    }
    PlayerReputation playerReputation;
    playerReputation.point=0;
    playerReputation.level=0;
    if(player_informations->public_and_private_informations.reputation.contains(type))
        playerReputation=player_informations->public_and_private_informations.reputation.value(type);
    #ifdef DEBUG_MESSAGE_CLIENT_REPUTATION
    emit message(QStringLiteral("Reputation %1 at level: %2 with point: %3").arg(type).arg(playerReputation.level).arg(playerReputation.point));
    #endif
    playerReputation.point+=point;
    do
    {
        if(playerReputation.level<0 && playerReputation.point>0)
        {
            playerReputation.level++;
            playerReputation.point+=CommonDatapack::commonDatapack.reputation.value(type).reputation_negative.at(-playerReputation.level);
            continue;
        }
        if(playerReputation.level>0 && playerReputation.point<0)
        {
            playerReputation.level--;
            playerReputation.point+=CommonDatapack::commonDatapack.reputation.value(type).reputation_negative.at(playerReputation.level);
            continue;
        }
        if(playerReputation.level<=0 && playerReputation.point<CommonDatapack::commonDatapack.reputation.value(type).reputation_negative.at(-playerReputation.level))
        {
            if((-playerReputation.level)<CommonDatapack::commonDatapack.reputation.value(type).reputation_negative.size())
            {
                playerReputation.point-=CommonDatapack::commonDatapack.reputation.value(type).reputation_negative.at(-playerReputation.level);
                playerReputation.level--;
            }
            else
            {
                #ifdef DEBUG_MESSAGE_CLIENT_REPUTATION
                emit message(QStringLiteral("Reputation %1 at level max: %2").arg(type).arg(playerReputation.level));
                #endif
                playerReputation.point=CommonDatapack::commonDatapack.reputation.value(type).reputation_negative.at(-playerReputation.level);
            }
            continue;
        }
        if(playerReputation.level>=0 && playerReputation.point<CommonDatapack::commonDatapack.reputation.value(type).reputation_positive.at(playerReputation.level))
        {
            if(playerReputation.level<CommonDatapack::commonDatapack.reputation.value(type).reputation_negative.size())
            {
                playerReputation.point-=CommonDatapack::commonDatapack.reputation.value(type).reputation_negative.at(playerReputation.level);
                playerReputation.level++;
            }
            else
            {
                #ifdef DEBUG_MESSAGE_CLIENT_REPUTATION
                emit message(QStringLiteral("Reputation %1 at level max: %2").arg(type).arg(playerReputation.level));
                #endif
                playerReputation.point=CommonDatapack::commonDatapack.reputation.value(type).reputation_negative.at(playerReputation.level);
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
            emit dbQuery(QStringLiteral("INSERT INTO `reputation`(`character`,`type`,`point`,`level`) VALUES(%1,\"%2\",%3,%4);")
                             .arg(player_informations->character_id)
                             .arg(SqlFunction::quoteSqlVariable(type))
                             .arg(playerReputation.point)
                             .arg(playerReputation.level)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QStringLiteral("INSERT INTO reputation(character,type,point,level) VALUES(%1,\"%2\",%3,%4);")
                             .arg(player_informations->character_id)
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
                emit dbQuery(QStringLiteral("UPDATE `reputation` SET `point`=%3,`level`=%4 WHERE `character`=%1 AND `type`=\"%2\";")
                             .arg(player_informations->character_id)
                             .arg(SqlFunction::quoteSqlVariable(type))
                             .arg(playerReputation.point)
                             .arg(playerReputation.level)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QStringLiteral("UPDATE reputation SET point=%3,level=%4 WHERE character=%1 AND type=\"%2\";")
                             .arg(player_informations->character_id)
                             .arg(SqlFunction::quoteSqlVariable(type))
                             .arg(playerReputation.point)
                             .arg(playerReputation.level)
                             );
            break;
        }
    }
    player_informations->public_and_private_informations.reputation[type]=playerReputation;
    #ifdef DEBUG_MESSAGE_CLIENT_REPUTATION
    emit message(QStringLiteral("New reputation %1 at level: %2 with point: %3").arg(type).arg(playerReputation.level).arg(playerReputation.point));
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
        emit error(QStringLiteral("Try escape when not allowed"));
        return false;
    }
}

LocalClientHandlerFight * LocalClientHandler::getLocalClientHandlerFight()
{
    return &localClientHandlerFight;
}

void LocalClientHandler::heal()
{
    if(localClientHandlerFight.isInFight())
    {
        emit error(QStringLiteral("Try do heal action when is in fight"));
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message(QStringLiteral("ask heal at %1 (%2,%3)").arg(this->map->map_file).arg(this->x).arg(this->y));
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
                    emit error(QStringLiteral("heal() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                    return;
                }
            }
            else
            {
                emit error(QStringLiteral("No valid map in this direction"));
                return;
            }
        break;
        default:
        emit error(QStringLiteral("Wrong direction to use a heal"));
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
                        emit error(QStringLiteral("heal() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                        return;
                    }
                }
                else
                {
                    emit error(QStringLiteral("No valid map in this direction"));
                    return;
                }
            break;
            default:
            emit error(QStringLiteral("Wrong direction to use a heal"));
            return;
        }
        if(!static_cast<MapServer*>(this->map)->heal.contains(QPair<quint8,quint8>(x,y)))
        {
            emit error(QStringLiteral("no heal point in this direction"));
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
        emit error(QStringLiteral("error: map: %1 (%2,%3), is in fight").arg(static_cast<MapServer *>(map)->map_file).arg(x).arg(y));
        return;
    }
    if(captureCityInProgress())
    {
        emit error("Try requestFight when is in capture city");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    emit message(QStringLiteral("request fight at %1 (%2,%3)").arg(this->map->map_file).arg(this->x).arg(this->y));
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
                    emit error(QStringLiteral("requestFight() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                    return;
                }
            }
            else
            {
                emit error(QStringLiteral("No valid map in this direction"));
                return;
            }
        break;
        default:
        emit error(QStringLiteral("Wrong direction to use a shop"));
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
                        emit error(QStringLiteral("requestFight() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                        return;
                    }
                }
                else
                {
                    emit error(QStringLiteral("No valid map in this direction"));
                    return;
                }
            break;
            default:
            emit error(QStringLiteral("Wrong direction to use a shop"));
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
            emit error(QStringLiteral("no fight with id %1 in this direction").arg(fightId));
            return;
        }
    }
    emit message(QStringLiteral("is now in fight (after a request) on map %1 (%2,%3) with the bot %4").arg(map->map_file).arg(x).arg(y).arg(fightId));
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
                emit error(QStringLiteral("You are already in clan"));
                return;
            }
            if(text.isEmpty())
            {
                emit error(QStringLiteral("You can't create clan with empty name"));
                return;
            }
            if(!player_informations->public_and_private_informations.allow.contains(ActionAllow_Clan))
            {
                emit error(QStringLiteral("You have not the right to create clan"));
                return;
            }
            GlobalServerData::serverPrivateVariables.maxClanId++;
            player_informations->public_and_private_informations.clan=GlobalServerData::serverPrivateVariables.maxClanId;
            createMemoryClan();
            clan->name=text;
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
                    emit dbQuery(QStringLiteral("INSERT INTO `clan`(`id`,`name`,`date`) VALUES(%1,\"%2\",%3);")
                             .arg(GlobalServerData::serverPrivateVariables.maxClanId)
                             .arg(SqlFunction::quoteSqlVariable(text))
                             .arg(QDateTime::currentMSecsSinceEpoch()/1000)
                             );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    emit dbQuery(QStringLiteral("INSERT INTO clan(id,name,date) VALUES(%1,\"%2\",%3);")
                             .arg(GlobalServerData::serverPrivateVariables.maxClanId)
                             .arg(SqlFunction::quoteSqlVariable(text))
                             .arg(QDateTime::currentMSecsSinceEpoch()/1000)
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
                emit error(QStringLiteral("You have not a clan"));
                return;
            }
            if(player_informations->public_and_private_informations.clan_leader)
            {
                emit error(QStringLiteral("You can't leave if you are the leader"));
                return;
            }
            removeFromClan();
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
                    emit dbQuery(QStringLiteral("UPDATE `character` SET `clan`=0 WHERE `id`=%1;")
                             .arg(player_informations->character_id)
                             );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    emit dbQuery(QStringLiteral("UPDATE character SET clan=0 WHERE id=%1;")
                             .arg(player_informations->character_id)
                             );
                break;
            }
        }
        break;
        case 0x03:
        {
            if(player_informations->public_and_private_informations.clan==0)
            {
                emit error(QStringLiteral("You have not a clan"));
                return;
            }
            if(!player_informations->public_and_private_informations.clan_leader)
            {
                emit error(QStringLiteral("You are not a leader to dissolve the clan"));
                return;
            }
            if(!clan->captureCityInProgress.isEmpty())
            {
                emit error(QStringLiteral("You can't disolv the clan if is in city capture"));
                return;
            }
            const QList<LocalClientHandler *> &players=clanList.value(player_informations->public_and_private_informations.clan)->players;
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
                        emit dbQuery(QStringLiteral("UPDATE `character` SET `clan`=0 WHERE `id`=%1;")
                                 .arg(players.at(index)->getPlayerId())
                                 );
                    break;
                    case ServerSettings::Database::DatabaseType_SQLite:
                        emit dbQuery(QStringLiteral("UPDATE character SET clan=0 WHERE id=%1;")
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
                    emit dbQuery(QStringLiteral("DELETE FROM `clan` WHERE `id`=%1")
                                 .arg(player_informations->public_and_private_informations.clan)
                                 );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    emit dbQuery(QStringLiteral("DELETE FROM clan WHERE id=%1")
                                 .arg(player_informations->public_and_private_informations.clan)
                                 );
                break;
            }
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    emit dbQuery(QStringLiteral("DELETE FROM `city` WHERE `city`='%1'")
                                 .arg(clan->capturedCity)
                                 );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    emit dbQuery(QStringLiteral("DELETE FROM city WHERE city='%1'")
                                 .arg(clan->capturedCity)
                                 );
                break;
            }
            //update the object
            clanList.remove(player_informations->public_and_private_informations.clan);
            GlobalServerData::serverPrivateVariables.cityStatusListReverse.remove(clan->clanId);
            GlobalServerData::serverPrivateVariables.cityStatusList[clan->captureCityInProgress].clan=0;
            delete clan;
            index=0;
            while(index<players.size())
            {
                if(players.at(index)==this)
                {
                    player_informations->public_and_private_informations.clan=0;
                    clan=NULL;
                    emit clanChange(player_informations->public_and_private_informations.clan);//to send to another thread the clan change, 0 to remove
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
                emit error(QStringLiteral("You have not a clan"));
                return;
            }
            if(!player_informations->public_and_private_informations.clan_leader)
            {
                emit error(QStringLiteral("You are not a leader to invite into the clan"));
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
                if(playerByPseudo.value(text)->inviteToClan(player_informations->public_and_private_informations.clan))
                    out << (quint8)0x01;
                else
                    out << (quint8)0x02;
            }
            else
            {
                if(!isFound)
                    emit message(QStringLiteral("Clan invite: Player %1 not found, is connected?").arg(text));
                if(haveAClan)
                    emit message(QStringLiteral("Clan invite: Player %1 is already into a clan").arg(text));
                out << (quint8)0x02;
            }
            emit postReply(query_id,outputData);
        }
        break;
        case 0x05:
        {
            if(player_informations->public_and_private_informations.clan==0)
            {
                emit error(QStringLiteral("You have not a clan"));
                return;
            }
            if(!player_informations->public_and_private_informations.clan_leader)
            {
                emit error(QStringLiteral("You are not a leader to invite into the clan"));
                return;
            }
            if(player_informations->public_and_private_informations.public_informations.pseudo==text)
            {
                emit error(QStringLiteral("You can't eject your self"));
                return;
            }
            bool isIntoTheClan=false;
            if(playerByPseudo.contains(text))
                if(playerByPseudo.value(text)->getClanId()==player_informations->public_and_private_informations.clan)
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
                    emit message(QStringLiteral("Clan invite: Player %1 not found, is connected?").arg(text));
                if(!isIntoTheClan)
                    emit message(QStringLiteral("Clan invite: Player %1 is not into your clan").arg(text));
                out << (quint8)0x02;
            }
            emit postReply(query_id,outputData);
            if(!isFound)
            {
                switch(GlobalServerData::serverSettings.database.type)
                {
                    default:
                    case ServerSettings::Database::DatabaseType_Mysql:
                        emit dbQuery(QStringLiteral("UPDATE `character` SET `clan`=0 WHERE `pseudo`=%1 AND `clan`=%2;")
                                 .arg(text)
                                 .arg(player_informations->public_and_private_informations.clan)
                                 );
                    break;
                    case ServerSettings::Database::DatabaseType_SQLite:
                        emit dbQuery(QStringLiteral("UPDATE character SET clan=0 WHERE pseudo=%1 AND clan=%2;")
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
            emit error(QStringLiteral("Action on the clan not found"));
        return;
    }
}

quint32 LocalClientHandler::getPlayerId() const
{
    if(player_informations->character_loaded)
        return player_informations->character_id;
    return 0;
}

void LocalClientHandler::haveClanInfo(const quint32 &clanId,const QString &clanName,const quint64 &cash)
{
    emit message(QStringLiteral("First client of the clan: %1, clanId: %2 to connect").arg(clanName).arg(clanId));
    player_informations->public_and_private_informations.clan=clanId;
    createMemoryClan();
    clanList[clanId]->name=clanName;
    clanList[clanId]->cash=cash;
}

void LocalClientHandler::sendClanInfo()
{
    if(player_informations->public_and_private_informations.clan==0)
        return;
    if(clan==NULL)
        return;
    emit message(QStringLiteral("Send the clan info: %1, clanId: %2, get the info").arg(clan->name).arg(player_informations->public_and_private_informations.clan));
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << clan->name;
    emit sendFullPacket(0xC2,0x000A,outputData);
}

void LocalClientHandler::dissolvedClan()
{
    player_informations->public_and_private_informations.clan=0;
    clan=NULL;
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
    if(clan==NULL)
        return false;
    inviteToClanList << clanId;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint32)clanId;
    out << clan->name;
    emit sendFullPacket(0xC2,0x000B,outputData);
    return false;
}

void LocalClientHandler::clanInvite(const bool &accept)
{
    if(!accept)
    {
        emit message(QStringLiteral("You have refused the clan invitation"));
        inviteToClanList.removeFirst();
        return;
    }
    emit message(QStringLiteral("You have accepted the clan invitation"));
    if(inviteToClanList.isEmpty())
    {
        emit error(QStringLiteral("Can't responde to clan invite, because no in suspend"));
        return;
    }
    player_informations->public_and_private_informations.clan_leader=false;
    player_informations->public_and_private_informations.clan=inviteToClanList.first();
    createMemoryClan();
    insertIntoAClan(inviteToClanList.first());
    inviteToClanList.removeFirst();
}

quint32 LocalClientHandler::clanId() const
{
    return player_informations->public_and_private_informations.clan;
}

void LocalClientHandler::insertIntoAClan(const quint32 &clanId)
{
    //add into db
    QString clan_leader;
    if(player_informations->public_and_private_informations.clan_leader)
        clan_leader=LocalClientHandler::text_1;
    else
        clan_leader=LocalClientHandler::text_0;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            emit dbQuery(QStringLiteral("UPDATE `character` SET `clan`=%1,`clan_leader`=%2 WHERE `id`=%3;")
                     .arg(clanId)
                     .arg(clan_leader)
                     .arg(player_informations->character_id)
                     );
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            emit dbQuery(QStringLiteral("UPDATE character SET clan=%1,clan_leader=%2 WHERE id=%3;")
                     .arg(clanId)
                     .arg(clan_leader)
                     .arg(player_informations->character_id)
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
            emit dbQuery(QStringLiteral("UPDATE `character` SET `clan`=0 WHERE `id`=%1;")
                     .arg(player_informations->character_id)
                     );
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            emit dbQuery(QStringLiteral("UPDATE character SET clan=0 WHERE id=%1;")
                     .arg(player_informations->character_id)
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
        emit error(QStringLiteral("Try capture city when is not in clan"));
        return;
    }
    if(!cancel)
    {
        if(captureCityInProgress())
        {
            emit error(QStringLiteral("Try capture city when is already into that's"));
            return;
        }
        if(localClientHandlerFight.isInFight())
        {
            emit error(QStringLiteral("Try capture city when is in fight"));
            return;
        }
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        emit message(QStringLiteral("ask zonecapture at %1 (%2,%3)").arg(this->map->map_file).arg(this->x).arg(this->y));
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
                        emit error(QStringLiteral("waitingForCityCaputre() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                        return;
                    }
                }
                else
                {
                    emit error(QStringLiteral("No valid map in this direction"));
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
                            emit error(QStringLiteral("waitingForCityCaputre() Can't move at this direction from %1 (%2,%3)").arg(map->map_file).arg(x).arg(y));
                            return;
                        }
                    }
                    else
                    {
                        emit error(QStringLiteral("No valid map in this direction"));
                        return;
                    }
                break;
                default:
                emit error(QStringLiteral("Wrong direction to use a zonecapture"));
                return;
            }
            if(!static_cast<MapServer*>(this->map)->zonecapture.contains(QPair<quint8,quint8>(x,y)))
            {
                emit error(QStringLiteral("no zonecapture point in this direction"));
                return;
            }
        }
        //send the zone capture
        const QString &zoneName=static_cast<MapServer*>(this->map)->zonecapture.value(QPair<quint8,quint8>(x,y));
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
            emit sendFullPacket(0xF0,0x0001,outputData);
            return;
        }
        if(captureCity.count(zoneName)>0)
        {
            emit error(QStringLiteral("already in capture city"));
            return;
        }
        captureCity[zoneName] << this;
        localClientHandlerFight.setInCityCapture(true);
    }
    else
    {
        if(clan->captureCityInProgress.isEmpty())
        {
            emit error(QStringLiteral("your clan is not in capture city"));
            return;
        }
        if(!captureCity[clan->captureCityInProgress].removeOne(this))
        {
            emit error(QStringLiteral("not in capture city"));
            return;
        }
        leaveTheCityCapture();
    }
}

void LocalClientHandler::leaveTheCityCapture()
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
    localClientHandlerFight.setInCityCapture(false);
    otherCityPlayerBattle=NULL;
}

void LocalClientHandler::startTheCityCapture()
{
    QHashIterator<QString,QList<LocalClientHandler *> > i(captureCity);
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
                        tempCaptureCityValidated.players.at(index)->getLocalClientHandlerFight()->battleFakeAccepted(tempCaptureCityValidated.players.at(sub_index)->getLocalClientHandlerFight());
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
                tempCaptureCityValidated.players.first()->getLocalClientHandlerFight()->botFightStart(tempCaptureCityValidated.bots.first());
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
void LocalClientHandler::fightOrBattleFinish(const bool &win, const quint32 &fightId)
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
                            getLocalClientHandlerFight()->battleFakeAccepted(captureCityValidated.players.at(index)->getLocalClientHandlerFight());
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
                        localClientHandlerFight.botFightStart(captureCityValidated.bots.first());
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
                        switch(GlobalServerData::serverSettings.database.type)
                        {
                            default:
                            case ServerSettings::Database::DatabaseType_Mysql:
                                emit dbQuery(QStringLiteral("DELETE FROM `city` WHERE `city`='%1'")
                                             .arg(clan->capturedCity)
                                             );
                            break;
                            case ServerSettings::Database::DatabaseType_SQLite:
                                emit dbQuery(QStringLiteral("DELETE FROM city WHERE city='%1'")
                                             .arg(clan->capturedCity)
                                             );
                            break;
                        }
                        if(!GlobalServerData::serverPrivateVariables.cityStatusList.contains(clan->captureCityInProgress))
                            GlobalServerData::serverPrivateVariables.cityStatusList[clan->captureCityInProgress].clan=0;
                        if(GlobalServerData::serverPrivateVariables.cityStatusList.value(clan->captureCityInProgress).clan!=0)
                            switch(GlobalServerData::serverSettings.database.type)
                            {
                                default:
                                case ServerSettings::Database::DatabaseType_Mysql:
                                    emit dbQuery(QStringLiteral("UPDATE `city` SET `clan`=%1 WHERE `city`='%2';")
                                                 .arg(clan->clanId)
                                                 .arg(clan->captureCityInProgress)
                                                 );
                                break;
                                case ServerSettings::Database::DatabaseType_SQLite:
                                    emit dbQuery(QStringLiteral("UPDATE city SET clan=%1 WHERE city='%2';")
                                                 .arg(clan->clanId)
                                                 .arg(clan->captureCityInProgress)
                                                 );
                                break;
                            }
                        else
                            switch(GlobalServerData::serverSettings.database.type)
                            {
                                default:
                                case ServerSettings::Database::DatabaseType_Mysql:
                                    emit dbQuery(QStringLiteral("INSERT INTO `city`(`clan`,`city`) VALUES(%1,'%2');")
                                                 .arg(clan->clanId)
                                                 .arg(clan->captureCityInProgress)
                                                 );
                                break;
                                case ServerSettings::Database::DatabaseType_SQLite:
                                    emit dbQuery(QStringLiteral("INSERT INTO city(clan,city) VALUES(%1,'%2');")
                                                 .arg(clan->clanId)
                                                 .arg(clan->captureCityInProgress)
                                                 );
                                break;
                            }
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

void LocalClientHandler::cityCaptureSendInWait(const CaptureCityValidated &captureCityValidated, const quint16 &number_of_player, const quint16 &number_of_clan)
{
    int index=0;
    while(index<captureCityValidated.players.size())
    {
        captureCityValidated.playersInFight.last()->cityCaptureInWait(number_of_player,number_of_clan);
        index++;
    }
}

quint16 LocalClientHandler::cityCapturePlayerCount(const CaptureCityValidated &captureCityValidated)
{
    return captureCityValidated.bots.size()+captureCityValidated.botsInFight.size()+captureCityValidated.players.size()+captureCityValidated.playersInFight.size();
}

quint16 LocalClientHandler::cityCaptureClanCount(const CaptureCityValidated &captureCityValidated)
{
    if(captureCityValidated.bots.isEmpty() && captureCityValidated.botsInFight.isEmpty())
        return captureCityValidated.clanSize.size();
    else
        return captureCityValidated.clanSize.size()+1;
}

void LocalClientHandler::cityCaptureBattle(const quint16 &number_of_player,const quint16 &number_of_clan)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x04;
    out << (quint16)number_of_player;
    out << (quint16)number_of_clan;
    emit sendFullPacket(0xF0,0x0001,outputData);
}

void LocalClientHandler::cityCaptureBotFight(const quint16 &number_of_player,const quint16 &number_of_clan,const quint32 &fightId)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x04;
    out << (quint16)number_of_player;
    out << (quint16)number_of_clan;
    out << (quint32)fightId;
    emit sendFullPacket(0xF0,0x0001,outputData);
}

void LocalClientHandler::cityCaptureInWait(const quint16 &number_of_player,const quint16 &number_of_clan)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x05;
    out << (quint16)number_of_player;
    out << (quint16)number_of_clan;
    emit sendFullPacket(0xF0,0x0001,outputData);
}

void LocalClientHandler::cityCaptureWin()
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x06;
    emit sendFullPacket(0xF0,0x0001,outputData);
}

void LocalClientHandler::previousCityCaptureNotFinished()
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x02;
    emit sendFullPacket(0xF0,0x0003,outputData);
}

void LocalClientHandler::resetAll()
{
    LocalClientHandler::playerByPseudo.clear();
    LocalClientHandler::captureCity.clear();
    LocalClientHandler::captureCityValidatedList.clear();
    LocalClientHandler::clanList.clear();
}

void LocalClientHandler::moveMonster(const bool &up,const quint8 &number)
{
    if(up)
        localClientHandlerFight.moveUpMonster(number-1);
    else
        localClientHandlerFight.moveDownMonster(number-1);
}

void LocalClientHandler::changeOfMonsterInFight(const quint32 &monsterId)
{
    localClientHandlerFight.changeOfMonsterInFight(monsterId);
}

void LocalClientHandler::getMarketList(const quint32 &query_id)
{
    if(getInTrade() || localClientHandlerFight.isInFight())
    {
        emit error(QStringLiteral("You can't use the market in trade/fight"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint64)player_informations->market_cash;
    out << (double)player_informations->market_bitcoin;
    int index;
    QList<MarketItem> marketItemList,marketOwnItemList;
    QList<MarketPlayerMonster> marketPlayerMonsterList,marketOwnPlayerMonsterList;
    //object filter
    index=0;
    while(index<GlobalServerData::serverPrivateVariables.marketItemList.size())
    {
        const MarketItem &marketObject=GlobalServerData::serverPrivateVariables.marketItemList.at(index);
        if(bitcoinEnabled() || marketObject.bitcoin==0)
        {
            if(marketObject.player==player_informations->character_id)
                marketOwnItemList << marketObject;
            else
                marketItemList << marketObject;
        }
        index++;
    }
    //monster filter
    index=0;
    while(index<GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.size())
    {
        const MarketPlayerMonster &marketPlayerMonster=GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.at(index);
        if(bitcoinEnabled() || marketPlayerMonster.bitcoin==0)
        {
            if(marketPlayerMonster.player==player_informations->character_id)
                marketOwnPlayerMonsterList << marketPlayerMonster;
            else
                marketPlayerMonsterList << marketPlayerMonster;
        }
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
        out << marketObject.bitcoin;
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
        out << marketPlayerMonster.bitcoin;
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
        out << marketObject.bitcoin;
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
        out << marketPlayerMonster.bitcoin;
        index++;
    }

    emit postReply(query_id,outputData);
}

void LocalClientHandler::buyMarketObject(const quint32 &query_id,const quint32 &marketObjectId,const quint32 &quantity)
{
    if(getInTrade() || localClientHandlerFight.isInFight())
    {
        emit error(QStringLiteral("You can't use the market in trade/fight"));
        return;
    }
    if(quantity<=0)
    {
        emit error(QStringLiteral("You can't use the market with null quantity"));
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
                emit postReply(query_id,outputData);
                return;
            }
            //check if have the price
            if((quantity*marketItem.cash)>player_informations->public_and_private_informations.cash)
            {
                out << (quint8)0x03;
                emit postReply(query_id,outputData);
                return;
            }
            if(marketItem.bitcoin>0)
            {
                if(!bitcoinEnabled())
                {
                    emit error(QStringLiteral("Try put in bitcoin but don't have bitcoin access"));
                    return;
                }
                if(!playerById.value(marketItem.player)->bitcoinEnabled())
                {
                    emit message(QStringLiteral("The other have not the bitcoin enabled to buy their object"));
                    out << (quint8)0x03;
                    emit postReply(query_id,outputData);
                    return;
                }
            }
            if((quantity*marketItem.bitcoin)>player_informations->public_and_private_informations.bitcoin)
            {
                out << (quint8)0x03;
                emit postReply(query_id,outputData);
                return;
            }
            //apply the buy
            if(marketItem.quantity==quantity)
            {
                switch(GlobalServerData::serverSettings.database.type)
                {
                    default:
                    case ServerSettings::Database::DatabaseType_Mysql:
                        emit dbQuery(QStringLiteral("DELETE FROM `item` WHERE `item`=%1 AND `character`=%2 AND `place`='market'")
                                     .arg(marketItem.item)
                                     .arg(marketItem.player)
                                     );
                    break;
                    case ServerSettings::Database::DatabaseType_SQLite:
                        emit dbQuery(QStringLiteral("DELETE FROM item WHERE item=%1 AND character=%2 AND place='market'")
                                     .arg(marketItem.item)
                                     .arg(marketItem.player)
                                     );
                    break;
                }
                GlobalServerData::serverPrivateVariables.marketItemList.removeAt(index);
            }
            else
            {
                GlobalServerData::serverPrivateVariables.marketItemList[index].quantity=marketItem.quantity-quantity;
                switch(GlobalServerData::serverSettings.database.type)
                {
                    default:
                    case ServerSettings::Database::DatabaseType_Mysql:
                        emit dbQuery(QStringLiteral("UPDATE `item` SET `quantity`=%1 WHERE `item`=%2 AND `character`=%3 AND `place`='market'")
                                     .arg(marketItem.quantity-quantity)
                                     .arg(marketItem.item)
                                     .arg(marketItem.player)
                                     );
                    break;
                    case ServerSettings::Database::DatabaseType_SQLite:
                        emit dbQuery(QStringLiteral("UPDATE item SET quantity=%1 WHERE item=%2 AND character=%3 AND place='market'")
                                     .arg(marketItem.quantity-quantity)
                                     .arg(marketItem.item)
                                     .arg(marketItem.player)
                                     );
                    break;
                }
            }
            removeCash(quantity*marketItem.cash);
            if(marketItem.bitcoin>0)
                removeBitcoin(quantity*marketItem.bitcoin);
            if(playerById.contains(marketItem.player))
            {
                if(!playerById.value(marketItem.player)->addMarketCashWithoutSave(quantity*marketItem.cash,quantity*marketItem.bitcoin))
                    emit message(QStringLiteral("Problem at market cash adding"));
            }
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    emit dbQuery(QStringLiteral("UPDATE `character` SET `market_cash`=`market_cash`+%1,`market_bitcoin`=`market_bitcoin`+%2 WHERE `id`=%3")
                                 .arg(quantity*marketItem.cash)
                                 .arg(quantity*marketItem.bitcoin)
                                 .arg(marketItem.player)
                                 );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    emit dbQuery(QStringLiteral("UPDATE character SET market_cash=market_cash+%1,market_bitcoin=market_bitcoin+%2 WHERE id=%3")
                                 .arg(quantity*marketItem.cash)
                                 .arg(quantity*marketItem.bitcoin)
                                 .arg(marketItem.player)
                                 );
                break;
            }
            addObject(marketItem.item,quantity);
            out << (quint8)0x01;
            emit postReply(query_id,outputData);
            return;
        }
        index++;
    }
    out << (quint8)0x03;
    emit postReply(query_id,outputData);
}

void LocalClientHandler::buyMarketMonster(const quint32 &query_id,const quint32 &monsterId)
{
    if(getInTrade() || localClientHandlerFight.isInFight())
    {
        emit error(QStringLiteral("You can't use the market in trade/fight"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    if(player_informations->public_and_private_informations.playerMonster.size()>=CATCHCHALLENGER_MONSTER_MAX_WEAR_ON_PLAYER)
    {
        out << (quint8)0x02;
        emit postReply(query_id,outputData);
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
            if(marketPlayerMonster.cash>player_informations->public_and_private_informations.cash)
            {
                out << (quint8)0x03;
                emit postReply(query_id,outputData);
                return;
            }
            if(marketPlayerMonster.bitcoin>0)
            {
                if(!bitcoinEnabled())
                {
                    emit error(QStringLiteral("Try put in bitcoin but don't have bitcoin access"));
                    return;
                }
                if(!playerById.value(marketPlayerMonster.player)->bitcoinEnabled())
                {
                    emit message(QStringLiteral("The other have not the bitcoin enabled to buy their object"));
                    out << (quint8)0x03;
                    emit postReply(query_id,outputData);
                    return;
                }
            }
            if(marketPlayerMonster.bitcoin>player_informations->public_and_private_informations.bitcoin)
            {
                out << (quint8)0x03;
                emit postReply(query_id,outputData);
                return;
            }
            //apply the buy
            GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.removeAt(index);
            removeCash(marketPlayerMonster.cash);
            if(marketPlayerMonster.bitcoin>0)
                removeBitcoin(marketPlayerMonster.bitcoin);
            if(playerById.contains(marketPlayerMonster.player))
            {
                if(!playerById.value(marketPlayerMonster.player)->addMarketCashWithoutSave(marketPlayerMonster.cash,marketPlayerMonster.bitcoin))
                    emit message(QStringLiteral("Problem at market cash adding"));
            }
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    emit dbQuery(QStringLiteral("UPDATE `character` SET `market_cash`=`market_cash`+%1,`market_bitcoin`=`market_bitcoin`+%2 WHERE `id`=%3")
                                 .arg(marketPlayerMonster.cash)
                                 .arg(marketPlayerMonster.bitcoin)
                                 .arg(marketPlayerMonster.player)
                                 );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    emit dbQuery(QStringLiteral("UPDATE character SET market_cash=market_cash+%1,market_bitcoin=market_bitcoin+%2 WHERE id=%3")
                                 .arg(marketPlayerMonster.cash)
                                 .arg(marketPlayerMonster.bitcoin)
                                 .arg(marketPlayerMonster.player)
                                 );
                break;
            }
            localClientHandlerFight.addPlayerMonster(marketPlayerMonster.monster);
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    emit dbQuery(QStringLiteral("UPDATE `monster` SET `place`='wear',`character`=%1,`position`=%2 WHERE `id`=%3")
                                 .arg(player_informations->character_id)
                                 .arg(localClientHandlerFight.getPlayerMonster().size())
                                 .arg(marketPlayerMonster.monster.id)
                                 );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    emit dbQuery(QStringLiteral("UPDATE monster SET place='wear',character=%1,position=%2 WHERE id=%3")
                                 .arg(player_informations->character_id)
                                 .arg(localClientHandlerFight.getPlayerMonster().size())
                                 .arg(marketPlayerMonster.monster.id)
                                 );
                break;
            }
            out << (quint8)0x01;
            emit postReply(query_id,outputData);
            return;
        }
        index++;
    }
    out << (quint8)0x03;
    emit postReply(query_id,outputData);
}

void LocalClientHandler::putMarketObject(const quint32 &query_id,const quint32 &objectId,const quint32 &quantity,const quint32 &price,const double &bitcoin)
{
    if(getInTrade() || localClientHandlerFight.isInFight())
    {
        emit error(QStringLiteral("You can't use the market in trade/fight"));
        return;
    }
    if(!bitcoinEnabled() && bitcoin>0)
    {
        emit error(QStringLiteral("Try put in bitcoin but don't have bitcoin access"));
        return;
    }
    if(quantity<=0)
    {
        emit error(QStringLiteral("You can't use the market with null quantity"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    if(objectQuantity(objectId)<quantity)
    {
        out << (quint8)0x02;
        emit postReply(query_id,outputData);
        return;
    }
    //search into the market
    int index=0;
    while(index<GlobalServerData::serverPrivateVariables.marketItemList.size())
    {
        const MarketItem &marketItem=GlobalServerData::serverPrivateVariables.marketItemList.at(index);
        if(marketItem.player==player_informations->character_id && marketItem.item==objectId)
        {
            removeObject(objectId,quantity);
            GlobalServerData::serverPrivateVariables.marketItemList[index].cash=price;
            GlobalServerData::serverPrivateVariables.marketItemList[index].bitcoin=bitcoin;
            GlobalServerData::serverPrivateVariables.marketItemList[index].quantity+=quantity;
            out << (quint8)0x01;
            emit postReply(query_id,outputData);
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    emit dbQuery(QStringLiteral("UPDATE item SET `quantity`=%1,`market_price`=%2,`market_bitcoin`=%3 WHERE `item`=%4 AND `character`=%5 AND `place`='market';")
                                 .arg(GlobalServerData::serverPrivateVariables.marketItemList.value(index).quantity)
                                 .arg(price)
                                 .arg(bitcoin)
                                 .arg(marketItem.item)
                                 .arg(marketItem.player)
                                 );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    emit dbQuery(QStringLiteral("UPDATE item SET quantity=%1,market_price=%2,market_bitcoin=%3 WHERE item=%4 AND character=%5 AND place='market';")
                                 .arg(GlobalServerData::serverPrivateVariables.marketItemList.value(index).quantity)
                                 .arg(price)
                                 .arg(bitcoin)
                                 .arg(marketItem.item)
                                 .arg(marketItem.player)
                                 );
                break;
            }
            return;
        }
        index++;
    }
    if(marketObjectIdList.isEmpty())
    {
        out << (quint8)0x02;
        emit postReply(query_id,outputData);
        emit message(QStringLiteral("No more id into marketObjectIdList"));
        return;
    }
    //append to the market
    removeObject(objectId,quantity);
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            emit dbQuery(QStringLiteral("INSERT INTO `item`(`item`,`character`,`quantity`,`place`,`market_price`,`market_bitcoin`) VALUES(%1,%2,%3,'market',%4,%5);")
                         .arg(objectId)
                         .arg(player_informations->character_id)
                         .arg(quantity)
                         .arg(price)
                         .arg(bitcoin)
                         );
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            emit dbQuery(QStringLiteral("INSERT INTO item(item,character,quantity,place,market_price,market_bitcoin) VALUES(%1,%2,%3,'market',%4,%5);")
                         .arg(objectId)
                         .arg(player_informations->character_id)
                         .arg(quantity)
                         .arg(price)
                         .arg(bitcoin)
                         );
        break;
    }
    MarketItem marketItem;
    marketItem.bitcoin=bitcoin;
    marketItem.cash=price;
    marketItem.item=objectId;
    marketItem.marketObjectId=marketObjectIdList.first();
    marketItem.player=player_informations->character_id;
    marketItem.quantity=quantity;
    marketObjectIdList.removeFirst();
    GlobalServerData::serverPrivateVariables.marketItemList << marketItem;
    out << (quint8)0x01;
    emit postReply(query_id,outputData);
}

void LocalClientHandler::putMarketMonster(const quint32 &query_id,const quint32 &monsterId,const quint32 &price,const double &bitcoin)
{
    if(getInTrade() || localClientHandlerFight.isInFight())
    {
        emit error(QStringLiteral("You can't use the market in trade/fight"));
        return;
    }
    if(!bitcoinEnabled() && bitcoin>0)
    {
        emit error(QStringLiteral("Try put in bitcoin but don't have bitcoin access"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    int index=0;
    while(index<player_informations->public_and_private_informations.playerMonster.size())
    {
        const PlayerMonster &playerMonster=player_informations->public_and_private_informations.playerMonster.at(index);
        if(playerMonster.id==monsterId)
        {
            if(!localClientHandlerFight.remainMonstersToFight(monsterId))
            {
                emit message(QStringLiteral("You can't put in market this msonter because you will be without monster to fight"));
                out << (quint8)0x02;
                emit postReply(query_id,outputData);
                return;
            }
            MarketPlayerMonster marketPlayerMonster;
            marketPlayerMonster.bitcoin=bitcoin;
            marketPlayerMonster.cash=price;
            marketPlayerMonster.monster=playerMonster;
            marketPlayerMonster.player=player_informations->character_id;
            player_informations->public_and_private_informations.playerMonster.removeAt(index);
            GlobalServerData::serverPrivateVariables.marketPlayerMonsterList << marketPlayerMonster;
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    emit dbQuery(QStringLiteral("UPDATE `monster` SET `place`='market',`market_price`=%1,`market_bitcoin`=%2 WHERE `id`=%3;")
                                 .arg(price)
                                 .arg(bitcoin)
                                 .arg(monsterId)
                                 );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    emit dbQuery(QStringLiteral("UPDATE monster SET place='market',market_price=%1,market_bitcoin=%2 WHERE id=%3;")
                                 .arg(price)
                                 .arg(bitcoin)
                                 .arg(monsterId)
                                 );
                break;
            }
            while(index<player_informations->public_and_private_informations.playerMonster.size())
            {
                const PlayerMonster &playerMonster=player_informations->public_and_private_informations.playerMonster.at(index);
                switch(GlobalServerData::serverSettings.database.type)
                {
                    default:
                    case ServerSettings::Database::DatabaseType_Mysql:
                        emit dbQuery(QStringLiteral("UPDATE `monster` SET `position`=%1 WHERE `id`=%2;")
                                     .arg(index+1)
                                     .arg(playerMonster.id)
                                     );
                    break;
                    case ServerSettings::Database::DatabaseType_SQLite:
                        emit dbQuery(QStringLiteral("UPDATE monster SET position=%1 WHERE id=%2;")
                                     .arg(index+1)
                                     .arg(playerMonster.id)
                                     );
                    break;
                }
                index++;
            }
            out << (quint8)0x01;
            emit postReply(query_id,outputData);
            return;
        }
        index++;
    }
    out << (quint8)0x02;
    emit postReply(query_id,outputData);
}

bool LocalClientHandler::bitcoinEnabled() const
{
    return GlobalServerData::serverPrivateVariables.bitcoin.enabled && player_informations->public_and_private_informations.bitcoin>=0;
}

void LocalClientHandler::recoverMarketCash(const quint32 &query_id)
{
    if(getInTrade() || localClientHandlerFight.isInFight())
    {
        emit error(QStringLiteral("You can't use the market in trade/fight"));
        return;
    }
    bool bitcoin_enabled=bitcoinEnabled();
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint64)player_informations->market_cash;
    if(bitcoin_enabled)
    {
        if(player_informations->market_bitcoin>=0)
            emit message(QStringLiteral("Get %1 bitcoin from the market").arg(player_informations->market_bitcoin));
        out << (double)player_informations->market_bitcoin;
        player_informations->public_and_private_informations.bitcoin+=player_informations->market_bitcoin;
        player_informations->market_bitcoin=0;
    }
    else
        out << (double)0;
    player_informations->public_and_private_informations.cash+=player_informations->market_cash;
    player_informations->market_cash=0;
    if(bitcoin_enabled)
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                emit dbQuery(QStringLiteral("UPDATE `character` SET `cash`=%1,`bitcoin`=%2,`market_cash`=0,`market_bitcoin`=0 WHERE `id`=%3;")
                             .arg(player_informations->public_and_private_informations.cash)
                             .arg(player_informations->public_and_private_informations.bitcoin)
                             .arg(player_informations->character_id)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QStringLiteral("UPDATE character SET cash=%1,bitcoin=%2,market_cash=0,market_bitcoin=0 WHERE id=%3;")
                             .arg(player_informations->public_and_private_informations.cash)
                             .arg(player_informations->public_and_private_informations.bitcoin)
                             .arg(player_informations->character_id)
                             );
            break;
        }
    else
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                emit dbQuery(QStringLiteral("UPDATE `character` SET `cash`=%1,`market_cash`=0,`market_bitcoin`=0 WHERE `id`=%2;")
                             .arg(player_informations->public_and_private_informations.cash)
                             .arg(player_informations->character_id)
                             );
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                emit dbQuery(QStringLiteral("UPDATE character SET cash=%1,market_cash=0,market_bitcoin=0 WHERE id=%2;")
                             .arg(player_informations->public_and_private_informations.cash)
                             .arg(player_informations->character_id)
                             );
            break;
        }
    emit postReply(query_id,outputData);
}

void LocalClientHandler::withdrawMarketObject(const quint32 &query_id,const quint32 &objectId,const quint32 &quantity)
{
    if(getInTrade() || localClientHandlerFight.isInFight())
    {
        emit error(QStringLiteral("You can't use the market in trade/fight"));
        return;
    }
    if(quantity<=0)
    {
        emit error(QStringLiteral("You can't use the market with null quantity"));
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
            if(marketItem.player!=player_informations->character_id)
            {
                out << (quint8)0x02;
                emit postReply(query_id,outputData);
                return;
            }
            if(marketItem.quantity<quantity)
            {
                out << (quint8)0x02;
                emit postReply(query_id,outputData);
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
                switch(GlobalServerData::serverSettings.database.type)
                {
                    default:
                    case ServerSettings::Database::DatabaseType_Mysql:
                        emit dbQuery(QStringLiteral("DELETE FROM `item` WHERE `item`=%1 AND `characterè=%2 AND `place`='market'")
                                     .arg(objectId)
                                     .arg(player_informations->character_id)
                                     );
                    break;
                    case ServerSettings::Database::DatabaseType_SQLite:
                        emit dbQuery(QStringLiteral("DELETE FROM item WHERE item=%1 AND character=%2 AND place='market'")
                                     .arg(objectId)
                                     .arg(player_informations->character_id)
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
                        emit dbQuery(QStringLiteral("UPDATE `item` SET `quantity`=%1 WHERE `item`=%2 AND `character`=%3 AND `place`='market'")
                                     .arg(GlobalServerData::serverPrivateVariables.marketItemList.value(index).quantity)
                                     .arg(objectId)
                                     .arg(player_informations->character_id)
                                     );
                    break;
                    case ServerSettings::Database::DatabaseType_SQLite:
                        emit dbQuery(QStringLiteral("UPDATE item SET quantity=%1 WHERE item=%2 AND character=%3 AND place='market'")
                                     .arg(GlobalServerData::serverPrivateVariables.marketItemList.value(index).quantity)
                                     .arg(objectId)
                                     .arg(player_informations->character_id)
                                     );
                    break;
                }
            }
            addObject(objectId,quantity);
            emit postReply(query_id,outputData);
            return;
        }
        index++;
    }
    out << (quint8)0x02;
    emit postReply(query_id,outputData);
}

void LocalClientHandler::withdrawMarketMonster(const quint32 &query_id,const quint32 &monsterId)
{
    if(getInTrade() || localClientHandlerFight.isInFight())
    {
        emit error(QStringLiteral("You can't use the market in trade/fight"));
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
            if(marketPlayerMonster.player!=player_informations->character_id)
            {
                out << (quint8)0x02;
                emit postReply(query_id,outputData);
                return;
            }
            if(player_informations->public_and_private_informations.playerMonster.size()>=CATCHCHALLENGER_MONSTER_MAX_WEAR_ON_PLAYER)
            {
                out << (quint8)0x02;
                emit postReply(query_id,outputData);
                return;
            }
            GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.removeAt(index);
            player_informations->public_and_private_informations.playerMonster << marketPlayerMonster.monster;
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    emit dbQuery(QStringLiteral("UPDATE `monster` SET `place`='wear',`position`=%1 WHERE `id`=%2;")
                                 .arg(player_informations->public_and_private_informations.playerMonster.size())
                                 .arg(player_informations->public_and_private_informations.playerMonster.last().id)
                                 );
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    emit dbQuery(QStringLiteral("UPDATE monster SET place='wear',position=%1 WHERE id=%2;")
                                 .arg(player_informations->public_and_private_informations.playerMonster.size())
                                 .arg(player_informations->public_and_private_informations.playerMonster.last().id)
                                 );
                break;
            }
            out << (quint8)0x01;
            out << (quint8)0x02;
            emit postReply(query_id,outputData+FacilityLib::privateMonsterToBinary(player_informations->public_and_private_informations.playerMonster.last()));
            return;
        }
        index++;
    }
    out << (quint8)0x02;
    emit postReply(query_id,outputData);
}

void LocalClientHandler::confirmEvolution(const quint32 &monsterId)
{
    localClientHandlerFight.confirmEvolution(monsterId);
}

bool LocalClientHandler::haveReputationRequirements(const QList<ReputationRequirements> &reputationList) const
{
    int index=0;
    while(index<reputationList.size())
    {
        const CatchChallenger::ReputationRequirements &reputation=reputationList.at(index);
        if(player_informations->public_and_private_informations.reputation.contains(reputation.type))
        {
            const PlayerReputation &playerReputation=player_informations->public_and_private_informations.reputation.value(reputation.type);
            if(!reputation.positif)
            {
                if(-reputation.level<playerReputation.level)
                {
                    emit message(QStringLiteral("reputation.level(%1)<playerReputation.level(%2)").arg(reputation.level).arg(playerReputation.level));
                    return false;
                }
            }
            else
            {
                if(reputation.level>playerReputation.level || playerReputation.point<0)
                {
                    emit message(QStringLiteral("reputation.level(%1)>playerReputation.level(%2) || playerReputation.point(%3)<0").arg(reputation.level).arg(playerReputation.level).arg(playerReputation.point));
                    return false;
                }
            }
        }
        else
            if(!reputation.positif)//default level is 0, but required level is negative
            {
                emit message(QStringLiteral("reputation.level(%1)<0 and no reputation.type=%2").arg(reputation.level).arg(reputation.type));
                return false;
            }
        index++;
    }
    return true;
}
