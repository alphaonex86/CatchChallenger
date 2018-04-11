#include "BotTargetList.h"
#include "ui_BotTargetList.h"
#include "../../client/base/interface/DatapackClientLoader.h"
#include "../../client/fight/interface/ClientFightEngine.h"
#include "../../general/base/CommonSettingsServer.h"
#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/FacilityLib.h"
#include "MapBrowse.h"

#include <chrono>
#include <QMessageBox>
#include <stdlib.h>
#include "MainWindow.h"

void BotTargetList::updatePlayerStep()
{
    if(MainWindow::multipleBotConnexion.haveAnError())
        return;
    CatchChallenger::Api_protocol * apiSelectedClient=NULL;
    const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
    if(selectedItems.size()==1)
    {
        const QString &pseudo=selectedItems.at(0)->text();
        if(!pseudoToBot.contains(pseudo))
            return;
        MultipleBotConnection::CatchChallengerClient * currentSelectedclient=pseudoToBot.value(pseudo);
        apiSelectedClient=currentSelectedclient->api;
    }
    if(ui->bots->count()==1)
    {
        QListWidgetItem * item=ui->bots->itemAt(0,0);
        if(!item->isSelected())
        {
            item->setSelected(true);
            on_bots_itemSelectionChanged();
            ui->groupBoxBot->setVisible(false);
        }
    }

    QHashIterator<CatchChallenger::Api_protocol *,ActionsAction::Player> i(actionsAction->clientList);
    while (i.hasNext()) {
        i.next();
        CatchChallenger::Api_protocol *api=i.key();
        ActionsAction::Player &player=actionsAction->clientList[i.key()];
        if(actionsAction->id_map_to_map.find(player.mapId)==actionsAction->id_map_to_map.cend())
            abort();
        if(api->getCaracterSelected())
        {
            CatchChallenger::Player_private_and_public_informations &playerInformations=api->get_player_informations();
            if(player.canMoveOnMap)
            {
                bool haveChange=false;
                if(player.target.localStep.empty())
                {
                    //was: !player.target.bestPath.empty() but block the first step
                    if(!player.target.bestPath.empty())/// need set out of here, \see startPlayerMove()
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
                        //look to move
                        CatchChallenger::Direction newDirectionToMove=newDirection;
                        switch(newDirection)
                        {
                            case CatchChallenger::Direction_look_at_bottom:
                                newDirectionToMove=CatchChallenger::Direction::Direction_move_at_bottom;
                            break;
                            case CatchChallenger::Direction_look_at_left:
                                newDirectionToMove=CatchChallenger::Direction::Direction_move_at_left;
                            break;
                            case CatchChallenger::Direction_look_at_right:
                                newDirectionToMove=CatchChallenger::Direction::Direction_move_at_right;
                            break;
                            case CatchChallenger::Direction_look_at_top:
                                newDirectionToMove=CatchChallenger::Direction::Direction_move_at_top;
                            break;
                            default:
                            break;
                        }

                        //get the item in front of to continue the progression
                        {
                            const MapServerMini * destMap=playerMap;
                            uint8_t x=player.x,y=player.y;
                            if(ActionsAction::move(api,newDirectionToMove,&destMap,&x,&y,false,false))
                            {
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
                                //std::cout << "The next case is: " << std::to_string(x) << "," << std::to_string(y) << std::endl;
                                std::pair<uint8_t,uint8_t> p(x,y);
                                if(playerMap->pointOnMap_Item.find(p)!=playerMap->pointOnMap_Item.cend())
                                {
                                    //std::cout << "The next case is: " << std::to_string(x) << "," << std::to_string(y) << ", have item on it" << std::endl;
                                    const MapServerMini::ItemOnMap &itemOnMap=playerMap->pointOnMap_Item.at(p);
                                    if(!itemOnMap.infinite && itemOnMap.visible)
                                        if(playerInformations.itemOnMap.find(itemOnMap.indexOfItemOnMap)==playerInformations.itemOnMap.cend())
                                        {
                                            /*std::cout << "The next case is: " << std::to_string(x) << "," << std::to_string(y)
                                                      << ", take the item, bot, itemOnMap.indexOfItemOnMap: " << std::to_string(itemOnMap.indexOfItemOnMap)
                                                      << ", item: " << std::to_string(itemOnMap.item)
                                                      << ", pseudo: " << api->getPseudo().toStdString() << std::endl;*/
                                            playerInformations.itemOnMap.insert(itemOnMap.indexOfItemOnMap);
                                            api->newDirection(CatchChallenger::MoveOnTheMap::directionToDirectionLook(newDirection));//move to look into the right next direction
                                            api->takeAnObjectOnMap();
                                            ActionsAction::add_to_inventory(api,itemOnMap.item);
                                        }
                                }
                            }
                            /*else
                                std::cerr << "The next case is: " << std::to_string(x) << "," << std::to_string(y) << " can't move" << std::endl;*/
                        }

                        if(!player.target.bestPath.empty())
                        {
                            const uint16_t &currentCodeZone=playerMap->step.at(1).map[player.x+player.y*playerMap->width];
                            if(currentCodeZone==0)
                                abort();
                            const MapServerMini::BlockObject * blockObject=playerMap->step.at(1).layers.at(currentCodeZone-1).blockObject;

                            //std::cout << "The player is into the zone: " << playerMap->map_file << " Block " << std::to_string(blockObject->id+1) << std::endl;
                            const MapServerMini::BlockObject * front=player.target.bestPath.front();
                            //std::cout << "The next zone to remove is: " << front->map->map_file << " Block " << std::to_string(front->id+1) << std::endl;

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
                                if(ActionsAction::canGoTo(api,newDirectionToMove,*playerMap,player.x,player.y,true,true))
                                {
                                    ActionsAction::move(api,newDirectionToMove,&playerMap,&player.x,&player.y,true,true);
                                    if(CatchChallenger::MoveOnTheMap::getLedge(*playerMap,player.x,player.y)==CatchChallenger::ParsedLayerLedges_NoLedges)
                                        api->newDirection(newDirectionToMove);
                                    //enter into new zone, drop the entry
                                    if(player.target.bestPath.empty())
                                        abort();
                                    player.target.bestPath.erase(player.target.bestPath.cbegin());
                                    player.mapId=playerMap->id;
                                    ActionsAction::checkOnTileEvent(player);
                                }
                                else
                                {
                                    if(player.target.bestPath.size()>2)
                                        abort();
                                    //std::cerr << "The current case is: " << std::to_string(player.x) << "," << std::to_string(player.y) << " can't do the next step for internal block change" << std::endl;
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

                        if(playerMap->step.size()<2)
                            abort();
                        const uint16_t &currentCodeZone=playerMap->step.at(1).map[player.x+player.y*playerMap->width];
                        if(currentCodeZone==0)
                            abort();
                        MapServerMini::BlockObject * blockObject=playerMap->step.at(1).layers.at(currentCodeZone-1).blockObject;

                        if(!player.target.bestPath.empty())
                        {
                            std::vector<MapServerMini::BlockObject::DestinationForPath> destinations;
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
                                            MapServerMini::BlockObject::DestinationForPath destinationForPath;
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
                                const std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > &returnPath=pathFindingWithCache(
                                            blockObject,
                                            static_cast<CatchChallenger::Orientation>(o),player.x,player.y,
                                            destinations,
                                            destinationIndexSelected,
                                            &ok);
                                if(!ok)
                                    abort();
                                player.target.linkPoint=pointsList.at(destinationIndexSelected);
                                player.target.localStep=returnPath;
                                BotTargetList::finishTheLocalStep(player);
                            }
                            else
                                std::cerr << "Unable to search the next path, the target should be into the current block and map" << std::endl;
                        }
                        else
                        {
                            //have finish the path and is on the final blockObject, then do internal path finding
                            if(blockObject==player.target.blockObject)
                            {
                                //not precise point for wilds monster
                                if(player.target.type==ActionsBotInterface::GlobalTarget::GlobalTargetType::WildMonster)
                                {
                                    wildMonsterTarget(player);
                                    if(apiSelectedClient==api)
                                        ui->label_action->setText("Start this: "+QString::fromStdString(BotTargetList::stepToString(player.target.localStep)));
                                }
                                else
                                {
                                    //do the final resolution path as do into startPlayerMove()
                                    std::vector<MapServerMini::BlockObject::DestinationForPath> destinations;
                                    std::vector<MapServerMini::BlockObject::LinkPoint> pointsList;
                                    const std::pair<uint8_t,uint8_t> &point=getNextPosition(blockObject,player.target/*hop list, first is the next hop*/);
                                    MapServerMini::BlockObject::DestinationForPath destinationForPath;
                                    destinationForPath.destination_orientation=CatchChallenger::Orientation::Orientation_none;
                                    destinationForPath.destination_x=point.first;
                                    destinationForPath.destination_y=point.second;
                                    destinations.push_back(destinationForPath);
                                    MapServerMini::BlockObject::LinkPoint linkPoint;
                                    linkPoint.type=MapServerMini::BlockObject::LinkType::SourceNone;
                                    linkPoint.x=point.first;
                                    linkPoint.y=point.second;
                                    pointsList.push_back(linkPoint);
                                    std::cout << "player.target.bestPath.empty(): blockObject!=player.target.blockObject && player.target.type!=ActionsBotInterface::GlobalTarget::GlobalTargetType::WildMonster" << std::endl;
                                    if(pointsList.size()!=destinations.size())
                                        abort();
                                    uint8_t o=api->getDirection();
                                    while(o>4)
                                        o-=4;
                                    bool ok=false;
                                    unsigned int destinationIndexSelected=0;
                                    const std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > &returnPath=pathFindingWithCache(
                                                blockObject,
                                                static_cast<CatchChallenger::Orientation>(o),player.x,player.y,
                                                destinations,
                                                destinationIndexSelected,
                                                &ok);
                                    if(!ok)
                                        abort();
                                    player.target.linkPoint=pointsList.at(destinationIndexSelected);
                                    player.target.localStep=returnPath;
                                    BotTargetList::finishTheLocalStep(player);
                                }
                            }
                        }
                    }
                }

                if(haveChange && api==apiSelectedClient)
                {
                    if(ui->trackThePlayer->isChecked())
                    {
                        mapId=player.mapId;
                        updateMapInformation();
                        updatePlayerInformation();
                    }
                    updatePlayerMap(true);
                }

                if(player.target.localStep.empty() && player.target.bestPath.empty()
                        && player.target.blockObject!=NULL && player.target.type!=ActionsBotInterface::GlobalTarget::GlobalTargetType::None)
                {
                    //get the next tile
                    if(player.target.type==ActionsBotInterface::GlobalTarget::GlobalTargetType::None)
                    {
                        std::cerr << "player.target.type==ActionsBotInterface::GlobalTarget::GlobalTargetType::None: " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
                        abort();
                    }
                    if(actionsAction->id_map_to_map.find(player.mapId)==actionsAction->id_map_to_map.cend())
                        return;
                    if(player.target.type==ActionsBotInterface::GlobalTarget::GlobalTargetType::None)
                    {
                        std::cerr << "player.target.type==ActionsBotInterface::GlobalTarget::GlobalTargetType::None: " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
                        abort();
                    }
                    const std::string &mapStdString=actionsAction->id_map_to_map.at(player.mapId);
                    CatchChallenger::CommonMap *map=actionsAction->map_list.at(mapStdString);
                    const MapServerMini *mapServer=static_cast<MapServerMini *>(map);
                    uint8_t x=player.x;
                    uint8_t y=player.y;
                    CatchChallenger::Direction direction;
                    if(player.target.type==ActionsBotInterface::GlobalTarget::GlobalTargetType::None)
                    {
                        std::cerr << "player.target.type==ActionsBotInterface::GlobalTarget::GlobalTargetType::None: " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
                        abort();
                    }
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
                    if(player.target.type==ActionsBotInterface::GlobalTarget::GlobalTargetType::None)
                    {
                        std::cerr << "player.target.type==ActionsBotInterface::GlobalTarget::GlobalTargetType::None: " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
                        abort();
                    }
                    if(!ActionsAction::moveWithoutTeleport(api,direction,&mapServer,&x,&y,false,false))
                    {
                        std::cerr << "Unable to do the last move before action: " << mapServer->map_file << "(" << std::to_string(x) << "," << std::to_string(y) << ") to " << std::to_string(direction) << std::endl;
                        ActionsAction::moveWithoutTeleport(api,direction,&mapServer,&x,&y,false,false);
                        abort();
                    }

                    if(player.target.type==ActionsBotInterface::GlobalTarget::GlobalTargetType::None)
                    {
                        std::cerr << "player.target.type==ActionsBotInterface::GlobalTarget::GlobalTargetType::None: " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
                        abort();
                    }
                    CatchChallenger::Player_private_and_public_informations &playerInformations=api->get_player_informations();
                    const QPair<uint8_t,uint8_t> QtPoint(x,y);
                    std::pair<uint8_t,uint8_t> p(x,y);
                    QString mapQtString=DatapackClientLoader::datapackLoader.getDatapackPath()+DatapackClientLoader::datapackLoader.getMainDatapackPath()+QString::fromStdString(mapServer->map_file);
                    if(!mapQtString.endsWith(".tmx"))
                        mapQtString+=".tmx";
                    //finish correctly the step
                    if(player.target.type==ActionsBotInterface::GlobalTarget::GlobalTargetType::None)
                    {
                        std::cerr << "player.target.type==ActionsBotInterface::GlobalTarget::GlobalTargetType::None: " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
                        abort();
                    }
                    api->stopMove();
                    if(player.target.type==ActionsBotInterface::GlobalTarget::GlobalTargetType::None)
                    {
                        std::cerr << "player.target.type==ActionsBotInterface::GlobalTarget::GlobalTargetType::None: " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
                        abort();
                    }
                    switch(player.target.type)
                    {
                        case ActionsBotInterface::GlobalTarget::GlobalTargetType::ItemOnMap:
                        {
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            {
                                std::unordered_set<uint32_t> known_indexOfItemOnMap;
                                for ( const auto &item : mapServer->pointOnMap_Item )
                                {
                                    const MapServerMini::ItemOnMap &itemOnMap=item.second;
                                    if(known_indexOfItemOnMap.find(itemOnMap.indexOfItemOnMap)!=known_indexOfItemOnMap.cend())
                                        abort();
                                    known_indexOfItemOnMap.insert(itemOnMap.indexOfItemOnMap);
                                }
                            }
                            #endif
                            bool found=false;
                            bool alreadyTake=false;
                            for(auto&entry:mapServer->pointOnMap_Item)
                            {
                                const MapServerMini::ItemOnMap &itemOnMap=entry.second;
                                if(itemOnMap.indexOfItemOnMap==player.target.extra)
                                {
                                    found=true;
                                    if(!itemOnMap.infinite)
                                    {
                                        if(playerInformations.itemOnMap.find(itemOnMap.indexOfItemOnMap)==playerInformations.itemOnMap.cend())
                                            playerInformations.itemOnMap.insert(itemOnMap.indexOfItemOnMap);
                                        else
                                        {
                                            alreadyTake=true;
                                            break;
                                        }
                                    }
                                    if(!alreadyTake)
                                    {
                                        /*std::cout << "The next case is: " << std::to_string(x) << "," << std::to_string(y)
                                                  << ", take the item, wish, itemOnMap.indexOfItemOnMap: " << std::to_string(itemOnMap.indexOfItemOnMap)
                                                  << ", item: " << std::to_string(itemOnMap.item)
                                                  << ", pseudo: " << api->getPseudo().toStdString() << std::endl;*/
                                        api->stopMove();
                                        api->takeAnObjectOnMap();
                                        ActionsAction::add_to_inventory(api,itemOnMap.item);
                                    }
                                    break;
                                }
                            }
                            if(!found)
                            {
                                std::cerr << "On the next tile don't found the montioned item on map" << std::endl;
                                abort();
                            }
                            /*if(!alreadyTake)
                            {
                                std::cerr << "On the next tile don't found the montioned item on map" << std::endl;
                                abort();
                            }can be take by auto take*/
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
                                //std::cout << api->getPseudo().toStdString() << ", useSeed(): " << std::to_string(x) << "," << std::to_string(y) << std::endl;
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
                        case ActionsBotInterface::GlobalTarget::GlobalTargetType::WildMonster:
                        {
                            bool returnedValue=wildMonsterTarget(player);
                            if(apiSelectedClient==api)
                                ui->label_action->setText("Start this: "+QString::fromStdString(BotTargetList::stepToString(player.target.localStep)));
                            if(returnedValue==true)
                                return;
                        }
                        break;
                        case ActionsBotInterface::GlobalTarget::GlobalTargetType::Fight:
                        {
                            const uint32_t &fightId=player.target.extra;
                            if(!ActionsAction::haveBeatBot(api,fightId) && player.fightEngine->getAbleToFight())
                            {
                                qDebug() <<  "is now in fight by target with: " << fightId;
                                player.canMoveOnMap=false;
                                player.api->stopMove();
                                player.api->requestFight(fightId);
                                QList<CatchChallenger::PlayerMonster> botFightMonstersTransformed;
                                const std::vector<CatchChallenger::BotFight::BotFightMonster> &monsters=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.botFights.at(fightId).monsters;
                                unsigned int index=0;
                                while(index<monsters.size())
                                {
                                    botFightMonstersTransformed << CatchChallenger::FacilityLib::botFightMonsterToPlayerMonster(monsters.at(index),CatchChallenger::ClientFightEngine::getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(monsters.at(index).id),monsters.at(index).level));
                                    index++;
                                }
                                player.fightEngine->setBotMonster(botFightMonstersTransformed,fightId);
                                player.lastFightAction.restart();
                            }
                            else
                            {
                                ActionsAction::resetTarget(player.target);
                                stopAll();
                                QMessageBox::critical(this,tr("Already beaten"),tr("This bot is already beaten (1): %1").arg(fightId));
                                return;
                            }
                        }
                        break;
                        default:
                            ActionsAction::resetTarget(player.target);
                            stopAll();
                            QMessageBox::critical(this,tr("Not coded"),tr("This target type is not coded (1): %1").arg(player.target.type));
                            return;
                        break;
                    }
                    ActionsAction::resetTarget(player.target);

                    if(api==apiSelectedClient)
                    {
                        updatePlayerInformation();
                        updatePlayerMap(true);
                    }
                }
            }
            else
            {
                //can't move: can be in fight
                if(player.fightEngine->isInFight())
                {
                    if(apiSelectedClient==api && !ui->groupBoxFight->isVisible())
                        updateFightStats();
                    if((player.lastFightAction.elapsed()/1000)>(5/*5s*/) && !player.fightEngine->catchInProgress())
                    {
                        player.lastFightAction.restart();
                        //do the code here
                        CatchChallenger::PlayerMonster *monster=player.fightEngine->getCurrentMonster();
                        if(monster==NULL)
                        {
                            std::cerr << ", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__)+"NULL pointer at updateCurrentMonsterInformation()" << std::endl;
                            return;
                        }
                        CatchChallenger::PublicPlayerMonster *othermonster=player.fightEngine->getOtherMonster();
                        if(othermonster==NULL)
                        {
                            std::cerr << ", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__)+"NULL pointer at updateCurrentMonsterInformation()" << std::endl;
                            return;
                        }
                        const CatchChallenger::Monster::Stat &wildMonsterStat=CatchChallenger::ClientFightEngine::getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters.at(othermonster->monster),othermonster->level);

                        //try capture
                        bool tryCaptureWithItem=false;
                        uint16_t itemToCapture=0;
                        uint32_t currentDiff=2000000;
                        if(player.fightEngine->isInFightWithWild())
                        {
                            if(playerInformations.encyclopedia_monster!=NULL)
                                if(!(playerInformations.encyclopedia_monster[othermonster->monster/8] & (1<<(7-othermonster->monster%8))))
                                {
                                    float bonusStat=1.0;
                                    if(othermonster->buffs.size())
                                    {
                                        bonusStat=0;
                                        unsigned int index=0;
                                        while(index<othermonster->buffs.size())
                                        {
                                            const CatchChallenger::PlayerBuff &playerBuff=othermonster->buffs.at(index);
                                            if(CatchChallenger::CommonDatapack::commonDatapack.monsterBuffs.find(playerBuff.buff)!=CatchChallenger::CommonDatapack::commonDatapack.monsterBuffs.cend())
                                            {
                                                const CatchChallenger::Buff &buff=CatchChallenger::CommonDatapack::commonDatapack.monsterBuffs.at(playerBuff.buff);
                                                if(playerBuff.level>0 && playerBuff.level<=buff.level.size())
                                                    bonusStat+=buff.level.at(playerBuff.level-1).capture_bonus;
                                                else
                                                {
                                                    std::cerr << "Buff level for wild monter not found: " << std::to_string(playerBuff.buff) << " at level " << std::to_string(playerBuff.level) << std::endl;
                                                    bonusStat+=1.0;
                                                }
                                            }
                                            else
                                            {
                                                std::cerr << "Buff for wild monter not found: " << std::to_string(playerBuff.buff) << std::endl;
                                                bonusStat+=1.0;
                                            }
                                            index++;
                                        }
                                        bonusStat/=othermonster->buffs.size();
                                    }

                                    for(const auto& n:playerInformations.items) {
                                        const CATCHCHALLENGER_TYPE_ITEM &item=n.first;
                                        const uint32_t &quantity=n.second;
                                        if(CatchChallenger::CommonDatapack::commonDatapack.items.trap.find(item)!=CatchChallenger::CommonDatapack::commonDatapack.items.trap.cend())
                                        {
                                            const uint32_t maxTempRate=12;
                                            const uint32_t minTempRate=5;
                                            //const uint32_t tryCapture=4;
                                            const CatchChallenger::Trap &trap=CatchChallenger::CommonDatapack::commonDatapack.items.trap.at(item);
                                            const uint32_t catch_rate=(uint32_t)CatchChallenger::CommonDatapack::commonDatapack.monsters.at(othermonster->monster).catch_rate;
                                            uint32_t tempRate=(catch_rate*(wildMonsterStat.hp*maxTempRate-othermonster->hp*minTempRate)*bonusStat*trap.bonus_rate)/(wildMonsterStat.hp*maxTempRate);
                                            bool valid=false;
                                            if(quantity>20)
                                                valid=true;
                                            else if(quantity>7 && tempRate>50)
                                                valid=true;
                                            else if(tempRate>150)
                                                valid=true;
                                            if(valid)
                                            {
                                                uint32_t newDiff=0;
                                                if(tempRate<=255)
                                                    newDiff=255-tempRate;
                                                else
                                                    newDiff=tempRate-255;
                                                if(newDiff<currentDiff)
                                                {
                                                    currentDiff=newDiff;
                                                    tryCaptureWithItem=true;
                                                    itemToCapture=item;
                                                }
                                            }
                                        }
                                    }
                                }
                        }
                        if(player.fightEngine->getPlayerMonster().size()>=CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters)
                            if(playerInformations.warehouse_playerMonster.size()>=CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters)
                                tryCaptureWithItem=false;
                        if(tryCaptureWithItem)
                        {
                            //api->useObject(itemToCapture);-> do into player.fightEngine->tryCatchClient(
                            ActionsAction::remove_to_inventory(api,itemToCapture);
                            std::cout << "Start this: Try capture with: " << std::to_string(itemToCapture) << ", now have only quantity: " << std::to_string(ActionsAction::itemQuantity(api,itemToCapture)) << std::endl;
                            if(!player.fightEngine->tryCatchClient(itemToCapture))//api->useObject(item); call into it
                            {
                                std::cerr << "!player.fightEngine->tryCatchClient(itemToCapture)" << std::endl;
                                abort();
                            }
                            ui->label_action->setText("Start this: Try capture with: "+QString::number(itemToCapture)+" for the monster "+QString::number(othermonster->monster));
                            if(api==apiSelectedClient)
                                updatePlayerInformation();
                            return;//need wait the server reply, monsterCatch(const bool &success)
                        }
                        else
                        {
                            //try an attack
                            unsigned int index=0;
                            bool useTheRescueSkill=true;
                            uint16_t skillUsed=0;
                            uint32_t skillPoint=0;
                            while(index<monster->skills.size())
                            {
                                const CatchChallenger::PlayerMonster::PlayerSkill &skill=monster->skills.at(index);
                                if(skill.endurance>0)
                                {
                                    const CatchChallenger::Skill &skillDatapack=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.at(skill.skill);
                                    const CatchChallenger::Skill::SkillList &skillList=skillDatapack.level.at(skill.level-1);
                                    unsigned int tempSkillPoint=0;
                                    unsigned int skillIndex=0;
                                    while(skillIndex<skillList.life.size())
                                    {
                                        const CatchChallenger::Skill::Life &lifeEffect=skillList.life.at(skillIndex);
                                        unsigned int skillDamage=0;
                                        if(lifeEffect.effect.type==CatchChallenger::QuantityType_Quantity)
                                            skillDamage=lifeEffect.effect.quantity;
                                        else
                                            skillDamage=lifeEffect.effect.quantity*wildMonsterStat.hp/100;
                                        if(lifeEffect.effect.on==CatchChallenger::ApplyOn::ApplyOn_AloneEnemy ||
                                                lifeEffect.effect.on==CatchChallenger::ApplyOn::ApplyOn_AllEnemy)
                                            tempSkillPoint+=skillDamage*lifeEffect.success/100;
                                        /// \todo multiply by the effectiveness of the attack, and if is of the same type
                                        skillIndex++;
                                    }
                                    if(skillPoint<tempSkillPoint || useTheRescueSkill)
                                    {
                                        skillPoint=tempSkillPoint;
                                        skillUsed=skill.skill;
                                        useTheRescueSkill=false;
                                    }
                                }
                                index++;
                            }
                            if(useTheRescueSkill)
                            {
                                if(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.find(0)!=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.cend())
                                    player.fightEngine->useSkill(0);
                                else
                                {
                                    std::cerr << "No skill found and no rescue skill" << std::endl;
                                    abort();
                                }
                            }
                            else
                                player.fightEngine->useSkill(skillUsed);
                            std::cout << "Start this: Try skill: " << std::to_string(skillUsed) << std::endl;
                        }

                        if(player.fightEngine->otherMonsterIsKO())
                            player.fightEngine->dropKOOtherMonster();
                        if(player.fightEngine->currentMonsterIsKO())
                        {
                            player.fightEngine->dropKOCurrentMonster();

                            if(player.fightEngine->haveAnotherMonsterOnThePlayerToFight())
                            {
                                if(player.fightEngine->isInFight())
                                {
                                    uint8_t maxLevel=0;
                                    uint8_t currentPos=0;
                                    const std::vector<CatchChallenger::PlayerMonster> &playerMonsters=player.fightEngine->getPlayerMonster();
                                    uint8_t index=0;
                                    while(index<playerMonsters.size())
                                    {
                                        const CatchChallenger::PlayerMonster &playerMonster=playerMonsters.at(index);
                                        if(playerMonster.hp>0)
                                        {
                                            if(playerMonster.level>maxLevel)
                                            {
                                                maxLevel=playerMonster.level;
                                                currentPos=index;
                                            }
                                        }
                                        index++;
                                    }
                                    if(maxLevel==0)
                                    {
                                        std::cerr << "player.fightEngine->haveAnotherMonsterOnThePlayerToFight() && player.fightEngine->isInFight(), unable to select other monster" << std::endl;
                                        abort();
                                    }
                                    if(!player.fightEngine->changeOfMonsterInFight(currentPos))
                                        return;
                                    player.api->changeOfMonsterInFightByPosition(currentPos);
                                }
                                else if(!player.canMoveOnMap)
                                {
                                    //win
                                    player.canMoveOnMap=true;
                                    player.fightEngine->fightFinished();
                                    CatchChallenger::PlayerMonster *monster=player.fightEngine->evolutionByLevelUp();
                                    if(monster!=NULL)
                                    {
                                        const CatchChallenger::Monster &monsterInformations=CatchChallenger::CommonDatapack::commonDatapack.monsters.at(monster->monster);
                                        unsigned int index=0;
                                        while(index<monsterInformations.evolutions.size())
                                        {
                                            const CatchChallenger::Monster::Evolution &evolution=monsterInformations.evolutions.at(index);
                                            if(evolution.type==CatchChallenger::Monster::EvolutionType_Level && evolution.data.level==monster->level)
                                            {
                                                const uint8_t &monsterEvolutionPostion=player.fightEngine->getPlayerMonsterPosition(monster);
                                                player.fightEngine->confirmEvolutionByPosition(monsterEvolutionPostion);//api call into it
                                                break;
                                            }
                                            index++;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                //loose, wait the server teleport, see ActionsAction::teleportTo()
                            }
                            if(apiSelectedClient==api)
                                ui->label_action->setText("Start this: "+QString::fromStdString(BotTargetList::stepToString(player.target.localStep)));
                        }

                        if(!player.canMoveOnMap && !player.fightEngine->isInFight())
                        {
                            //win
                            player.canMoveOnMap=true;
                            player.fightEngine->fightFinished();
                            CatchChallenger::PlayerMonster *monster=player.fightEngine->evolutionByLevelUp();
                            if(monster!=NULL)
                            {
                                const CatchChallenger::Monster &monsterInformations=CatchChallenger::CommonDatapack::commonDatapack.monsters.at(monster->monster);
                                unsigned int index=0;
                                while(index<monsterInformations.evolutions.size())
                                {
                                    const CatchChallenger::Monster::Evolution &evolution=monsterInformations.evolutions.at(index);
                                    if(evolution.type==CatchChallenger::Monster::EvolutionType_Level && evolution.data.level==monster->level)
                                    {
                                        const uint8_t &monsterEvolutionPostion=player.fightEngine->getPlayerMonsterPosition(monster);
                                        player.fightEngine->confirmEvolutionByPosition(monsterEvolutionPostion);//api call into it
                                        break;
                                    }
                                    index++;
                                }
                            }
                        }

                        if(apiSelectedClient==api)
                            updateFightStats();
                   }
                }
            }
        }
    }
}
