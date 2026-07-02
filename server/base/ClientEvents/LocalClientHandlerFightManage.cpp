#include "../Client.hpp"

#include "../MapServer.hpp"
#include "../MapManagement/MapVisibilityAlgorithm.hpp"

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
    CATCHCHALLENGER_TYPE_MAPID new_map_index=0;
    const MapVisibilityAlgorithm * new_map=Client::mapAndPosIfMoveInLookingDirectionJumpColision(new_map_index,new_x,new_y);
    if(new_map==nullptr)
    {
        errorOutput("Can't move at this direction to heal");
        return;
    }
    if(new_map->heal.find(std::pair<uint8_t,uint8_t>(new_x,new_y))==new_map->heal.cend())
    {
        //counter fallback: the nurse can sit one tile behind a single counter wall
        bool foundHeal=false;
        if(MoveOnTheMap::isHardBlock(*new_map,new_x,new_y))
        {
            CATCHCHALLENGER_TYPE_MAPID m2=new_map_index;
            COORD_TYPE x2=new_x,y2=new_y;
            const MapVisibilityAlgorithm * mb=Client::mapAndPosJumpOneColisionMore(m2,x2,y2);
            if(mb!=nullptr && mb->heal.find(std::pair<uint8_t,uint8_t>(x2,y2))!=mb->heal.cend())
            {
                new_map=mb;
                new_map_index=m2;
                new_x=x2;
                new_y=y2;
                foundHeal=true;
            }
        }
        if(!foundHeal)
        {
            errorOutput("no heal point in this direction");
            return;
        }
    }
    //send the shop items (no taxes from now)
    healAllMonsters();
    rescue=unvalidated_rescue;
}

void Client::requestFight()
{
    if(mapIndex>=65535)
        return;
    if(isInFight())
    {
        errorOutput("error: requestFight, is in fight");
        return;
    }
    #ifndef CATCHCHALLENGER_SERVER
    if(captureCityInProgress())
    {
        errorOutput("Try requestFight when is in capture city");
        return;
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("request fight at "+this->map->map_file+" ("+std::to_string(this->x)+","+std::to_string(this->y)+")");
    #endif
    COORD_TYPE new_x=0,new_y=0;
    CATCHCHALLENGER_TYPE_MAPID new_map_index=0;
    const MapVisibilityAlgorithm * new_map=Client::mapAndPosIfMoveInLookingDirectionJumpColision(new_map_index,new_x,new_y);
    if(new_map==nullptr)
    {
        errorOutput("Can't move at this direction from requestFight");
        return;
    }
    const std::pair<uint8_t,uint8_t> pos(new_x,new_y);//if need x,y, then above code is not used
    if(new_map->botsFightTrigger.find(pos)!=new_map->botsFightTrigger.cend())
    {
        const uint8_t botsFightId=new_map->botsFightTrigger.at(pos);
        if(haveBeatBot(new_map_index,botsFightId))
        {
            errorOutput("You can't rebeat this fighter");
            return;
        }
        normalOutput("is now in fight (after a request) on map with the bot "+std::to_string(botsFightId));
        botFightStart(std::pair<CATCHCHALLENGER_TYPE_MAPID/*mapId*/,uint8_t/*botId*/>(new_map_index,botsFightId));
        return;
    }
    errorOutput("no fight with in this direction");
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
