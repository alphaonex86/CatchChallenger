#include "BotTargetList.h"
#include "ui_BotTargetList.h"
#include "../../client/base/interface/DatapackClientLoader.h"
#include "../../client/fight/interface/ClientFightEngine.h"
#include "../../general/base/CommonSettingsServer.h"
#include "MapBrowse.h"

#include <chrono>
#include <QMessageBox>

void BotTargetList::updatePlayerStep()
{
    QHashIterator<CatchChallenger::Api_protocol *,ActionsAction::Player> i(actionsAction->clientList);
    while (i.hasNext()) {
        i.next();
        CatchChallenger::Api_protocol *api=i.key();
        ActionsAction::Player &player=actionsAction->clientList[i.key()];
        if(actionsAction->id_map_to_map.find(player.mapId)==actionsAction->id_map_to_map.cend())
            abort();
        if(api->getCaracterSelected())
        {
            if(ui->bots->count()==1)
            {
                QListWidgetItem * item=ui->bots->itemAt(0,0);
                if(!item->isSelected())
                {
                    item->setSelected(true);
                    on_bots_itemSelectionChanged();
                }
            }
            CatchChallenger::Player_private_and_public_informations &player_private_and_public_informations=api->get_player_informations();
            bool haveChange=false;
            if(player.target.localStep.empty())
            {
                if(!player.target.bestPath.empty())
                {
                    haveChange=true;
                    if(actionsAction->id_map_to_map.find(player.mapId)==actionsAction->id_map_to_map.cend())
                        abort();
                    const std::string &playerMapStdString=actionsAction->id_map_to_map.at(player.mapId);
                    const MapServerMini * playerMap=static_cast<const MapServerMini *>(actionsAction->map_list.at(playerMapStdString));
                    CatchChallenger::Direction newDirection=player.api->getDirection();/*=CatchChallenger::Direction::Direction_look_at_bottom no continue on same direction*/
                    switch(player.target.linkPoint.type)
                    {
                        case MapServerMini::BlockObject::LinkType::SourceTopMap:
                        case MapServerMini::BlockObject::LinkType::SourceInternalTopBlock:
                            newDirection=CatchChallenger::Direction::Direction_move_at_top;
                        break;
                        case MapServerMini::BlockObject::LinkType::SourceRightMap:
                        case MapServerMini::BlockObject::LinkType::SourceInternalRightBlock:
                            newDirection=CatchChallenger::Direction::Direction_move_at_right;
                        break;
                        case MapServerMini::BlockObject::LinkType::SourceBottomMap:
                        case MapServerMini::BlockObject::LinkType::SourceInternalBottomBlock:
                            newDirection=CatchChallenger::Direction::Direction_move_at_bottom;
                        break;
                        case MapServerMini::BlockObject::LinkType::SourceLeftMap:
                        case MapServerMini::BlockObject::LinkType::SourceInternalLeftBlock:
                            newDirection=CatchChallenger::Direction::Direction_move_at_left;
                        break;
                        default:
                        break;
                    }

                    //get the item in front of to continue the progression
                    {
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
                                        ActionsAction::add_to_inventory(api,itemOnMap.item);
                                    }
                            }
                        }
                        else
                            std::cerr << "The next case is: " << std::to_string(x) << "," << std::to_string(y) << " can't move" << std::endl;
                    }

                    if(!player.target.bestPath.empty())
                    {
                        const uint16_t &currentCodeZone=playerMap->step.at(1).map[player.x+player.y*playerMap->width];
                        if(currentCodeZone==0)
                            abort();
                        const MapServerMini::BlockObject * blockObject=playerMap->step.at(1).layers.at(currentCodeZone-1).blockObject;

                        std::cout << "The player is into the zone: " << playerMap->map_file << " Block " << std::to_string(blockObject->id+1) << std::endl;
                        const MapServerMini::BlockObject * front=player.target.bestPath.front();
                        std::cout << "The next zone to remove is: " << front->map->map_file << " Block " << std::to_string(front->id+1) << std::endl;

                        if(blockObject==front)
                            player.target.bestPath.erase(player.target.bestPath.cbegin());
                    }

                    //do the final move
                    switch(player.target.linkPoint.type)
                    {
                        case MapServerMini::BlockObject::LinkType::SourceTopMap:
                        case MapServerMini::BlockObject::LinkType::SourceInternalTopBlock:
                        case MapServerMini::BlockObject::LinkType::SourceRightMap:
                        case MapServerMini::BlockObject::LinkType::SourceInternalRightBlock:
                        case MapServerMini::BlockObject::LinkType::SourceBottomMap:
                        case MapServerMini::BlockObject::LinkType::SourceInternalBottomBlock:
                        case MapServerMini::BlockObject::LinkType::SourceLeftMap:
                        case MapServerMini::BlockObject::LinkType::SourceInternalLeftBlock:
                            if(ActionsAction::canGoTo(api,newDirection,*playerMap,player.x,player.y,true,true))
                            {
                                ActionsAction::move(api,newDirection,&playerMap,&player.x,&player.y,true,true);
                                api->newDirection(newDirection);
                            }
                            else
                            {
                                if(player.target.bestPath.size()>2)
                                    abort();
                                std::cerr << "The current case is: " << std::to_string(player.x) << "," << std::to_string(player.y) << " can't do the next step for internal block change" << std::endl;
                                player.target.bestPath.clear();
                            }
                        break;
                        default:
                        break;
                    }

                    //change the look below
                    switch(newDirection)
                    {
                        case CatchChallenger::Direction::Direction_move_at_top:
                        case CatchChallenger::Direction::Direction_look_at_top:
                            api->newDirection(CatchChallenger::Direction::Direction_look_at_top);
                        break;
                        case CatchChallenger::Direction::Direction_move_at_bottom:
                        case CatchChallenger::Direction::Direction_look_at_bottom:
                            api->newDirection(CatchChallenger::Direction::Direction_look_at_bottom);
                        break;
                        case CatchChallenger::Direction::Direction_move_at_right:
                        case CatchChallenger::Direction::Direction_look_at_right:
                            api->newDirection(CatchChallenger::Direction::Direction_look_at_right);
                        break;
                        case CatchChallenger::Direction::Direction_move_at_left:
                        case CatchChallenger::Direction::Direction_look_at_left:
                            api->newDirection(CatchChallenger::Direction::Direction_look_at_left);
                        break;
                        default:
                        abort();
                        break;
                    }

                    if(!player.target.bestPath.empty())
                    {
                        if(playerMap->step.size()<2)
                            abort();
                        const uint16_t &currentCodeZone=playerMap->step.at(1).map[player.x+player.y*playerMap->width];
                        if(currentCodeZone==0)
                            abort();
                        const MapServerMini::BlockObject * blockObject=playerMap->step.at(1).layers.at(currentCodeZone-1).blockObject;

                        std::vector<DestinationForPath> destinations;
                        std::vector<MapServerMini::BlockObject::LinkPoint> pointsList;
                        uint8_t o=api->getDirection();
                        while(o>4)
                            o-=4;
                        if(!player.target.bestPath.empty())
                        {
                            //if the target is on the same block
                            const MapServerMini::BlockObject * nextBlock=player.target.bestPath.front();
                            if(blockObject->links.find(nextBlock)==blockObject->links.cend())
                                abort();
                            const std::vector<MapServerMini::BlockObject::LinkCondition> &linkConditions=blockObject->links.at(nextBlock).linkConditions;
                            unsigned int conditionIndex=0;
                            while(conditionIndex<linkConditions.size())
                            {
                                const MapServerMini::BlockObject::LinkCondition &condition=linkConditions.at(conditionIndex);
                                if(ActionsAction::mapConditionIsRepected(api,condition.condition))
                                {
                                    pointsList=condition.points;
                                    if(pointsList.empty())
                                        abort();
                                    unsigned int index=0;
                                    while(index<pointsList.size())
                                    {
                                        const MapServerMini::BlockObject::LinkPoint &point=pointsList.at(index);
                                        DestinationForPath destinationForPath;
                                        destinationForPath.destination_x=point.x;
                                        destinationForPath.destination_y=point.y;
                                        switch(point.type)
                                        {
                                            case MapServerMini::BlockObject::LinkType::SourceTopMap:
                                            case MapServerMini::BlockObject::LinkType::SourceInternalTopBlock:
                                                destinationForPath.destination_orientation=CatchChallenger::Orientation::Orientation_top;
                                            break;
                                            case MapServerMini::BlockObject::LinkType::SourceRightMap:
                                            case MapServerMini::BlockObject::LinkType::SourceInternalRightBlock:
                                                destinationForPath.destination_orientation=CatchChallenger::Orientation::Orientation_right;
                                            break;
                                            case MapServerMini::BlockObject::LinkType::SourceBottomMap:
                                            case MapServerMini::BlockObject::LinkType::SourceInternalBottomBlock:
                                                destinationForPath.destination_orientation=CatchChallenger::Orientation::Orientation_bottom;
                                            break;
                                            case MapServerMini::BlockObject::LinkType::SourceLeftMap:
                                            case MapServerMini::BlockObject::LinkType::SourceInternalLeftBlock:
                                                destinationForPath.destination_orientation=CatchChallenger::Orientation::Orientation_left;
                                            break;
                                            default:
                                                destinationForPath.destination_orientation=CatchChallenger::Orientation::Orientation_none;
                                            break;
                                        }
                                        destinations.push_back(destinationForPath);

                                        index++;
                                    }
                                }
                                conditionIndex++;
                            }
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
                            player.target.linkPoint=pointsList.at(destinationIndexSelected);
                            player.target.localStep=returnPath;
                        }
                        else
                            std::cerr << "Unable to search the next path, the target should be into the current block and map" << std::endl;
                    }
                }
            }

            const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
            if(selectedItems.size()!=1)
                return;
            const QString &pseudo=selectedItems.at(0)->text();
            if(!pseudoToBot.contains(pseudo))
                return;
            MultipleBotConnection::CatchChallengerClient * client=pseudoToBot.value(pseudo);
            if(haveChange && api==client->api)
            {
                if(ui->trackThePlayer->isChecked())
                {
                    mapId=player.mapId;
                    updateMapInformation();
                    updatePlayerInformation();
                }
                updatePlayerMap(true);
            }

            if(player.target.localStep.empty() && player.target.bestPath.empty() && player.target.blockObject!=NULL && player.target.type!=ActionsBotInterface::GlobalTarget::GlobalTargetType::None)
            {
                //get the next tile
                if(actionsAction->id_map_to_map.find(player.mapId)==actionsAction->id_map_to_map.cend())
                    return;
                const std::string &mapStdString=actionsAction->id_map_to_map.at(player.mapId);
                CatchChallenger::CommonMap *map=actionsAction->map_list.at(mapStdString);
                const MapServerMini *mapServer=static_cast<MapServerMini *>(map);
                uint8_t x=player.x;
                uint8_t y=player.y;
                CatchChallenger::Direction direction;
                switch(api->getDirection())
                {
                    case CatchChallenger::Direction_look_at_left:
                        direction=CatchChallenger::Direction_move_at_left;
                    break;
                    case CatchChallenger::Direction_look_at_right:
                        direction=CatchChallenger::Direction_move_at_right;
                    break;
                    case CatchChallenger::Direction_look_at_top:
                        direction=CatchChallenger::Direction_move_at_top;
                    break;
                    case CatchChallenger::Direction_look_at_bottom:
                        direction=CatchChallenger::Direction_move_at_bottom;
                    break;
                    default:
                        direction=api->getDirection();
                }
                if(!ActionsAction::moveWithoutTeleport(api,direction,&mapServer,&x,&y,false,false))
                    abort();

                CatchChallenger::Player_private_and_public_informations &playerInformations=api->get_player_informations();
                const QPair<uint8_t,uint8_t> QtPoint(x,y);
                std::pair<uint8_t,uint8_t> p(x,y);
                QString mapQtString=DatapackClientLoader::datapackLoader.getDatapackPath()+DatapackClientLoader::datapackLoader.getMainDatapackPath()+QString::fromStdString(mapServer->map_file);
                if(!mapQtString.endsWith(".tmx"))
                    mapQtString+=".tmx";
                //finish correctly the step
                api->stopMove();
                switch(player.target.type)
                {
                    case ActionsBotInterface::GlobalTarget::GlobalTargetType::ItemOnMap:
                    {
                        bool found=false;
                        for(auto&entry:mapServer->pointOnMap_Item)
                        {
                            const MapServerMini::ItemOnMap &itemOnMap=entry.second;
                            if(itemOnMap.indexOfItemOnMap==player.target.extra)
                            {
                                if(!itemOnMap.infinite)
                                    player_private_and_public_informations.itemOnMap.insert(itemOnMap.indexOfItemOnMap);
                                api->stopMove();
                                api->takeAnObjectOnMap();
                                ActionsAction::add_to_inventory(api,itemOnMap.item);
                                found=true;
                                break;
                            }
                        }
                        if(!found)
                        {
                            std::cerr << "On the next tile don't found the montioned item on map" << std::endl;
                            abort();
                        }
                    }
                    break;
                    case ActionsBotInterface::GlobalTarget::GlobalTargetType::Heal:
                        api->heal();
                        player.fightEngine->healAllMonsters();
                    break;
                    case ActionsBotInterface::GlobalTarget::GlobalTargetType::Dirt:
                    {
                        bool haveSeedToPlant=false;
                        const uint32_t &itemId=BotTargetList::getSeedToPlant(api,&haveSeedToPlant);
                        if(!DatapackClientLoader::datapackLoader.itemToPlants.contains(itemId))
                            abort();
                        const uint8_t &plant=DatapackClientLoader::datapackLoader.itemToPlants.value(itemId);
                        if(!DatapackClientLoader::datapackLoader.plantOnMap.contains(mapQtString))
                            abort();
                        if(!DatapackClientLoader::datapackLoader.plantOnMap.value(mapQtString).contains(QtPoint))
                            abort();
                        const uint16_t &indexOnMapPlant=DatapackClientLoader::datapackLoader.plantOnMap.value(mapQtString).value(QtPoint);
                        if(playerInformations.plantOnMap.find(indexOnMapPlant)==playerInformations.plantOnMap.cend())
                        {
                            ActionsAction::remove_to_inventory(api,itemId);

                            ActionsBotInterface::Player::SeedInWaiting seedInWaiting;
                            seedInWaiting.indexOnMap=indexOnMapPlant;
                            seedInWaiting.seedItemId=itemId;
                            player.seed_in_waiting << seedInWaiting;
                            std::cout << "useSeed(): " << std::to_string(x) << "," << std::to_string(y) << std::endl;
                            api->useSeed(plant);
                            CatchChallenger::PlayerPlant playerPlant;
                            playerPlant.mature_at=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()/1000+CatchChallenger::CommonDatapack::commonDatapack.plants.at(plant).fruits_seconds;
                            playerPlant.plant=plant;
                            playerInformations.plantOnMap[indexOnMapPlant]=playerPlant;
                        }
                        else
                        {
                            if(CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer==true)
                                abort();
                        }
                    }
                    break;
                    case ActionsBotInterface::GlobalTarget::GlobalTargetType::Plant:
                    {
                        const uint8_t &plant=DatapackClientLoader::datapackLoader.itemToPlants.value(player.target.extra/*itemUsed*/);
                        if(!DatapackClientLoader::datapackLoader.plantOnMap.contains(mapQtString))
                            abort();
                        if(!DatapackClientLoader::datapackLoader.plantOnMap.value(mapQtString).contains(QtPoint))
                            abort();
                        const uint16_t &indexOnMapPlant=DatapackClientLoader::datapackLoader.plantOnMap.value(mapQtString).value(QtPoint);
                        if(CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer==true)
                            if(playerInformations.plantOnMap.find(indexOnMapPlant)==playerInformations.plantOnMap.cend())
                                abort();
                        std::cout << "collectMaturePlant(): " << std::to_string(x) << "," << std::to_string(y) << std::endl;
                        api->collectMaturePlant();
                        if(CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer==false)
                        {
                            ActionsBotInterface::Player::ClientPlantInCollecting clientPlantInCollecting;
                            clientPlantInCollecting.indexOnMap=indexOnMapPlant;
                            clientPlantInCollecting.plant_id=plant;
                            clientPlantInCollecting.seconds_to_mature=0;
                            player.plant_collect_in_waiting << clientPlantInCollecting;
                        }
                        else
                        {
                            if(!DatapackClientLoader::datapackLoader.plantOnMap.contains(mapQtString))
                                abort();
                            if(!DatapackClientLoader::datapackLoader.plantOnMap.value(mapQtString).contains(QtPoint))
                                abort();
                            playerInformations.plantOnMap.erase(indexOnMapPlant);
                        }
                    }
                    break;
                    default:
                        QMessageBox::critical(this,tr("Not coded"),tr("This target type is not coded (1): %1").arg(player.target.type));
                        player.target.blockObject=NULL;
                        player.target.extra=0;
                        player.target.linkPoint.x=0;
                        player.target.linkPoint.y=0;
                        player.target.linkPoint.type=MapServerMini::BlockObject::LinkType::SourceNone;
                        player.target.type=ActionsBotInterface::GlobalTarget::GlobalTargetType::None;
                        return;
                    break;
                }
                player.target.blockObject=NULL;
                player.target.extra=0;
                player.target.linkPoint.x=0;
                player.target.linkPoint.y=0;
                player.target.linkPoint.type=MapServerMini::BlockObject::LinkType::SourceNone;
                player.target.type=ActionsBotInterface::GlobalTarget::GlobalTargetType::None;

                if(api==client->api)
                {
                    updatePlayerInformation();
                    updatePlayerMap(true);
                }
            }
        }
    }
}
