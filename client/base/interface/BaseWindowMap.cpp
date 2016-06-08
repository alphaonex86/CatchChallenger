#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "../../../general/base/CommonSettingsServer.h"

using namespace CatchChallenger;

void BaseWindow::stopped_in_front_of(CatchChallenger::Map_client *map, uint8_t x, uint8_t y)
{
    if(stopped_in_front_of_check_bot(map,x,y))
        return;
    else if(CatchChallenger::MoveOnTheMap::isDirt(*map,x,y))
    {
        int index=0;
        while(index<map->plantList.size())
        {
            if(map->plantList.at(index)->x==x && map->plantList.at(index)->y==y)
            {
                uint64_t current_time=QDateTime::currentMSecsSinceEpoch()/1000;
                if(map->plantList.at(index)->mature_at<=current_time)
                    showTip(tr("To recolt the plant press <i>Enter</i>"));
                else
                    showTip(tr("This plant is growing and can't be collected"));
                return;
            }
            index++;
        }
        showTip(tr("To plant a seed press <i>Enter</i>"));
        return;
    }
    else
    {
        if(!CatchChallenger::MoveOnTheMap::isWalkable(*map,x,y))
        {
            //check bot with border
            CatchChallenger::CommonMap * current_map=map;
            switch(MapController::mapController->getDirection())
            {
                case CatchChallenger::Direction_look_at_left:
                if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_left,*map,x,y,false))
                    if(CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_left,&current_map,&x,&y,false))
                        stopped_in_front_of_check_bot(map,x,y);
                break;
                case CatchChallenger::Direction_look_at_right:
                if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_right,*map,x,y,false))
                    if(CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_right,&current_map,&x,&y,false))
                        stopped_in_front_of_check_bot(map,x,y);
                break;
                case CatchChallenger::Direction_look_at_top:
                if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_top,*map,x,y,false))
                    if(CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_top,&current_map,&x,&y,false))
                        stopped_in_front_of_check_bot(map,x,y);
                break;
                case CatchChallenger::Direction_look_at_bottom:
                if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_bottom,*map,x,y,false))
                    if(CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_bottom,&current_map,&x,&y,false))
                        stopped_in_front_of_check_bot(map,x,y);
                break;
                default:
                break;
            }
        }
    }
}

bool BaseWindow::stopped_in_front_of_check_bot(CatchChallenger::Map_client *map, uint8_t x, uint8_t y)
{
    if(!map->bots.contains(QPair<uint8_t,uint8_t>(x,y)))
        return false;
    showTip(tr("To interact with the bot press <i><b>Enter</b></i>"));
    return true;
}

//return -1 if not found, else the index
int32_t BaseWindow::havePlant(CatchChallenger::Map_client *map, uint8_t x, uint8_t y) const
{
    if(CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer==false)
    {
        int index=0;
        while(index<map->plantList.size())
        {
            if(map->plantList.at(index)->x==x && map->plantList.at(index)->y==y)
                return index;
            index++;
        }
        return -1;
    }
    else
    {
        if(!DatapackClientLoader::datapackLoader.plantOnMap.contains(QString::fromStdString(map->map_file)))
            return -1;
        if(!DatapackClientLoader::datapackLoader.plantOnMap.value(QString::fromStdString(map->map_file)).contains(QPair<uint8_t,uint8_t>(x,y)))
            return -1;
        int index=0;
        while(index<map->plantList.size())
        {
            if(map->plantList.at(index)->x==x && map->plantList.at(index)->y==y)
                return index;
            index++;
        }
        return -1;
    }
}

void BaseWindow::actionOnNothing()
{
    ui->IG_dialog->setVisible(false);
}

void BaseWindow::actionOn(Map_client *map, uint8_t x, uint8_t y)
{
    if(ui->IG_dialog->isVisible())
        ui->IG_dialog->setVisible(false);
    if(actionOnCheckBot(map,x,y))
        return;
    else if(CatchChallenger::MoveOnTheMap::isDirt(*map,x,y))
    {
        /* -1 == not found */
        int index=havePlant(map,x,y);
        if(index>=0)
        {
            uint64_t current_time=QDateTime::currentMSecsSinceEpoch()/1000;
            if(map->plantList.at(index)->mature_at<=current_time)
            {
                if(CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer==false)
                {
                    ClientPlantInCollecting clientPlantInCollecting;
                    clientPlantInCollecting.map=QString::fromStdString(map->map_file);
                    clientPlantInCollecting.plant_id=map->plantList.at(index)->plant_id;
                    clientPlantInCollecting.seconds_to_mature=0;
                    clientPlantInCollecting.x=x;
                    clientPlantInCollecting.y=y;
                    plant_collect_in_waiting << clientPlantInCollecting;
                    addQuery(QueryType_CollectPlant);
                    emit collectMaturePlant();
                }
                else
                {
                    if(!DatapackClientLoader::datapackLoader.plantOnMap.contains(QString::fromStdString(map->map_file)))
                        return;
                    if(!DatapackClientLoader::datapackLoader.plantOnMap.value(QString::fromStdString(map->map_file)).contains(QPair<uint8_t,uint8_t>(x,y)))
                        return;
                    emit collectMaturePlant();

                    CatchChallenger::Api_client_real::client->remove_plant(MapController::mapController->getMap(QString::fromStdString(map->map_file))->logicalMap.id,x,y);
                    CatchChallenger::Api_client_real::client->plant_collected(Plant_collect::Plant_collect_correctly_collected);
                }
            }
            else
                showTip(tr("This plant is growing and can't be collected"));
        }
        else
        {
            SeedInWaiting seedInWaiting;
            seedInWaiting.map=QString::fromStdString(map->map_file);
            seedInWaiting.x=x;
            seedInWaiting.y=y;
            seed_in_waiting << seedInWaiting;

            selectObject(ObjectType_Seed);
        }
        return;
    }
    else if(map->itemsOnMap.contains(QPair<uint8_t,uint8_t>(x,y)))
    {
        const Map_client::ItemOnMapForClient &item=map->itemsOnMap.value(QPair<uint8_t,uint8_t>(x,y));
        if(itemOnMap.find(item.indexOfItemOnMap)==itemOnMap.cend())
        {
            if(!item.infinite)
                itemOnMap.insert(item.indexOfItemOnMap);
            add_to_inventory(item.item);
            CatchChallenger::Api_client_real::client->takeAnObjectOnMap();
        }
    }
    else
    {
        //check bot with border
        CatchChallenger::CommonMap * current_map=map;
        switch(MapController::mapController->getDirection())
        {
            case CatchChallenger::Direction_look_at_left:
            if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_left,*map,x,y,false))
                if(CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_left,&current_map,&x,&y,false))
                    actionOnCheckBot(map,x,y);
            break;
            case CatchChallenger::Direction_look_at_right:
            if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_right,*map,x,y,false))
                if(CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_right,&current_map,&x,&y,false))
                    actionOnCheckBot(map,x,y);
            break;
            case CatchChallenger::Direction_look_at_top:
            if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_top,*map,x,y,false))
                if(CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_top,&current_map,&x,&y,false))
                    actionOnCheckBot(map,x,y);
            break;
            case CatchChallenger::Direction_look_at_bottom:
            if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_bottom,*map,x,y,false))
                if(CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_bottom,&current_map,&x,&y,false))
                    actionOnCheckBot(map,x,y);
            break;
            default:
            break;
        }
    }
}

bool BaseWindow::actionOnCheckBot(CatchChallenger::Map_client *map, uint8_t x, uint8_t y)
{
    if(!map->bots.contains(QPair<uint8_t,uint8_t>(x,y)))
        return false;
    actualBot=map->bots.value(QPair<uint8_t,uint8_t>(x,y));
    isInQuest=false;
    goToBotStep(1);
    return true;
}

void BaseWindow::botFightCollision(CatchChallenger::Map_client *map, uint8_t x, uint8_t y)
{
    if(!map->bots.contains(QPair<uint8_t,uint8_t>(x,y)))
    {
        newError(tr("Internal error"),"Bot trigged but no bot at this place");
        return;
    }
    uint8_t step=1;
    actualBot=map->bots.value(QPair<uint8_t,uint8_t>(x,y));
    isInQuest=false;
    if(actualBot.step.find(step)==actualBot.step.cend())
    {
        newError(tr("Internal error"),"Bot trigged but no step found");
        return;
    }
    if(*actualBot.step.at(step)->Attribute(std::string("type"))=="fight")
    {
        if(actualBot.step.at(step)->Attribute(std::string("fightid"))==NULL)
        {
            showTip(tr("Bot step missing data error, repport this error please"));
            return;
        }
        bool ok;
        uint32_t fightId=stringtouint32(*actualBot.step.at(step)->Attribute(std::string("fightid")),&ok);
        if(!ok)
        {
            showTip(tr("Bot step wrong data type error, repport this error please"));
            return;
        }
        botFight(fightId);
        return;
    }
    else
    {
        newError(tr("Internal error"),"Bot trigged but not found");
        return;
    }
}

void BaseWindow::blockedOn(const MapVisualiserPlayer::BlockedOn &blockOnVar)
{
    switch(blockOnVar)
    {
        case MapVisualiserPlayer::BlockedOn_ZoneFight:
        case MapVisualiserPlayer::BlockedOn_Fight:
            qDebug() << "You can't enter to the fight zone if you are not able to fight";
            showTip(tr("You can't enter to the fight zone if you are not able to fight"));
        break;
        case MapVisualiserPlayer::BlockedOn_ZoneItem:
            qDebug() << "You can't enter to this zone without the correct item";
            showTip(tr("You can't enter to this zone without the correct item"));
        break;
        case MapVisualiserPlayer::BlockedOn_RandomNumber:
            qDebug() << "You can't enter to the fight zone, because have not random number";
            showTip(tr("You can't enter to the fight zone, because have not random number"));
        break;
        default:
            qDebug() << "You can't enter to the zone, for unknown reason";
            showTip(tr("You can't enter to the zone"));
        break;
    }
}

void BaseWindow::pathFindingNotFound()
{
    showTip(tr("No path to go here"));
    //showTip(tr("Path finding disabled"));
}

void BaseWindow::currentMapLoaded()
{
    qDebug() << "BaseWindow::currentMapLoaded(): map: " << MapController::mapController->currentMap() << " with type: " << MapController::mapController->currentMapType();
    //name
    {
        MapVisualiserThread::Map_full *mapFull=MapController::mapController->currentMapFull();
        QString visualName;
        if(!mapFull->zone.isEmpty())
            if(DatapackClientLoader::datapackLoader.zonesExtra.contains(mapFull->zone))
                visualName=DatapackClientLoader::datapackLoader.zonesExtra.value(mapFull->zone).name;
        if(visualName.isEmpty())
            visualName=mapFull->name;
        if(!visualName.isEmpty() && lastPlaceDisplayed!=visualName)
        {
            lastPlaceDisplayed=visualName;
            showPlace(tr("You arrive at <b><i>%1</i></b>").arg(visualName));
        }
    }
    const QString &type=MapController::mapController->currentMapType();
    //sound
    {
        bool noSound=false;
        const QString &backgroundsound=MapController::mapController->currentBackgroundsound();
        if(!DatapackClientLoader::datapackLoader.audioAmbiance.contains(type) && backgroundsound.isEmpty())
        {
            while(!ambianceList.isEmpty())
            {
                #ifndef CATCHCHALLENGER_NOAUDIO
                libvlc_media_player_stop(ambianceList.first().player);
                libvlc_media_player_release(ambianceList.first().player);
                Audio::audio.removePlayer(ambianceList.first().player);
                #endif
                ambianceList.removeFirst();
            }
            noSound=true;
        }
        if(!noSound)
        {
            QString file;
            if(DatapackClientLoader::datapackLoader.audioAmbiance.contains(type))
                file=DatapackClientLoader::datapackLoader.audioAmbiance.value(type);
            else
                file=backgroundsound;
            while(!ambianceList.isEmpty())
            {
                if(ambianceList.first().file==file)
                {
                    noSound=true;
                    break;
                }
                #ifndef CATCHCHALLENGER_NOAUDIO
                libvlc_media_player_stop(ambianceList.first().player);
                libvlc_media_player_release(ambianceList.first().player);
                Audio::audio.removePlayer(ambianceList.first().player);
                #endif
                ambianceList.removeFirst();
            }
            if(!noSound)
            {
                #ifndef CATCHCHALLENGER_NOAUDIO
                if(Audio::audio.vlcInstance && QFileInfo(file).isFile())
                {
                    // Create a new Media
                    libvlc_media_t *vlcMedia = libvlc_media_new_path(Audio::audio.vlcInstance, QDir::toNativeSeparators(file).toUtf8().constData());
                    if(vlcMedia!=NULL)
                    {
                        Ambiance ambiance;
                        // Create a new libvlc player
                        ambiance.player = libvlc_media_player_new_from_media (vlcMedia);
                        // Release the media
                        libvlc_media_release(vlcMedia);
                        libvlc_media_add_option(vlcMedia, "input-repeat=-1");
                        // And start playback
                        libvlc_media_player_play(ambiance.player);

                        ambiance.file=file;
                        ambianceList << ambiance;

                        Audio::audio.addPlayer(ambiance.player);
                    }
                }
                #endif
            }
        }
    }
    //color
    {
        if(visualCategory!=type)
        {
            visualCategory=type;
            if(DatapackClientLoader::datapackLoader.visualCategories.contains(type))
            {
                const QList<DatapackClientLoader::VisualCategory::VisualCategoryCondition> &conditions=DatapackClientLoader::datapackLoader.visualCategories.value(type).conditions;
                int index=0;
                while(index<conditions.size())
                {
                    const DatapackClientLoader::VisualCategory::VisualCategoryCondition &condition=conditions.at(index);
                    if(condition.event<events.size())
                    {
                        if(events.at(condition.event)==condition.eventValue)
                        {
                            MapController::mapController->setColor(condition.color);
                            break;
                        }
                    }
                    else
                        qDebug() << QStringLiteral("event for condition out of range: %1 for %2 event(s)").arg(condition.event).arg(events.size());
                    index++;
                }
                if(index==conditions.size())
                    MapController::mapController->setColor(DatapackClientLoader::datapackLoader.visualCategories.value(type).defaultColor);
            }
            else
                MapController::mapController->setColor(Qt::transparent);
        }
    }
}
