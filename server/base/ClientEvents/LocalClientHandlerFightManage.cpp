#include "../Client.hpp"

#include "../MapServer.hpp"
#include "../MapManagement/Map_server_MapVisibility_Simple_StoreOnSender.hpp"

using namespace CatchChallenger;

bool Client::learnSkillByMonsterPosition(const uint8_t &monsterPosition, const uint16_t &skill)
{
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("learnSkill("+std::to_string(monsterId)+","+std::to_string(skill)+")");
    #endif
    if(monsterPosition>=public_and_private_informations.monsters.size())
    {
        errorOutput("The monster is not found: "+std::to_string(monsterPosition));
        return false;
    }
    return learnSkillInternal(monsterPosition,skill);
}

void Client::heal()
{
    if(mapIndex>=65535)
        return;
    if(isInFight())
    {
        errorOutput("Try do heal action when is in fight");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("ask heal at "+this->map->map_file+" ("+std::to_string(this->x)+","+std::to_string(this->y)+")");
    #endif
    COORD_TYPE new_x=0,new_y=0;
    const Map_server_MapVisibility_Simple_StoreOnSender * new_map=Client::mapAndPosIfMoveInLookingDirectionJumpColision(new_x,new_y);
    if(new_map==nullptr)
    {
        errorOutput("Can't move at this direction to heal");
        return;
    }
    const std::pair<uint8_t,uint8_t> pos(new_x,new_y);
    if(new_map->heal.find(pos)==new_map->heal.cend())
    {
        errorOutput("no heal point in this direction");
        return;
    }
    //send the shop items (no taxes from now)
    healAllMonsters();
    rescue=unvalidated_rescue;
}

void Client::requestFight(const CATCHCHALLENGER_TYPE_MAPID &mapId,const CATCHCHALLENGER_TYPE_BOTID &botId)
{
    if(mapIndex>=65535)
        return;
    if(isInFight())
    {
        errorOutput("error: requestFight, is in fight");
        return;
    }
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(captureCityInProgress())
    {
        errorOutput("Try requestFight when is in capture city");
        return;
    }
    #endif
    if(haveBeatBot(mapId,botId))
    {
        errorOutput("You can't rebeat this fighter");
        return;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("request fight at "+this->map->map_file+" ("+std::to_string(this->x)+","+std::to_string(this->y)+")");
    #endif
    COORD_TYPE new_x=0,new_y=0;
    const Map_server_MapVisibility_Simple_StoreOnSender * new_map=Client::mapAndPosIfMoveInLookingDirectionJumpColision(new_x,new_y);
    if(new_map==nullptr)
    {
        errorOutput("Can't move at this direction from requestFight");
        return;
    }
    const std::pair<uint8_t,uint8_t> pos(new_x,new_y);//if need x,y, then above code is not used
    if(new_map->botsFightTrigger.find(pos)!=new_map->botsFightTrigger.cend())
    {
        const uint8_t botsFightId=new_map->botsFightTrigger.at(pos);
        if(botsFightId==botId)
        {
            normalOutput("is now in fight (after a request) on map with the bot "+std::to_string(botId));
            botFightStart(std::pair<CATCHCHALLENGER_TYPE_MAPID/*mapId*/,uint8_t/*botId*/>(new_map->id,botId));
            return;
        }
    }
    errorOutput("no fight with id "+std::to_string(botId)+" in this direction");
}

bool Client::otherPlayerIsInRange(Client &otherPlayer)
{
    return getMapId()==otherPlayer.getMapId();
}

void Client::moveMonster(const bool &up,const uint8_t &number)
{
    if(up)
        moveUpMonster(number);
    else
        moveDownMonster(number);
}
