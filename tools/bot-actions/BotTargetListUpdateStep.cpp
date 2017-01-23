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
                    switch(player.target.localType)
                    {
                        case MapServerMini::BlockObject::LinkType::SourceNone:
                        break;
                        default:
                        break;
                    }
                    player.target.bestPath.erase(player.target.bestPath.cbegin());
                    uint8_t o=player.direction;
                    while(o>4)
                        o-=4;

                    if(!player.target.bestPath.empty())
                    {
                        const MapServerMini::BlockObject * const blockObject=player.target.bestPath.at(0);
                        const std::pair<uint8_t,uint8_t> &point=getNextPosition(blockObject,player.target/*hop list, first is the next hop*/);

                        const std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > &returnPath=pathFinding(
                                    blockObject,
                                    static_cast<CatchChallenger::Orientation>(o),player.x,player.y,
                                    CatchChallenger::Orientation::Orientation_none,point.first,point.second
                                    );
                        player.target.localStep=returnPath;
                        const MapServerMini::BlockObject * const nextBlock=player.target.bestPath.at(0);
                        //search the next position
                        for(const auto& n:blockObject->links) {
                            const MapServerMini::BlockObject * const tempNextBlock=n.first;
                            const MapServerMini::BlockObject::LinkInformation &linkInformation=n.second;
                            if(tempNextBlock==nextBlock)
                            {
                                const MapServerMini::BlockObject::LinkPoint &firstPoint=linkInformation.points.at(0);
                                player.target.localType=firstPoint.type;
                                break;
                            }
                        }
                    }
                    else
                    {
                        const std::string &playerMapStdString=actionsAction->id_map_to_map.at(player.mapId);
                        const MapServerMini * const playerMap=static_cast<const MapServerMini *>(actionsAction->map_list.at(playerMapStdString));
                        if(playerMap->step.size()<2)
                            abort();
                        const MapServerMini::MapParsedForBot &stepPlayer=playerMap->step.at(1);
                        const uint8_t playerCodeZone=stepPlayer.map[player.x+player.y*playerMap->width];
                        const MapServerMini::MapParsedForBot::Layer &layer=stepPlayer.layers.at(playerCodeZone-1);
                        const MapServerMini::BlockObject * const blockObject=layer.blockObject;
                        std::pair<uint8_t,uint8_t> point;
                        switch(player.target.type)
                        {
                            case ActionsBotInterface::GlobalTarget::GlobalTargetType::Heal:
                                for(const auto& n:blockObject->heal) {
                                    point=n;
                                    break;
                                }
                            break;
                            case ActionsBotInterface::GlobalTarget::Dirt:
                            {
                                const DatapackClientLoader::PlantIndexContent &plantOrDirt=DatapackClientLoader::datapackLoader.plantIndexOfOnMap.value(player.target.extra);
                                point.first=plantOrDirt.x;
                                point.second=plantOrDirt.y;
                            }
                            break;
                            case ActionsBotInterface::GlobalTarget::Plant:
                            {
                                const DatapackClientLoader::PlantIndexContent &plantOrDirt=DatapackClientLoader::datapackLoader.plantIndexOfOnMap.value(player.target.extra);
                                point.first=plantOrDirt.x;
                                point.second=plantOrDirt.y;
                            }
                            break;
                            default:
                                QMessageBox::critical(this,tr("Not coded"),tr("This target type is not coded"));
                                return;
                            break;
                        }
                        switch(player.target.type)
                        {
                            case ActionsBotInterface::GlobalTarget::GlobalTargetType::Heal:
                            case ActionsBotInterface::GlobalTarget::Dirt:
                            case ActionsBotInterface::GlobalTarget::Plant:
                            {
                                const std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > &returnPath=pathFinding(
                                            blockObject,
                                            static_cast<CatchChallenger::Orientation>(o),player.x,player.y,
                                            CatchChallenger::Orientation::Orientation_none,point.first,point.second
                                            );
                                player.target.localStep=returnPath;
                                player.target.localType=MapServerMini::BlockObject::LinkType::SourceNone;
                            }
                            break;
                            default:
                            break;
                        }
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
                player.target.localType=MapServerMini::BlockObject::LinkType::SourceNone;
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
