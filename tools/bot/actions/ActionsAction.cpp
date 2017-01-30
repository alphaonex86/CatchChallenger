#include "ActionsAction.h"
#include "../../general/base/CommonSettingsServer.h"
#include "../../general/base/CommonDatapack.h"

ActionsAction *ActionsAction::actionsAction=NULL;

ActionsAction::ActionsAction()
{
    connect(&moveTimer,&QTimer::timeout,this,&ActionsAction::doMove);
    connect(&textTimer,&QTimer::timeout,this,&ActionsAction::doText);
    textTimer.start(1000);
    flat_map_list=NULL;
    loaded=0;
    ActionsAction::actionsAction=this;
}

ActionsAction::~ActionsAction()
{
}

void ActionsAction::insert_player(CatchChallenger::Api_protocol *api,const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction)
{
    Q_UNUSED(player);
    Q_UNUSED(mapId);
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(direction);

    const CatchChallenger::Player_private_and_public_informations &player_private_and_public_informations=api->get_player_informations();
    Player &botplayer=clientList[api];
    botplayer.fightEngine->addPlayerMonster(player_private_and_public_informations.playerMonster);
    ActionsBotInterface::insert_player(api,player,mapId,x,y,direction);
    connect(api,&CatchChallenger::Api_protocol::new_chat_text,      actionsAction,&ActionsAction::new_chat_text,Qt::QueuedConnection);
    connect(api,&CatchChallenger::Api_protocol::seed_planted,   actionsAction,&ActionsAction::seed_planted_slot);
    connect(api,&CatchChallenger::Api_protocol::plant_collected,   actionsAction,&ActionsAction::plant_collected_slot);

    if(!moveTimer.isActive())
        moveTimer.start(player_private_and_public_informations.public_informations.speed);
}

bool ActionsAction::canGoTo(CatchChallenger::Api_protocol *api,const CatchChallenger::Direction &direction,const MapServerMini &map,COORD_TYPE x,COORD_TYPE y,const bool &checkCollision, const bool &allowTeleport)
{
    switch(direction)
    {
        case CatchChallenger::Direction_move_at_left:
        case CatchChallenger::Direction_move_at_right:
        case CatchChallenger::Direction_move_at_top:
        case CatchChallenger::Direction_move_at_bottom:
        break;
        default:
            abort();//wrong direction send
    }

    CatchChallenger::Player_private_and_public_informations &player=api->get_player_informations();
    Player &botplayer=clientList[api];
    CatchChallenger::ParsedLayerLedges ledge;
    const std::pair<uint8_t,uint8_t> point(x,y);
    if(map.pointOnMap_Item.find(point)!=map.pointOnMap_Item.cend())
    {
        const MapServerMini::ItemOnMap &item=map.pointOnMap_Item.at(point);
        if(item.visible)
            if(item.infinite || player.itemOnMap.find(item.indexOfItemOnMap)==player.itemOnMap.cend())
                return false;
    }
    ledge=CatchChallenger::MoveOnTheMap::getLedge(map,x,y);
    if(ledge!=CatchChallenger::ParsedLayerLedges_NoLedges)
        switch(direction)
        {
            case CatchChallenger::Direction_move_at_bottom:
            if(ledge!=CatchChallenger::ParsedLayerLedges_LedgesBottom)
                return false;
            break;
            case CatchChallenger::Direction_move_at_top:
            if(ledge!=CatchChallenger::ParsedLayerLedges_LedgesTop)
                return false;
            break;
            case CatchChallenger::Direction_move_at_left:
            if(ledge!=CatchChallenger::ParsedLayerLedges_LedgesLeft)
                return false;
            break;
            case CatchChallenger::Direction_move_at_right:
            if(ledge!=CatchChallenger::ParsedLayerLedges_LedgesRight)
                return false;
            break;
            default:
            break;
        }
    /*if(CatchChallenger::ClientFightEngine::fightEngine.isInFight())
    {
        qDebug() << "Strange, try move when is in fight";
        return false;
    }*/
    const MapServerMini *new_map=&map;
    switch(direction)
    {
        case CatchChallenger::Direction_move_at_left:
            if(x>0)
                x-=1;
            else
            {
                if(map.border.left.map==NULL)
                    return false;
                x=map.border.left.map->width-1;
                y+=map.border.left.y_offset;
                new_map=static_cast<const MapServerMini *>(map.border.left.map);
            }
        break;
        case CatchChallenger::Direction_move_at_right:
            if(x<(map.width-1))
                x+=1;
            else
            {
                if(map.border.right.map==NULL)
                    return false;
                x=0;
                y+=map.border.right.y_offset;
                new_map=static_cast<const MapServerMini *>(map.border.right.map);
            }
        break;
        case CatchChallenger::Direction_move_at_top:
            if(y>0)
                y-=1;
            else
            {
                if(map.border.top.map==NULL)
                    return false;
                y=map.border.top.map->height-1;
                x+=map.border.top.x_offset;
                new_map=static_cast<const MapServerMini *>(map.border.top.map);
            }
        break;
        case CatchChallenger::Direction_move_at_bottom:
            if(y<(map.height-1))
                y+=1;
            else
            {
                if(map.border.bottom.map==NULL)
                    return false;
                y=0;
                x+=map.border.bottom.x_offset;
                new_map=static_cast<const MapServerMini *>(map.border.bottom.map);
            }
        break;
        default:
            return false;
    }
    {
        //bot colision
        if(checkCollision)
        {
            if(!isWalkable(*new_map,x,y))
                return false;
            if(isDirt(*new_map,x,y))
                return false;
        }
        if(!allowTeleport)
            if(needBeTeleported(*new_map,x,y))
                return false;
    }

    {
        int list_size=new_map->teleporter_list_size;
        int index=0;
        while(index<list_size)
        {
            const CatchChallenger::CommonMap::Teleporter &teleporter=new_map->teleporter[index];
            if(teleporter.source_x==x && teleporter.source_y==y)
            {
                switch(teleporter.condition.type)
                {
                    case CatchChallenger::MapConditionType_None:
                    case CatchChallenger::MapConditionType_Clan://not do for now
                    break;
                    case CatchChallenger::MapConditionType_FightBot:
                        /*if(!haveBeatBot(teleporter.condition.value))
                        {
                            if(!new_map->teleport_condition_texts.at(index).isEmpty())
                                emit teleportConditionNotRespected(new_map->teleport_condition_texts.at(index));
                            return false;
                        }*/
                    break;
                    case CatchChallenger::MapConditionType_Item:
                        /*if(items==NULL)
                            break;
                        if(items->find(teleporter.condition.value)==items->cend())
                        {
                            if(!new_map->teleport_condition_texts.at(index).isEmpty())
                                emit teleportConditionNotRespected(new_map->teleport_condition_texts.at(index));
                            return false;
                        }*/
                    break;
                    case CatchChallenger::MapConditionType_Quest:
                        /*if(quests==NULL)
                            break;
                        if(quests->find(teleporter.condition.value)==quests->cend())
                        {
                            if(!new_map->teleport_condition_texts.at(index).isEmpty())
                                emit teleportConditionNotRespected(new_map->teleport_condition_texts.at(index));
                            return false;
                        }
                        if(!quests->at(teleporter.condition.value).finish_one_time)
                        {
                            if(!new_map->teleport_condition_texts.at(index).isEmpty())
                                emit teleportConditionNotRespected(new_map->teleport_condition_texts.at(index));
                            return false;
                        }*/
                    break;
                    default:
                    break;
                }
            }
            index++;
        }
    }

    {
        std::pair<uint8_t,uint8_t> pos(x,y);
        if(new_map->botsFightTrigger.find(pos)!=new_map->botsFightTrigger.cend())
        {
            std::vector<uint32_t> botFightList=new_map->botsFightTrigger.at(pos);
            unsigned int index=0;
            while(index<botFightList.size())
            {
                if(!haveBeatBot(api,botFightList.at(index)))
                {
                    if(!botplayer.fightEngine->getAbleToFight())
                    {
                        //emit blockedOn(MapVisualiserPlayer::BlockedOn_Fight);
                        return false;
                    }
                }
                index++;
            }
        }
    }
    const CatchChallenger::MonstersCollisionValue &monstersCollisionValue=CatchChallenger::MoveOnTheMap::getZoneCollision(*new_map,x,y);
    if(!monstersCollisionValue.walkOn.empty())
    {
        unsigned int index=0;
        while(index<monstersCollisionValue.walkOn.size())
        {
            const CatchChallenger::MonstersCollision &monstersCollision=CatchChallenger::CommonDatapack::commonDatapack.monstersCollision.at(monstersCollisionValue.walkOn.at(index));
            if(monstersCollision.item==0 || player.items.find(monstersCollision.item)!=player.items.cend())
            {
                if(!monstersCollisionValue.walkOnMonsters.at(index).defaultMonsters.empty())
                {
                    if(!botplayer.fightEngine->getAbleToFight())
                    {
                        //emit blockedOn(MapVisualiserPlayer::BlockedOn_ZoneFight);
                        return false;
                    }
                    if(!botplayer.fightEngine->canDoRandomFight(*new_map,x,y))
                    {
                        //emit blockedOn(MapVisualiserPlayer::BlockedOn_RandomNumber);
                        return false;
                    }
                }
                return true;
            }
            index++;
        }
        //emit blockedOn(MapVisualiserPlayer::BlockedOn_ZoneItem);
        return false;
    }
    return true;
}

bool ActionsAction::move(CatchChallenger::Api_protocol *api,CatchChallenger::Direction direction, const MapServerMini ** map, COORD_TYPE *x, COORD_TYPE *y, const bool &checkCollision, const bool &allowTeleport)
{
    switch(direction)
    {
        case CatchChallenger::Direction_move_at_left:
        case CatchChallenger::Direction_move_at_right:
        case CatchChallenger::Direction_move_at_top:
        case CatchChallenger::Direction_move_at_bottom:
        break;
        default:
            abort();//wrong direction send
    }
    if(!moveWithoutTeleport(api,direction,map,x,y,checkCollision,allowTeleport))
        return false;
    if(allowTeleport)
        teleport(map,x,y);
    return true;
}

bool ActionsAction::teleport(const MapServerMini **map, COORD_TYPE *x, COORD_TYPE *y)
{
    const CatchChallenger::CommonMap::Teleporter * const teleporter=(*map)->teleporter;
    const uint8_t &teleporter_list_size=(*map)->teleporter_list_size;
    uint8_t index=0;
    while(index<teleporter_list_size)
    {
        if(teleporter[index].source_x==*x && teleporter[index].source_y==*y)
        {
            *x=teleporter[index].destination_x;
            *y=teleporter[index].destination_y;
            *map=static_cast<const MapServerMini *>(teleporter[index].map);
            return true;
        }
        index++;
    }
    return false;
}

bool ActionsAction::moveWithoutTeleport(CatchChallenger::Api_protocol *api,CatchChallenger::Direction direction, const MapServerMini ** map, COORD_TYPE *x, COORD_TYPE *y, const bool &checkCollision, const bool &allowTeleport)
{
    switch(direction)
    {
        case CatchChallenger::Direction_move_at_left:
        case CatchChallenger::Direction_move_at_right:
        case CatchChallenger::Direction_move_at_top:
        case CatchChallenger::Direction_move_at_bottom:
        break;
        default:
            abort();//wrong direction send
    }
    if(*map==NULL)
        return false;
    if(!canGoTo(api,direction,**map,*x,*y,checkCollision,allowTeleport))
        return false;
    switch(direction)
    {
        case CatchChallenger::Direction_move_at_left:
            if(*x>0)
                *x-=1;
            else
            {
                *x=(*map)->border.left.map->width-1;
                *y+=(*map)->border.left.y_offset;
                *map=static_cast<const MapServerMini *>((*map)->border.left.map);
            }
            return true;
        break;
        case CatchChallenger::Direction_move_at_right:
            if(*x<((*map)->width-1))
                *x+=1;
            else
            {
                *x=0;
                *y+=(*map)->border.right.y_offset;
                *map=static_cast<const MapServerMini *>((*map)->border.right.map);
            }
            return true;
        break;
        case CatchChallenger::Direction_move_at_top:
            if(*y>0)
                *y-=1;
            else
            {
                *y=(*map)->border.top.map->height-1;
                *x+=(*map)->border.top.x_offset;
                *map=static_cast<const MapServerMini *>((*map)->border.top.map);
            }
            return true;
        break;
        case CatchChallenger::Direction_move_at_bottom:
            if(*y<((*map)->height-1))
                *y+=1;
            else
            {
                *y=0;
                *x+=(*map)->border.bottom.x_offset;
                *map=static_cast<const MapServerMini *>((*map)->border.bottom.map);
            }
            return true;
        break;
        default:
            return false;
    }
}

void ActionsAction::checkOnTileEvent(Player &player)
{
/*    if(event)
        player.target.localStep.clear();*/
}

void ActionsAction::doMove()
{
    QHashIterator<CatchChallenger::Api_protocol *,Player> i(clientList);
    while (i.hasNext()) {
        i.next();
        CatchChallenger::Api_protocol *api=i.key();
        Player &player=clientList[i.key()];
        if(id_map_to_map.find(player.mapId)==id_map_to_map.cend())
            abort();
        const std::string &playerMapStdString=actionsAction->id_map_to_map.at(player.mapId);
        const MapServerMini * playerMap=static_cast<const MapServerMini *>(actionsAction->map_list.at(playerMapStdString));
        //DebugClass::debugConsole(QStringLiteral("MainWindow::doStep(), do_step: %1, socket->isValid():%2, map!=NULL: %3").arg(do_step).arg(socket->isValid()).arg(map!=NULL));
        if(api->getCaracterSelected())
        {
            if(!player.target.localStep.empty())
            {
                std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> &step=player.target.localStep[0];
                if(step.second==0)
                    abort();
                step.second--;
                //need just continue to walk
                const CatchChallenger::Direction direction=(CatchChallenger::Direction)((uint8_t)step.first+4);
                if(player.direction==direction)
                {
                    if(canGoTo(api,direction,*playerMap,player.x,player.y,true,true))
                    {
                        api->newDirection(player.direction);
                        if(step.second==0)
                            player.target.localStep.erase(player.target.localStep.cbegin());
                        move(api,direction,&playerMap,&player.x,&player.y,true,true);
                        player.mapId=playerMap->id;
                        checkOnTileEvent(player);
                    }
                    else
                    {
                        //stop
                        player.direction=(CatchChallenger::Direction)((uint8_t)player.direction-4);
                        api->newDirection(player.direction);
                        player.target.localStep.clear();
                        player.target.bestPath.clear();
                    }
                }
                //need start to walk or direction change
                else
                {
                    if(canGoTo(api,direction,*playerMap,player.x,player.y,true,true))
                    {
                        player.direction=direction;
                        api->newDirection(player.direction);
                        if(step.second==0)
                            player.target.localStep.erase(player.target.localStep.cbegin());
                        move(api,direction,&playerMap,&player.x,&player.y,true,true);
                        player.mapId=playerMap->id;
                        checkOnTileEvent(player);
                    }
                    else
                    {
                        //turn on the new direction
                        const CatchChallenger::Direction &newDirection=(CatchChallenger::Direction)((uint8_t)direction-4);
                        if(newDirection!=player.direction)
                        {
                            player.direction=newDirection;
                            api->newDirection(player.direction);
                        }
                        player.target.localStep.clear();
                        player.target.bestPath.clear();
                    }
                }
            }
            else
            {
                //stop the player if is not stopped
                if(player.direction>4)
                {
                    player.direction=(CatchChallenger::Direction)((uint8_t)player.direction-4);
                    api->newDirection(player.direction);
                }
            }
        }
    }
}

void ActionsAction::doText()
{
    if(!randomText)
        return;
    if(clientList.isEmpty())
        return;

    QList<CatchChallenger::Api_protocol *> clientListApi;
    QHashIterator<CatchChallenger::Api_protocol *,Player> i(clientList);
    while (i.hasNext()) {
        i.next();
        clientListApi << i.key();
    }
    CatchChallenger::Api_protocol *api=clientListApi.at(rand()%clientListApi.size());
    //DebugClass::debugConsole(QStringLiteral("MainWindow::doStep(), do_step: %1, socket->isValid():%2, map!=NULL: %3").arg(do_step).arg(socket->isValid()).arg(map!=NULL));
    if(api->getCaracterSelected())
    {
        if(CommonSettingsServer::commonSettingsServer.chat_allow_local && rand()%10==0)
        {
            switch(rand()%3)
            {
                case 0:
                    api->sendChatText(CatchChallenger::Chat_type_local,"What's up?");
                break;
                case 1:
                    api->sendChatText(CatchChallenger::Chat_type_local,"Have good day!");
                break;
                case 2:
                    api->sendChatText(CatchChallenger::Chat_type_local,"... and now, what I have win :)");
                break;
            }
        }
        else
        {
            if(CommonSettingsServer::commonSettingsServer.chat_allow_all && rand()%100==0)
            {
                switch(rand()%4)
                {
                    case 0:
                        api->sendChatText(CatchChallenger::Chat_type_all,"Hello world! :)");
                    break;
                    case 1:
                        api->sendChatText(CatchChallenger::Chat_type_all,"It's so good game!");
                    break;
                    case 2:
                        api->sendChatText(CatchChallenger::Chat_type_all,"This game have reason to ask donations!");
                    break;
                    case 3:
                        api->sendChatText(CatchChallenger::Chat_type_all,"Donate if you can!");
                    break;
                }
            }
        }
    }
}

void ActionsAction::new_chat_text(const CatchChallenger::Chat_type &chat_type,const QString &text,const QString &pseudo,const CatchChallenger::Player_type &type)
{
    if(!globalChatRandomReply && chat_type!=CatchChallenger::Chat_type_pm)
        return;

    Q_UNUSED(text);
    Q_UNUSED(pseudo);
    Q_UNUSED(type);
    CatchChallenger::Api_protocol *api = qobject_cast<CatchChallenger::Api_protocol *>(sender());
    if(api==NULL)
        return;

    Q_UNUSED(type);
    switch(chat_type)
    {
        case CatchChallenger::Chat_type_all:
        if(CommonSettingsServer::commonSettingsServer.chat_allow_all)
            switch(rand()%(100*clientList.size()))
            {
                case 0:
                    api->sendChatText(CatchChallenger::Chat_type_local,"I'm according "+pseudo);
                break;
                default:
                break;
            }
        break;
        case CatchChallenger::Chat_type_local:
        if(CommonSettingsServer::commonSettingsServer.chat_allow_local)
            switch(rand()%(3*clientList.size()))
            {
                case 0:
                    api->sendChatText(CatchChallenger::Chat_type_local,"You are in right "+pseudo);
                break;
            }
        break;
        case CatchChallenger::Chat_type_pm:
        if(CommonSettingsServer::commonSettingsServer.chat_allow_private)
        {
            if(text==QStringLiteral("version"))
                api->sendPM(QStringLiteral("Version %1 %2").arg(name()).arg(version()),pseudo);
            else
                api->sendPM(QStringLiteral("Hello %1, I'm few bit busy for now").arg(pseudo),pseudo);
        }
        break;
        default:
        break;
    }
}

void ActionsAction::have_inventory_slot(const std::unordered_map<uint16_t,uint32_t> &items, const std::unordered_map<uint16_t, uint32_t> &warehouse_items)
{
    CatchChallenger::Api_protocol *api = qobject_cast<CatchChallenger::Api_protocol *>(sender());
    have_inventory(api,items,warehouse_items);
}

void ActionsAction::have_inventory(CatchChallenger::Api_protocol *api,const std::unordered_map<uint16_t,uint32_t> &items, const std::unordered_map<uint16_t, uint32_t> &warehouse_items)
{
    if(api==NULL)
        return;
    CatchChallenger::Player_private_and_public_informations &player=api->get_player_informations();

    player.items=items;
    player.warehouse_items=warehouse_items;
}

void ActionsAction::add_to_inventory(CatchChallenger::Api_protocol *api, const uint32_t &item, const uint32_t &quantity)
{
    QList<QPair<uint16_t,uint32_t> > items;
    items << QPair<uint16_t,uint32_t>(item,quantity);
    add_to_inventory(api,items);
}

void ActionsAction::add_to_inventory(CatchChallenger::Api_protocol *api,const QList<QPair<uint16_t,uint32_t> > &items)
{
    int index=0;
    QHash<uint16_t,uint32_t> tempHash;
    while(index<items.size())
    {
        tempHash[items.at(index).first]=items.at(index).second;
        index++;
    }
    add_to_inventory(api,tempHash);
}

void ActionsAction::add_to_inventory_slot(const QHash<uint16_t,uint32_t> &items)
{
    CatchChallenger::Api_protocol *api = qobject_cast<CatchChallenger::Api_protocol *>(sender());
    add_to_inventory(api,items);
}

void ActionsAction::add_to_inventory(CatchChallenger::Api_protocol *api,const QHash<uint16_t,uint32_t> &items)
{
    if(api==NULL)
        return;

    CatchChallenger::Player_private_and_public_informations &player=api->get_player_informations();
    if(items.empty())
        return;

    QHashIterator<uint16_t,uint32_t> i(items);
    while (i.hasNext()) {
        i.next();

        const uint16_t &item=i.key();
        if(player.encyclopedia_item!=NULL)
            player.encyclopedia_item[item/8]|=(1<<(7-item%8));
        else
            std::cerr << "encyclopedia_item is null, unable to set" << std::endl;
        //add really to the list
        if(player.items.find(item)!=player.items.cend())
            player.items[item]+=i.value();
        else
            player.items[item]=i.value();
    }
}

void ActionsAction::remove_to_inventory(CatchChallenger::Api_protocol *api,const uint32_t &itemId,const uint32_t &quantity)
{
    QHash<uint16_t,uint32_t> items;
    items[itemId]=quantity;
    remove_to_inventory(api,items);
}

void ActionsAction::remove_to_inventory_slot(const QHash<uint16_t,uint32_t> &items)
{
    CatchChallenger::Api_protocol *api = qobject_cast<CatchChallenger::Api_protocol *>(sender());
    remove_to_inventory(api,items);
}

void ActionsAction::remove_to_inventory(CatchChallenger::Api_protocol *api,const QHash<uint16_t,uint32_t> &items)
{
    if(api==NULL)
        return;
    CatchChallenger::Player_private_and_public_informations &player=api->get_player_informations();

    QHashIterator<uint16_t,uint32_t> i(items);
    while (i.hasNext()) {
        i.next();

        //add really to the list
        if(player.items.find(i.key())!=player.items.cend())
        {
            if(player.items.at(i.key())<=i.value())
                player.items.erase(i.key());
            else
                player.items[i.key()]-=i.value();
        }
    }
}

uint32_t ActionsAction::itemQuantity(CatchChallenger::Api_protocol *api,const uint32_t &itemId)
{
    const CatchChallenger::Player_private_and_public_informations &player=api->get_player_informations();
    if(player.items.find(itemId)!=player.items.cend())
        return player.items.at(itemId);
    return 0;
}

bool ActionsAction::isWalkable(const MapServerMini &map, const uint8_t &x, const uint8_t &y)
{
    if(map.parsed_layer.walkable==NULL)
        return false;
    return map.parsed_layer.walkable[x+y*(map.width)];
}

bool ActionsAction::isDirt(const MapServerMini &map, const uint8_t &x, const uint8_t &y)
{
    if(map.parsed_layer.dirt==NULL)
        return false;
    return map.parsed_layer.dirt[x+y*(map.width)];
}

bool ActionsAction::needBeTeleported(const MapServerMini &map, const COORD_TYPE &x, const COORD_TYPE &y)
{
    const CatchChallenger::CommonMap::Teleporter * const teleporter=map.teleporter;
    int8_t index=0;
    while(index<map.teleporter_list_size)
    {
        if(teleporter[index].source_x==x && teleporter[index].source_y==y)
            return true;
        index++;
    }
    return false;
}
