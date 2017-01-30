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
                    switch(player.target.linkPoint.type)
                    {
                        case MapServerMini::BlockObject::LinkType::SourceTopMap:
                        case MapServerMini::BlockObject::LinkType::SourceInternalTopBlock:
                            if(ActionsAction::canGoTo(api,CatchChallenger::Direction::Direction_move_at_top,*playerMap,player.x,player.y,true,true))
                            {
                                ActionsAction::move(api,CatchChallenger::Direction::Direction_move_at_top,&playerMap,&player.x,&player.y,true,true);
                                api->newDirection(CatchChallenger::Direction::Direction_move_at_top);
                            }
                            else
                            {
                                if(player.target.bestPath.size()>2)
                                    abort();
                                player.target.bestPath.clear();
                                //change the look below
                            }
                        break;
                        case MapServerMini::BlockObject::LinkType::SourceRightMap:
                        case MapServerMini::BlockObject::LinkType::SourceInternalRightBlock:
                            if(ActionsAction::canGoTo(api,CatchChallenger::Direction::Direction_move_at_right,*playerMap,player.x,player.y,true,true))
                            {
                                ActionsAction::move(api,CatchChallenger::Direction::Direction_move_at_right,&playerMap,&player.x,&player.y,true,true);
                                api->newDirection(CatchChallenger::Direction::Direction_move_at_right);
                            }
                            else
                            {
                                if(player.target.bestPath.size()>2)
                                    abort();
                                player.target.bestPath.clear();
                                //change the look below
                            }
                        break;
                        case MapServerMini::BlockObject::LinkType::SourceBottomMap:
                        case MapServerMini::BlockObject::LinkType::SourceInternalBottomBlock:
                            if(ActionsAction::canGoTo(api,CatchChallenger::Direction::Direction_move_at_bottom,*playerMap,player.x,player.y,true,true))
                            {
                                ActionsAction::move(api,CatchChallenger::Direction::Direction_move_at_bottom,&playerMap,&player.x,&player.y,true,true);
                                api->newDirection(CatchChallenger::Direction::Direction_move_at_bottom);
                            }
                            else
                            {
                                if(player.target.bestPath.size()>2)
                                    abort();
                                player.target.bestPath.clear();
                                //change the look below
                            }
                        break;
                        case MapServerMini::BlockObject::LinkType::SourceLeftMap:
                        case MapServerMini::BlockObject::LinkType::SourceInternalLeftBlock:
                            if(ActionsAction::canGoTo(api,CatchChallenger::Direction::Direction_move_at_left,*playerMap,player.x,player.y,true,true))
                            {
                                ActionsAction::move(api,CatchChallenger::Direction::Direction_move_at_left,&playerMap,&player.x,&player.y,true,true);
                                api->newDirection(CatchChallenger::Direction::Direction_move_at_left);
                            }
                            else
                            {
                                if(player.target.bestPath.size()>2)
                                    abort();
                                player.target.bestPath.clear();
                                //change the look below
                            }
                        break;
                        default:
                        break;
                    }
                    if(!player.target.bestPath.empty())
                    {
                        player.target.bestPath.erase(player.target.bestPath.cbegin());
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
                        //if the target is on the same block
                        const MapServerMini::BlockObject * nextBlock=player.target.bestPath.front();
                        if(blockObject->links.find(nextBlock)==blockObject->links.cend())
                            abort();
                        pointsList=blockObject->links.at(nextBlock).points;
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
                            linkInformationList.push_back(linkInformation);

                            index++;
                        }
                        bool ok=false;
                        unsigned int destinationIndexSelected=0;
                        const std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > &returnPath=pathFinding(
                                    layer.blockObject,
                                    static_cast<CatchChallenger::Orientation>(o),player.x,player.y,
                                    destinations,
                                    destinationIndexSelected,
                                    layer.blockObject,
                                    &ok);
                        if(!ok)
                            abort();
                        player.target.linkPoint=pointsList.at(destinationIndexSelected);
                        player.target.localStep=returnPath;
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
                switch(player.direction)
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
                        direction=player.direction;
                }
                if(!ActionsAction::moveWithoutTeleport(api,direction,&mapServer,&x,&y,false,false))
                    abort();

                CatchChallenger::Player_private_and_public_informations &playerInformations=api->get_player_informations();
                const QPair<uint8_t,uint8_t> QtPoint{x,y};
                QString mapQtString=DatapackClientLoader::datapackLoader.getDatapackPath()+DatapackClientLoader::datapackLoader.getMainDatapackPath()+QString::fromStdString(mapServer->map_file);
                if(!mapQtString.endsWith(".tmx"))
                    mapQtString+=".tmx";
                //finish correctly the step
                switch(player.target.type)
                {
                    case ActionsBotInterface::GlobalTarget::GlobalTargetType::Heal:
                        api->heal();
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
                        QMessageBox::critical(this,tr("Not coded"),tr("This target type is not coded"));
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
