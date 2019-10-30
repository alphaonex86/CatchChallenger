#include "BotTargetList.h"
#include "ui_BotTargetList.h"
#include "../../client/qt/QtDatapackClientLoader.h"
#include "../../client/qt/fight/interface/ClientFightEngine.h"
#include "MapBrowse.h"

#include <chrono>
#include <QMessageBox>
#include <iostream>

//if(target.bestPath.empty()) return real target, else near search the next block
std::pair<uint8_t, uint8_t> BotTargetList::getNextPosition(const MapServerMini::BlockObject * const blockObject,ActionsBotInterface::GlobalTarget &target)
{
    std::pair<uint8_t,uint8_t> point(0,0);
    if(target.blockObject==NULL)
    {
        std::cerr << "error: target.blockObject==NULL into getNextPosition()" << std::endl;
        return point;
    }
    if(target.bestPath.empty())
    {
        switch(target.type)
        {
            case ActionsBotInterface::GlobalTarget::ItemOnMap:
            for(auto it = blockObject->pointOnMap_Item.begin();it!=blockObject->pointOnMap_Item.cend();++it)
            {
                const MapServerMini::ItemOnMap &itemOnMap=it->second;
                if(itemOnMap.indexOfItemOnMap==target.extra)
                    return it->first;
            }
            std::cerr << "BotTargetList::getNextPosition() ActionsBotInterface::GlobalTarget::ItemOnMap: " << std::to_string(target.extra) << std::endl;
            abort();
            break;
            case ActionsBotInterface::GlobalTarget::Fight:
                for(auto it = blockObject->botsFight.begin();it!=blockObject->botsFight.cend();++it)
                {
                    const std::vector<uint16_t> &botsFightList=it->second;
                    if(vectorcontainsAtLeastOne(botsFightList,(uint16_t)target.extra))
                        return it->first;
                }
                std::cerr << "BotTargetList::getNextPosition() ActionsBotInterface::GlobalTarget::Fight: " << std::to_string(target.extra) << std::endl;
                abort();
            break;
            case ActionsBotInterface::GlobalTarget::Shop:
                for(auto it = blockObject->shops.begin();it!=blockObject->shops.cend();++it)
                {
                    const std::vector<uint16_t> &shops=it->second;
                    if(vectorcontainsAtLeastOne(shops,(uint16_t)target.extra))
                        return it->first;
                }
                std::cerr << "BotTargetList::getNextPosition() ActionsBotInterface::GlobalTarget::Shop: " << std::to_string(target.extra) << std::endl;
                abort();
            break;
            case ActionsBotInterface::GlobalTarget::Heal:
                for(auto it = blockObject->heal.begin();it!=blockObject->heal.cend();++it)
                    return *it;
                std::cerr << "BotTargetList::getNextPosition() ActionsBotInterface::GlobalTarget::Heal" << std::endl;
                abort();
            break;
            case ActionsBotInterface::GlobalTarget::WildMonster:
                abort();
            break;
            case ActionsBotInterface::GlobalTarget::Dirt:
            {
                const DatapackClientLoader::PlantIndexContent &plantOrDirt=QtDatapackClientLoader::datapackLoader->plantIndexOfOnMap.at(target.extra);
                point.first=plantOrDirt.x;
                point.second=plantOrDirt.y;
                return point;
            }
            break;
            case ActionsBotInterface::GlobalTarget::Plant:
            {
                const DatapackClientLoader::PlantIndexContent &plantOrDirt=QtDatapackClientLoader::datapackLoader->plantIndexOfOnMap.at(target.extra);
                point.first=plantOrDirt.x;
                point.second=plantOrDirt.y;
                return point;
            }
            break;
            default:
                ActionsAction::resetTarget(target);
                stopAll();
                QMessageBox::critical(this,tr("Not coded"),tr("This target type is not coded (3): %1").arg(target.type));
                abort();
            break;
        }
    }
    else
        abort();
    abort();
    return point;
}

bool BotTargetList::wildMonsterTarget(ActionsBotInterface::Player &player)
{
    /// \warning path finding here cause performance problem

    if(!player.target.bestPath.empty())
            return true;
    if(!player.target.localStep.empty())
        return true;

    player.target.wildCycle++;
    if(player.target.wildCycle>10)
    {
        player.target.wildCycle=0;
        return false;
    }

    // method: choose random direction, if can go 1, detect max and random to max

    const std::string &mapStdString=actionsAction->id_map_to_map.at(player.mapId);
    CatchChallenger::CommonMap *map=actionsAction->map_list.at(mapStdString);
    const MapServerMini *mapServer=static_cast<MapServerMini *>(map);
    if(mapServer->step.size()<2)
        abort();
    const MapServerMini::MapParsedForBot &stepMap=mapServer->step.at(1);
    const uint16_t &currentCodeZone=stepMap.map[player.x+player.y*mapServer->width];
    if(currentCodeZone==0)
        abort();

    //random direction list and try one step
    std::vector<CatchChallenger::Orientation> dlist{
        CatchChallenger::Orientation::Orientation_bottom,
        CatchChallenger::Orientation::Orientation_top,
        CatchChallenger::Orientation::Orientation_left,
        CatchChallenger::Orientation::Orientation_right
    };
    std::shuffle(dlist.begin(), dlist.end(), g);
    for (const auto &n:dlist) {
        uint8_t x=player.x;
        uint8_t y=player.y;
        /* not use if(ActionsAction::canGoTo(api,newDirectionToMove,*playerMap,x,y,true,true))
         * - simplified algo improve the performance
         * - no need multi-map algo */
        bool canGO=false;
        switch(n)
        {
            case CatchChallenger::Orientation::Orientation_bottom:
                if(y<(map->height-1))
                    if(currentCodeZone==stepMap.map[x+(y+1)*mapServer->width])
                        canGO=true;
            break;
            case CatchChallenger::Orientation::Orientation_top:
                if(y>0)
                    if(currentCodeZone==stepMap.map[x+(y-1)*mapServer->width])
                        canGO=true;
            break;
            case CatchChallenger::Orientation::Orientation_right:
                if(x<(map->width-1))
                    if(currentCodeZone==stepMap.map[(x+1)+y*mapServer->width])
                        canGO=true;
            break;
            case CatchChallenger::Orientation::Orientation_left:
                if(x>0)
                    if(currentCodeZone==stepMap.map[(x-1)+y*mapServer->width])
                        canGO=true;
            break;
            default:
            abort();
            break;
        }

        if(canGO)
        {
            uint8_t step=0;
            //continue at least 10 to detect the max
            switch(n)
            {
                case CatchChallenger::Orientation::Orientation_bottom:
                    while(y<(map->height-1) && currentCodeZone==stepMap.map[x+(y+1)*mapServer->width] && (y-player.y)<10)
                        y++;
                    step=y-player.y;
                break;
                case CatchChallenger::Orientation::Orientation_top:
                    while(y>0 && currentCodeZone==stepMap.map[x+(y-1)*mapServer->width] && (player.y-y)<10)
                        y--;
                    step=player.y-y;
                break;
                case CatchChallenger::Orientation::Orientation_right:
                    while(x<(map->width-1) && currentCodeZone==stepMap.map[(x+1)+y*mapServer->width] && (x-player.x)<10)
                        x++;
                    step=x-player.x;
                break;
                case CatchChallenger::Orientation::Orientation_left:
                    while(x>0 && currentCodeZone==stepMap.map[(x-1)+y*mapServer->width] && (player.x-x)<10)
                        x--;
                    step=player.x-x;
                break;
                default:
                abort();
                break;
            }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(step==0)
            {
                const std::string &debugMapString=actionsAction->id_map_to_map.at(player.mapId);
                std::cout << player.api->getPseudo() << ": localStep: " << BotTargetList::stepToString(player.target.localStep)
                          << " from " << debugMapString << " " << std::to_string(player.x) << "," << std::to_string(player.y)
                          << ", n: " << n << ", currentCodeZone: " << currentCodeZone
                          << ", " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
                abort();
            }
            #endif
            //choose random step
            if(step>1)
                step=1+rand()%step;//1 to step
            std::pair<CatchChallenger::Orientation,uint8_t> newEntry(n,step);
            player.target.localStep.clear();
            player.target.localStep.push_back(newEntry);

            const std::string &debugMapString=actionsAction->id_map_to_map.at(player.mapId);
            std::cout << player.api->getPseudo() << ": localStep: " << BotTargetList::stepToString(player.target.localStep)
                      << " from " << debugMapString << " " << std::to_string(player.x) << "," << std::to_string(player.y)
                      << ", " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;

            //reset some values
            MapServerMini::BlockObject::LinkPoint linkPoint;
            linkPoint.type=MapServerMini::BlockObject::LinkType::SourceNone;
            linkPoint.inLedge=false;
            linkPoint.x=player.x;
            linkPoint.y=player.y;
            player.target.linkPoint=linkPoint;

            return true;
        }
    }
    return false;
}

void BotTargetList::finishTheLocalStep(ActionsAction::Player &player)
{
    Q_UNUSED(player);
    /*if(player.target.bestPath.empty() && !player.target.local
    Step.empty() && player.target.type==ActionsBotInterface::GlobalTarget::GlobalTargetType::ItemOnMap)
    {
        std::pair<CatchChallenger::Orientation,uint8_t> &lastPair=player.target.localStep.back();
        if(lastPair.second>1)
            lastPair.second--;
        else
        {
            lastPair.second=0;
            lastPair.first=moveToLook(lastPair.first);
        }
    }do directly at direction parsing*/
}
