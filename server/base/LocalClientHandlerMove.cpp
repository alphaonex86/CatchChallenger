#include "Client.hpp"

#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/FacilityLib.hpp"
#include "PreparedDBQuery.hpp"
#include "GlobalServerData.hpp"
#include "MapServer.hpp"

using namespace CatchChallenger;

bool Client::moveThePlayer(const uint8_t &previousMovedUnit,const Direction &direction)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(this->x>=this->map->width)
    {
        std::cerr << "x to out of map: " << this->x << " > " << this->map->width << " (" << this->map->map_file << ")" << std::endl;
        abort();
        return false;
    }
    if(this->y>=this->map->height)
    {
        std::cerr << "y to out of map: " << this->y << " > " << this->map->height << " (" << this->map->map_file << ")" << std::endl;
        abort();
        return false;
    }
    #endif

    #ifdef DEBUG_MESSAGE_CLIENT_MOVE
    std::cout << "moveThePlayer(): for player (" << x << "," << y << "): " << public_and_private_informations.public_informations.simplifiedId
              << ", previousMovedUnit: " << previousMovedUnit << " (" << MoveOnTheMap::directionToString(getLastDirection())
              << "), next direction: " << MoveOnTheMap::directionToString(direction) << std::endl;
    #endif
    const bool &returnValue=MapBasicMove::moveThePlayer(previousMovedUnit,direction);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(returnValue)
    {
        if(this->x>=this->map->width)
        {
            std::cerr << "x to out of map: " << this->x << " > " << this->map->width << " (" << this->map->map_file << ")" << std::endl;
            abort();
            return false;
        }
        if(this->y>=this->map->height)
        {
            std::cerr << "y to out of map: " << this->y << " > " << this->map->height << " (" << this->map->map_file << ")" << std::endl;
            abort();
            return false;
        }
    }
    #endif
    return returnValue;
}

bool Client::singleMove(const Direction &direction)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(this->x>=this->map->width)
    {
        std::cerr << "x to out of map: " << this->x << " > " << this->map->width << " (" << this->map->map_file << ")" << std::endl;
        abort();
        return false;
    }
    if(this->y>=this->map->height)
    {
        std::cerr << "x to out of map: " << this->y << " > " << this->map->height << " (" << this->map->map_file << ")" << std::endl;
        abort();
        return false;
    }
    #endif

    if(isInFight())//check if is in fight
    {
        errorOutput("error: try move when is in fight");
        return false;
    }
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(captureCityInProgress())
    {
        errorOutput("Try move when is in capture city");
        return false;
    }
    #endif
    const auto &now = sFrom1970();
    if(oldEvents.oldEventList.size()>0 && (now-oldEvents.time)>30/*30s*/)
    {
        errorOutput("Try move but lost of event sync");
        return false;
    }
    COORD_TYPE x=this->x,y=this->y;
    temp_direction=direction;
    CommonMap* map=this->map;
    #ifdef DEBUG_MESSAGE_CLIENT_MOVE
    normalOutput("Client::singleMove(), go in this direction: "+MoveOnTheMap::directionToString(direction)+" with map: "+map->map_file+"("+std::to_string(x)+","+std::to_string(y)+")");
    #endif
    if(!MoveOnTheMap::canGoTo(direction,*map,x,y,true))
    {
        errorOutput("Client::singleMove(), can't go into this direction: "+MoveOnTheMap::directionToString(direction)+" with map: "+map->map_file+"("+std::to_string(x)+","+std::to_string(y)+")");
        return false;
    }
    if(!MoveOnTheMap::moveWithoutTeleport(direction,&map,&x,&y,false,true))
    {
        errorOutput("Client::singleMove(), can go but move failed into this direction: "+MoveOnTheMap::directionToString(direction)+" with map: "+map->map_file+"("+std::to_string(x)+","+std::to_string(y)+")");
        return false;
    }

    const CommonMap::Teleporter* const teleporter_list=map->teleporter;
    uint8_t index_search=0;
    while(index_search<map->teleporter_list_size)
    {
        const CommonMap::Teleporter &teleporter=teleporter_list[index_search];
        if(teleporter.source_x==x && teleporter.source_y==y)
        {
            switch(teleporter.condition.type)
            {
                case CatchChallenger::MapConditionType_None:
                case CatchChallenger::MapConditionType_Clan://not do for now
                /// \todo check if the clan own the city
                break;
                case CatchChallenger::MapConditionType_FightBot:
                {
                    if(public_and_private_informations.bot_already_beaten==NULL)
                    {
                        errorOutput("Need have public_and_private_informations.bot_already_beaten!=NULL to use this teleporter: "+std::to_string(teleporter.condition.data.fightBot)+" with map: "+map->map_file+"("+std::to_string(x)+","+std::to_string(y)+")");
                        return false;
                    }
                    const auto &fightId=teleporter.condition.data.fightBot;
                    if(!(public_and_private_informations.bot_already_beaten[fightId/8] & (1<<(7-fightId%8))))
                    {
                        errorOutput("Need have FightBot win to use this teleporter: "+std::to_string(teleporter.condition.data.fightBot)+" with map: "+map->map_file+"("+std::to_string(x)+","+std::to_string(y)+")");
                        return false;
                    }
                }
                break;
                case CatchChallenger::MapConditionType_Item:
                    if(public_and_private_informations.items.find(teleporter.condition.data.item)==public_and_private_informations.items.cend())
                    {
                        errorOutput("Need have item to use this teleporter: "+std::to_string(teleporter.condition.data.item)+" with map: "+map->map_file+"("+std::to_string(x)+","+std::to_string(y)+")");
                        return false;
                    }
                break;
                case CatchChallenger::MapConditionType_Quest:
                    if(public_and_private_informations.quests.find(teleporter.condition.data.quest)==public_and_private_informations.quests.cend())
                    {
                        errorOutput("Need have quest to use this teleporter: "+std::to_string(teleporter.condition.data.quest)+" with map: "+map->map_file+"("+std::to_string(x)+","+std::to_string(y)+")");
                        return false;
                    }
                    if(!public_and_private_informations.quests.at(teleporter.condition.data.quest).finish_one_time)
                    {
                        errorOutput("Need have finish the quest to use this teleporter: "+std::to_string(teleporter.condition.data.quest)+" with map: "+map->map_file+"("+std::to_string(x)+","+std::to_string(y)+")");
                        return false;
                    }
                break;
                default:
                break;
            }
            if(teleporter.map==NULL)
            {
                errorOutput("This teleporter no valid teleporter.map==NULL: "+std::to_string(teleporter.condition.data.item)+" with map: "+map->map_file+"("+std::to_string(x)+","+std::to_string(y)+")");
                return false;
            }
            x=teleporter.destination_x;
            y=teleporter.destination_y;
            map=teleporter.map;
            break;
        }
        index_search++;
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
    this->x=x;
    this->y=y;
    {
        MapServer* mapServer=static_cast<MapServer*>(map);
        if(mapServer->rescue.find(std::pair<uint8_t,uint8_t>(x,y))!=mapServer->rescue.cend())
        {
            unvalidated_rescue.map=map;
            unvalidated_rescue.x=x;
            unvalidated_rescue.y=y;
            unvalidated_rescue.orientation=mapServer->rescue.at(std::pair<uint8_t,uint8_t>(x,y));
        }
    }
    if(botFightCollision(map,x,y))
        return true;
    if(public_and_private_informations.repel_step<=0)
    {
        //merge the result event:
        std::vector<uint8_t> mergedEvents(GlobalServerData::serverPrivateVariables.events);
        if(oldEvents.oldEventList.size()>0)
        {
            unsigned int index=0;
            while(index<oldEvents.oldEventList.size())
            {
                mergedEvents[oldEvents.oldEventList.at(index).event]=oldEvents.oldEventList.at(index).eventValue;
                index++;
            }
        }
        if(generateWildFightIfCollision(map,x,y,public_and_private_informations.items,mergedEvents))
        {
            if(map!=NULL)
                normalOutput("Client::singleMove(), now is in front of wild monster with map: "+map->map_file+"("+std::to_string(x)+","+std::to_string(y)+")");
            else
                normalOutput("Client::singleMove(), now is in front of wild monster)");
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
            std::cerr << "x to out of map: "+std::to_string(this->x)+" > "+std::to_string(this->map->width)+" ("+this->map->map_file+")" << std::endl;
            abort();
            return false;
        }
        if(this->y>=this->map->height)
        {
            std::cerr << "y to out of map: "+std::to_string(this->y)+" > "+std::to_string(this->map->height)+" ("+this->map->map_file+")" << std::endl;
            abort();
            return false;
        }
    }
    #endif

    return true;
}
