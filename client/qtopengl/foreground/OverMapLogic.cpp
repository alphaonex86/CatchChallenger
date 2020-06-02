#include "OverMapLogic.hpp"
#include "../ConnexionManager.hpp"
#include "../AudioGL.hpp"
#include "../background/CCMap.hpp"
#include "../cc/QtDatapackClientLoader.hpp"

OverMapLogic::OverMapLogic(CCMap *ccmap, ConnexionManager *connexionManager)
{
    multiplayer=true;
    lastReplyTimeValue=0;
    worseQueryTime=0;
    this->connexionManager=connexionManager;
    this->ccmap=ccmap;

    if(connect(ccmap,&CCMap::pathFindingNotFound,this,&OverMapLogic::pathFindingNotFound))
        abort();
    if(connect(ccmap,&CCMap::repelEffectIsOver,this,&OverMapLogic::repelEffectIsOver))
        abort();
    if(connect(ccmap,&CCMap::teleportConditionNotRespected,this,&OverMapLogic::teleportConditionNotRespected))
        abort();
    if(connect(ccmap,&CCMap::send_player_direction,connexionManager->client,&CatchChallenger::Api_protocol_Qt::send_player_direction))
        abort();
    if(connect(ccmap,&CCMap::stopped_in_front_of,this,&OverMapLogic::stopped_in_front_of))
        abort();
    if(connect(ccmap,&CCMap::actionOn,this,&OverMapLogic::actionOn))
        abort();
    if(connect(ccmap,&CCMap::actionOnNothing,this,&OverMapLogic::actionOnNothing))
        abort();
    if(connect(ccmap,&CCMap::blockedOn,this,&OverMapLogic::blockedOn))
        abort();
    if(connect(ccmap,&CCMap::error,this,&OverMapLogic::error))
        abort();
    if(connect(ccmap,&CCMap::errorWithTheCurrentMap,this,&OverMapLogic::errorWithTheCurrentMap))
        abort();
    if(connect(ccmap,&CCMap::currentMapLoaded,this,&OverMapLogic::currentMapLoaded))
        abort();

    checkQueryTime.start(200);
    if(!connect(&checkQueryTime,&QTimer::timeout,   this,&OverMapLogic::detectSlowDown))
        abort();
    tip_timeout.setInterval(8000);
    gain_timeout.setInterval(500);
    tip_timeout.setSingleShot(true);
    gain_timeout.setSingleShot(true);
}

void OverMapLogic::resetAll()
{
    lastReplyTimeValue=0;
    worseQueryTime=0;
    add_to_inventoryGainList.clear();
    add_to_inventoryGainTime.clear();
    add_to_inventoryGainExtraList.clear();
    add_to_inventoryGainExtraTime.clear();
    currentAmbianceFile.clear();
    visualCategory.clear();
    OverMap::resetAll();
    #ifndef CATCHCHALLENGER_NOAUDIO
    AudioGL::audio->stopCurrentAmbiance();
    #endif
}

void OverMapLogic::lastReplyTime(const uint32_t &time)
{
    //ui->labelLastReplyTime->setText(tr("Last reply time: %1ms").arg(time));
    if(lastReplyTimeValue<=700 || lastReplyTimeSince.elapsed()>15*1000)
    {
        lastReplyTimeValue=time;
        lastReplyTimeSince.restart();
    }
    else
    {
        if(time>700)
        {
            lastReplyTimeValue=time;
            lastReplyTimeSince.restart();
        }
    }
    updateTheTurtle();
}

void OverMapLogic::pathFindingNotFound()
{
    showTip("No path to go here");
}

void OverMapLogic::repelEffectIsOver()
{
    showTip("The repel effect is over");
}

void OverMapLogic::teleportConditionNotRespected(const std::string &text)
{
    showTip(text);
}

void OverMapLogic::stopped_in_front_of(CatchChallenger::Map_client *map, uint8_t x, uint8_t y)
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
            switch(ccmap->mapController.getDirection())
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

void OverMapLogic::setIG_dialog(QString text,QString name)
{
    this->IG_dialog_textString=text;
    this->IG_dialog_nameString=name;
    this->IG_dialog_text->setHtml(text);
    this->IG_dialog_name->setHtml(name);
}

void OverMapLogic::connectAllSignals()
{

    //for bug into solo player: Api_client_real -> Api_protocol
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtlastReplyTime,      this,&OverMapLogic::lastReplyTime,    Qt::UniqueConnection))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtclanActionFailed,   this,&OverMapLogic::clanActionFailed, Qt::UniqueConnection))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtclanActionSuccess,  this,&OverMapLogic::clanActionSuccess,Qt::UniqueConnection))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtclanDissolved,      this,&OverMapLogic::clanDissolved,    Qt::UniqueConnection))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtclanInformations,   this,&OverMapLogic::clanInformations, Qt::UniqueConnection))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtclanInvite,         this,&OverMapLogic::clanInvite,       Qt::UniqueConnection))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtcityCapture,        this,&OverMapLogic::cityCapture,      Qt::UniqueConnection))
        abort();

    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtcaptureCityYourAreNotLeader,                this,&OverMapLogic::captureCityYourAreNotLeader,              Qt::UniqueConnection))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtcaptureCityYourLeaderHaveStartInOtherCity,  this,&OverMapLogic::captureCityYourLeaderHaveStartInOtherCity,Qt::UniqueConnection))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtcaptureCityPreviousNotFinished,             this,&OverMapLogic::captureCityPreviousNotFinished,           Qt::UniqueConnection))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtcaptureCityStartBattle,                     this,&OverMapLogic::captureCityStartBattle,                   Qt::UniqueConnection))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtcaptureCityStartBotFight,                   this,&OverMapLogic::captureCityStartBotFight,                 Qt::UniqueConnection))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtcaptureCityDelayedStart,                    this,&OverMapLogic::captureCityDelayedStart,                  Qt::UniqueConnection))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtcaptureCityWin,                             this,&OverMapLogic::captureCityWin,                           Qt::UniqueConnection))
        abort();

    //inventory
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::Qthave_inventory,     this,&OverMapLogic::have_inventory))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::Qtadd_to_inventory,   this,&OverMapLogic::add_to_inventory_slot))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::Qtremove_to_inventory,this,&OverMapLogic::remove_to_inventory_slot))
        abort();

    //plants
    if(!connect(this,&OverMapLogic::useSeed,              connexionManager->client,&CatchChallenger::Api_client_real::useSeed))
        abort();
    if(!connect(this,&OverMapLogic::collectMaturePlant,   connexionManager->client,&CatchChallenger::Api_client_real::collectMaturePlant))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::Qtinsert_plant,   this,&OverMapLogic::insert_plant))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::Qtremove_plant,   this,&OverMapLogic::remove_plant))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::Qtseed_planted,   this,&OverMapLogic::seed_planted))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::Qtplant_collected,this,&OverMapLogic::plant_collected))
        abort();
    //crafting
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtrecipeUsed,     this,&OverMapLogic::recipeUsed))
        abort();
    //trade
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QttradeRequested,             this,&OverMapLogic::tradeRequested))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QttradeAcceptedByOther,       this,&OverMapLogic::tradeAcceptedByOther))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QttradeCanceledByOther,       this,&OverMapLogic::tradeCanceledByOther))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QttradeFinishedByOther,       this,&OverMapLogic::tradeFinishedByOther))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QttradeValidatedByTheServer,  this,&OverMapLogic::tradeValidatedByTheServer))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QttradeAddTradeCash,          this,&OverMapLogic::tradeAddTradeCash))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QttradeAddTradeObject,        this,&OverMapLogic::tradeAddTradeObject))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QttradeAddTradeMonster,       this,&OverMapLogic::tradeAddTradeMonster))
        abort();
    //inventory
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtobjectUsed,                 this,&OverMapLogic::objectUsed))
        abort();
    //shop
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QthaveShopList,               this,&OverMapLogic::haveShopList))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QthaveSellObject,             this,&OverMapLogic::haveSellObject))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QthaveBuyObject,              this,&OverMapLogic::haveBuyObject))
        abort();
    //factory
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QthaveFactoryList,            this,&OverMapLogic::haveFactoryList))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QthaveSellFactoryObject,      this,&OverMapLogic::haveSellFactoryObject))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QthaveBuyFactoryObject,       this,&OverMapLogic::haveBuyFactoryObject))
        abort();
    //battle
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtbattleRequested,            this,&OverMapLogic::battleRequested))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtbattleAcceptedByOther,      this,&OverMapLogic::battleAcceptedByOther))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtbattleCanceledByOther,      this,&OverMapLogic::battleCanceledByOther))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtsendBattleReturn,           this,&OverMapLogic::sendBattleReturn))
        abort();
    //market
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtmarketList,                 this,&OverMapLogic::marketList))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtmarketBuy,                  this,&OverMapLogic::marketBuy))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtmarketBuyMonster,           this,&OverMapLogic::marketBuyMonster))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtmarketPut,                  this,&OverMapLogic::marketPut))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtmarketGetCash,              this,&OverMapLogic::marketGetCash))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtmarketWithdrawCanceled,     this,&OverMapLogic::marketWithdrawCanceled))
       abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtmarketWithdrawObject,       this,&OverMapLogic::marketWithdrawObject))
       abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtmarketWithdrawMonster,      this,&OverMapLogic::marketWithdrawMonster))
       abort();
    //fight
/*    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtteleportTo,this,&OverMapLogic::teleportTo,Qt::UniqueConnection))
       abort();*/
}

void OverMapLogic::actionOn(CatchChallenger::Map_client *map, uint8_t x, uint8_t y)
{
    setIG_dialog(QString());
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
                if(QtDatapackClientLoader::datapackLoader->plantOnMap.find(map->map_file)==
                        QtDatapackClientLoader::datapackLoader->plantOnMap.cend())
                    return;
                const std::unordered_map<std::pair<uint8_t,uint8_t>,uint16_t,pairhash> &plant=QtDatapackClientLoader::datapackLoader->plantOnMap.at(map->map_file);
                if(plant.find(std::pair<uint8_t,uint8_t>(x,y))==plant.cend())
                    return;
                emit collectMaturePlant();

                connexionManager->client->remove_plant(ccmap->mapController.getMap(map->map_file)->logicalMap.id,x,y);
                connexionManager->client->plant_collected(CatchChallenger::Plant_collect::Plant_collect_correctly_collected);
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
        CatchChallenger::Player_private_and_public_informations &informations=connexionManager->client->get_player_informations();
        const CatchChallenger::Map_client::ItemOnMapForClient &item=map->itemsOnMap.at(std::pair<uint8_t,uint8_t>(x,y));
        if(informations.itemOnMap.find(item.indexOfItemOnMap)==informations.itemOnMap.cend())
        {
            if(!item.infinite)
                informations.itemOnMap.insert(item.indexOfItemOnMap);
            add_to_inventory(item.item);
            connexionManager->client->takeAnObjectOnMap();
        }
    }
    else
    {
        //check bot with border
        CatchChallenger::CommonMap * current_map=map;
        switch(ccmap->mapController.getDirection())
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

void OverMapLogic::actionOnNothing()
{
    setIG_dialog(QString());
}

int32_t OverMapLogic::havePlant(CatchChallenger::Map_client *map, uint8_t x, uint8_t y) const
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

void OverMapLogic::blockedOn(const MapVisualiserPlayer::BlockedOn &blockOnVar)
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

void OverMapLogic::errorWithTheCurrentMap()
{
    if(connexionManager->state()!=QAbstractSocket::ConnectedState)
        return;
    connexionManager->client->tryDisconnect();
    resetAll();
    error(tr("The current map into the datapack is in error (not found, read failed, wrong format, corrupted, ...)\nReport the bug to the datapack maintainer.").toStdString());
}

void OverMapLogic::currentMapLoaded()
{
    qDebug() << "OverMapLogic::currentMapLoaded(): map: " << QString::fromStdString(ccmap->mapController.currentMap())
             << " with type: " << QString::fromStdString(ccmap->mapController.currentMapType());
    //name
    {
        Map_full *mapFull=ccmap->mapController.currentMapFull();
        std::string visualName;
        if(!mapFull->zone.empty())
            if(QtDatapackClientLoader::datapackLoader->zonesExtra.find(mapFull->zone)!=
                    QtDatapackClientLoader::datapackLoader->zonesExtra.cend())
            {
                const QtDatapackClientLoader::ZoneExtra &zoneExtra=QtDatapackClientLoader::datapackLoader->zonesExtra.at(mapFull->zone);
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
    const std::string &type=ccmap->mapController.currentMapType();
    #ifndef CATCHCHALLENGER_NOAUDIO
    //sound
    {
        std::vector<std::string> soundList;
        const std::string &backgroundsound=ccmap->mapController.currentBackgroundsound();
        //map sound
        if(!backgroundsound.empty() && !vectorcontainsAtLeastOne(soundList,backgroundsound))
            soundList.push_back(backgroundsound);
        //zone sound
        Map_full *mapFull=ccmap->mapController.currentMapFull();
        if(!mapFull->zone.empty())
            if(QtDatapackClientLoader::datapackLoader->zonesExtra.find(mapFull->zone)!=QtDatapackClientLoader::datapackLoader->zonesExtra.cend())
            {
                const QtDatapackClientLoader::ZoneExtra &zoneExtra=QtDatapackClientLoader::datapackLoader->zonesExtra.at(mapFull->zone);
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
            const std::string &fileToSearchMain=connexionManager->client->datapackPathMain()+soundList.at(index);
            if(QFileInfo(QString::fromStdString(fileToSearchMain)).isFile())
            {
                finalSound=fileToSearchMain;
                break;
            }
            //search into base datapack
            const std::string &fileToSearchBase=connexionManager->client->datapackPathBase()+soundList.at(index);
            if(QFileInfo(QString::fromStdString(fileToSearchBase)).isFile())
            {
                finalSound=fileToSearchBase;
                break;
            }
            index++;
        }

        //set the sound
        if(!finalSound.empty() && currentAmbianceFile!=finalSound)
            AudioGL::audio->startAmbiance(finalSound);
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
                const std::vector<QtDatapackClientLoader::VisualCategory::VisualCategoryCondition> &conditions=
                        QtDatapackClientLoader::datapackLoader->visualCategories.at(type).conditions;
                unsigned int index=0;
                while(index<conditions.size())
                {
                    const QtDatapackClientLoader::VisualCategory::VisualCategoryCondition &condition=conditions.at(index);
                    const std::vector<uint8_t> events=connexionManager->client->getEvents();
                    if(condition.event<events.size())
                    {
                        if(events.at(condition.event)==condition.eventValue)
                        {
                            ccmap->mapController.setColor(QColor(condition.color.r,condition.color.g,condition.color.b,condition.color.a));
                            break;
                        }
                    }
                    else
                        qDebug() << QStringLiteral("event for condition out of range: %1 for %2 event(s)").arg(condition.event).arg(events.size());
                    index++;
                }
                if(index==conditions.size())
                {
                    QtDatapackClientLoader::CCColor defaultColor=QtDatapackClientLoader::datapackLoader->visualCategories.at(type).defaultColor;
                    ccmap->mapController.setColor(QColor(defaultColor.r,defaultColor.g,defaultColor.b,defaultColor.a),15000);
                }
            }
            else
                ccmap->mapController.setColor(Qt::transparent);
        }
    }
}

void OverMapLogic::tipTimeout()
{
    hideTip();
}

void OverMapLogic::gainTimeout()
{
    unsigned int index=0;
    while(index<add_to_inventoryGainTime.size())
    {
        if(add_to_inventoryGainTime.at(index).elapsed()>8000)
        {
            add_to_inventoryGainTime.erase(add_to_inventoryGainTime.cbegin()+index);
            add_to_inventoryGainList.erase(add_to_inventoryGainList.cbegin()+index);
        }
        else
            index++;
    }
    index=0;
    while(index<add_to_inventoryGainExtraTime.size())
    {
        if(add_to_inventoryGainExtraTime.at(index).elapsed()>8000)
        {
            add_to_inventoryGainExtraTime.erase(add_to_inventoryGainExtraTime.cbegin()+index);
            add_to_inventoryGainExtraList.erase(add_to_inventoryGainExtraList.cbegin()+index);
        }
        else
            index++;
    }
    if(add_to_inventoryGainTime.empty() && add_to_inventoryGainExtraTime.empty())
        hideGain();
    else
    {
        gain_timeout.start();
        composeAndDisplayGain();
    }
}

void OverMapLogic::showTip(const std::string &tip)
{
    tipString=QString::fromStdString(tip);
    this->tip->setHtml(tipString);
    tip_timeout.start();
}

void OverMapLogic::hideTip()
{
    tipString.clear();
    this->tip->setHtml(QString());
}

void OverMapLogic::showPlace(const std::string &place)
{
    add_to_inventoryGainExtraList.push_back(place);
    add_to_inventoryGainExtraTime.push_back(QTime::currentTime());
    //ui->gain->setVisible(true);
    composeAndDisplayGain();
    gain_timeout.start();
}

void OverMapLogic::showGain()
{
    gainTimeout();
    //ui->gain->setVisible(true);
    composeAndDisplayGain();
    gain_timeout.start();
}

void OverMapLogic::hideGain()
{
    gainString.clear();
    gain->setHtml(QString());
}

void OverMapLogic::composeAndDisplayGain()
{
    std::string text;
    text+=stringimplode(add_to_inventoryGainExtraList,"<br />");
    if(!add_to_inventoryGainList.empty() && !text.empty())
        text+="<br />";
    if(add_to_inventoryGainList.size()>1)
        text+=tr("You have obtained: ").toStdString()+"<ul><li>"+
                stringimplode(add_to_inventoryGainList,"</li><li>")
                +"</li></ul>";
    else if(!add_to_inventoryGainList.empty())
        text+=tr("You have obtained: ").toStdString()+stringimplode(add_to_inventoryGainList,"");
    gainString=QString::fromStdString(text);
    gain->setHtml(gainString);
}

void OverMapLogic::addQuery(const QueryType &queryType)
{
    queryList.push_back(queryType);
    updateQueryList();
}

void OverMapLogic::removeQuery(const QueryType &queryType)
{
    vectorremoveOne(queryList,queryType);
    updateQueryList();
}

void OverMapLogic::updateQueryList()
{
    QStringList queryStringList;
    unsigned int index=0;
    while(index<queryList.size() && index<5)
    {
        switch(queryList.at(index))
        {
            case QueryType_Seed:
                queryStringList << tr("Planting seed...");
            break;
            case QueryType_CollectPlant:
                queryStringList << tr("Collecting plant...");
            break;
            default:
                queryStringList << tr("Unknown action...");
            break;
        }
        index++;
    }
    persistant_tipString=queryStringList.join("<br />");
    persistant_tip->setHtml(persistant_tipString);
}

void OverMapLogic::detectSlowDown()
{
    if(connexionManager->client==nullptr)
        return;
    uint32_t queryCount=0;
    worseQueryTime=0;
    std::vector<std::string> middleQueryList;
    const std::map<uint8_t,uint64_t> &values=connexionManager->client->getQuerySendTimeList();
    queryCount+=values.size();
    for(const auto n : values) {
        middleQueryList.push_back(std::to_string(n.first));
        std::time_t result = std::time(nullptr);
        if((uint64_t)result>=n.second)
        {
            const uint32_t &time=result-n.second;
            if(time>worseQueryTime)
                worseQueryTime=time;
        }
    }
    /*if(queryCount>0)
        ui->labelQueryList->setText(
                    tr("Running query: %1 (%3 and %4), query with worse time: %2ms")
                    .arg(queryCount)
                    .arg(worseQueryTime)
                    .arg(QString::fromStdString(stringimplode(connexionManager->client->getQueryRunningList(),';')))
                    .arg(QString::fromStdString(stringimplode(middleQueryList,";")))
                    );
    else
        ui->labelQueryList->setText(tr("No query running"));*/
    updateTheTurtle();
}

void OverMapLogic::updateTheTurtle()
{
    if(!multiplayer)
    {
        if(!labelSlow->isVisible())
            return;
        labelSlow->hide();
        return;
    }
    if(lastReplyTimeValue>700 && lastReplyTimeSince.elapsed()<15*1000)
    {
        //qDebug() << "The last query was slow:" << lastReplyTimeValue << ">" << 700 << " && " << lastReplyTimeSince.elapsed() << "<" << 15*1000;
        if(labelSlow->isVisible())
            return;
        labelSlow->setToolTip(tr("The last query was slow (%1ms)").arg(lastReplyTimeValue));
        labelSlow->show();
        return;
    }
    if(worseQueryTime>700)
    {
        if(labelSlow->isVisible())
            return;
        labelSlow->setToolTip(tr("Remain query in suspend (%1ms ago)").arg(worseQueryTime));
        labelSlow->show();
        return;
    }
    if(!labelSlow->isVisible())
        return;
    labelSlow->hide();
}

bool OverMapLogic::stopped_in_front_of_check_bot(CatchChallenger::Map_client *map, uint8_t x, uint8_t y)
{
    if(map->bots.find(std::pair<uint8_t,uint8_t>(x,y))==map->bots.cend())
        return false;
    showTip(tr("To interact with the bot press <i><b>Enter</b></i>").toStdString());
    return true;
}

bool OverMapLogic::actionOnCheckBot(CatchChallenger::Map_client *map, uint8_t x, uint8_t y)
{
    if(map->bots.find(std::pair<uint8_t,uint8_t>(x,y))==map->bots.cend())
        return false;
    actualBot=map->bots.at(std::pair<uint8_t,uint8_t>(x,y));
    isInQuest=false;
    goToBotStep(1);
    return true;
}

void OverMapLogic::teleportTo(const uint32_t &mapId,const uint16_t &x,const uint16_t &y,const CatchChallenger::Direction &direction)
{
    Q_UNUSED(mapId);
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(direction);
    if(fightEngine.currentMonsterIsKO() && !fightEngine.haveAnotherMonsterOnThePlayerToFight())//then is dead, is teleported to the last rescue point
    {
        qDebug() << "tp on loose: " << fightTimerFinish;
        if(fightTimerFinish)
            loose();
        else
            fightTimerFinish=true;
    }
    else
        qDebug() << "normal tp";
}

void OverMapLogic::loose()
{
    qDebug() << "loose()";
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    PublicPlayerMonster *currentMonster=fightEngine.getCurrentMonster();
    if(currentMonster!=NULL)
        if((int)currentMonster->hp!=ui->progressBarFightBottomHP->value())
        {
            emit error(QStringLiteral("Current monster damage don't match with the internal value (loose && currentMonster): %1!=%2")
                       .arg(currentMonster->hp)
                       .arg(ui->progressBarFightBottomHP->value())
                       .toStdString()
                       );
            return;
        }
    PublicPlayerMonster *otherMonster=fightEngine.getOtherMonster();
    if(otherMonster!=NULL)
        if((int)otherMonster->hp!=ui->progressBarFightTopHP->value())
        {
            emit error(QStringLiteral("Current monster damage don't match with the internal value (loose && otherMonster): %1!=%2")
                       .arg(otherMonster->hp)
                       .arg(ui->progressBarFightTopHP->value())
                       .toStdString()
                       );
            return;
        }
    #endif
    if(CatchChallenger::CommonDatapack::commonDatapack.monsters.empty())
        return;
    fightEngine.healAllMonsters();
    fightEngine.fightFinished();
    ccmap->mapController.unblock();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    fightTimerFinish=false;
    escape=false;
    doNextActionStep=DoNextActionStep_Start;
    load_monsters();
    switch(battleType)
    {
        case BattleType_Bot:
            if(botFightList.empty())
            {
                emit error("battle info not found at collision");
                return;
            }
            botFightList.erase(botFightList.cbegin());
            fightId=0;
        break;
        case BattleType_OtherPlayer:
            if(battleInformationsList.empty())
            {
                emit error("battle info not found at collision");
                return;
            }
            battleInformationsList.erase(battleInformationsList.cbegin());
        break;
        default:
        break;
    }
    if(!battleInformationsList.empty())
    {
        const BattleInformations &battleInformations=battleInformationsList.front();
        battleInformationsList.erase(battleInformationsList.cbegin());
        battleAcceptedByOtherFull(battleInformations);
    }
    else if(!botFightList.empty())
    {
        uint16_t fightId=botFightList.front();
        botFightList.erase(botFightList.cbegin());
        botFight(fightId);
    }
    checkEvolution();
}

void OverMapLogic::clanActionSuccess(const uint32_t &clanId)
{
    Player_private_and_public_informations &playerInformations=client->get_player_informations();
    switch(actionClan.front())
    {
        case ActionClan_Create:
            if(playerInformations.clan==0)
            {
                playerInformations.clan=clanId;
                playerInformations.clan_leader=true;
            }
            updateClanDisplay();
            showTip(tr("The clan is created").toStdString());
        break;
        case ActionClan_Leave:
        case ActionClan_Dissolve:
            playerInformations.clan=0;
            updateClanDisplay();
            showTip(tr("You are leaved the clan").toStdString());
        break;
        case ActionClan_Invite:
            showTip(tr("You have correctly invited the player").toStdString());
        break;
        case ActionClan_Eject:
            showTip(tr("You have correctly ejected the player from clan").toStdString());
        break;
        default:
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"ActionClan unknown");
        return;
    }
    actionClan.erase(actionClan.cbegin());
}

void OverMapLogic::clanActionFailed()
{
    switch(actionClan.front())
    {
        case ActionClan_Create:
            updateClanDisplay();
        break;
        case ActionClan_Leave:
        case ActionClan_Dissolve:
        break;
        case ActionClan_Invite:
            showTip(tr("You have failed to invite the player").toStdString());
        break;
        case ActionClan_Eject:
            showTip(tr("You have failed to eject the player from clan").toStdString());
        break;
        default:
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"ActionClan unknown");
        return;
    }
    actionClan.erase(actionClan.cbegin());
}

void OverMapLogic::clanDissolved()
{
    Player_private_and_public_informations &playerInformations=client->get_player_informations();
    haveClanInformations=false;
    clanName.clear();
    playerInformations.clan=0;
    updateClanDisplay();
}

void OverMapLogic::updateClanDisplay()
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();//do a crash due to reference
    //const CatchChallenger::Player_private_and_public_informations playerInformations=client->get_player_informations_ro();
    ui->tabWidgetTrainerCard->setTabEnabled(4,playerInformations.clan!=0);
    ui->clanGrouBoxNormal->setVisible(!playerInformations.clan_leader);
    ui->clanGrouBoxLeader->setVisible(playerInformations.clan_leader);
    ui->clanGrouBoxInformations->setVisible(haveClanInformations);
    if(haveClanInformations)
    {
        if(clanName.empty())
            ui->clanName->setText(tr("Your clan"));
        else
            ui->clanName->setText(QString::fromStdString(clanName));
    }
    chat->setClan(playerInformations.clan!=0);
}

void OverMapLogic::on_clanActionLeave_clicked()
{
    actionClan.push_back(ActionClan_Leave);
    client->leaveClan();
}

void OverMapLogic::on_clanActionDissolve_clicked()
{
    actionClan.push_back(ActionClan_Dissolve);
    client->dissolveClan();
}

void OverMapLogic::on_clanActionInvite_clicked()
{
    bool ok;
    std::string text = QInputDialog::getText(this,tr("Give the player pseudo"),tr("Player pseudo to invite:"),QLineEdit::Normal,QString(), &ok).toStdString();
    if(ok && !text.empty())
    {
        actionClan.push_back(ActionClan_Invite);
        client->inviteClan(text);
    }
}

void OverMapLogic::on_clanActionEject_clicked()
{
    bool ok;
    std::string text = QInputDialog::getText(this,tr("Give the player pseudo"),tr("Player pseudo to invite:"),QLineEdit::Normal,QString(), &ok).toStdString();
    if(ok && !text.empty())
    {
        actionClan.push_back(ActionClan_Eject);
        client->ejectClan(text);
    }
}

void OverMapLogic::clanInformations(const std::string &name)
{
    haveClanInformations=true;
    clanName=name;
    updateClanDisplay();
}

void OverMapLogic::clanInvite(const uint32_t &clanId,const std::string &name)
{
    QMessageBox::StandardButton button=QMessageBox::question(this,tr("Invite"),tr("The clan %1 invite you to become a member. Do you accept?")
                                                             .arg(QStringLiteral("<b>%1</b>").arg(QString::fromStdString(name))));
    client->inviteAccept(button==QMessageBox::Yes);
    if(button==QMessageBox::Yes)
    {
        Player_private_and_public_informations &playerInformations=client->get_player_informations();
        playerInformations.clan=clanId;
        playerInformations.clan_leader=false;
        haveClanInformations=false;
        updateClanDisplay();
    }
}

void OverMapLogic::cityCaptureUpdateTime()
{
    if(city.capture.frenquency==City::Capture::Frequency_week)
        nextCatch=QDateTime::fromMSecsSinceEpoch(QDateTime::currentMSecsSinceEpoch()+24*3600*7*1000);
    else
        nextCatch=QDateTime::fromMSecsSinceEpoch(FacilityLib::nextCaptureTime(city));
    nextCityCatchTimer.start(static_cast<uint32_t>(nextCatch.toMSecsSinceEpoch()-QDateTime::currentMSecsSinceEpoch()));
}

void OverMapLogic::updatePageZoneCatch()
{
    if(QDateTime::currentMSecsSinceEpoch()<nextCatchOnScreen.toMSecsSinceEpoch())
    {
        int sec=static_cast<uint32_t>(nextCatchOnScreen.toMSecsSinceEpoch()-QDateTime::currentMSecsSinceEpoch())/1000+1;
        std::string timeText;
        if(sec>3600*24*365*50)
            timeText="Time player: bug";
        else
            timeText=FacilityLibClient::timeToString(sec);
        ui->zonecaptureWaitTime->setText(tr("Remaining time: %1")
                                         .arg(QStringLiteral("<b>%1</b>")
                                         .arg(QString::fromStdString(timeText)))
                                         );
        ui->zonecaptureCancel->setVisible(true);
    }
    else
    {
        ui->zonecaptureCancel->setVisible(false);
        ui->zonecaptureWaitTime->setText("<i>"+tr("In waiting of players list")+"</i>");
        updater_page_zonecatch.stop();
    }
}

void OverMapLogic::on_zonecaptureCancel_clicked()
{
    updater_page_zonecatch.stop();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    client->waitingForCityCapture(true);
    zonecatch=false;
}

void OverMapLogic::captureCityYourAreNotLeader()
{
    updater_page_zonecatch.stop();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    showTip(tr("You are not a clan leader to start a city capture").toStdString());
    zonecatch=false;
}

void OverMapLogic::captureCityYourLeaderHaveStartInOtherCity(const std::string &zone)
{
    updater_page_zonecatch.stop();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    if(QtDatapackClientLoader::datapackLoader->zonesExtra.find(zone)!=QtDatapackClientLoader::datapackLoader->zonesExtra.cend())
        showTip(tr("Your clan leader have start a capture for another city").toStdString()+": <b>"+
                QtDatapackClientLoader::datapackLoader->zonesExtra.at(zone).name+
                "</b>");
    else
        showTip(tr("Your clan leader have start a capture for another city").toStdString());
    zonecatch=false;
}

void OverMapLogic::captureCityPreviousNotFinished()
{
    updater_page_zonecatch.stop();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    showTip(tr("Previous capture of this city is not finished").toStdString());
    zonecatch=false;
}

void OverMapLogic::captureCityStartBattle(const uint16_t &player_count,const uint16_t &clan_count)
{
    ui->zonecaptureCancel->setVisible(false);
    ui->zonecaptureWaitTime->setText("<i>"+tr("%1 and %2 in wainting to capture the city").arg("<b>"+tr("%n player(s)","",player_count)+"</b>").arg("<b>"+tr("%n clan(s)","",clan_count)+"</b>")+"</i>");
    updater_page_zonecatch.stop();
}

void OverMapLogic::captureCityStartBotFight(const uint16_t &player_count,const uint16_t &clan_count,const uint16_t &fightId)
{
    ui->zonecaptureCancel->setVisible(false);
    ui->zonecaptureWaitTime->setText("<i>"+tr("%1 and %2 in wainting to capture the city").arg("<b>"+tr("%n player(s)","",player_count)+"</b>").arg("<b>"+tr("%n clan(s)","",clan_count)+"</b>")+"</i>");
    updater_page_zonecatch.stop();
    botFight(fightId);
}

void OverMapLogic::captureCityDelayedStart(const uint16_t &player_count,const uint16_t &clan_count)
{
    ui->zonecaptureCancel->setVisible(false);
    ui->zonecaptureWaitTime->setText("<i>"+tr("In waiting fight.")+" "+tr("%1 and %2 in wainting to capture the city").arg("<b>"+tr("%n player(s)","",player_count)+"</b>").arg("<b>"+tr("%n clan(s)","",clan_count)+"</b>")+"</i>");
    updater_page_zonecatch.stop();
}

void OverMapLogic::captureCityWin()
{
    updater_page_zonecatch.stop();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    if(!zonecatchName.empty())
        showTip(tr("Your clan win the city").toStdString()+": <b>"+
                zonecatchName+"</b>");
    else
        showTip(tr("Your clan win the city").toStdString());
    zonecatch=false;
}

void OverMapLogic::have_inventory(const std::unordered_map<uint16_t,uint32_t> &items, const std::unordered_map<uint16_t, uint32_t> &warehouse_items)
{
    CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations();
    playerInformations.items=items;
    playerInformations.warehouse_items=warehouse_items;
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "OverMapLogic::have_inventory()";
    #endif
    haveInventory=true;
    updateConnectingStatus();
}

void OverMapLogic::load_inventory()
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "OverMapLogic::load_inventory()";
    #endif
    if(!haveInventory || !datapackIsParsed)
        return;
    ui->inventoryInformation->setVisible(false);
    ui->inventoryUse->setVisible(false);
    ui->inventoryDestroy->setVisible(false);
    ui->inventory->clear();
    items_graphical.clear();
    items_to_graphical.clear();
    auto i=playerInformations.items.begin();
    while (i!=playerInformations.items.cend())
    {
        bool show=!inSelection;
        if(inSelection)
        {
            switch(waitedObjectType)
            {
                case ObjectType_Seed:
                    //reputation requierement control is into load_plant_inventory() NOT: on_listPlantList_itemSelectionChanged()
                    if(QtDatapackClientLoader::datapackLoader->itemToPlants.find(i->first)!=QtDatapackClientLoader::datapackLoader->itemToPlants.cend())
                        show=true;
                break;
                case ObjectType_UseInFight:
                    if(fightEngine.isInFightWithWild() && CommonDatapack::commonDatapack.items.trap.find(i->first)!=CommonDatapack::commonDatapack.items.trap.cend())
                        show=true;
                    else if(CommonDatapack::commonDatapack.items.monsterItemEffect.find(i->first)!=CommonDatapack::commonDatapack.items.monsterItemEffect.cend())
                        show=true;
                    else
                        show=false;
                break;
                default:
                qDebug() << "waitedObjectType is unknow into load_inventory()";
                break;
            }
        }
        if(show)
        {
            QListWidgetItem *item=new QListWidgetItem();
            items_to_graphical[i->first]=item;
            items_graphical[item]=i->first;
            if(QtDatapackClientLoader::datapackLoader->itemsExtra.find(i->first)!=QtDatapackClientLoader::datapackLoader->itemsExtra.cend())
            {
                item->setIcon(QtDatapackClientLoader::datapackLoader->QtitemsExtra.at(i->first).image);
                if(i->second>1)
                    item->setText(QString::number(i->second));
                item->setToolTip(QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra.at(i->first).name));
            }
            else
            {
                item->setIcon(QtDatapackClientLoader::datapackLoader->defaultInventoryImage());
                if(i->second>1)
                    item->setText(QStringLiteral("id: %1 (x%2)").arg(i->first).arg(i->second));
                else
                    item->setText(QStringLiteral("id: %1").arg(i->first));
            }
            ui->inventory->addItem(item);
        }
        ++i;
    }
}


void OverMapLogic::add_to_inventory_slot(const std::unordered_map<uint16_t,uint32_t> &items)
{
    add_to_inventory(items);
}

void OverMapLogic::add_to_inventory(const uint16_t &item,const uint32_t &quantity,const bool &showGain)
{
    std::vector<std::pair<uint16_t,uint32_t> > items;
    items.push_back(std::pair<uint16_t,uint32_t>(item,quantity));
    add_to_inventory(items,showGain);
}

void OverMapLogic::add_to_inventory(const std::vector<std::pair<uint16_t,uint32_t> > &items,const bool &showGain)
{
    unsigned int index=0;
    std::unordered_map<uint16_t,uint32_t> tempHash;
    while(index<items.size())
    {
        tempHash[items.at(index).first]=items.at(index).second;
        index++;
    }
    add_to_inventory(tempHash,showGain);
}

void OverMapLogic::add_to_inventory(const std::unordered_map<uint16_t,uint32_t> &items,const bool &showGain)
{
    Player_private_and_public_informations &playerInformations=client->get_player_informations();
    if(items.empty())
        return;
    if(showGain)
    {
        std::vector<std::string> objects;

        for( const auto& n : items ) {
            const uint16_t &item=n.first;
            const uint32_t &quantity=n.second;
            if(playerInformations.encyclopedia_item!=NULL)
                playerInformations.encyclopedia_item[item/8]|=(1<<(7-item%8));
            else
                std::cerr << "encyclopedia_item is null, unable to set" << std::endl;
            //add really to the list
            if(playerInformations.items.find(item)!=playerInformations.items.cend())
                playerInformations.items[item]+=quantity;
            else
                playerInformations.items[item]=quantity;

            QPixmap image;
            std::string name;
            if(QtDatapackClientLoader::datapackLoader->itemsExtra.find(item)!=QtDatapackClientLoader::datapackLoader->itemsExtra.cend())
            {
                image=QtDatapackClientLoader::datapackLoader->QtitemsExtra.at(item).image;
                name=QtDatapackClientLoader::datapackLoader->itemsExtra.at(item).name;
            }
            else
            {
                image=QtDatapackClientLoader::datapackLoader->defaultInventoryImage();
                name="id: %1"+std::to_string(item);
            }

            image=image.scaled(24,24);
            QByteArray byteArray;
            QBuffer buffer(&byteArray);
            image.save(&buffer, "PNG");
            if(objects.size()<16)
            {
                if(item>1)
                    objects.push_back("<b>"+std::to_string(quantity)+"x</b> "+
                                      name+" <img src=\"data:image/png;base64,"+QString(byteArray.toBase64()).toStdString()+"\" />");
                else
                    objects.push_back(name+" <img src=\"data:image/png;base64,"+QString(byteArray.toBase64()).toStdString()+"\" />");
            }
        }
        if(objects.size()>=16)
            objects.push_back("...");
        add_to_inventoryGainList.push_back(stringimplode(objects,", "));
        add_to_inventoryGainTime.push_back(QTime::currentTime());
        OverMapLogic::showGain();
    }
    else
    {
        //add without show
        for( const auto& n : items ) {
            const uint16_t &item=n.first;
            const uint32_t &quantity=n.second;
            if(playerInformations.encyclopedia_item!=NULL)
                playerInformations.encyclopedia_item[item/8]|=(1<<(7-item%8));
            else
                std::cerr << "encyclopedia_item is null, unable to set" << std::endl;
            //add really to the list
            if(playerInformations.items.find(item)!=playerInformations.items.cend())
                playerInformations.items[item]+=quantity;
            else
                playerInformations.items[item]=quantity;
        }
    }

    load_inventory();
    load_plant_inventory();
    on_listCraftingList_itemSelectionChanged();
}

void OverMapLogic::remove_to_inventory(const uint16_t &itemId,const uint32_t &quantity)
{
    std::unordered_map<uint16_t,uint32_t> items;
    items[itemId]=quantity;
    remove_to_inventory(items);
}

void OverMapLogic::remove_to_inventory_slot(const std::unordered_map<uint16_t,uint32_t> &items)
{
    remove_to_inventory(items);
}

void OverMapLogic::remove_to_inventory(const std::unordered_map<uint16_t,uint32_t> &items)
{
    Player_private_and_public_informations &playerInformations=client->get_player_informations();
    for( const auto& n : items ) {
        const uint16_t &item=n.first;
        const uint32_t &quantity=n.second;
        //add really to the list
        if(playerInformations.items.find(item)!=playerInformations.items.cend())
        {
            if(playerInformations.items.at(item)<=quantity)
                playerInformations.items.erase(item);
            else
                playerInformations.items[item]-=quantity;
        }
    }
    load_inventory();
    load_plant_inventory();
}

void OverMapLogic::load_plant_inventory()
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "OverMapLogic::load_plant_inventory()";
    #endif
    if(!haveInventory || !datapackIsParsed)
        return;
    ui->listPlantList->clear();
    plants_items_graphical.clear();
    plants_items_to_graphical.clear();
    auto i=playerInformations.items.begin();
    while(i!=playerInformations.items.cend())
    {
        if(QtDatapackClientLoader::datapackLoader->itemToPlants.find(i->first)!=
                QtDatapackClientLoader::datapackLoader->itemToPlants.cend())
        {
            const uint8_t &plantId=QtDatapackClientLoader::datapackLoader->itemToPlants.at(i->first);
            QListWidgetItem *item;
            item=new QListWidgetItem();
            plants_items_to_graphical[plantId]=item;
            plants_items_graphical[item]=plantId;
            if(QtDatapackClientLoader::datapackLoader->itemsExtra.find(i->first)!=
                    QtDatapackClientLoader::datapackLoader->itemsExtra.cend())
            {
                item->setIcon(QtDatapackClientLoader::datapackLoader->QtitemsExtra[i->first].image);
                item->setText(QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra[i->first].name)+
                        "\n"+tr("Quantity: %1").arg(i->second));
            }
            else
            {
                item->setIcon(QtDatapackClientLoader::datapackLoader->defaultInventoryImage());
                item->setText(QStringLiteral("item id: %1").arg(i->first)+"\n"+tr("Quantity: %1").arg(i->second));
            }
            if(!haveReputationRequirements(CatchChallenger::CommonDatapack::commonDatapack.plants.at(plantId).requirements.reputation))
            {
                item->setText(item->text()+"\n"+tr("You don't have the requirements"));
                item->setFont(disableIntoListFont);
                item->setForeground(disableIntoListBrush);
            }
            ui->listPlantList->addItem(item);
        }
        ++i;
    }
}

void OverMapLogic::load_crafting_inventory()
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "OverMapLogic::load_crafting_inventory()";
    #endif
    ui->listCraftingList->clear();
    crafting_recipes_items_to_graphical.clear();
    crafting_recipes_items_graphical.clear();
    Player_private_and_public_informations informations=client->get_player_informations();
    if(informations.recipes==NULL)
    {
        qDebug() << "OverMapLogic::load_crafting_inventory(), crafting null";
        return;
    }
    uint16_t index=0;
    while(index<=CatchChallenger::CommonDatapack::commonDatapack.craftingRecipesMaxId)
    {
        uint16_t recipe=index;
        if(informations.recipes[recipe/8] & (1<<(7-recipe%8)))
        {
            //load the material item
            if(CatchChallenger::CommonDatapack::commonDatapack.craftingRecipes.find(recipe)
                    !=CatchChallenger::CommonDatapack::commonDatapack.craftingRecipes.cend())
            {
                QListWidgetItem *item=new QListWidgetItem();
                if(QtDatapackClientLoader::datapackLoader->itemsExtra.find(CatchChallenger::CommonDatapack::commonDatapack.craftingRecipes[recipe].doItemId)!=
                        QtDatapackClientLoader::datapackLoader->itemsExtra.cend())
                {
                    item->setIcon(QtDatapackClientLoader::datapackLoader->QtitemsExtra[CatchChallenger::CommonDatapack::commonDatapack.craftingRecipes[recipe]
                            .doItemId].image);
                    item->setText(QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra[CatchChallenger::CommonDatapack::commonDatapack.craftingRecipes[recipe]
                            .doItemId].name));
                }
                else
                {
                    item->setIcon(QtDatapackClientLoader::datapackLoader->defaultInventoryImage());
                    item->setText(tr("Unknow item: %1").arg(CatchChallenger::CommonDatapack::commonDatapack.craftingRecipes[recipe].doItemId));
                }
                crafting_recipes_items_to_graphical[recipe]=item;
                crafting_recipes_items_graphical[item]=recipe;
                ui->listCraftingList->addItem(item);
            }
            else
                qDebug() << QStringLiteral("OverMapLogic::load_crafting_inventory(), crafting id not found into crafting recipe").arg(recipe);
        }
        ++index;
    }
    on_listCraftingList_itemSelectionChanged();
}

std::string OverMapLogic::reputationRequirementsToText(const ReputationRequirements &reputationRequirements)
{
    if(reputationRequirements.reputationId>=CatchChallenger::CommonDatapack::commonDatapack.reputation.size())
    {
        std::cerr << "reputationRequirements.reputationId" << reputationRequirements.reputationId
                  << ">=CatchChallenger::CommonDatapack::commonDatapack.reputation.size()"
                  << CatchChallenger::CommonDatapack::commonDatapack.reputation.size() << std::endl;
        return tr("Unknown reputation id: %1").arg(reputationRequirements.reputationId).toStdString();
    }
    const Reputation &reputation=CatchChallenger::CommonDatapack::commonDatapack.reputation.at(reputationRequirements.reputationId);
    if(QtDatapackClientLoader::datapackLoader->reputationExtra.find(reputation.name)==
            QtDatapackClientLoader::datapackLoader->reputationExtra.cend())
    {
        std::cerr << "!QtDatapackClientLoader::datapackLoader->reputationExtra.contains("+reputation.name+")" << std::endl;
        return tr("Unknown reputation name: %1").arg(QString::fromStdString(reputation.name)).toStdString();
    }
    const QtDatapackClientLoader::ReputationExtra &reputationExtra=QtDatapackClientLoader::datapackLoader->reputationExtra.at(reputation.name);
    if(reputationRequirements.positif)
    {
        if(reputationRequirements.level>=reputationExtra.reputation_positive.size())
        {
            std::cerr << "No text for level "+std::to_string(reputationRequirements.level)+" for reputation "+reputationExtra.name << std::endl;
            return QStringLiteral("No text for level %1 for reputation %2")
                    .arg(reputationRequirements.level)
                    .arg(QString::fromStdString(reputationExtra.name))
                    .toStdString();
        }
        else
            return reputationExtra.reputation_positive.at(reputationRequirements.level);
    }
    else
    {
        if(reputationRequirements.level>=reputationExtra.reputation_negative.size())
        {
            std::cerr << "No text for level "+std::to_string(reputationRequirements.level)+" for reputation "+reputationExtra.name << std::endl;
            return QStringLiteral("No text for level %1 for reputation %2")
                    .arg(reputationRequirements.level)
                    .arg(QString::fromStdString(reputationExtra.name))
                    .toStdString();
        }
        else
            return reputationExtra.reputation_negative.at(reputationRequirements.level);
    }
}
