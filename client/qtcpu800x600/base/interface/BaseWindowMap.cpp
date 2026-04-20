#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../../general/base/CommonSettingsServer.hpp"
#include "../../../libqtcatchchallenger/maprender/QMap_client.hpp"
#include "../../../libcatchchallenger/Api_protocol.hpp"
#include "../AutoArgs.h"
#include <QTimer>
#include <QCoreApplication>
#include <QKeyEvent>
#include <iostream>
#ifndef CATCHCHALLENGER_NOAUDIO
#include "../../../libqtcatchchallenger/Audio.hpp"
#endif

using namespace CatchChallenger;

void BaseWindow::stopped_in_front_of(CatchChallenger::Map_client *map, const CATCHCHALLENGER_TYPE_MAPID &mapIndex, uint8_t x, uint8_t y)
{
    std::cerr << "BaseWindow::stopped_in_front_of() mapIndex=" << mapIndex << " x=" << std::to_string(x) << " y=" << std::to_string(y) << std::endl;
    if(stopped_in_front_of_check_bot(mapIndex,x,y))
        return;
    else if(CatchChallenger::MoveOnTheMap::isDirt(*map,x,y))
    {
        showTip(tr("To plant a seed press <i>Enter</i>").toStdString());
        return;
    }
    else
    {
        if(!CatchChallenger::MoveOnTheMap::isWalkable(*map,x,y))
        {
            //check bot with border
            CATCHCHALLENGER_TYPE_MAPID borderMapIndex=mapIndex;
            switch(mapController->getDirection())
            {
                case CatchChallenger::Direction_look_at_left:
                if(QtDatapackClientLoader::datapackLoader->canGoTo(CatchChallenger::Direction_move_at_left,mapIndex,x,y,false))
                    if(QtDatapackClientLoader::datapackLoader->move(CatchChallenger::Direction_move_at_left,borderMapIndex,x,y,false))
                        stopped_in_front_of_check_bot(borderMapIndex,x,y);
                break;
                case CatchChallenger::Direction_look_at_right:
                if(QtDatapackClientLoader::datapackLoader->canGoTo(CatchChallenger::Direction_move_at_right,mapIndex,x,y,false))
                    if(QtDatapackClientLoader::datapackLoader->move(CatchChallenger::Direction_move_at_right,borderMapIndex,x,y,false))
                        stopped_in_front_of_check_bot(borderMapIndex,x,y);
                break;
                case CatchChallenger::Direction_look_at_top:
                if(QtDatapackClientLoader::datapackLoader->canGoTo(CatchChallenger::Direction_move_at_top,mapIndex,x,y,false))
                    if(QtDatapackClientLoader::datapackLoader->move(CatchChallenger::Direction_move_at_top,borderMapIndex,x,y,false))
                        stopped_in_front_of_check_bot(borderMapIndex,x,y);
                break;
                case CatchChallenger::Direction_look_at_bottom:
                if(QtDatapackClientLoader::datapackLoader->canGoTo(CatchChallenger::Direction_move_at_bottom,mapIndex,x,y,false))
                    if(QtDatapackClientLoader::datapackLoader->move(CatchChallenger::Direction_move_at_bottom,borderMapIndex,x,y,false))
                        stopped_in_front_of_check_bot(borderMapIndex,x,y);
                break;
                default:
                break;
            }
        }
    }
}

bool BaseWindow::stopped_in_front_of_check_bot(const CATCHCHALLENGER_TYPE_MAPID &mapIndex, uint8_t x, uint8_t y)
{
    std::cerr << "BaseWindow::stopped_in_front_of_check_bot() mapIndex=" << mapIndex << " x=" << std::to_string(x) << " y=" << std::to_string(y) << std::endl;
    const CatchChallenger::Bot *bot=QtDatapackClientLoader::datapackLoader->getBot(mapIndex,x,y);
    if(bot==nullptr)
    {
        std::cerr << "  getBot returned nullptr" << std::endl;
        return false;
    }
    std::cerr << "  bot found! botId=" << std::to_string(bot->botId) << " name=" << bot->name << " steps=" << bot->step.size() << std::endl;
    showTip(tr("To interact with the bot press <i><b>Enter</b></i>").toStdString());
    return true;
}

//return -1 if not found, else the index
int32_t BaseWindow::havePlant(const CatchChallenger::CommonMap *map, uint8_t x, uint8_t y) const
{
    (void)map;
    (void)x;
    (void)y;
    // Plant globally visible feature was dropped
    return -1;
}

void BaseWindow::actionOnNothing()
{
    ui->IG_dialog->setVisible(false);
}

void BaseWindow::actionOn(Map_client *map, const CATCHCHALLENGER_TYPE_MAPID &mapIndex, uint8_t x, uint8_t y)
{
    if(ui->IG_dialog->isVisible())
        ui->IG_dialog->setVisible(false);
    if(actionOnCheckBot(mapIndex,x,y))
        return;
    else if(CatchChallenger::MoveOnTheMap::isDirt(*map,x,y))
    {
        // Plant globally visible feature was dropped; just offer seed planting
        SeedInWaiting seedInWaiting;
        seedInWaiting.map=std::string();
        seedInWaiting.x=x;
        seedInWaiting.y=y;
        seed_in_waiting.push_back(seedInWaiting);

        selectObject(ObjectType_Seed);
        return;
    }
    else if(map->items.find(std::pair<uint8_t,uint8_t>(x,y))!=map->items.cend())
    {
        const ItemOnMap &item=map->items.at(std::pair<uint8_t,uint8_t>(x,y));
        add_to_inventory(item.item);
        client->takeAnObjectOnMap();
    }
    else
    {
        //check bot with border
        CATCHCHALLENGER_TYPE_MAPID borderMapIndex=mapIndex;
        switch(mapController->getDirection())
        {
            case CatchChallenger::Direction_look_at_left:
            if(QtDatapackClientLoader::datapackLoader->canGoTo(CatchChallenger::Direction_move_at_left,mapIndex,x,y,false))
                if(QtDatapackClientLoader::datapackLoader->move(CatchChallenger::Direction_move_at_left,borderMapIndex,x,y,false))
                    actionOnCheckBot(borderMapIndex,x,y);
            break;
            case CatchChallenger::Direction_look_at_right:
            if(QtDatapackClientLoader::datapackLoader->canGoTo(CatchChallenger::Direction_move_at_right,mapIndex,x,y,false))
                if(QtDatapackClientLoader::datapackLoader->move(CatchChallenger::Direction_move_at_right,borderMapIndex,x,y,false))
                    actionOnCheckBot(borderMapIndex,x,y);
            break;
            case CatchChallenger::Direction_look_at_top:
            if(QtDatapackClientLoader::datapackLoader->canGoTo(CatchChallenger::Direction_move_at_top,mapIndex,x,y,false))
                if(QtDatapackClientLoader::datapackLoader->move(CatchChallenger::Direction_move_at_top,borderMapIndex,x,y,false))
                    actionOnCheckBot(borderMapIndex,x,y);
            break;
            case CatchChallenger::Direction_look_at_bottom:
            if(QtDatapackClientLoader::datapackLoader->canGoTo(CatchChallenger::Direction_move_at_bottom,mapIndex,x,y,false))
                if(QtDatapackClientLoader::datapackLoader->move(CatchChallenger::Direction_move_at_bottom,borderMapIndex,x,y,false))
                    actionOnCheckBot(borderMapIndex,x,y);
            break;
            default:
            break;
        }
    }
}

bool BaseWindow::actionOnCheckBot(const CATCHCHALLENGER_TYPE_MAPID &mapIndex, uint8_t x, uint8_t y)
{
    const CatchChallenger::Bot *bot=QtDatapackClientLoader::datapackLoader->getBot(mapIndex,x,y);
    if(bot==nullptr)
        return false;
    actualBot=*bot;
    isInQuest=false;
    goToBotStep(1);
    return true;
}

void BaseWindow::botFightCollision(CatchChallenger::Map_client *map, uint8_t x, uint8_t y)
{
    // Bot data is now in BaseMap::botsFightTrigger and BaseMap::botFights
    if(map->botsFightTrigger.find(std::pair<uint8_t,uint8_t>(x,y))==map->botsFightTrigger.cend())
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"Bot trigged but no bot at this place");
        return;
    }
    const uint8_t botNpcId=map->botsFightTrigger.at(std::pair<uint8_t,uint8_t>(x,y));
    if(map->botFights.find(botNpcId)==map->botFights.cend())
    {
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"Bot trigged but no bot fight found for npc id");
        return;
    }
    (void)map->botFights.at(botNpcId); // validate the fight exists
    botFight(static_cast<uint16_t>(botNpcId));
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

void BaseWindow::pathFindingInternalError()
{
    showTip(tr("Internal error to resolv the path to go here").toStdString());
    //showTip(tr("Path finding disabled"));
}

void BaseWindow::currentMapLoaded()
{
    qDebug() << "BaseWindow::currentMapLoaded(): map: " << mapController->currentMap()
             << " with type: " << QString::fromStdString(mapController->currentMapType());
    if(AutoArgs::closeWhenOnMap)
    {
        std::cerr << "AutoArgs: --closewhenonmap, exiting in 1s" << std::endl;
        QTimer::singleShot(1000,QCoreApplication::instance(),&QCoreApplication::quit);
    }
    if(AutoArgs::closeWhenOnMapAfter>0 && !closeWhenOnMapAfterTimer_.isActive())
    {
        std::cerr << "AutoArgs: --closewhenonmapafter=" << AutoArgs::closeWhenOnMapAfter
                  << ", toggling direction each 1s, quitting in " << AutoArgs::closeWhenOnMapAfter << "s" << std::endl;
        closeWhenOnMapAfterRemaining_=AutoArgs::closeWhenOnMapAfter;
        connect(&closeWhenOnMapAfterTimer_,&QTimer::timeout,this,&BaseWindow::closeWhenOnMapAfterToggle);
        closeWhenOnMapAfterTimer_.start(1000);
    }
    if(AutoArgs::dropSendDataAfterOnMap && !CatchChallenger::Api_protocol::dropOutputAfterOnMap)
    {
        std::cerr << "AutoArgs: --dropsenddataafteronmap, dropping all client->server traffic from now on" << std::endl;
        CatchChallenger::Api_protocol::dropOutputAfterOnMap=true;
    }
    //name
    {
        QMap_client *mapFull=mapController->currentMapFull();
        std::string visualName;
        if(!mapFull->zone.empty())
            if(QtDatapackClientLoader::datapackLoader->has_zoneExtra(mapFull->zone))
            {
                const DatapackClientLoader::ZoneExtra &zoneExtra=QtDatapackClientLoader::datapackLoader->get_zoneExtra(mapFull->zone);
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
        QMap_client *mapFull=mapController->currentMapFull();
        if(!mapFull->zone.empty())
            if(QtDatapackClientLoader::datapackLoader->has_zoneExtra(mapFull->zone))
            {
                const DatapackClientLoader::ZoneExtra &zoneExtra=QtDatapackClientLoader::datapackLoader->get_zoneExtra(mapFull->zone);
                if(zoneExtra.audioAmbiance.find(type)!=zoneExtra.audioAmbiance.cend())
                {
                    const std::string &backgroundsound=zoneExtra.audioAmbiance.at(type);
                    if(!backgroundsound.empty() && !vectorcontainsAtLeastOne(soundList,backgroundsound))
                        soundList.push_back(backgroundsound);
                }
            }
        //general sound
        if(QtDatapackClientLoader::datapackLoader->has_audioAmbiance(type))
        {
            const std::string &backgroundsound=QtDatapackClientLoader::datapackLoader->get_audioAmbiance(type);
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
            if(QtDatapackClientLoader::datapackLoader->has_visualCategory(type))
            {
                const std::vector<DatapackClientLoader::VisualCategory::VisualCategoryCondition> &conditions=
                        QtDatapackClientLoader::datapackLoader->get_visualCategory(type).conditions;
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
                    const DatapackClientLoader::CCColor &c=QtDatapackClientLoader::datapackLoader->get_visualCategory(type).defaultColor;
                    mapController->setColor(QColor(c.r,c.g,c.b,c.a));
                }
            }
            else
                mapController->setColor(Qt::transparent);
        }
    }
}

void BaseWindow::closeWhenOnMapAfterToggle()
{
    closeWhenOnMapAfterRemaining_--;
    if(closeWhenOnMapAfterRemaining_<=0)
    {
        closeWhenOnMapAfterTimer_.stop();
        std::cerr << "AutoArgs: --closewhenonmapafter time elapsed, exiting" << std::endl;
        QCoreApplication::quit();
        return;
    }
    CatchChallenger::Direction current=mapController->getDirection();
    Qt::Key key;
    CatchChallenger::Direction next;
    switch(current)
    {
        case CatchChallenger::Direction_look_at_bottom:
            key=Qt::Key_Up; next=CatchChallenger::Direction_look_at_top;
        break;
        case CatchChallenger::Direction_look_at_top:
            key=Qt::Key_Down; next=CatchChallenger::Direction_look_at_bottom;
        break;
        case CatchChallenger::Direction_look_at_left:
            key=Qt::Key_Right; next=CatchChallenger::Direction_look_at_right;
        break;
        case CatchChallenger::Direction_look_at_right:
            key=Qt::Key_Left; next=CatchChallenger::Direction_look_at_left;
        break;
        default:
            key=Qt::Key_Up; next=CatchChallenger::Direction_look_at_top;
        break;
    }
    std::cerr << "AutoArgs: --closewhenonmapafter direction " << (int)next
              << " (" << closeWhenOnMapAfterRemaining_ << "s remaining)" << std::endl;
    {
        QKeyEvent press(QEvent::KeyPress, key, Qt::NoModifier);
        QKeyEvent release(QEvent::KeyRelease, key, Qt::NoModifier);
        mapController->keyPressEvent(&press);
        mapController->keyReleaseEvent(&release);
    }
    if(client!=nullptr)
        client->send_player_direction(next);
}
