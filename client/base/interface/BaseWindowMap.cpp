#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "../../../general/base/CommonSettingsServer.h"
#ifndef CATCHCHALLENGER_NOAUDIO
#include "../Audio.h"
#endif

using namespace CatchChallenger;

void BaseWindow::stopped_in_front_of(CatchChallenger::Map_client *map, uint8_t x, uint8_t y)
{
    if(stopped_in_front_of_check_bot(map,x,y))
        return;
    else if(CatchChallenger::MoveOnTheMap::isDirt(*map,x,y))
    {
        unsigned int index=0;
        while(index<map->plantList.size())
        {
            if(map->plantList.at(index)->x==x && map->plantList.at(index)->y==y)
            {
                uint64_t current_time=QDateTime::currentMSecsSinceEpoch()/1000;
                if(map->plantList.at(index)->mature_at<=current_time)
                    showTip(tr("To recolt the plant press <i>Enter</i>").toStdString());
                else
                    showTip(tr("This plant is growing and can't be collected").toStdString());
                return;
            }
            index++;
        }
        showTip(tr("To plant a seed press <i>Enter</i>").toStdString());
        return;
    }
    else
    {
        if(!CatchChallenger::MoveOnTheMap::isWalkable(*map,x,y))
        {
            //check bot with border
            CatchChallenger::CommonMap * current_map=map;
            switch(mapController->getDirection())
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
    if(map->bots.find(std::pair<uint8_t,uint8_t>(x,y))==map->bots.cend())
        return false;
    showTip(tr("To interact with the bot press <i><b>Enter</b></i>").toStdString());
    return true;
}

//return -1 if not found, else the index
int32_t BaseWindow::havePlant(CatchChallenger::Map_client *map, uint8_t x, uint8_t y) const
{
    if(CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer==false)
    {
        unsigned int index=0;
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
        if(DatapackClientLoader::datapackLoader.plantOnMap.find(map->map_file)==
                DatapackClientLoader::datapackLoader.plantOnMap.cend())
            return -1;
        const std::unordered_map<std::pair<uint8_t,uint8_t>,uint16_t,pairhash> &plant=DatapackClientLoader::datapackLoader.plantOnMap.at(map->map_file);
        if(plant.find(std::pair<uint8_t,uint8_t>(x,y))==plant.cend())
            return -1;
        unsigned int index=0;
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
                    clientPlantInCollecting.map=map->map_file;
                    clientPlantInCollecting.plant_id=map->plantList.at(index)->plant_id;
                    clientPlantInCollecting.seconds_to_mature=0;
                    clientPlantInCollecting.x=x;
                    clientPlantInCollecting.y=y;
                    plant_collect_in_waiting.push_back(clientPlantInCollecting);
                    addQuery(QueryType_CollectPlant);
                    emit collectMaturePlant();
                }
                else
                {
                    if(DatapackClientLoader::datapackLoader.plantOnMap.find(map->map_file)==
                            DatapackClientLoader::datapackLoader.plantOnMap.cend())
                        return;
                    const std::unordered_map<std::pair<uint8_t,uint8_t>,uint16_t,pairhash> &plant=DatapackClientLoader::datapackLoader.plantOnMap.at(map->map_file);
                    if(plant.find(std::pair<uint8_t,uint8_t>(x,y))==plant.cend())
                        return;
                    emit collectMaturePlant();

                    client->remove_plant(mapController->getMap(QString::fromStdString(map->map_file))->logicalMap.id,x,y);
                    client->plant_collected(Plant_collect::Plant_collect_correctly_collected);
                }
            }
            else
                showTip(tr("This plant is growing and can't be collected").toStdString());
        }
        else
        {
            SeedInWaiting seedInWaiting;
            seedInWaiting.map=map->map_file;
            seedInWaiting.x=x;
            seedInWaiting.y=y;
            seed_in_waiting.push_back(seedInWaiting);

            selectObject(ObjectType_Seed);
        }
        return;
    }
    else if(map->itemsOnMap.find(std::pair<uint8_t,uint8_t>(x,y))!=map->itemsOnMap.cend())
    {
        Player_private_and_public_informations &informations=client->get_player_informations();
        const Map_client::ItemOnMapForClient &item=map->itemsOnMap.value(std::pair<uint8_t,uint8_t>(x,y));
        if(informations.itemOnMap.find(item.indexOfItemOnMap)==informations.itemOnMap.cend())
        {
            if(!item.infinite)
                informations.itemOnMap.insert(item.indexOfItemOnMap);
            add_to_inventory(item.item);
            client->takeAnObjectOnMap();
        }
    }
    else
    {
        //check bot with border
        CatchChallenger::CommonMap * current_map=map;
        switch(mapController->getDirection())
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
    if(!map->bots.contains(std::pair<uint8_t,uint8_t>(x,y)))
        return false;
    actualBot=map->bots.value(std::pair<uint8_t,uint8_t>(x,y));
    isInQuest=false;
    goToBotStep(1);
    return true;
}

void BaseWindow::botFightCollision(CatchChallenger::Map_client *map, uint8_t x, uint8_t y)
{
    if(!map->bots.contains(std::pair<uint8_t,uint8_t>(x,y)))
    {
        newError(tr("Internal error")+", file: "+QString(__FILE__)+":"+QString::number(__LINE__),"Bot trigged but no bot at this place");
        return;
    }
    uint8_t step=1;
    actualBot=map->bots.value(std::pair<uint8_t,uint8_t>(x,y));
    isInQuest=false;
    if(actualBot.step.find(step)==actualBot.step.cend())
    {
        newError(tr("Internal error")+", file: "+QString(__FILE__)+":"+QString::number(__LINE__),"Bot trigged but no step found");
        return;
    }
    if(*actualBot.step.at(step)->Attribute(std::string("type"))==std::string("fight"))
    {
        if(actualBot.step.at(step)->Attribute(std::string("fightid"))==NULL)
        {
            showTip(tr("Bot step missing data error, repport this error please"));
            return;
        }
        bool ok;
        uint16_t fightId=stringtouint16(*actualBot.step.at(step)->Attribute(std::string("fightid")),&ok);
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
        newError(tr("Internal error")+", file: "+QString(__FILE__)+":"+QString::number(__LINE__),"Bot trigged but not found");
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
    qDebug() << "BaseWindow::currentMapLoaded(): map: " << mapController->currentMap() << " with type: " << mapController->currentMapType();
    //name
    {
        MapVisualiserThread::Map_full *mapFull=mapController->currentMapFull();
        std::string visualName;
        if(!mapFull->zone.isEmpty())
            if(DatapackClientLoader::datapackLoader.zonesExtra.contains(mapFull->zone))
            {
                const DatapackClientLoader::ZoneExtra &zoneExtra=DatapackClientLoader::datapackLoader.zonesExtra.value(mapFull->zone);
                visualName=zoneExtra.name;
            }
        if(visualName.isEmpty())
            visualName=mapFull->name;
        if(!visualName.isEmpty() && lastPlaceDisplayed!=visualName)
        {
            lastPlaceDisplayed=visualName;
            showPlace(tr("You arrive at <b><i>%1</i></b>").arg(visualName));
        }
    }
    const std::string &type=mapController->currentMapType();
    #ifndef CATCHCHALLENGER_NOAUDIO
    //sound
    {
        QStringList soundList;
        const std::string &backgroundsound=mapController->currentBackgroundsound();
        //map sound
        if(!backgroundsound.isEmpty() && !soundList.contains(backgroundsound))
            soundList << backgroundsound;
        //zone sound
        MapVisualiserThread::Map_full *mapFull=mapController->currentMapFull();
        if(!mapFull->zone.isEmpty())
            if(DatapackClientLoader::datapackLoader.zonesExtra.contains(mapFull->zone))
            {
                const DatapackClientLoader::ZoneExtra &zoneExtra=DatapackClientLoader::datapackLoader.zonesExtra.value(mapFull->zone);
                if(zoneExtra.audioAmbiance.contains(type))
                {
                    const std::string &backgroundsound=zoneExtra.audioAmbiance.value(type);
                    if(!backgroundsound.isEmpty() && !soundList.contains(backgroundsound))
                        soundList << backgroundsound;
                }
            }
        //general sound
        if(DatapackClientLoader::datapackLoader.audioAmbiance.contains(type))
        {
            const std::string &backgroundsound=DatapackClientLoader::datapackLoader.audioAmbiance.value(type);
            if(!backgroundsound.isEmpty() && !soundList.contains(backgroundsound))
                soundList << backgroundsound;
        }

        std::string finalSound;
        int index=0;
        while(index<soundList.size())
        {
            //search into main datapack
            const std::string &fileToSearchMain=QDir::toNativeSeparators(client->datapackPathMain()+soundList.at(index));
            if(QFileInfo(fileToSearchMain).isFile())
            {
                finalSound=fileToSearchMain;
                break;
            }
            //search into base datapack
            const std::string &fileToSearchBase=QDir::toNativeSeparators(client->datapackPathBase()+soundList.at(index));
            if(QFileInfo(fileToSearchBase).isFile())
            {
                finalSound=fileToSearchBase;
                break;
            }
            index++;
        }

        //set the sound
        if(Audio::audio.vlcInstance && !finalSound.isEmpty() && currentAmbiance.file!=finalSound)
        {
            // reset the audio ambiance
            if(currentAmbiance.manager!=NULL)
            {
                libvlc_event_detach(currentAmbiance.manager,libvlc_MediaPlayerEncounteredError,BaseWindow::vlceventStatic,currentAmbiance.player);
                libvlc_event_detach(currentAmbiance.manager,libvlc_MediaPlayerEndReached,BaseWindow::vlceventStatic,currentAmbiance.player);
                libvlc_media_player_stop(currentAmbiance.player);
                libvlc_media_player_release(currentAmbiance.player);
                Audio::audio.removePlayer(currentAmbiance.player);
                currentAmbiance.manager=NULL;
                currentAmbiance.player=NULL;
                currentAmbiance.file.clear();
            }

            // Create a new Media
            libvlc_media_t *vlcMedia = libvlc_media_new_path(Audio::audio.vlcInstance, finalSound.toUtf8().constData());
            if(vlcMedia!=NULL)
            {
                Ambiance ambiance;
                // Create a new libvlc player
                ambiance.player = libvlc_media_player_new_from_media (vlcMedia);
                // Get event manager for the player instance
                ambiance.manager = libvlc_media_player_event_manager(ambiance.player);
                // Attach the event handler to the media player error's events
                libvlc_event_attach(ambiance.manager,libvlc_MediaPlayerEncounteredError,BaseWindow::vlceventStatic,ambiance.player);
                libvlc_event_attach(ambiance.manager,libvlc_MediaPlayerEndReached,BaseWindow::vlceventStatic,ambiance.player);
                // Release the media
                libvlc_media_release(vlcMedia);
                // And start playback
                libvlc_media_player_play(ambiance.player);

                ambiance.file=finalSound;
                currentAmbiance=ambiance;

                Audio::audio.addPlayer(ambiance.player);
            }
        }
    }
    #endif
    //color
    {
        if(visualCategory!=type)
        {
            visualCategory=type;
            if(DatapackClientLoader::datapackLoader.visualCategories.contains(type))
            {
                const std::vector<DatapackClientLoader::VisualCategory::VisualCategoryCondition> &conditions=DatapackClientLoader::datapackLoader.visualCategories.value(type).conditions;
                int index=0;
                while(index<conditions.size())
                {
                    const DatapackClientLoader::VisualCategory::VisualCategoryCondition &condition=conditions.at(index);
                    if(condition.event<events.size())
                    {
                        if(events.at(condition.event)==condition.eventValue)
                        {
                            mapController->setColor(condition.color);
                            break;
                        }
                    }
                    else
                        qDebug() << QStringLiteral("event for condition out of range: %1 for %2 event(s)").arg(condition.event).arg(events.size());
                    index++;
                }
                if(index==conditions.size())
                    mapController->setColor(DatapackClientLoader::datapackLoader.visualCategories.value(type).defaultColor);
            }
            else
                mapController->setColor(Qt::transparent);
        }
    }
}

#ifndef CATCHCHALLENGER_NOAUDIO
void BaseWindow::vlceventStatic(const libvlc_event_t *event, void *ptr)
{
    libvlc_media_player_t * const player=static_cast<libvlc_media_player_t *>(ptr);
    Q_UNUSED(player);
    switch(event->type)
    {
        case libvlc_MediaPlayerEncounteredError:
        {
            const char * string=libvlc_errmsg();
            if(string==NULL)
                qDebug() << "vlc error";
            else
                qDebug() << string;
        }
        break;
        case libvlc_MediaPlayerEndReached:
            //BaseWindow::baseWindow->audioLoopRestart(player);
            //audioLoop(player);
        break;
        default:
            qDebug() << "vlc event: " << event->type;
        break;
    }
}

void BaseWindow::audioLoop(void *player)
{
    libvlc_media_player_t * const vlcPlayer=static_cast<libvlc_media_player_t *>(player);
    libvlc_media_player_stop(vlcPlayer);
    libvlc_media_player_play(vlcPlayer);
}
#endif

