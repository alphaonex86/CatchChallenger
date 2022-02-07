#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../../general/base/CommonSettingsServer.hpp"
#ifndef CATCHCHALLENGER_NOAUDIO
#include "../../../libqtcatchchallenger/Audio.hpp"
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
    /*if(CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer==false)
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
    else*/
    {
        if(QtDatapackClientLoader::datapackLoader->plantOnMap.find(map->map_file)==
                QtDatapackClientLoader::datapackLoader->plantOnMap.cend())
            return -1;
        const std::unordered_map<std::pair<uint8_t,uint8_t>,uint16_t,pairhash> &plant=QtDatapackClientLoader::datapackLoader->plantOnMap.at(map->map_file);
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
                /*if(CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer==false)
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
                else*/
                {
                    if(QtDatapackClientLoader::datapackLoader->plantOnMap.find(map->map_file)==
                            QtDatapackClientLoader::datapackLoader->plantOnMap.cend())
                        return;
                    const std::unordered_map<std::pair<uint8_t,uint8_t>,uint16_t,pairhash> &plant=QtDatapackClientLoader::datapackLoader->plantOnMap.at(map->map_file);
                    if(plant.find(std::pair<uint8_t,uint8_t>(x,y))==plant.cend())
                        return;
                    emit collectMaturePlant();

                    client->remove_plant(mapController->getMap(map->map_file)->logicalMap.id,x,y);
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
        const Map_client::ItemOnMapForClient &item=map->itemsOnMap.at(std::pair<uint8_t,uint8_t>(x,y));
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
    if(map->bots.find(std::pair<uint8_t,uint8_t>(x,y))==map->bots.cend())
        return false;
    actualBot=map->bots.at(std::pair<uint8_t,uint8_t>(x,y));
    isInQuest=false;
    goToBotStep(1);
    return true;
}

void BaseWindow::botFightCollision(CatchChallenger::Map_client *map, uint8_t x, uint8_t y)
{
    if(map->bots.find(std::pair<uint8_t,uint8_t>(x,y))==map->bots.cend())
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"Bot trigged but no bot at this place");
        return;
    }
    uint8_t step=1;
    actualBot=map->bots.at(std::pair<uint8_t,uint8_t>(x,y));
    isInQuest=false;
    if(actualBot.step.find(step)==actualBot.step.cend())
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"Bot trigged but no step found");
        return;
    }
    const tinyxml2::XMLElement * stepXml=actualBot.step.at(step);
    if(stepXml->Attribute("type")==NULL)
        return;
    if(strcmp(stepXml->Attribute("type"),"fight")==0)
    {
        if(stepXml->Attribute("fightid")==NULL)
        {
            showTip(tr("Bot step missing data error, repport this error please").toStdString());
            return;
        }
        bool ok;
        const uint16_t fightId=stringtouint16(stepXml->Attribute("fightid"),&ok);
        if(!ok)
        {
            showTip(tr("Bot step wrong data type error, repport this error please").toStdString());
            return;
        }
        botFight(fightId);
        return;
    }
    else
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"Bot trigged but not found");
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
            showTip(tr("You can't enter to the fight zone if you are not able to fight").toStdString());
        break;
        case MapVisualiserPlayer::BlockedOn_ZoneItem:
            qDebug() << "You can't enter to this zone without the correct item";
            showTip(tr("You can't enter to this zone without the correct item").toStdString());
        break;
        case MapVisualiserPlayer::BlockedOn_RandomNumber:
            qDebug() << "You can't enter to the fight zone, because have not random number";
            showTip(tr("You can't enter to the fight zone, because have not random number").toStdString());
        break;
        default:
            qDebug() << "You can't enter to the zone, for unknown reason";
            showTip(tr("You can't enter to the zone").toStdString());
        break;
    }
}

void BaseWindow::pathFindingNotFound()
{
    showTip(tr("No path to go here").toStdString());
    //showTip(tr("Path finding disabled"));
}

void BaseWindow::currentMapLoaded()
{
    qDebug() << "BaseWindow::currentMapLoaded(): map: " << QString::fromStdString(mapController->currentMap())
             << " with type: " << QString::fromStdString(mapController->currentMapType());
    //name
    {
        Map_full *mapFull=mapController->currentMapFull();
        std::string visualName;
        if(!mapFull->zone.empty())
            if(QtDatapackClientLoader::datapackLoader->zonesExtra.find(mapFull->zone)!=
                    QtDatapackClientLoader::datapackLoader->zonesExtra.cend())
            {
                const DatapackClientLoader::ZoneExtra &zoneExtra=QtDatapackClientLoader::datapackLoader->zonesExtra.at(mapFull->zone);
                visualName=zoneExtra.name;
            }
        if(visualName.empty())
            visualName=mapFull->name;
        if(!visualName.empty() && lastPlaceDisplayed!=visualName)
        {
            lastPlaceDisplayed=visualName;
            showPlace(tr("You arrive at <b><i>%1</i></b>")
                      .arg(QString::fromStdString(visualName))
                      .toStdString());
        }
    }
    const std::string &type=mapController->currentMapType();
    #ifndef CATCHCHALLENGER_NOAUDIO
    //sound
    {
        std::vector<std::string> soundList;
        const std::string &backgroundsound=mapController->currentBackgroundsound();
        //map sound
        if(!backgroundsound.empty() && !vectorcontainsAtLeastOne(soundList,backgroundsound))
            soundList.push_back(backgroundsound);
        //zone sound
        Map_full *mapFull=mapController->currentMapFull();
        if(!mapFull->zone.empty())
            if(QtDatapackClientLoader::datapackLoader->zonesExtra.find(mapFull->zone)!=QtDatapackClientLoader::datapackLoader->zonesExtra.cend())
            {
                const DatapackClientLoader::ZoneExtra &zoneExtra=QtDatapackClientLoader::datapackLoader->zonesExtra.at(mapFull->zone);
                if(zoneExtra.audioAmbiance.find(type)!=zoneExtra.audioAmbiance.cend())
                {
                    const std::string &backgroundsound=zoneExtra.audioAmbiance.at(type);
                    if(!backgroundsound.empty() && !vectorcontainsAtLeastOne(soundList,backgroundsound))
                        soundList.push_back(backgroundsound);
                }
            }
        //general sound
        if(QtDatapackClientLoader::datapackLoader->audioAmbiance.find(type)!=QtDatapackClientLoader::datapackLoader->audioAmbiance.cend())
        {
            const std::string &backgroundsound=QtDatapackClientLoader::datapackLoader->audioAmbiance.at(type);
            if(!backgroundsound.empty() && !vectorcontainsAtLeastOne(soundList,backgroundsound))
                soundList.push_back(backgroundsound);
        }

        std::string finalSound;
        unsigned int index=0;
        while(index<soundList.size())
        {
            //search into main datapack
            const std::string &fileToSearchMain=client->datapackPathMain()+soundList.at(index);
            if(QFileInfo(QString::fromStdString(fileToSearchMain)).isFile())
            {
                finalSound=fileToSearchMain;
                break;
            }
            //search into base datapack
            const std::string &fileToSearchBase=client->datapackPathBase()+soundList.at(index);
            if(QFileInfo(QString::fromStdString(fileToSearchBase)).isFile())
            {
                finalSound=fileToSearchBase;
                break;
            }
            index++;
        }

        //set the sound
        if(!finalSound.empty() && currentAmbiance.file!=finalSound)
        {
            // reset the audio ambiance
            if(currentAmbiance.player!=NULL)
            {
                Audio::audio->removePlayer(currentAmbiance.player);
                currentAmbiance.player->stop();
                currentAmbiance.buffer->close();
                delete currentAmbiance.player;
                delete currentAmbiance.buffer;
                delete currentAmbiance.data;
                currentAmbiance.player=NULL;
                currentAmbiance.buffer=NULL;
                currentAmbiance.data=NULL;
                currentAmbiance.file.clear();
            }

            Ambiance ambiance;
            ambiance.player = new QAudioOutput(Audio::audio->format());
            // Create a new Media
            if(ambiance.player!=NULL)
            {
                //decode file
                ambiance.data=new QByteArray;
                if(Audio::decodeOpus(finalSound,*ambiance.data))
                {
                    ambiance.buffer=new QInfiniteBuffer(ambiance.data);
                    ambiance.buffer->open(QBuffer::ReadOnly);
                    ambiance.buffer->seek(0);
                    ambiance.player->start(ambiance.buffer);

                    ambiance.file=finalSound;
                    currentAmbiance=ambiance;

                    Audio::audio->addPlayer(ambiance.player);
                }
                else
                {
                    delete ambiance.player;
                    delete ambiance.data;
                }
            }
        }
    }
    #endif
    //color
    {
        if(visualCategory!=type)
        {
            visualCategory=type;
            if(QtDatapackClientLoader::datapackLoader->visualCategories.find(type)!=
                    QtDatapackClientLoader::datapackLoader->visualCategories.cend())
            {
                const std::vector<DatapackClientLoader::VisualCategory::VisualCategoryCondition> &conditions=
                        QtDatapackClientLoader::datapackLoader->visualCategories.at(type).conditions;
                unsigned int index=0;
                while(index<conditions.size())
                {
                    const DatapackClientLoader::VisualCategory::VisualCategoryCondition &condition=conditions.at(index);
                    if(condition.event<events.size())
                    {
                        if(events.at(condition.event)==condition.eventValue)
                        {
                            mapController->setColor(QColor(condition.color.r,condition.color.g,condition.color.b,condition.color.a));
                            break;
                        }
                    }
                    else
                        qDebug() << QStringLiteral("event for condition out of range: %1 for %2 event(s)").arg(condition.event).arg(events.size());
                    index++;
                }
                if(index==conditions.size())
                {
                    const DatapackClientLoader::CCColor &c=QtDatapackClientLoader::datapackLoader->visualCategories.at(type).defaultColor;
                    mapController->setColor(QColor(c.r,c.g,c.b,c.a));
                }
            }
            else
                mapController->setColor(Qt::transparent);
        }
    }
}
