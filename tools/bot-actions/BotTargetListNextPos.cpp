#include "BotTargetList.h"
#include "ui_BotTargetList.h"
#include "../../client/base/interface/DatapackClientLoader.h"
#include "../../client/fight/interface/ClientFightEngine.h"
#include "MapBrowse.h"

#include <chrono>
#include <QMessageBox>

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
                const DatapackClientLoader::PlantIndexContent &plantOrDirt=DatapackClientLoader::datapackLoader.plantIndexOfOnMap.value(target.extra);
                point.first=plantOrDirt.x;
                point.second=plantOrDirt.y;
                return point;
            }
            break;
            case ActionsBotInterface::GlobalTarget::Plant:
            {
                const DatapackClientLoader::PlantIndexContent &plantOrDirt=DatapackClientLoader::datapackLoader.plantIndexOfOnMap.value(target.extra);
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
    if(!player.target.bestPath.empty())
        return true;
    if(!player.target.localStep.empty() && !player.target.wildBackwardStep.empty())
        return true;
    if(player.target.wildBackwardStep.empty())
    {
        if(!player.target.wildBackwardStep.empty())
            abort();
        player.target.wildCycle=0;

        const std::string &mapStdString=actionsAction->id_map_to_map.at(player.mapId);
        CatchChallenger::CommonMap *map=actionsAction->map_list.at(mapStdString);
        const MapServerMini *mapServer=static_cast<MapServerMini *>(map);
        if(mapServer->step.size()<2)
            abort();
        const uint16_t &currentCodeZone=mapServer->step.at(1).map[player.x+player.y*mapServer->width];
        if(currentCodeZone==0)
            abort();
        MapServerMini::BlockObject * blockObject=mapServer->step.at(1).layers.at(currentCodeZone-1).blockObject;
        if(blockObject->block.size()<1)
        {
            std::cerr << "Error: Block list is empty" << std::endl;
            abort();
        }
        if(blockObject->block.size()<2)
        {
            std::cerr << "Error: Block list is too small" << std::endl;
            abort();//todo
        }

        const std::pair<uint8_t,uint8_t> &point1=blockObject->block.at(rand()%blockObject->block.size());
        std::pair<uint8_t,uint8_t> point2=blockObject->block.at(rand()%blockObject->block.size());
        while(point2==point1)
            point2=blockObject->block.at(rand()%blockObject->block.size());

        uint8_t directionToPoint1=0,directionToPoint2=0;
        //to the point 1
        {
            uint8_t o=player.api->getDirection();
            while(o>4)
                o-=4;
            std::vector<MapServerMini::BlockObject::DestinationForPath> destinations;
            MapServerMini::BlockObject::DestinationForPath destinationForPath;
            destinationForPath.destination_x=point1.first;
            destinationForPath.destination_y=point1.second;
            destinationForPath.destination_orientation=CatchChallenger::Orientation::Orientation_none;
            destinations.push_back(destinationForPath);
            bool ok=false;
            unsigned int destinationIndexSelected=0;
            const std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > &returnPath=pathFinding(
                        blockObject,
                        static_cast<CatchChallenger::Orientation>(o),player.x,player.y,
                        destinations,
                        destinationIndexSelected,
                        &ok);
            if(!ok)
                abort();
            MapServerMini::BlockObject::LinkPoint linkPoint;
            linkPoint.type=MapServerMini::BlockObject::LinkType::SourceNone;
            linkPoint.inLedge=false;
            linkPoint.x=point1.first;
            linkPoint.y=point1.second;
            player.target.linkPoint=linkPoint;
            player.target.localStep=returnPath;
            if(!returnPath.empty())
            {
                directionToPoint1=returnPath.back().first;
                while(directionToPoint1>4)
                    directionToPoint1-=4;
            }
            else
                directionToPoint1=static_cast<CatchChallenger::Orientation>(o);
        }
        //point 1 to 2
        {
            std::vector<MapServerMini::BlockObject::DestinationForPath> destinations;
            MapServerMini::BlockObject::DestinationForPath destinationForPath;
            destinationForPath.destination_x=point2.first;
            destinationForPath.destination_y=point2.second;
            destinationForPath.destination_orientation=CatchChallenger::Orientation::Orientation_none;
            destinations.push_back(destinationForPath);
            bool ok=false;
            unsigned int destinationIndexSelected=0;
            const std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > &returnPath=pathFinding(
                        blockObject,
                        static_cast<CatchChallenger::Orientation>(directionToPoint1),point1.first,point1.second,
                        destinations,
                        destinationIndexSelected,
                        &ok);
            if(!ok)
                abort();
            player.target.wildForwardStep=returnPath;
            if(!returnPath.empty())
            {
                directionToPoint1=returnPath.back().first;
                while(directionToPoint1>4)
                    directionToPoint1-=4;
            }
            else
                directionToPoint1=static_cast<CatchChallenger::Orientation>(directionToPoint1);
        }
        //point 2 to 1
        {
            std::vector<MapServerMini::BlockObject::DestinationForPath> destinations;
            MapServerMini::BlockObject::DestinationForPath destinationForPath;
            destinationForPath.destination_x=point1.first;
            destinationForPath.destination_y=point1.second;
            destinationForPath.destination_orientation=CatchChallenger::Orientation::Orientation_none;
            destinations.push_back(destinationForPath);
            bool ok=false;
            unsigned int destinationIndexSelected=0;
            const std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > &returnPath=pathFinding(
                        blockObject,
                        static_cast<CatchChallenger::Orientation>(directionToPoint2),point2.first,point2.second,
                        destinations,
                        destinationIndexSelected,
                        &ok);
            if(!ok)
                abort();
            player.target.wildBackwardStep=returnPath;
        }
    }
    else
    {
        if(player.target.localStep.empty())
        {
            if(player.target.wildCycle<10)
            {
                if((player.target.wildCycle%2)==0)
                {
                    //is into point 1, go to 2
                    player.target.localStep=player.target.wildForwardStep;
                }
                else
                {
                    //is into point 1, go to 2
                    player.target.localStep=player.target.wildBackwardStep;
                }
                player.target.wildCycle++;
                return true;
            }
            else
                return false;
        }
    }
    return true;
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
