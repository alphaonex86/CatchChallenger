#include "Client.hpp"

#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/FacilityLib.hpp"
#include "PreparedDBQuery.hpp"
#include "GlobalServerData.hpp"
#include "MapServer.hpp"

using namespace CatchChallenger;

bool Client::learnSkillByMonsterPosition(const uint8_t &monsterPosition, const uint16_t &skill)
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("learnSkill("+std::to_string(monsterId)+","+std::to_string(skill)+")");
    #endif
    if(monsterPosition>=public_and_private_informations.playerMonster.size())
    {
        errorOutput("The monster is not found: "+std::to_string(monsterPosition));
        return false;
    }
    CommonMap *map=this->map;
    uint8_t x=this->x;
    uint8_t y=this->y;
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
                    errorOutput("learnSkill() Can't move at top from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
                    return false;
                }
            }
            else
            {
                errorOutput("No valid map in this direction");
                return false;
            }
        break;
        default:
        errorOutput("Wrong direction to use a learn skill");
        return false;
    }
    const MapServer * const mapServer=static_cast<MapServer*>(map);
    const std::pair<uint8_t,uint8_t> pos(x,y);
    if(mapServer->learn.find(pos)==mapServer->learn.end())
    {
        switch(direction)
        {
            case Direction_look_at_top:
            case Direction_look_at_right:
            case Direction_look_at_bottom:
            case Direction_look_at_left:
            direction=lookToMove(direction);
            break;
            default:
            break;
        }
        switch(direction)
        {
            /// \warning: Not loop but move here due to first transform set: direction=lookToMove(direction);
            case Direction_look_at_top:
            case Direction_look_at_right:
            case Direction_look_at_bottom:
            case Direction_look_at_left:
            case Direction_move_at_top:
            case Direction_move_at_right:
            case Direction_move_at_bottom:
            case Direction_move_at_left:
                if(MoveOnTheMap::canGoTo(direction,*map,x,y,false))
                {
                    if(!MoveOnTheMap::move(direction,&map,&x,&y,false))
                    {
                        errorOutput("learnSkill() Can't move at top from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
                        return false;
                    }
                }
                else
                {
                    errorOutput("No valid map in this direction");
                    return false;
                }
            break;
            default:
            break;
        }
        if(mapServer->learn.find(pos)==mapServer->learn.end())
        {
            errorOutput("not learn skill into this direction");
            return false;
        }
    }
    return learnSkillInternal(monsterPosition,skill);
}

void Client::heal()
{
    if(isInFight())
    {
        errorOutput("Try do heal action when is in fight");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("ask heal at "+this->map->map_file+" ("+std::to_string(this->x)+","+std::to_string(this->y)+")");
    #endif
    CommonMap *map=this->map;
    uint8_t x=this->x;
    uint8_t y=this->y;
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
                    errorOutput("heal() Can't move at this direction from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
                    return;
                }
            }
            else
            {
                errorOutput("No valid map in this direction");
                return;
            }
        break;
        default:
        errorOutput("Wrong direction to use a heal");
        return;
    }
    const MapServer * const mapServer=static_cast<MapServer*>(map);
    const std::pair<uint8_t,uint8_t> pos(x,y);
    //check if is shop
    if(mapServer->heal.find(pos)==mapServer->heal.cend())
    {
        Direction direction=getLastDirection();
        switch(direction)
        {
            /// \warning: Not loop but move here due to first transform set: direction=lookToMove(direction);
            case Direction_look_at_top:
            case Direction_look_at_right:
            case Direction_look_at_bottom:
            case Direction_look_at_left:
            direction=lookToMove(direction);
            break;
            default:
            return;
        }
        switch(direction)
        {
            /// \warning: Not loop but move here due to first transform set: direction=lookToMove(direction);
            case Direction_look_at_top:
            case Direction_look_at_right:
            case Direction_look_at_bottom:
            case Direction_look_at_left:
            case Direction_move_at_top:
            case Direction_move_at_right:
            case Direction_move_at_bottom:
            case Direction_move_at_left:
                if(MoveOnTheMap::canGoTo(direction,*map,x,y,false))
                {
                    if(!MoveOnTheMap::move(direction,&map,&x,&y,false))
                    {
                        errorOutput("heal() Can't move at this direction from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
                        return;
                    }
                }
                else
                {
                    errorOutput("No valid map in this direction");
                    return;
                }
            break;
            default:
            errorOutput("Wrong direction to use a heal");
            return;
        }
        const MapServer * const mapServer=static_cast<MapServer*>(map);
        const std::pair<uint8_t,uint8_t> pos(x,y);
        if(mapServer->heal.find(pos)==mapServer->heal.cend())
        {
            errorOutput("no heal point in this direction");
            return;
        }
    }
    //send the shop items (no taxes from now)
    healAllMonsters();
    rescue=unvalidated_rescue;
}

void Client::requestFight(const uint16_t &fightId)
{
    if(isInFight())
    {
        errorOutput("error: map: "+this->map->map_file+" ("+std::to_string(this->x)+","+std::to_string(this->y)+"), is in fight");
        return;
    }
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(captureCityInProgress())
    {
        errorOutput("Try requestFight when is in capture city");
        return;
    }
    #endif
    if(public_and_private_informations.bot_already_beaten==NULL)
    {
        errorOutput("You can't beat this fighter, public_and_private_informations.bot_already_beaten==NULL");
        return;
    }
    if(public_and_private_informations.bot_already_beaten[fightId/8] & (1<<(7-fightId%8)))
    {
        errorOutput("You can't rebeat this fighter");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("request fight at "+this->map->map_file+" ("+std::to_string(this->x)+","+std::to_string(this->y)+")");
    #endif
    CommonMap *map=this->map;
    uint8_t x=this->x;
    uint8_t y=this->y;
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
                    errorOutput("requestFight() Can't move at this direction from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
                    return;
                }
            }
            else
            {
                errorOutput("No valid map in this direction");
                return;
            }
        break;
        default:
        errorOutput("Wrong direction to use a shop");
        return;
    }
    const MapServer * const mapServer=static_cast<MapServer*>(map);
    const std::pair<uint8_t,uint8_t> pos(x,y);
    //check if is shop
    bool found=false;
    if(mapServer->botsFight.find(pos)!=mapServer->botsFight.cend())
    {
        const std::vector<uint16_t> &botsFightList=mapServer->botsFight.at(pos);
        if(vectorcontainsAtLeastOne(botsFightList,fightId))
            found=true;
    }
    if(!found)
    {
        Direction direction=getLastDirection();
        switch(direction)
        {
            /// \warning: Not loop but move here due to first transform set: direction=lookToMove(direction);
            case Direction_look_at_top:
            case Direction_look_at_right:
            case Direction_look_at_bottom:
            case Direction_look_at_left:
            direction=lookToMove(direction);
            break;
            default:
            return;
        }
        switch(direction)
        {
            /// \warning: Not loop but move here due to first transform set: direction=lookToMove(direction);
            case Direction_look_at_top:
            case Direction_look_at_right:
            case Direction_look_at_bottom:
            case Direction_look_at_left:
            case Direction_move_at_top:
            case Direction_move_at_right:
            case Direction_move_at_bottom:
            case Direction_move_at_left:
                if(MoveOnTheMap::canGoTo(direction,*map,x,y,false))
                {
                    if(!MoveOnTheMap::move(direction,&map,&x,&y,false))
                    {
                        errorOutput("requestFight() Can't move at this direction from "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+")");
                        return;
                    }
                }
                else
                {
                    errorOutput("No valid map in this direction");
                    return;
                }
            break;
            default:
            errorOutput("Wrong direction to use a shop");
            return;
        }
        const MapServer * const mapServer=static_cast<MapServer*>(map);
        const std::pair<uint8_t,uint8_t> pos(x,y);
        if(mapServer->botsFight.find(pos)!=mapServer->botsFight.cend())
        {
            const std::vector<uint16_t> &botsFightList=static_cast<MapServer*>(this->map)->botsFight.at(pos);
            if(vectorcontainsAtLeastOne(botsFightList,fightId))
                found=true;
        }
        if(!found)
        {
            errorOutput("no fight with id "+std::to_string(fightId)+" in this direction");
            return;
        }
    }
    normalOutput("is now in fight (after a request) on map "+map->map_file+" ("+std::to_string(x)+","+std::to_string(y)+") with the bot "+std::to_string(fightId));
    botFightStart(fightId);
}

bool Client::otherPlayerIsInRange(Client * otherPlayer)
{
    if(getMap()==NULL)
        return false;
    return getMap()==otherPlayer->getMap();
}

void Client::moveMonster(const bool &up,const uint8_t &number)
{
    if(up)
        moveUpMonster(number-1);
    else
        moveDownMonster(number-1);
}
