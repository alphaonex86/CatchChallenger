#include "ActionsAction.h"
#include "../../general/base/CommonSettingsServer.h"
#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/CommonDatapackServerSpec.h"
#include "../../general/base/FacilityLib.h"
#include "../../client/fight/interface/ClientFightEngine.h"

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
    if(player_private_and_public_informations.public_informations.simplifiedId==player.simplifiedId)
    {
        botplayer.fightEngine->addPlayerMonster(player_private_and_public_informations.playerMonster);
        ActionsBotInterface::insert_player(api,player,mapId,x,y,direction);
        connect(api,&CatchChallenger::Api_protocol::new_chat_text,      actionsAction,&ActionsAction::new_chat_text,Qt::QueuedConnection);
        connect(api,&CatchChallenger::Api_protocol::seed_planted,   actionsAction,&ActionsAction::seed_planted_slot);
        connect(api,&CatchChallenger::Api_protocol::plant_collected,   actionsAction,&ActionsAction::plant_collected_slot);

        if(!moveTimer.isActive())
            moveTimer.start(player_private_and_public_informations.public_informations.speed);
    }
}

void ActionsAction::insert_player_all(CatchChallenger::Api_protocol *api,const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction)
{
    (void)mapId;
    (void)x;
    (void)y;
    (void)direction;
    Player &botplayer=clientList[api];
    if(player.simplifiedId!=botplayer.api->get_player_informations().public_informations.simplifiedId)
    {
        botplayer.visiblePlayers[player.simplifiedId]=player;
        botplayer.viewedPlayers << QString::fromStdString(player.pseudo);
    }
}

void ActionsAction::newEvent(CatchChallenger::Api_protocol *api,const uint8_t &event,const uint8_t &event_value)
{
    forcedEvent(api,event,event_value);
}

void ActionsAction::forcedEvent(CatchChallenger::Api_protocol *api,const uint8_t &event,const uint8_t &event_value)
{
    Player &botplayer=clientList[api];
    botplayer.events[event]=event_value;
}

void ActionsAction::newRandomNumber_slot(const QByteArray &data)
{
    CatchChallenger::Api_protocol *api = qobject_cast<CatchChallenger::Api_protocol *>(sender());
    if(api==NULL)
        return;
    if(!clientList.contains(api))
        return;
    clientList[api].fightEngine->newRandomNumber(data);
}

void ActionsAction::setEvents_slot(const QList<QPair<uint8_t,uint8_t> > &events)
{
    CatchChallenger::Api_protocol *api = qobject_cast<CatchChallenger::Api_protocol *>(sender());
    if(api==NULL)
        return;
    setEvents(api,events);
}

void ActionsAction::newEvent_slot(const uint8_t &event,const uint8_t &event_value)
{
    CatchChallenger::Api_protocol *api = qobject_cast<CatchChallenger::Api_protocol *>(sender());
    if(api==NULL)
        return;
    newEvent(api,event,event_value);
}

void ActionsAction::setEvents(CatchChallenger::Api_protocol *api,const QList<QPair<uint8_t,uint8_t> > &events)
{
    Player &botplayer=clientList[api];
    botplayer.events.clear();
    unsigned int index=0;
    while(index<botplayer.events.size())
    {
        const QPair<uint8_t,uint8_t> event=events.at(index);
        if(event.first>=CatchChallenger::CommonDatapack::commonDatapack.events.size())
        {
            std::cerr << "ActionsAction::setEvents() event out of range" << std::endl;
            break;
        }
        if(event.second>=CatchChallenger::CommonDatapack::commonDatapack.events.at(event.first).values.size())
        {
            std::cerr << "ActionsAction::setEvents() event value out of range" << std::endl;
            break;
        }
        while(botplayer.events.size()<=event.first)
            botplayer.events.push_back(0);
        botplayer.events[event.first]=event.second;
        index++;
    }
    while((uint32_t)botplayer.events.size()<CatchChallenger::CommonDatapack::commonDatapack.events.size())
        botplayer.events.push_back(0);
    if((uint32_t)botplayer.events.size()>CatchChallenger::CommonDatapack::commonDatapack.events.size())
        while((uint32_t)botplayer.events.size()>CatchChallenger::CommonDatapack::commonDatapack.events.size())
            botplayer.events.pop_back();
    index=0;
    while(index<botplayer.events.size())
    {
        forcedEvent(api,index,botplayer.events.at(index));
        index++;
    }
}

void ActionsAction::dropAllPlayerOnTheMap(CatchChallenger::Api_protocol *api)
{
    Player &botplayer=clientList[api];
    botplayer.visiblePlayers.clear();
}

void ActionsAction::remove_player(CatchChallenger::Api_protocol *api, const uint16_t &id)
{
    Player &botplayer=clientList[api];
    botplayer.visiblePlayers.remove(id);
}

bool ActionsAction::mapConditionIsRepected(const CatchChallenger::Api_protocol *api,const CatchChallenger::MapCondition &condition)
{
    switch(condition.type)
    {
        case CatchChallenger::MapConditionType_None:
        return true;
        case CatchChallenger::MapConditionType_Clan://not do for now
        break;
        case CatchChallenger::MapConditionType_FightBot:
            if(!haveBeatBot(api,condition.value))
                return false;
        break;
        case CatchChallenger::MapConditionType_Item:
        {
            const CatchChallenger::Player_private_and_public_informations &playerInformations=api->get_player_informations_ro();
            if(playerInformations.items.find(condition.value)==playerInformations.items.cend())
                return false;
        }
        break;
        case CatchChallenger::MapConditionType_Quest:
        {
            const CatchChallenger::Player_private_and_public_informations &playerInformations=api->get_player_informations_ro();
             if(playerInformations.quests.find(condition.value)==playerInformations.quests.cend())
                return false;
            if(!playerInformations.quests.at(condition.value).finish_one_time)
                return false;
        }
        break;
        default:
        break;
    }
    return true;
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
            const std::pair<uint8_t,uint8_t> point(x,y);
            if(new_map->pointOnMap_Item.find(point)!=new_map->pointOnMap_Item.cend())
            {
                const MapServerMini::ItemOnMap &item=new_map->pointOnMap_Item.at(point);
                if(item.visible)
                    if(item.infinite || player.itemOnMap.find(item.indexOfItemOnMap)==player.itemOnMap.cend())
                        return false;
            }
            //into check colision because some time just need get the near tile
            if(!allowTeleport)
                if(needBeTeleported(*new_map,x,y))
                    return false;
        }
    }

    {
        int list_size=new_map->teleporter_list_size;
        int index=0;
        while(index<list_size)
        {
            const CatchChallenger::CommonMap::Teleporter &teleporter=new_map->teleporter[index];
            if(teleporter.source_x==x && teleporter.source_y==y)
            {
                if(!mapConditionIsRepected(api,teleporter.condition))
                    return false;
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
                        std::cerr << "!botplayer.fightEngine->canDoRandomFight(*new_map,x,y)" << std::endl;
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

bool ActionsAction::checkOnTileEvent(Player &player)
{
    std::pair<uint8_t,uint8_t> pos(player.x,player.y);
    const std::string &playerMapStdString=actionsAction->id_map_to_map.at(player.mapId);
    const MapServerMini * playerMap=static_cast<const MapServerMini *>(actionsAction->map_list.at(playerMapStdString));
    if(playerMap->botsFightTrigger.find(pos)!=playerMap->botsFightTrigger.cend())
    {
        std::vector<uint32_t> botFightList=playerMap->botsFightTrigger.at(pos);
        unsigned int index=0;
        while(index<botFightList.size())
        {
            const uint32_t &fightId=botFightList.at(index);
            if(!haveBeatBot(player.api,fightId))
            {
                qDebug() <<  "is now in fight with: " << fightId;
                player.canMoveOnMap=false;
                player.api->stopMove();
                QList<CatchChallenger::PlayerMonster> botFightMonstersTransformed;
                const std::vector<CatchChallenger::BotFight::BotFightMonster> &monsters=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.botFights.at(fightId).monsters;
                unsigned int index=0;
                while(index<monsters.size())
                {
                    botFightMonstersTransformed << CatchChallenger::FacilityLib::botFightMonsterToPlayerMonster(monsters.at(index),CatchChallenger::ClientFightEngine::getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(monsters.at(index).id),monsters.at(index).level));
                    index++;
                }
                player.fightEngine->setBotMonster(botFightMonstersTransformed);
                player.lastFightAction.restart();
                return true;
            }
            index++;
        }
    }
    //check if is in fight collision, but only if is move
    if(player.repel_step<=0)
    {
        const CatchChallenger::Player_private_and_public_informations &playerInformationsRO=player.api->get_player_informations_ro();
        const std::string &playerMapStdString=actionsAction->id_map_to_map.at(player.mapId);
        const MapServerMini * playerMap=static_cast<const MapServerMini *>(actionsAction->map_list.at(playerMapStdString));
        if(player.fightEngine->generateWildFightIfCollision(playerMap,player.x,player.y,playerInformationsRO.items,player.events))
        {
            player.canMoveOnMap=false;
            player.api->stopMove();
            player.lastFightAction.restart();
            return true;
        }
    }
    else
        player.repel_step--;
    return false;
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
        if(api->getCaracterSelected() && player.canMoveOnMap)
        {
            CatchChallenger::Player_private_and_public_informations &player_private_and_public_informations=api->get_player_informations();
            //get item if in front of
            if(!player.target.localStep.empty())
            {
                std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> &step=player.target.localStep[0];
                if(step.second==0)
                    abort();
                step.second--;
                //need just continue to walk
                if(step.first<1 || step.first>4)
                    abort();
                const CatchChallenger::Direction direction=(CatchChallenger::Direction)((uint8_t)step.first+4);

                //get the item in front of to continue the progression
                {
                    const CatchChallenger::Direction &newDirection=direction;
                    const MapServerMini * destMap=playerMap;
                    uint8_t x=player.x,y=player.y;
                    if(ActionsAction::move(api,newDirection,&destMap,&x,&y,false,false))
                    {
                        //std::cout << "The next case is: " << std::to_string(x) << "," << std::to_string(y) << std::endl;
                        std::pair<uint8_t,uint8_t> p(x,y);
                        if(playerMap->pointOnMap_Item.find(p)!=playerMap->pointOnMap_Item.cend())
                        {
                            //std::cout << "The next case is: " << std::to_string(x) << "," << std::to_string(y) << ", have item on it" << std::endl;
                            const MapServerMini::ItemOnMap &itemOnMap=playerMap->pointOnMap_Item.at(p);
                            if(!itemOnMap.infinite && itemOnMap.visible)
                                if(player_private_and_public_informations.itemOnMap.find(itemOnMap.indexOfItemOnMap)==player_private_and_public_informations.itemOnMap.cend())
                                {
                                    std::cout << "The next case is: " << std::to_string(x) << "," << std::to_string(y) << ", take the item" << std::endl;
                                    player_private_and_public_informations.itemOnMap.insert(itemOnMap.indexOfItemOnMap);
                                    api->newDirection(CatchChallenger::MoveOnTheMap::directionToDirectionLook(newDirection));//move to look into the right next direction
                                    api->takeAnObjectOnMap();
                                    add_to_inventory(api,itemOnMap.item);
                                }
                        }
                    }
                    else
                        std::cerr << "The next case is: " << std::to_string(x) << "," << std::to_string(y) << " can't move in direction: " << std::to_string(newDirection) << " to get the item" << std::endl;
                }

                if(canGoTo(api,direction,*playerMap,player.x,player.y,true,true))
                {
                    api->newDirection(direction);
                    if(step.second==0)
                        player.target.localStep.erase(player.target.localStep.cbegin());
                    if(!move(api,direction,&playerMap,&player.x,&player.y,true,true))
                    {
                        std::cerr << "Blocked on: " << std::to_string(player.x) << "," << std::to_string(player.y) << ", can't move in the direction: " << std::to_string(direction) << std::endl;
                        abort();
                    }
                    player.mapId=playerMap->id;
                    checkOnTileEvent(player);
                }
                else
                {
                    std::cerr << "Blocked on: " << std::to_string(player.x) << "," << std::to_string(player.y) << ", can't move in the direction: " << std::to_string(direction) << std::endl;
                    if(player.target.bestPath.size()>1)
                    {
                        std::cerr << "Something is wrong  to go to the destination, path finding buggy? block not walkable?" << std::endl;
                        abort();
                    }
                    if(player.target.localStep.size()>1)
                    {
                        std::cerr << "Something is wrong  to go to the destination, path finding buggy? block not walkable?" << std::endl;
                        abort();
                    }
                    if(player.target.localStep.size()==1)
                        if(player.target.localStep.at(0).second>1)
                        {
                            std::cerr << "Something is wrong  to go to the destination, path finding buggy? block not walkable?" << std::endl;
                            abort();
                        }
                    //turn on the new direction
                    const CatchChallenger::Direction &newDirection=(CatchChallenger::Direction)((uint8_t)direction-4);
                    api->newDirection(newDirection);
                    player.target.localStep.clear();
                    player.target.bestPath.clear();
                }
            }
            else
            {
                //stop the player if is not stopped
                api->stopMove();
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

    /*Q_UNUSED(type);
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
    }*/
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

uint32_t ActionsAction::itemQuantity(const CatchChallenger::Api_protocol *api,const uint32_t &itemId)
{
    const CatchChallenger::Player_private_and_public_informations &player=api->get_player_informations_ro();
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

void ActionsAction::monsterCatch(const bool &success)
{
    CatchChallenger::Api_protocol *api = qobject_cast<CatchChallenger::Api_protocol *>(sender());
    if(api==NULL)
        return;
    if(!clientList.contains(api))
        return;
    Player &player=clientList[api];
    if(player.fightEngine->playerMonster_catchInProgress.isEmpty())
    {
        std::cerr << "Internal bug: cupture monster list is emtpy" << std::endl;
        return;
    }
    if(!success)
        player.fightEngine->generateOtherAttack();
    else
    {
        player.canMoveOnMap=true;
        if(player.fightEngine->getPlayerMonster().size()>=CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters)
        {
            CatchChallenger::Player_private_and_public_informations &playerInformations=player.api->get_player_informations();
            if(playerInformations.warehouse_playerMonster.size()>=CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters)
            {
                std::cerr << "You have already the maximum number of monster into you warehouse" << std::endl;
                abort();
            }
            playerInformations.warehouse_playerMonster.push_back(player.fightEngine->playerMonster_catchInProgress.first());
        }
        else
            player.fightEngine->addPlayerMonster(player.fightEngine->playerMonster_catchInProgress.first());
    }
    player.fightEngine->playerMonster_catchInProgress.removeFirst();
}
