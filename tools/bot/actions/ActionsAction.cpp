#include "ActionsAction.h"
#include "../../general/base/CommonSettingsServer.hpp"
#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"
#include "../../general/base/FacilityLib.hpp"
#include "../../general/fight/CommonFightEngine.hpp"
#include "../../client/libqtcatchchallenger/Api_client_real.hpp"
#include "../bot-actions/BotTargetList.h"
#include <iostream>

ActionsAction *ActionsAction::actionsAction=NULL;

ActionsAction::ActionsAction()
{
    if(!connect(&moveTimer,&QTimer::timeout,this,&ActionsAction::doMove))
        abort();
    if(!connect(&textTimer,&QTimer::timeout,this,&ActionsAction::doText))
        abort();
    textTimer.start(1000);
    flat_map_list=NULL;
    loaded=0;
    allMapIsLoaded=false;
    ActionsAction::actionsAction=this;
    minitemprice=0;
    maxitemprice=0;
    mStop=false;
}

ActionsAction::~ActionsAction()
{
    mStop=true;
}

void ActionsAction::stopAll()
{
    moveTimer.stop();
    textTimer.stop();
    mStop=true;
}

void ActionsAction::insert_player(CatchChallenger::Api_protocol_Qt  *api,const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction)
{
    if(mStop)
        return;
    Q_UNUSED(player);
    Q_UNUSED(mapId);
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(direction);

    //after allMapIsLoaded because is after allMapIsLoaded the api is loaded
    if(!allMapIsLoaded)
    {
        DelayedMapPlayerChange delayedMapPlayerChange;
        delayedMapPlayerChange.direction=direction;
        delayedMapPlayerChange.mapId=mapId;
        delayedMapPlayerChange.player=player;
        delayedMapPlayerChange.x=x;
        delayedMapPlayerChange.y=y;
        delayedMapPlayerChange.type=DelayedMapPlayerChangeType_Insert;
        delayedMessage[api].push_back(delayedMapPlayerChange);
        return;
    }

    const CatchChallenger::Player_private_and_public_informations &player_private_and_public_informations=api->get_player_informations();
    if(clientList.find(api)==clientList.cend())
    {
        std::cerr << "clientList.find(api)==clientList.cend()" << std::endl;
        abort();
    }
    Player &botplayer=clientList[api];
    if(botplayer.api==NULL)
    {
        std::cerr << "clientList.find(api)==NULL" << std::endl;
        abort();
    }
    //after allMapIsLoaded because is after allMapIsLoaded the api is loaded
    if(player_private_and_public_informations.public_informations.simplifiedId==player.simplifiedId)
    {
        botplayer.api->addPlayerMonster(player_private_and_public_informations.playerMonster);
        ActionsBotInterface::insert_player(api,player,mapId,x,y,direction);
        if(!connect(api,&CatchChallenger::Api_protocol_Qt::Qtnew_chat_text,      actionsAction,&ActionsAction::new_chat_text,Qt::QueuedConnection))
            abort();
        if(!connect(api,&CatchChallenger::Api_protocol_Qt::Qtseed_planted,       actionsAction,&ActionsAction::seed_planted_slot))
            abort();
        if(!connect(api,&CatchChallenger::Api_protocol_Qt::Qtplant_collected,    actionsAction,&ActionsAction::plant_collected_slot))
            abort();

        if(!moveTimer.isActive())
            moveTimer.start(player_private_and_public_informations.public_informations.speed);

        checkOnTileEvent(botplayer,false);
    }
}

void ActionsAction::insert_player_all(CatchChallenger::Api_protocol_Qt  *api,const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction)
{
    (void)mapId;
    (void)x;
    (void)y;
    (void)direction;

    if(!allMapIsLoaded)
    {
        DelayedMapPlayerChange delayedMapPlayerChange;
        delayedMapPlayerChange.direction=direction;
        delayedMapPlayerChange.mapId=mapId;
        delayedMapPlayerChange.player=player;
        delayedMapPlayerChange.x=x;
        delayedMapPlayerChange.y=y;
        delayedMapPlayerChange.type=DelayedMapPlayerChangeType_InsertAll;
        delayedMessage[api].push_back(delayedMapPlayerChange);
        return;
    }

    if(clientList.find(api)==clientList.cend())
    {
        std::cerr << "clientList.find(api)==clientList.cend()" << std::endl;
        abort();
    }
    Player &botplayer=clientList[api];
    if(botplayer.api==NULL)
    {
        std::cerr << "clientList.find(api)==NULL" << std::endl;
        abort();
    }
    if(player.simplifiedId!=botplayer.api->get_player_informations().public_informations.simplifiedId)
    {
        botplayer.visiblePlayers[player.simplifiedId]=player;
        botplayer.viewedPlayers.insert(QString::fromStdString(player.pseudo));
    }
}

void ActionsAction::newEvent(CatchChallenger::Api_protocol_Qt  *api,const uint8_t &event,const uint8_t &event_value)
{
    forcedEvent(api,event,event_value);
}

void ActionsAction::forcedEvent(CatchChallenger::Api_protocol_Qt  *api,const uint8_t &event,const uint8_t &event_value)
{
    if(clientList.find(api)==clientList.cend())
    {
        std::cerr << "clientList.find(api)==clientList.cend()" << std::endl;
        abort();
    }
    Player &botplayer=clientList[api];
    if(botplayer.api==NULL)
    {
        std::cerr << "clientList.find(api)==NULL" << std::endl;
        abort();
    }
    botplayer.events[event]=event_value;
}

void ActionsAction::newRandomNumber_slot(const std::string &data)
{
    CatchChallenger::Api_client_real *api = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(api==NULL)
        return;
    if(clientList.find(api)==clientList.cend())
    {
        std::cerr << "clientList.find(api)==clientList.cend()" << std::endl;
        abort();
    }
    Player &botplayer=clientList[api];
    if(botplayer.api==NULL)
    {
        std::cerr << "clientList.find(api)==NULL" << std::endl;
        abort();
    }
    botplayer.api->newRandomNumber(data);
}

void ActionsAction::setEvents_slot(const std::vector<std::pair<uint8_t,uint8_t> > &events)
{
    CatchChallenger::Api_client_real *api = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(api==NULL)
        return;
    setEvents(api,events);
}

void ActionsAction::newEvent_slot(const uint8_t &event,const uint8_t &event_value)
{
    CatchChallenger::Api_client_real *api = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(api==NULL)
        return;
    newEvent(api,event,event_value);
}

void ActionsAction::setEvents(CatchChallenger::Api_protocol_Qt  *api, const std::vector<std::pair<uint8_t, uint8_t> > &events)
{
    if(clientList.find(api)==clientList.cend())
    {
        std::cerr << "clientList.find(api)==clientList.cend()" << std::endl;
        abort();
    }
    Player &botplayer=clientList[api];
    if(botplayer.api==NULL)
    {
        std::cerr << "clientList.find(api)==NULL" << std::endl;
        abort();
    }
    botplayer.events.clear();
    unsigned int index=0;
    while(index<botplayer.events.size())
    {
        const std::pair<uint8_t,uint8_t> event=events.at(index);
        if(event.first>=CatchChallenger::CommonDatapack::commonDatapack.get_events().size())
        {
            std::cerr << "ActionsAction::setEvents() event out of range" << std::endl;
            break;
        }
        if(event.second>=CatchChallenger::CommonDatapack::commonDatapack.get_events().at(event.first).values.size())
        {
            std::cerr << "ActionsAction::setEvents() event value out of range" << std::endl;
            break;
        }
        while(botplayer.events.size()<=event.first)
            botplayer.events.push_back(0);
        botplayer.events[event.first]=event.second;
        index++;
    }
    while((uint32_t)botplayer.events.size()<CatchChallenger::CommonDatapack::commonDatapack.get_events().size())
        botplayer.events.push_back(0);
    if((uint32_t)botplayer.events.size()>CatchChallenger::CommonDatapack::commonDatapack.get_events().size())
        while((uint32_t)botplayer.events.size()>CatchChallenger::CommonDatapack::commonDatapack.get_events().size())
            botplayer.events.pop_back();
    index=0;
    while(index<botplayer.events.size())
    {
        forcedEvent(api,index,botplayer.events.at(index));
        index++;
    }
}

void ActionsAction::dropAllPlayerOnTheMap(CatchChallenger::Api_protocol_Qt  *api)
{
    if(clientList.find(api)==clientList.cend())
    {
        std::cerr << "clientList.find(api)==clientList.cend()" << std::endl;
        abort();
    }
    Player &botplayer=clientList[api];
    if(botplayer.api==NULL)
    {
        std::cerr << "clientList.find(api)==NULL" << std::endl;
        abort();
    }
    botplayer.visiblePlayers.clear();
    delayedMessage[api].clear();
}

void ActionsAction::remove_player(CatchChallenger::Api_protocol_Qt  *api, const uint16_t &id)
{
    if(clientList.find(api)==clientList.cend())
    {
        std::cerr << "clientList.find(api)==clientList.cend()" << std::endl;
        abort();
    }
    Player &botplayer=clientList[api];
    if(botplayer.api==NULL)
    {
        std::cerr << "clientList.find(api)==NULL" << std::endl;
        abort();
    }
    if(allMapIsLoaded)
        botplayer.visiblePlayers.erase(id);
    else
    {
        DelayedMapPlayerChange delayedMapPlayerChange;
        delayedMapPlayerChange.direction=CatchChallenger::Direction::Direction_look_at_bottom;
        delayedMapPlayerChange.mapId=0;
        delayedMapPlayerChange.player.skinId=0;
        delayedMapPlayerChange.player.simplifiedId=id;
        delayedMapPlayerChange.player.speed=0;
        delayedMapPlayerChange.player.type=CatchChallenger::Player_type_normal;
        delayedMapPlayerChange.x=0;
        delayedMapPlayerChange.y=0;
        delayedMapPlayerChange.type=DelayedMapPlayerChangeType_Delete;
        delayedMessage[api].push_back(delayedMapPlayerChange);
    }
}

bool ActionsAction::mapConditionIsRepected(const CatchChallenger::Api_protocol_Qt  *api,const CatchChallenger::MapCondition &condition)
{
    switch(condition.type)
    {
        case CatchChallenger::MapConditionType_None:
        return true;
        case CatchChallenger::MapConditionType_Clan://not do for now
        break;
        case CatchChallenger::MapConditionType_FightBot:
            if(!haveBeatBot(api,condition.data.fightBot))
                return false;
        break;
        case CatchChallenger::MapConditionType_Item:
        {
            const CatchChallenger::Player_private_and_public_informations &playerInformations=api->get_player_informations_ro();
            if(playerInformations.items.find(condition.data.item)==playerInformations.items.cend())
                return false;
        }
        break;
        case CatchChallenger::MapConditionType_Quest:
        {
            const CatchChallenger::Player_private_and_public_informations &playerInformations=api->get_player_informations_ro();
             if(playerInformations.quests.find(condition.data.quest)==playerInformations.quests.cend())
                return false;
            if(!playerInformations.quests.at(condition.data.quest).finish_one_time)
                return false;
        }
        break;
        default:
        break;
    }
    return true;
}

bool ActionsAction::canGoTo(CatchChallenger::Api_protocol_Qt  *api,const CatchChallenger::Direction &direction,const MapServerMini &map,COORD_TYPE x,COORD_TYPE y,const bool &checkCollision, const bool &allowTeleport,const bool &debug)
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
    if(clientList.find(api)==clientList.cend())
    {
        std::cerr << "clientList.find(api)==clientList.cend()" << std::endl;
        abort();
    }
    Player &botplayer=clientList[api];
    if(botplayer.api==NULL)
    {
        std::cerr << "clientList.find(api)==NULL" << std::endl;
        abort();
    }
    CatchChallenger::ParsedLayerLedges ledge;
    ledge=CatchChallenger::MoveOnTheMap::getLedge(map,x,y);
    if(ledge!=CatchChallenger::ParsedLayerLedges_NoLedges)
        switch(direction)
        {
            case CatchChallenger::Direction_move_at_bottom:
            if(ledge!=CatchChallenger::ParsedLayerLedges_LedgesBottom)
            {
                if(debug)
                    std::cerr << "!canGoTo(): " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
                return false;
            }
            break;
            case CatchChallenger::Direction_move_at_top:
            if(ledge!=CatchChallenger::ParsedLayerLedges_LedgesTop)
            {
                if(debug)
                    std::cerr << "!canGoTo(): " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
                return false;
            }
            break;
            case CatchChallenger::Direction_move_at_left:
            if(ledge!=CatchChallenger::ParsedLayerLedges_LedgesLeft)
            {
                if(debug)
                    std::cerr << "!canGoTo(): " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
                return false;
            }
            break;
            case CatchChallenger::Direction_move_at_right:
            if(ledge!=CatchChallenger::ParsedLayerLedges_LedgesRight)
            {
                if(debug)
                    std::cerr << "!canGoTo(): " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
                return false;
            }
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
                {
                    if(debug)
                        std::cerr << "!canGoTo(): " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
                    return false;
                }
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
                {
                    if(debug)
                        std::cerr << "!canGoTo(): " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
                    return false;
                }
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
                {
                    if(debug)
                        std::cerr << "!canGoTo(): " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
                    return false;
                }
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
                {
                    if(debug)
                        std::cerr << "!canGoTo(): " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
                    return false;
                }
                y=0;
                x+=map.border.bottom.x_offset;
                new_map=static_cast<const MapServerMini *>(map.border.bottom.map);
            }
        break;
        default:
        {
            if(debug)
                std::cerr << "!canGoTo(): " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
            return false;
        }
    }
    {
        //bot colision
        if(checkCollision)
        {
            if(!isWalkable(*new_map,x,y))
            {
                if(debug)
                    std::cerr << "!canGoTo(): " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
                return false;
            }
            if(isDirt(*new_map,x,y))
            {
                if(debug)
                    std::cerr << "!canGoTo(): " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
                return false;
            }
            const std::pair<uint8_t,uint8_t> point(x,y);
            if(new_map->pointOnMap_Item.find(point)!=new_map->pointOnMap_Item.cend())
            {
                const MapServerMini::ItemOnMap &item=new_map->pointOnMap_Item.at(point);
                if(item.visible)
                    if(item.infinite || player.itemOnMap.find(item.indexOfItemOnMap)==player.itemOnMap.cend())
                    {
                        if(debug)
                            std::cerr << "!canGoTo(): " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
                        return false;
                    }
            }
            //into check colision because some time just need get the near tile
            if(!allowTeleport)
                if(needBeTeleported(*new_map,x,y))
                {
                    if(debug)
                        std::cerr << "!canGoTo(): " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
                    return false;
                }
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
                {
                    if(debug)
                        std::cerr << "!canGoTo(): " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
                    return false;
                }
            }
            index++;
        }
    }

    {
        std::pair<uint8_t,uint8_t> pos(x,y);
        if(new_map->botsFightTrigger.find(pos)!=new_map->botsFightTrigger.cend())
        {
            std::vector<uint16_t> botFightList=new_map->botsFightTrigger.at(pos);
            unsigned int index=0;
            while(index<botFightList.size())
            {
                if(!haveBeatBot(api,botFightList.at(index)))
                {
                    if(!botplayer.api->getAbleToFight())
                    {
                        if(debug)
                            std::cerr << "!canGoTo(): " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
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
            const CatchChallenger::MonstersCollision &monstersCollision=CatchChallenger::CommonDatapack::commonDatapack.get_monstersCollision().at(monstersCollisionValue.walkOn.at(index));
            if(monstersCollision.item==0 || player.items.find(monstersCollision.item)!=player.items.cend())
            {
                if(!monstersCollisionValue.walkOnMonsters.at(index).defaultMonsters.empty())
                {
                    if(!botplayer.api->getAbleToFight())
                    {
                        if(debug)
                            std::cerr << "!canGoTo(): " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
                        //emit blockedOn(MapVisualiserPlayer::BlockedOn_ZoneFight);
                        return false;
                    }
                    if(!botplayer.api->canDoRandomFight(*new_map,x,y))
                    {
                        std::cerr << "!botplayer.api->canDoRandomFight(*new_map,x,y)" << std::endl;
                        abort();
                        //emit blockedOn(MapVisualiserPlayer::BlockedOn_RandomNumber);
                        return false;
                    }
                }
                return true;
            }
            index++;
        }
        //emit blockedOn(MapVisualiserPlayer::BlockedOn_ZoneItem);
        if(debug)
            std::cerr << "!canGoTo(): " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
        return false;
    }
    return true;
}

bool ActionsAction::move(CatchChallenger::Api_protocol_Qt  *api,CatchChallenger::Direction direction, const MapServerMini ** map, COORD_TYPE *x, COORD_TYPE *y, const bool &checkCollision, const bool &allowTeleport)
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

bool ActionsAction::moveWithoutTeleport(CatchChallenger::Api_protocol_Qt  *api,CatchChallenger::Direction direction, const MapServerMini ** map, COORD_TYPE *x, COORD_TYPE *y, const bool &checkCollision, const bool &allowTeleport)
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

bool ActionsAction::checkOnTileEvent(Player &player, bool haveDoStep)
{
    if(actionsAction->id_map_to_map.empty())
    {
        std::cerr << "ActionsAction::checkOnTileEvent() need to be call after all the map is loaded" << std::endl;
        abort();
    }
    std::pair<uint8_t,uint8_t> pos(player.x,player.y);
    const std::string &playerMapStdString=actionsAction->id_map_to_map.at(player.mapId);
    const MapServerMini * playerMap=static_cast<const MapServerMini *>(actionsAction->map_list.at(playerMapStdString));
    if(playerMap->botsFightTrigger.find(pos)!=playerMap->botsFightTrigger.cend())
    {
        std::vector<uint16_t> botFightList=playerMap->botsFightTrigger.at(pos);
        unsigned int index=0;
        while(index<botFightList.size())
        {
            const uint32_t &fightId=botFightList.at(index);
            if(!haveBeatBot(player.api,fightId))
            {
                qDebug() <<  "is now in fight with: " << fightId;
                player.canMoveOnMap=false;
                player.api->stopMove();
                std::vector<CatchChallenger::PlayerMonster> botFightMonstersTransformed;
                const std::vector<CatchChallenger::BotFight::BotFightMonster> &monsters=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_botFights().at(fightId).monsters;
                unsigned int index=0;
                while(index<monsters.size())
                {
                    botFightMonstersTransformed.push_back(
                                CatchChallenger::FacilityLib::botFightMonsterToPlayerMonster(
                                    monsters.at(index),CatchChallenger::CommonFightEngine::getStat(
                                        CatchChallenger::CommonDatapack::commonDatapack.get_monsters().at(monsters.at(index).id),
                                        monsters.at(index).level)
                                    )
                                );
                    index++;
                }
                player.api->setBotMonster(botFightMonstersTransformed,fightId);
                player.lastFightAction.restart();
                return true;
            }
            index++;
        }
    }
    //check if is in fight collision, but only if is move
    if(haveDoStep)
    {
        if(player.repel_step<=0)
        {
            const CatchChallenger::Player_private_and_public_informations &playerInformationsRO=player.api->get_player_informations_ro();

            if(player.events.empty())
                std::cerr << playerInformationsRO.public_informations.pseudo << ": player.events.empty()" << std::endl;

            const std::string &playerMapStdString=actionsAction->id_map_to_map.at(player.mapId);
            const MapServerMini * playerMap=static_cast<const MapServerMini *>(actionsAction->map_list.at(playerMapStdString));
            if(player.api->generateWildFightIfCollision(playerMap,player.x,player.y,playerInformationsRO.items,player.events))
            {
                player.canMoveOnMap=false;
                player.api->stopMove();
                player.lastFightAction.restart();
                return true;
            }
        }
        else
            player.repel_step--;
    }
    return false;
}

void ActionsAction::doMove()
{
    if(mStop)
        return;
    for (const auto &n:clientList) {
        CatchChallenger::Api_protocol_Qt  *api=n.first;
        Player &player=clientList[api];
        if(id_map_to_map.find(player.mapId)==id_map_to_map.cend())
            abort();
        const std::string &playerMapStdString=actionsAction->id_map_to_map.at(player.mapId);
        const MapServerMini * playerMap=static_cast<const MapServerMini *>(actionsAction->map_list.at(playerMapStdString));
        //DebugClass::debugConsole(QStringLiteral("MainWindow::doStep(), do_step: %1, socket->isValid():%2, map!=NULL: %3").arg(do_step).arg(socket->isValid()).arg(map!=NULL));
        if(api->getCaracterSelected() && player.canMoveOnMap && !player.api->isInFight())
        {
            CatchChallenger::Player_private_and_public_informations &player_private_and_public_informations=api->get_player_informations();
            //get item if in front of
            if(!player.target.localStep.empty())
            {
                std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> &step=player.target.localStep[0];
                if(step.second!=0 || step.first!=CatchChallenger::Orientation::Orientation_none)
                {
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
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            {
                                std::unordered_set<uint32_t> known_indexOfItemOnMap;
                                for ( const auto &item : playerMap->pointOnMap_Item )
                                {
                                    const MapServerMini::ItemOnMap &itemOnMap=item.second;
                                    if(known_indexOfItemOnMap.find(itemOnMap.indexOfItemOnMap)!=known_indexOfItemOnMap.cend())
                                        abort();
                                    known_indexOfItemOnMap.insert(itemOnMap.indexOfItemOnMap);
                                }
                            }
                            #endif
                            std::pair<uint8_t,uint8_t> p(x,y);
                            if(playerMap->pointOnMap_Item.find(p)!=playerMap->pointOnMap_Item.cend())
                            {
                                //std::cout << "The next case is: " << std::to_string(x) << "," << std::to_string(y) << ", have item on it" << std::endl;
                                const MapServerMini::ItemOnMap &itemOnMap=playerMap->pointOnMap_Item.at(p);
                                if(!itemOnMap.infinite && itemOnMap.visible)
                                    if(player_private_and_public_informations.itemOnMap.find(itemOnMap.indexOfItemOnMap)==player_private_and_public_informations.itemOnMap.cend())
                                    {
                                        /*std::cout << "The next case is: " << std::to_string(x) << "," << std::to_string(y)
                                                  << ", take the item, action, itemOnMap.indexOfItemOnMap: " << std::to_string(itemOnMap.indexOfItemOnMap)
                                                  << ", item: " << std::to_string(itemOnMap.item)
                                                  << ", pseudo: " << api->getPseudo().toStdString() << std::endl;*/
                                        player_private_and_public_informations.itemOnMap.insert(itemOnMap.indexOfItemOnMap);
                                        api->newDirection(CatchChallenger::MoveOnTheMap::directionToDirectionLook(newDirection));//move to look into the right next direction
                                        api->takeAnObjectOnMap();
                                        add_to_inventory(api,itemOnMap.item);
                                    }
                            }
                        }
                        /*else
                            std::cerr << "The next case is: " << std::to_string(x) << "," << std::to_string(y) << " can't move in direction: " << std::to_string(newDirection) << " to get the item" << std::endl;*/
                    }

                    if(canGoTo(api,direction,*playerMap,player.x,player.y,true,true))
                    {
                        if(player.target.bestPath.empty() && step.second==0 && player.target.localStep.size()==1 && player.target.type==ActionsBotInterface::GlobalTarget::GlobalTargetType::ItemOnMap)
                            api->newDirection((CatchChallenger::Direction)(direction-4));
                        else
                        {
                            api->newDirection(direction);
                            if(!move(api,direction,&playerMap,&player.x,&player.y,true,true))
                            {
                                std::cerr << "Blocked on: " << std::to_string(player.x) << "," << std::to_string(player.y) << ", can't move in the direction: " << std::to_string(direction) << std::endl;
                                abort();
                            }
                            player.mapId=playerMap->id;
                            checkOnTileEvent(player);
                        }
                        if(step.second==0)
                            player.target.localStep.erase(player.target.localStep.cbegin());
                    }
                    else
                    {
                        std::cerr << "Blocked on: " << playerMap->map_file << " " << std::to_string(player.x) << "," << std::to_string(player.y) << ", can't move in the direction: " << std::to_string(direction) << " for " << api->getPseudo() << std::endl;
                        abort();
                        if(player.target.bestPath.size()>1)
                        {
                            std::cerr << "Something is wrong to go to the destination, path finding buggy? block not walkable?" << std::endl;
                            std::cerr << "player.api->getAbleToFight(): " << player.api->getAbleToFight() << std::endl;
                            std::cerr << "player.api->canDoRandomFight(*new_map,x,y): " << player.api->canDoRandomFight(*playerMap,player.x,player.y) << std::endl;
                            canGoTo(api,direction,*playerMap,player.x,player.y,true,true,true);
                            abort();
                        }
                        if(player.target.localStep.size()>1)
                        {
                            std::cerr << "Something is wrong  to go to the destination, path finding buggy? block not walkable?" << std::endl;
                            std::cerr << "player.api->getAbleToFight(): " << player.api->getAbleToFight() << std::endl;
                            std::cerr << "player.api->canDoRandomFight(*new_map,x,y): " << player.api->canDoRandomFight(*playerMap,player.x,player.y) << std::endl;
                            canGoTo(api,direction,*playerMap,player.x,player.y,true,true,true);
                            abort();
                        }
                        if(player.target.localStep.size()==1)
                            if(player.target.localStep.at(0).second>1)
                            {
                                std::cerr << "Something is wrong  to go to the destination, path finding buggy? block not walkable?" << std::endl;
                                std::cerr << "player.api->getAbleToFight(): " << player.api->getAbleToFight() << std::endl;
                                std::cerr << "player.api->canDoRandomFight(*new_map,x,y): " << player.api->canDoRandomFight(*playerMap,player.x,player.y) << std::endl;
                                canGoTo(api,direction,*playerMap,player.x,player.y,true,true,true);
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
                    player.target.localStep.erase(player.target.localStep.cbegin());

                /*const std::string &debugMapString=actionsAction->id_map_to_map.at(player.mapId);
                    std::cout << player.api->getPseudo() << ": localStep: " << BotTargetList::stepToString(player.target.localStep)
                              << " from " << debugMapString << " " << std::to_string(player.x) << "," << std::to_string(player.y)
                              << ", " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;*/
            }
            else
            {
                /*if(CatchChallenger::MoveOnTheMap::getLedge(*playerMap,player.x,player.y)==CatchChallenger::ParsedLayerLedges_NoLedges)
                //stop the player if is not stopped, for ledge better into BotTargetList::updatePlayerStep()
                    api->stopMove();*/
            }
        }
    }
}

void ActionsAction::doText()
{
    if(mStop)
        return;
    if(!randomText)
        return;
    if(clientList.empty())
        return;

    QList<CatchChallenger::Api_protocol_Qt  *> clientListApi;

    for (const auto &n:clientList) {
        clientListApi << n.first;
    }
    CatchChallenger::Api_protocol_Qt  *api=clientListApi.at(rand()%clientListApi.size());
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

void ActionsAction::new_chat_text(const CatchChallenger::Chat_type &chat_type,const std::string &text,
                                  const std::string &pseudo,const CatchChallenger::Player_type &type)
{
    if(!globalChatRandomReply && chat_type!=CatchChallenger::Chat_type_pm)
        return;

    Q_UNUSED(text);
    Q_UNUSED(pseudo);
    Q_UNUSED(type);
    CatchChallenger::Api_client_real *api = qobject_cast<CatchChallenger::Api_client_real *>(sender());
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
    CatchChallenger::Api_client_real *api = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    have_inventory(api,items,warehouse_items);
}

void ActionsAction::have_inventory(CatchChallenger::Api_protocol_Qt  *api,const std::unordered_map<uint16_t,uint32_t> &items, const std::unordered_map<uint16_t, uint32_t> &warehouse_items)
{
    if(api==NULL)
        return;
    CatchChallenger::Player_private_and_public_informations &player=api->get_player_informations();

    player.items=items;
    player.warehouse_items=warehouse_items;
}

void ActionsAction::add_to_inventory(CatchChallenger::Api_protocol_Qt  *api, const uint32_t &item, const uint32_t &quantity)
{
    std::vector<std::pair<uint16_t,uint32_t> > items;
    items.push_back(std::pair<uint16_t,uint32_t>(item,quantity));
    add_to_inventory(api,items);
}

void ActionsAction::add_to_inventory(CatchChallenger::Api_protocol_Qt  *api,const std::vector<std::pair<uint16_t,uint32_t> > &items)
{
    unsigned int index=0;
    std::unordered_map<uint16_t,uint32_t> tempHash;
    while(index<items.size())
    {
        tempHash[items.at(index).first]=items.at(index).second;
        index++;
    }
    add_to_inventory(api,tempHash);
}

void ActionsAction::add_to_inventory_slot(const std::unordered_map<uint16_t,uint32_t> &items)
{
    CatchChallenger::Api_client_real *api = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    add_to_inventory(api,items);
}

void ActionsAction::add_to_inventory(CatchChallenger::Api_protocol_Qt  *api,const std::unordered_map<uint16_t,uint32_t> &items)
{
    if(api==NULL)
        return;

    CatchChallenger::Player_private_and_public_informations &player=api->get_player_informations();
    if(items.empty())
        return;

    for( const auto& n : items ) {
        const uint16_t &item=n.first;
        if(player.encyclopedia_item!=NULL)
            player.encyclopedia_item[item/8]|=(1<<(7-item%8));
        else
            std::cerr << "encyclopedia_item is null, unable to set" << std::endl;
        //add really to the list
        if(player.items.find(item)!=player.items.cend())
            player.items[item]+=n.second;
        else
            player.items[item]=n.second;
    }
}

void ActionsAction::remove_to_inventory(CatchChallenger::Api_protocol_Qt  *api,const uint32_t &itemId,const uint32_t &quantity)
{
    std::unordered_map<uint16_t,uint32_t> items;
    items[itemId]=quantity;
    remove_to_inventory(api,items);
}

void ActionsAction::remove_to_inventory_slot(const std::unordered_map<uint16_t,uint32_t> &items)
{
    CatchChallenger::Api_client_real *api = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    remove_to_inventory(api,items);
}

void ActionsAction::remove_to_inventory(CatchChallenger::Api_protocol_Qt  *api,const std::unordered_map<uint16_t,uint32_t> &items)
{
    if(api==NULL)
        return;
    CatchChallenger::Player_private_and_public_informations &player=api->get_player_informations();

    for( const auto& n : items ) {
        //add really to the list
        if(player.items.find(n.first)!=player.items.cend())
        {
            if(player.items.at(n.first)<=n.second)
                player.items.erase(n.first);
            else
                player.items[n.first]-=n.second;
        }
    }
}

uint32_t ActionsAction::itemQuantity(const CatchChallenger::Api_protocol_Qt  *api, const uint16_t &itemId)
{
    const CatchChallenger::Player_private_and_public_informations &player=api->get_player_informations_ro();
    if(player.items.find(itemId)!=player.items.cend())
        return player.items.at(itemId);
    return 0;
}

bool ActionsAction::isWalkable(const MapServerMini &map, const uint8_t &x, const uint8_t &y)
{
    if(map.parsed_layer.simplifiedMap==NULL)
        return false;
    return map.parsed_layer.simplifiedMap[x+y*(map.width)]<200;
}

bool ActionsAction::isDirt(const MapServerMini &map, const uint8_t &x, const uint8_t &y)
{
    if(map.parsed_layer.simplifiedMap==NULL)
        return false;
    return map.parsed_layer.simplifiedMap[x+y*(map.width)]==249;
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
    std::cout << "ActionsAction::monsterCatch(" << std::to_string(success) << ")" << std::endl;
    CatchChallenger::Api_client_real *api = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(api==NULL)
        return;
    if(clientList.find(api)==clientList.cend())
    {
        std::cerr << "clientList.find(api)==clientList.cend()" << std::endl;
        abort();
    }
    Player &player=clientList[api];
    if(player.api==NULL)
    {
        std::cerr << "clientList.find(api)==NULL" << std::endl;
        abort();
    }
    if(player.api->playerMonster_catchInProgress.empty())
    {
        std::cerr << "Internal bug: cupture monster list is emtpy" << std::endl;
        return;
    }
    if(!success)
    {
        const CatchChallenger::Skill::AttackReturn &attackReturn=player.api->generateOtherAttack();
        std::cout << "Start this: Try capture failed, do monster attack: " << std::to_string(attackReturn.attack) << std::endl;
    }
    else
    {
        std::cout << "Start this: Try capture success" << std::endl;
        player.canMoveOnMap=true;
        if(player.api->getPlayerMonster().size()>=CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters)
        {
            CatchChallenger::Player_private_and_public_informations &playerInformations=player.api->get_player_informations();
            if(playerInformations.warehouse_playerMonster.size()>=CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters)
            {
                std::cerr << "You have already the maximum number of monster into you warehouse" << std::endl;
                abort();
            }
            playerInformations.warehouse_playerMonster.push_back(player.api->playerMonster_catchInProgress.front());
        }
        else
            player.api->addPlayerMonster(player.api->playerMonster_catchInProgress.front());
    }
    player.api->playerMonster_catchInProgress.erase(player.api->playerMonster_catchInProgress.cbegin());
    player.api->fightFinished();
}

void ActionsAction::teleportTo(const uint32_t &mapId,const uint16_t &x,const uint16_t &y,const CatchChallenger::Direction &direction)
{
    Q_UNUSED(direction);
    //std::cout << "ActionsAction::teleportTo()" << std::endl;
    CatchChallenger::Api_client_real *api = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(api==NULL)
        return;
    if(clientList.find(api)==clientList.cend())
        return;
    Player &player=clientList[api];

    player.mapId=mapId;
    player.x=x;
    player.y=y;
    api->teleportDone();
    ActionsAction::resetTarget(player.target);
    if(player.api->currentMonsterIsKO() && !player.api->haveAnotherMonsterOnThePlayerToFight())//then is dead, is teleported to the last rescue point
    {
        std::cout << "tp after loose" << std::endl;
        player.canMoveOnMap=true;
        player.api->healAllMonsters();
        player.api->fightFinished();

        CatchChallenger::PlayerMonster *monster=player.api->evolutionByLevelUp();
        if(monster!=NULL)
        {
            const CatchChallenger::Monster &monsterInformations=CatchChallenger::CommonDatapack::commonDatapack.get_monsters().at(monster->monster);
            unsigned int index=0;
            while(index<monsterInformations.evolutions.size())
            {
                const CatchChallenger::Monster::Evolution &evolution=monsterInformations.evolutions.at(index);
                if(evolution.type==CatchChallenger::Monster::EvolutionType_Level && evolution.data.level==monster->level)
                {
                    const uint8_t &monsterEvolutionPostion=player.api->getPlayerMonsterPosition(monster);
                    player.api->confirmEvolutionByPosition(monsterEvolutionPostion);//api call into it
                    return;
                }
                index++;
            }
        }
    }
    else
        std::cout << "normal tp" << std::endl;
}

void ActionsAction::resetTarget(GlobalTarget &target)
{
    //reset the target
    /*target.wildForwardStep.clear();
    target.wildBackwardStep.clear();*/
    target.wildCycle=0;
    target.blockObject=NULL;
    target.extra=0;
    target.linkPoint.x=0;
    target.linkPoint.y=0;
    target.linkPoint.type=MapServerMini::BlockObject::LinkType::SourceNone;
    target.type=ActionsBotInterface::GlobalTarget::GlobalTargetType::None;
    target.sinceTheLastAction.restart();
}
