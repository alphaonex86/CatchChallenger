#include "OverMapLogic.hpp"
#include "../ConnexionManager.hpp"
#include "../AudioGL.hpp"
#include "../QGraphicsPixmapItemClick.hpp"
#include "../background/CCMap.hpp"
#include "../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../libqtcatchchallenger/maprender/QMap_client.hpp"
#include "../above/inventory/Inventory.hpp"
#include "../above/player/MonsterSelect.hpp"
#include "../above/TextInput.hpp"
#include "../above/inventory/Plant.hpp"
#include "../above/inventory/Crafting.hpp"
#include "../above/player/Player.hpp"
#include "../above/player/Reputations.hpp"
#include "../above/player/Quests.hpp"
#include "../above/player/FinishedQuests.hpp"
#include "../above/Animation.hpp"
#include "../above/Encyclopedia.hpp"
#include "../above/Shop.hpp"
#include "../above/Factory.hpp"
#include "../above/Trade.hpp"
#include "../above/Warehouse.hpp"
#include "../../../general/base/FacilityLib.hpp"
#include "../../../general/base/CommonDatapack.hpp"
#include "Battle.hpp"
#include <iostream>
#include <QBuffer>
#include <QCoreApplication>
#if defined(CATCHCHALLENGER_SOLO) && ! defined(CATCHCHALLENGER_NO_TCPSOCKET) && defined(CATCHCHALLENGER_SOLO) && defined(CATCHCHALLENGER_MULTI)
#include "../../../server/qt/InternalServer.hpp"
#endif

OverMapLogic::OverMapLogic()
{
    inventory=nullptr;
    monsterSelect=nullptr;
    textInput=nullptr;
    plant=nullptr;
    crafting=nullptr;
    inventoryIndex=0;
    inSelection=false;
    waitedObjectType=ObjectType_All;

    player=nullptr;
    reputations=nullptr;
    quests=nullptr;
    finishedQuests=nullptr;
    playerIndex=0;

    animation=nullptr;
    encyclopedia=nullptr;
    shop=nullptr;
    currentBotFightId=0xFFFF;
    currentBotFightMapId=0xFFFF;
    factory=nullptr;
    trade=nullptr;
    warehouse=nullptr;

    multiplayer=true;
    lastReplyTimeValue=0;
    worseQueryTime=0;

    checkQueryTime.start(200);
    if(!connect(&checkQueryTime,&QTimer::timeout,   this,&OverMapLogic::detectSlowDown))
        abort();
    tip_timeout.setInterval(8000);
    gain_timeout.setInterval(500);
    tip_timeout.setSingleShot(true);
    gain_timeout.setSingleShot(true);

    if(!connect(&tip_timeout,&QTimer::timeout,              this,&OverMapLogic::tipTimeout))
        abort();
    if(!connect(&gain_timeout,&QTimer::timeout,             this,&OverMapLogic::gainTimeout))
        abort();
    if(!connect(&nextCityCatchTimer,&QTimer::timeout,     this,&OverMapLogic::cityCaptureUpdateTime))
        abort();
    if(!connect(&updater_page_zonecatch,&QTimer::timeout, this,&OverMapLogic::updatePageZoneCatch))
        abort();
}

void OverMapLogic::setVar(CCMap *ccmap, ConnexionManager *connexionManager)
{
    this->ccmap=ccmap;

    if(!connect(ccmap,&CCMap::pathFindingNotFound,this,&OverMapLogic::pathFindingNotFound))
        abort();
    if(!connect(ccmap,&CCMap::repelEffectIsOver,this,&OverMapLogic::repelEffectIsOver))
        abort();
    if(!connect(ccmap,&CCMap::teleportConditionNotRespected,this,&OverMapLogic::teleportConditionNotRespected))
        abort();
    if(!connect(ccmap,&CCMap::send_player_direction,connexionManager->client,&CatchChallenger::Api_protocol_Qt::send_player_direction))
        abort();
    if(!connect(ccmap,&CCMap::stopped_in_front_of,this,&OverMapLogic::stopped_in_front_of))
        abort();
    if(!connect(ccmap,&CCMap::actionOn,this,&OverMapLogic::actionOn))
        abort();
    if(!connect(ccmap,&CCMap::actionOnNothing,this,&OverMapLogic::actionOnNothing))
        abort();
    //Escape on the map closes an open Sign/NPC dialog
    if(!connect(ccmap,&CCMap::escapePressed,this,&OverMap::IG_dialog_close))
        abort();
    //arrows/Return while the dialog is open select/activate its hyperlinks
    //(D-pad + A button, physical keyboard and the automation KEY verb all
    //converge on this one path — see MapVisualiserPlayer::keyPressEvent)
    if(!connect(ccmap,&CCMap::dialogKeyPressed,this,&OverMap::IG_dialog_key))
        abort();
    if(!connect(ccmap,&CCMap::blockedOn,this,&OverMapLogic::blockedOn))
        abort();
    if(!connect(ccmap,&CCMap::error,this,&OverMapLogic::error))
        abort();
    if(!connect(ccmap,&CCMap::errorWithTheCurrentMap,this,&OverMapLogic::errorWithTheCurrentMap))
        abort();
    if(!connect(ccmap,&CCMap::currentMapLoaded,this,&OverMapLogic::currentMapLoaded))
        abort();
    if(!connect(ccmap,&CCMap::wildFightCollision,this,&OverMapLogic::wildFightCollision))
        abort();
    if(!connect(ccmap,&CCMap::botFightCollision,this,&OverMapLogic::botFightCollision))
        abort();

    OverMap::setVar(ccmap,connexionManager);

    if(!connect(bag,&CustomButton::clicked,this,&OverMapLogic::bag_open))
        abort();
    if(!connect(playerBackground,&QGraphicsPixmapItemClick::clicked,this,&OverMapLogic::player_open))
        abort();
    if(!connect(opentolan,&CustomButton::clicked,this,&OverMapLogic::opentolan_open))
        abort();
    #if defined(CATCHCHALLENGER_SOLO) && !defined(CATCHCHALLENGER_NO_TCPSOCKET) && defined(CATCHCHALLENGER_SOLO) && defined(CATCHCHALLENGER_MULTI)
    if(CatchChallenger::InternalServer::internalServer!=nullptr)
        if(!connect(CatchChallenger::InternalServer::internalServer,&CatchChallenger::InternalServer::emitLanPort,this,&OverMapLogic::displayLanPort))
            abort();
    #endif
}

void OverMapLogic::resetAll()
{
    lastStepUsed=0;
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
    actualBot.botId=0;
    actualBot.name.clear();
    actualBot.properties.clear();
    actualBot.skin.clear();
    actualBot.step.clear();
    actionClan.clear();
    isInQuest=false;
    questId=0;
    clanName.clear();
    haveClanInformations=false;
    city.capture.day=CatchChallenger::City::Capture::Day::Monday;
    city.capture.frenquency=CatchChallenger::City::Capture::Frequency::Frequency_week;
    city.capture.hour=0;
    city.capture.minute=0;
    zonecatchName.clear();
    zonecatch=false;
    tempQuantityForSell=0;
    waitToSell=false;
    itemsToSell.clear();
    itemsToBuy.clear();
    objectInUsing.clear();
    monster_to_deposit.clear();
    monster_to_withdraw.clear();
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

void OverMapLogic::stopped_in_front_of(const CATCHCHALLENGER_TYPE_MAPID &mapIndex, const COORD_TYPE &x, const COORD_TYPE &y)
{
    if(stopped_in_front_of_check_bot(mapIndex,x,y))
        return;
    const CatchChallenger::CommonMap &map=QtDatapackClientLoader::datapackLoader->getMap(mapIndex);
    if(CatchChallenger::MoveOnTheMap::isDirt(map,x,y))
    {
        const CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations_ro();
        const std::pair<COORD_TYPE,COORD_TYPE> pos(x,y);
        std::map<uint16_t, CatchChallenger::Player_private_and_public_informations_Map>::const_iterator mapIt=playerInformations.mapData.find(mapIndex);
        if(mapIt!=playerInformations.mapData.cend())
        {
            std::map<std::pair<uint8_t,uint8_t>, CatchChallenger::PlayerPlant>::const_iterator plantIt=mapIt->second.plants.find(pos);
            if(plantIt!=mapIt->second.plants.cend())
            {
                uint64_t current_time=QDateTime::currentMSecsSinceEpoch()/1000;
                if(plantIt->second.mature_at<=current_time)
                    showTip(tr("To recolt the plant press <i>Enter</i>").toStdString());
                else
                    showTip(tr("This plant is growing and can't be collected").toStdString());
                return;
            }
        }
        showTip(tr("To plant a seed press <i>Enter</i>").toStdString());
        return;
    }
}

void OverMapLogic::setIG_dialog(QString text,QString name)
{
    this->IG_dialog_textString=text;
    this->IG_dialog_nameString=name;
    this->IG_dialog_text->setHtml(text);
    this->IG_dialog_name->setHtml(name);
    //re-list the body's hyperlinks and pre-select the first one (D-pad/A support)
    IG_dialog_links_rebuild();
    //mirror the current dialog text to the QLocalServer automation channel so an
    //external controller can observe it via GETDIALOG (read-only; empty=closed).
    if(ccmap!=nullptr)
        ccmap->mapController.setRemoteDialogText(text);
}

void OverMapLogic::IG_dialog_close()
{
    OverMap::IG_dialog_close();//clears IG_dialog_textString (hides the box)
    if(ccmap!=nullptr)
        ccmap->mapController.setRemoteDialogText(QString());
}

void OverMapLogic::runDialogOverflowSelfTest()
{
    //a deliberately oversized, server-style message: long unbreakable tokens
    //(to exercise wrap-anywhere) plus many lines (a tall body)
    QString longText;
    int line=0;
    while(line<80)
    {
        longText+=QStringLiteral("Line %1: the quick brown fox jumps over the lazy dog, "
                                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnop "
                                 "verylongunbreakabletokenwithoutanyspaces_%1_end.<br/>").arg(line);
        line++;
    }
    setIG_dialog(longText,QStringLiteral("TestSign"));
    //let the scene paint a few frames so OverMap::paint() lays the dialog out
    QTimer::singleShot(1500,this,&OverMapLogic::dialogOverflowSelfTestCheck);
}

void OverMapLogic::dialogOverflowSelfTestCheck()
{
    //The body is word-wrapped to IG_dialog_text->textWidth(); verify the rendered
    //text actually wrapped (its width stays within that wrap width) and stays
    //inside the dialog box, so long server text never spills out the side.
    const int wrapWidth=static_cast<int>(IG_dialog_text->textWidth());
    const int docW=static_cast<int>(IG_dialog_text->boundingRect().width());
    const int docH=static_cast<int>(IG_dialog_text->boundingRect().height());
    const int boxW=IG_dialog_textBack->width();
    const bool dialogVisible=IG_dialog_textBack->isVisible();
    const bool wrapped=(wrapWidth>0 && docW<=wrapWidth+2);
    const bool fitsBox=(docW<=boxW+1);
    const bool ok=dialogVisible && wrapped && fitsBox;
    std::cerr << "[OVERFLOWTEST] qtopengl wrapWidth=" << wrapWidth
              << " doc=" << docW << "x" << docH
              << " box=" << boxW
              << " wrapped=" << (wrapped?"yes":"no")
              << " fitsBox=" << (fitsBox?"yes":"no")
              << " -> " << (ok?"PASS":"FAIL") << std::endl;
    QCoreApplication::exit(ok?0:3);
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
    /*if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::Qthave_inventory,     this,&OverMapLogic::have_inventory))
        abort();*/
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::Qtadd_to_inventory,   this,&OverMapLogic::add_to_inventory_slot))
        abort();
    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::Qtremove_to_inventory,this,&OverMapLogic::remove_to_inventory_slot))
        abort();

    //plants
    if(!connect(this,&OverMapLogic::useSeed,              connexionManager->client,&CatchChallenger::Api_client_real::useSeed))
        abort();
    if(!connect(this,&OverMapLogic::collectMaturePlant,   connexionManager->client,&CatchChallenger::Api_client_real::collectMaturePlant))
        abort();
    //plant signals dropped, plant is now globally visible and sync
    //crafting
    /*if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtrecipeUsed,     this,&OverMapLogic::recipeUsed))
        abort();*/
    /*//trade
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
        abort();*/
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
    /*//battle
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
       abort();*/
    //fight
/*    if(!connect(connexionManager->client,&CatchChallenger::Api_client_real::QtteleportTo,this,&OverMapLogic::teleportTo,Qt::UniqueConnection))
       abort();*/
}

void OverMapLogic::ensureInventory()
{
    if(inventory==nullptr)
    {
        inventory=new Inventory();
        if(!connect(inventory,&Inventory::setAbove,this,&OverMapLogic::setAbove))
            abort();
        if(!connect(inventory,&Inventory::sendNext,this,&OverMapLogic::inventoryNext))
            abort();
        if(!connect(inventory,&Inventory::sendBack,this,&OverMapLogic::inventoryBack))
            abort();
        if(!connect(inventory,&Inventory::useItem,this,&OverMapLogic::inventoryItemUsed))
            abort();
        if(!connect(inventory,&Inventory::deleteItem,this,&OverMapLogic::inventoryItemDeleted))
            abort();
    }
}

void OverMapLogic::ensurePlant()
{
    if(plant==nullptr)
    {
        plant=new Plant();
        if(!connect(plant,&Plant::setAbove,this,&OverMapLogic::setAbove))
            abort();
        if(!connect(plant,&Plant::sendNext,this,&OverMapLogic::inventoryNext))
            abort();
        if(!connect(plant,&Plant::sendBack,this,&OverMapLogic::inventoryBack))
            abort();
        if(!connect(plant,&Plant::useItem,this,&OverMapLogic::plantItemUsed))
            abort();
    }
}

void OverMapLogic::ensureMonsterSelect()
{
    if(monsterSelect==nullptr)
    {
        monsterSelect=new MonsterSelect();
        if(!connect(monsterSelect,&MonsterSelect::setAbove,this,&OverMapLogic::setAbove))
            abort();
        if(!connect(monsterSelect,&MonsterSelect::useMonster,this,&OverMapLogic::monsterSelected))
            abort();
        if(!connect(monsterSelect,&MonsterSelect::canceled,this,&OverMapLogic::monsterSelectCanceled))
            abort();
    }
}

void OverMapLogic::monsterSelected(const uint8_t &monsterPosition)
{
    //the picker already closed itself; the target monster position is the itemId
    objectSelection(true,monsterPosition,1);
}

void OverMapLogic::monsterSelectCanceled()
{
    objectSelection(false);
}

void OverMapLogic::clanNameEntered(const QString &clanName)
{
    const std::string name=clanName.trimmed().toStdString();
    if(!name.empty())
        connexionManager->client->createClan(name);
}

//The Inventory/Plant ObjectType enums mirror ours 1:1, so selectObject casts
//between them; lock that assumption at compile time.
static_assert((int)OverMapLogic::ObjectType_UseInFight==(int)Inventory::ObjectType_UseInFight,
              "OverMapLogic::ObjectType and Inventory::ObjectType must stay in sync");

void OverMapLogic::selectObject(const ObjectType &objectType)
{
    inSelection=true;
    waitedObjectType=objectType;
    switch(objectType)
    {
        case ObjectType_Seed:
            ensurePlant();
            plant->setVar(connexionManager,true);
            setAbove(plant);
        break;
        case ObjectType_Sell:
        case ObjectType_Trade:
        case ObjectType_All:
        case ObjectType_UseInFight:
            ensureInventory();
            inventory->setVar(connexionManager,static_cast<Inventory::ObjectType>(objectType),true);
            setAbove(inventory);
        break;
        case ObjectType_ItemOnMonster:
        case ObjectType_ItemOnMonsterOutOfFight:
        case ObjectType_ItemEvolutionOnMonster:
        case ObjectType_ItemLearnOnMonster:
        case ObjectType_MonsterToTrade:
        case ObjectType_MonsterToFight:
        case ObjectType_MonsterToFightKO:
            //pick a team monster as the target
            ensureMonsterSelect();
            monsterSelect->setVar(connexionManager);
            setAbove(monsterSelect);
        break;
        default:
            inSelection=false;
            waitedObjectType=ObjectType_All;
            showTip(tr("This selection is not available yet").toStdString());
        break;
    }
}

void OverMapLogic::inventoryItemUsed(const uint16_t &item)
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=
            connexionManager->client->get_player_informations_ro();
    if(inSelection)
    {
        //picking an item in a Sell/Trade/UseInFight/All picker: validate it is
        //still owned, then hand it to objectSelection. Quantity picker is a
        //follow-up; sell/trade one unit at a time for now.
        if(playerInformations.items.find(item)==playerInformations.items.cend())
        {
            objectSelection(false);
            return;
        }
        objectSelection(true,item,1);
        return;
    }
    //normal bag use: dispatch by item kind (mirrors 800x600 on_inventory_itemActivated)
    CatchChallenger::CommonDatapack &dp=CatchChallenger::CommonDatapack::commonDatapack;
    if(dp.has_itemToCraftingRecipe(item))
    {
        const uint16_t recipeId=dp.get_itemToCraftingRecipe(item);
        const CatchChallenger::CraftingRecipe &recipe=dp.get_craftingRecipe(recipeId);
        if(!connexionManager->client->haveReputationRequirements(recipe.requirements.reputation))
        {
            showTip(tr("You don't have the reputation requirements").toStdString());
            return;
        }
        if(playerInformations.recipes!=NULL && (playerInformations.recipes[recipeId/8] & (1<<(7-recipeId%8))))
        {
            showTip(tr("You already know this recipe").toStdString());
            return;
        }
        objectInUsing.push_back(item);
        if(dp.get_item(item).consumeAtUse)
            remove_to_inventory(item);
        connexionManager->client->useObject(item);
    }
    else if(dp.has_repel(item))
    {
        ccmap->mapController.addRepelStep(dp.get_repel(item));
        objectInUsing.push_back(item);
        if(dp.get_item(item).consumeAtUse)
            remove_to_inventory(item);
        connexionManager->client->useObject(item);
    }
    else if(dp.has_monsterItemEffect(item) || dp.has_monsterItemEffectOutOfFight(item))
    {
        objectInUsing.push_back(item);
        if(dp.get_item(item).consumeAtUse)
            remove_to_inventory(item);
        selectObject(ObjectType_ItemOnMonster);
    }
    else if(dp.has_evolutionItem(item))
    {
        objectInUsing.push_back(item);
        if(dp.get_item(item).consumeAtUse)
            remove_to_inventory(item);
        selectObject(ObjectType_ItemEvolutionOnMonster);
    }
    else if(dp.has_itemToLearn(item))
    {
        objectInUsing.push_back(item);
        if(dp.get_item(item).consumeAtUse)
            remove_to_inventory(item);
        selectObject(ObjectType_ItemLearnOnMonster);
    }
    else
        showTip(tr("This object can't be used here").toStdString());
}

void OverMapLogic::inventoryItemDeleted(const uint16_t &/*item*/)
{
    //item destroy is ported in a later step
    showTip(tr("Destroying items is not available yet").toStdString());
}

void OverMapLogic::plantItemUsed(const uint16_t &item)
{
    if(inSelection)
    {
        objectSelection(true,item,1);
        return;
    }
    //outside seed selection a plant item has no direct use
}

void OverMapLogic::actionOn(const CATCHCHALLENGER_TYPE_MAPID &mapIndex, const COORD_TYPE &x, const COORD_TYPE &y)
{
    setIG_dialog(QString());
    if(actionOnCheckBot(mapIndex,x,y))
        return;
    const CatchChallenger::CommonMap &map=QtDatapackClientLoader::datapackLoader->getMap(mapIndex);
    if(CatchChallenger::MoveOnTheMap::isDirt(map,x,y))
    {
        const CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations_ro();
        const std::pair<COORD_TYPE,COORD_TYPE> pos(x,y);
        std::map<uint16_t, CatchChallenger::Player_private_and_public_informations_Map>::const_iterator mapIt=playerInformations.mapData.find(mapIndex);
        bool hasPlant=false;
        if(mapIt!=playerInformations.mapData.cend())
        {
            std::map<std::pair<uint8_t,uint8_t>, CatchChallenger::PlayerPlant>::const_iterator plantIt=mapIt->second.plants.find(pos);
            if(plantIt!=mapIt->second.plants.cend())
            {
                hasPlant=true;
                uint64_t current_time=QDateTime::currentMSecsSinceEpoch()/1000;
                if(plantIt->second.mature_at<=current_time)
                {
                    //plant is now globally visible and sync
                    emit collectMaturePlant();
                }
                else
                    showTip(tr("This plant is growing and can't be collected").toStdString());
            }
        }
        if(!hasPlant)
        {
            SeedInWaiting seedInWaiting;
            seedInWaiting.map=mapIndex;
            seedInWaiting.x=x;
            seedInWaiting.y=y;
            seed_in_waiting.push_back(seedInWaiting);

            selectObject(ObjectType_Seed);
        }
        return;
    }
    else if(map.items.find(std::pair<uint8_t,uint8_t>(x,y))!=map.items.cend())
    {
        CatchChallenger::Player_private_and_public_informations &informations=connexionManager->client->get_player_informations();
        const CatchChallenger::ItemOnMap &item=map.items.at(std::pair<uint8_t,uint8_t>(x,y));
        const std::pair<uint8_t,uint8_t> pos(x,y);
        if(informations.mapData.find(mapIndex)==informations.mapData.cend() ||
           informations.mapData.at(mapIndex).items.find(pos)==informations.mapData.at(mapIndex).items.cend())
        {
            if(!item.infinite)
                informations.mapData[mapIndex].items.insert(pos);
            add_to_inventory(item.item);
            connexionManager->client->takeAnObjectOnMap();
        }
    }
}

void OverMapLogic::actionOnNothing()
{
    setIG_dialog(QString());
}

int32_t OverMapLogic::havePlant(const CATCHCHALLENGER_TYPE_MAPID &mapIndex, const COORD_TYPE &x, const COORD_TYPE &y) const
{
    //isDirt() already checked by caller, check if plant exists at this position in mapData
    const CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations_ro();
    std::map<uint16_t, CatchChallenger::Player_private_and_public_informations_Map>::const_iterator mapIt=playerInformations.mapData.find(mapIndex);
    if(mapIt!=playerInformations.mapData.cend())
    {
        const std::pair<COORD_TYPE,COORD_TYPE> pos(x,y);
        if(mapIt->second.plants.find(pos)!=mapIt->second.plants.cend())
            return 0;
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

void OverMapLogic::bag_open()
{
    ensureInventory();
    inventory->setVar(connexionManager,Inventory::ObjectType::ObjectType_All,false);
    setAbove(inventory);
    inventoryIndex=0;
}

void OverMapLogic::displayLanPort(uint16_t port)
{
    showTip(tr("Open to lan on port %1").arg(port).toStdString());
    connexionManager->client->add_system_text(CatchChallenger::Chat_type_system_important,tr("Open to lan on port %1").arg(port).toStdString());
    new_system_text(CatchChallenger::Chat_type_system_important,tr("Open to lan on port %1").arg(port).toStdString());
}

void OverMapLogic::opentolan_open()
{
    #if defined(CATCHCHALLENGER_SOLO) && !defined(CATCHCHALLENGER_NO_TCPSOCKET) && defined(CATCHCHALLENGER_SOLO) && defined(CATCHCHALLENGER_MULTI)
    if(CatchChallenger::InternalServer::internalServer!=nullptr)
        CatchChallenger::InternalServer::internalServer->openToLan(tr("%1's server").arg(QString::fromStdString(connexionManager->client->getPseudo())),true);
    #endif
}

void OverMapLogic::inventoryNext()
{
    switch(inventoryIndex)
    {
        case 0:
        ensurePlant();
        plant->setVar(connexionManager,false);
        setAbove(plant);
        break;
        case 1:
        if(crafting==nullptr)
        {
            crafting=new Crafting();
            if(!connect(crafting,&Crafting::setAbove,this,&OverMapLogic::setAbove))
                abort();
            if(!connect(crafting,&Crafting::sendNext,this,&OverMapLogic::inventoryNext))
                abort();
            if(!connect(crafting,&Crafting::sendBack,this,&OverMapLogic::inventoryBack))
                abort();
            if(!connect(crafting,&Crafting::remove_to_inventory,this,&OverMapLogic::remove_to_inventory_slotpair))
                abort();
            if(!connect(crafting,&Crafting::add_to_inventory,this,&OverMapLogic::add_to_inventory_slotpair))
                abort();
            if(!connect(crafting,&Crafting::appendReputationRewards,this,&OverMapLogic::appendReputationRewards))
                abort();
        }
        crafting->setVar(connexionManager);
        setAbove(crafting);
        break;
        default:
        ensureInventory();
        inventory->setVar(connexionManager,Inventory::ObjectType::ObjectType_All,false);
        setAbove(inventory);
        inventoryIndex=0;
        return;
    }
    inventoryIndex++;
}

void OverMapLogic::inventoryBack()
{
    switch(inventoryIndex)
    {
        case 1:
        ensureInventory();
        inventory->setVar(connexionManager,Inventory::ObjectType::ObjectType_All,false);
        setAbove(inventory);
        break;
        case 2:
        ensurePlant();
        plant->setVar(connexionManager,false);
        setAbove(plant);
        break;
        default:
        if(crafting==nullptr)
        {
            crafting=new Crafting();
            if(!connect(crafting,&Crafting::setAbove,this,&OverMapLogic::setAbove))
                abort();
            if(!connect(crafting,&Crafting::sendNext,this,&OverMapLogic::inventoryNext))
                abort();
            if(!connect(crafting,&Crafting::sendBack,this,&OverMapLogic::inventoryBack))
                abort();
            if(!connect(crafting,&Crafting::add_to_inventory,this,&OverMapLogic::add_to_inventory_slotpair))
                abort();
            if(!connect(crafting,&Crafting::remove_to_inventory,this,&OverMapLogic::remove_to_inventory_slotpair))
                abort();
            if(!connect(crafting,&Crafting::appendReputationRewards,this,&OverMapLogic::appendReputationRewards))
                abort();
        }
        crafting->setVar(connexionManager,false);
        setAbove(crafting);
        inventoryIndex=2;
        return;
    }
    inventoryIndex--;
}

void OverMapLogic::player_open()
{
    if(player==nullptr)
    {
        player=new Player();
        if(!connect(player,&Player::setAbove,this,&OverMapLogic::setAbove))
            abort();
        if(!connect(player,&Player::sendNext,this,&OverMapLogic::playerNext))
            abort();
        if(!connect(player,&Player::sendBack,this,&OverMapLogic::playerBack))
            abort();
    }
    player->setVar(connexionManager);
    setAbove(player);
    playerIndex=0;
}

void OverMapLogic::playerNext()
{
    switch(playerIndex)
    {
        case 0:
        if(reputations==nullptr)
        {
            reputations=new Reputations();
            if(!connect(reputations,&Reputations::setAbove,this,&OverMapLogic::setAbove))
                abort();
            if(!connect(reputations,&Reputations::sendNext,this,&OverMapLogic::playerNext))
                abort();
            if(!connect(reputations,&Reputations::sendBack,this,&OverMapLogic::playerBack))
                abort();
        }
        reputations->setVar(connexionManager);
        setAbove(reputations);
        break;
        case 1:
        if(quests==nullptr)
        {
            quests=new Quests();
            if(!connect(quests,&Quests::setAbove,this,&OverMapLogic::setAbove))
                abort();
            if(!connect(quests,&Quests::sendNext,this,&OverMapLogic::playerNext))
                abort();
            if(!connect(quests,&Quests::sendBack,this,&OverMapLogic::playerBack))
                abort();
        }
        quests->setVar(connexionManager);
        setAbove(quests);
        break;
        case 2:
        if(finishedQuests==nullptr)
        {
            finishedQuests=new FinishedQuests();
            if(!connect(finishedQuests,&FinishedQuests::setAbove,this,&OverMapLogic::setAbove))
                abort();
            if(!connect(finishedQuests,&FinishedQuests::sendNext,this,&OverMapLogic::playerNext))
                abort();
            if(!connect(finishedQuests,&FinishedQuests::sendBack,this,&OverMapLogic::playerBack))
                abort();
        }
        finishedQuests->setVar(connexionManager);
        setAbove(finishedQuests);
        break;
        default:
        if(player==nullptr)
        {
            player=new Player();
            if(!connect(player,&Player::setAbove,this,&OverMapLogic::setAbove))
                abort();
            if(!connect(player,&Player::sendNext,this,&OverMapLogic::playerNext))
                abort();
            if(!connect(player,&Player::sendBack,this,&OverMapLogic::playerBack))
                abort();
        }
        player->setVar(connexionManager);
        setAbove(player);
        playerIndex=0;
        return;
    }
    playerIndex++;
}

void OverMapLogic::playerBack()
{
    switch(playerIndex)
    {
        case 1:
        if(player==nullptr)
        {
            player=new Player();
            if(!connect(player,&Player::setAbove,this,&OverMapLogic::setAbove))
                abort();
            if(!connect(player,&Player::sendNext,this,&OverMapLogic::playerNext))
                abort();
            if(!connect(player,&Player::sendBack,this,&OverMapLogic::playerBack))
                abort();
        }
        player->setVar(connexionManager);
        setAbove(player);
        break;
        case 2:
        if(reputations==nullptr)
        {
            reputations=new Reputations();
            if(!connect(reputations,&Reputations::setAbove,this,&OverMapLogic::setAbove))
                abort();
            if(!connect(reputations,&Reputations::sendNext,this,&OverMapLogic::playerNext))
                abort();
            if(!connect(reputations,&Reputations::sendBack,this,&OverMapLogic::playerBack))
                abort();
        }
        reputations->setVar(connexionManager);
        setAbove(reputations);
        break;
        case 3:
        if(quests==nullptr)
        {
            quests=new Quests();
            if(!connect(quests,&Quests::setAbove,this,&OverMapLogic::setAbove))
                abort();
            if(!connect(quests,&Quests::sendNext,this,&OverMapLogic::playerNext))
                abort();
            if(!connect(quests,&Quests::sendBack,this,&OverMapLogic::playerBack))
                abort();
        }
        quests->setVar(connexionManager);
        setAbove(quests);
        break;
        default:
        if(finishedQuests==nullptr)
        {
            finishedQuests=new FinishedQuests();
            if(!connect(finishedQuests,&FinishedQuests::setAbove,this,&OverMapLogic::setAbove))
                abort();
            if(!connect(finishedQuests,&FinishedQuests::sendNext,this,&OverMapLogic::playerNext))
                abort();
            if(!connect(finishedQuests,&FinishedQuests::sendBack,this,&OverMapLogic::playerBack))
                abort();
        }
        finishedQuests->setVar(connexionManager);
        setAbove(finishedQuests);
        playerIndex=2;
        return;
    }
    playerIndex--;
}

void OverMapLogic::currentMapLoaded()
{
    qDebug() << "OverMapLogic::currentMapLoaded(): map: " << ccmap->mapController.currentMap()
             << " with type: " << QString::fromStdString(ccmap->mapController.currentMapType());
    //name
    {
        CatchChallenger::QMap_client *mapFull=ccmap->mapController.currentMapFull();
        std::string visualName;
        if(!mapFull->zone.empty())
            if(QtDatapackClientLoader::datapackLoader->has_zoneExtra(mapFull->zone))
            {
                const QtDatapackClientLoader::ZoneExtra &zoneExtra=QtDatapackClientLoader::datapackLoader->get_zoneExtra(mapFull->zone);
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
        CatchChallenger::QMap_client *mapFull=ccmap->mapController.currentMapFull();
        if(!mapFull->zone.empty())
            if(QtDatapackClientLoader::datapackLoader->has_zoneExtra(mapFull->zone))
            {
                const QtDatapackClientLoader::ZoneExtra &zoneExtra=QtDatapackClientLoader::datapackLoader->get_zoneExtra(mapFull->zone);
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
            if(QtDatapackClientLoader::datapackLoader->has_visualCategory(type))
            {
                const std::vector<QtDatapackClientLoader::VisualCategory::VisualCategoryCondition> &conditions=
                        QtDatapackClientLoader::datapackLoader->get_visualCategory(type).conditions;
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
                    QtDatapackClientLoader::CCColor defaultColor=QtDatapackClientLoader::datapackLoader->get_visualCategory(type).defaultColor;
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
    { QElapsedTimer t; t.start(); add_to_inventoryGainExtraTime.push_back(t); }
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
    //std::cout << "Show gain: \"" << text << "\"" << std::endl;
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
    for(const std::pair<const uint8_t, uint64_t> n : values) {
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

bool OverMapLogic::stopped_in_front_of_check_bot(const CATCHCHALLENGER_TYPE_MAPID &mapIndex, const COORD_TYPE &x, const COORD_TYPE &y)
{
    /*    if(map->bots.find(std::pair<uint8_t,uint8_t>(x,y))==map->bots.cend())                                                                                                 
              return false;                                                                                                                                                     
          showTip(tr("To interact with the bot press <i><b>Enter</b></i>").toStdString());                                                                                      
          return true;*/
    (void)mapIndex;
    (void)x;
    (void)y;
    // Bot detection is handled by the maprender layer
    return false;
}

bool OverMapLogic::actionOnCheckBot(const CATCHCHALLENGER_TYPE_MAPID &mapIndex, const COORD_TYPE &x, const COORD_TYPE &y)
{
    //Resolve the bot/sign sitting on the faced tile. parseAction() only emits
    //actionOn(): it is up to us to look the bot up and open its dialog. Same
    //path as the cpu800x600 client (BaseWindow::actionOnCheckBot).
    const CatchChallenger::Bot *bot=QtDatapackClientLoader::datapackLoader->getBot(mapIndex,x,y);
    if(bot==nullptr)
        return false;
    actualBot=*bot;
    isInQuest=false;
    goToBotStep(1);
    return true;
}

/*void OverMapLogic::on_clanActionLeave_clicked()
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
}*/

void OverMapLogic::cityCaptureUpdateTime()
{
    if(city.capture.frenquency==CatchChallenger::City::Capture::Frequency_week)
        nextCatch=QDateTime::fromMSecsSinceEpoch(QDateTime::currentMSecsSinceEpoch()+24*3600*7*1000);
    else
        nextCatch=QDateTime::fromMSecsSinceEpoch(CatchChallenger::FacilityLib::nextCaptureTime(city));
    {
        const int64_t timerInterval=nextCatch.toMSecsSinceEpoch()-QDateTime::currentMSecsSinceEpoch();
        if(timerInterval>0)
            nextCityCatchTimer.start(static_cast<uint32_t>(timerInterval));
        else
        {
            std::cerr << "cityCaptureUpdateTime(): next capture time is in the past (" << timerInterval << "ms), stop timer" << std::endl;
            nextCityCatchTimer.stop();
        }
    }
}

void OverMapLogic::updatePageZoneCatch()
{
    /// \todo do into above dialog
/*    if(QDateTime::currentMSecsSinceEpoch()<nextCatchOnScreen.toMSecsSinceEpoch())
    {
        int sec=static_cast<uint32_t>(nextCatchOnScreen.toMSecsSinceEpoch()-QDateTime::currentMSecsSinceEpoch())/1000+1;
        std::string timeText;
        if(sec>3600*24*365*50)
            timeText="Time player: bug";
        else
            timeText=CatchChallenger::FacilityLibClient::timeToString(sec);
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
    }*/
}

void OverMapLogic::on_zonecaptureCancel_clicked()
{
    //return of updatePageZoneCatch()
    updater_page_zonecatch.stop();
/*    ui->stackedWidget->setCurrentWidget(ui->page_map);
    client->waitingForCityCapture(true);*/
    zonecatch=false;
}

void OverMapLogic::captureCityYourAreNotLeader()
{
    updater_page_zonecatch.stop();
    /// \todo close dialog into updatePageZoneCatch()
    //ui->stackedWidget->setCurrentWidget(ui->page_map);
    showTip(tr("You are not a clan leader to start a city capture").toStdString());
    zonecatch=false;
}

void OverMapLogic::captureCityYourLeaderHaveStartInOtherCity(const std::string &zone)
{
    updater_page_zonecatch.stop();
    //ui->stackedWidget->setCurrentWidget(ui->page_map);
    /// \todo close dialog into updatePageZoneCatch()
    if(QtDatapackClientLoader::datapackLoader->has_zoneExtra(zone))
        showTip(tr("Your clan leader have start a capture for another city").toStdString()+": <b>"+
                QtDatapackClientLoader::datapackLoader->get_zoneExtra(zone).name+
                "</b>");
    else
        showTip(tr("Your clan leader have start a capture for another city").toStdString());
    zonecatch=false;
}

void OverMapLogic::captureCityPreviousNotFinished()
{
    updater_page_zonecatch.stop();
    /// \todo close dialog into updatePageZoneCatch()
    //ui->stackedWidget->setCurrentWidget(ui->page_map);
    showTip(tr("Previous capture of this city is not finished").toStdString());
    zonecatch=false;
}

void OverMapLogic::captureCityStartBattle(const uint16_t &/*player_count*/,const uint16_t &/*clan_count*/)
{
    /// \todo set text into captureCityStartBattle() and disable cancel button
    /*ui->zonecaptureCancel->setVisible(false);
    ui->zonecaptureWaitTime->setText("<i>"+tr("%1 and %2 in wainting to capture the city").arg("<b>"+tr("%n player(s)","",player_count)+"</b>").arg("<b>"+tr("%n clan(s)","",clan_count)+"</b>")+"</i>");*/
    updater_page_zonecatch.stop();
}

void OverMapLogic::captureCityStartBotFight(const uint16_t &/*player_count*/,const uint16_t &/*clan_count*/,const uint16_t &fightId)
{
    /// \todo set text into captureCityStartBattle() and disable cancel button
    /*ui->zonecaptureCancel->setVisible(false);
    ui->zonecaptureWaitTime->setText("<i>"+tr("%1 and %2 in wainting to capture the city").arg("<b>"+tr("%n player(s)","",player_count)+"</b>").arg("<b>"+tr("%n clan(s)","",clan_count)+"</b>")+"</i>");*/
    updater_page_zonecatch.stop();
    botFight(fightId);
}

void OverMapLogic::captureCityDelayedStart(const uint16_t &/*player_count*/,const uint16_t &/*clan_count*/)
{
    /// \todo set text into captureCityStartBattle() and disable cancel button
    /*ui->zonecaptureCancel->setVisible(false);
    ui->zonecaptureWaitTime->setText("<i>"+tr("In waiting fight.")+" "+tr("%1 and %2 in wainting to capture the city").arg("<b>"+tr("%n player(s)","",player_count)+"</b>").arg("<b>"+tr("%n clan(s)","",clan_count)+"</b>")+"</i>");*/
    updater_page_zonecatch.stop();
}

void OverMapLogic::captureCityWin()
{
    updater_page_zonecatch.stop();
    /// \todo close captureCityStartBattle()
    //ui->stackedWidget->setCurrentWidget(ui->page_map);
    if(!zonecatchName.empty())
        showTip(tr("Your clan win the city").toStdString()+": <b>"+
                zonecatchName+"</b>");
    else
        showTip(tr("Your clan win the city").toStdString());
    zonecatch=false;
}

/*void OverMapLogic::have_inventory(const std::unordered_map<uint16_t,uint32_t> &items, const std::unordered_map<uint16_t, uint32_t> &warehouse_items)
{
    CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations();
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
    const CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations_ro();
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
    std::map<uint16_t, uint32_t>::const_iterator i=playerInformations.items.begin();
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
                    if(fightEngine.isInFightWithWild() && CommonDatapack::commonDatapack.trap.find(i->first)!=CommonDatapack::commonDatapack.trap.cend())
                        show=true;
                    else if(CommonDatapack::commonDatapack.monsterItemEffect.find(i->first)!=CommonDatapack::commonDatapack.monsterItemEffect.cend())
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
}*/

void OverMapLogic::add_to_inventory_slot(const std::unordered_map<uint16_t,uint32_t> &items)
{
    add_to_inventory(items);
}

void OverMapLogic::add_to_inventory_slotpair(const uint16_t &item, const uint32_t &quantity, const bool &showGain)
{
    add_to_inventory(item,quantity,showGain);
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
    CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations();
    if(items.empty())
        return;
    if(showGain)
    {
        std::vector<std::string> objects;

        for( const std::pair<const uint16_t, uint32_t>& n : items ) {
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
            if(QtDatapackClientLoader::datapackLoader->has_itemExtra(item))
            {
                image=QPixmap(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_itemExtra(item).imagePath));
                name=QtDatapackClientLoader::datapackLoader->get_itemExtra(item).name;
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
        { QElapsedTimer t; t.start(); add_to_inventoryGainTime.push_back(t); }
        OverMapLogic::showGain();
    }
    else
    {
        //add without show
        for( const std::pair<const uint16_t, uint32_t>& n : items ) {
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

    /*load_inventory();
    load_plant_inventory();
    on_listCraftingList_itemSelectionChanged();*/
}

void OverMapLogic::remove_to_inventory(const uint16_t &itemId,const uint32_t &quantity)
{
    std::unordered_map<uint16_t,uint32_t> items;
    items[itemId]=quantity;
    remove_to_inventory(items);
}

void OverMapLogic::remove_to_inventory_slotpair(const uint16_t &itemId, const uint32_t &quantity)
{
    remove_to_inventory(itemId,quantity);
}

void OverMapLogic::remove_to_inventory_slot(const std::unordered_map<uint16_t,uint32_t> &items)
{
    remove_to_inventory(items);
}

void OverMapLogic::remove_to_inventory(const std::unordered_map<uint16_t,uint32_t> &items)
{
    CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations();
    for( const std::pair<const uint16_t, uint32_t>& n : items ) {
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
    /*load_inventory();
    load_plant_inventory();*/
}

/*void OverMapLogic::load_plant_inventory()
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations_ro();
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "OverMapLogic::load_plant_inventory()";
    #endif
    if(!haveInventory || !datapackIsParsed)
        return;
    ui->listPlantList->clear();
    plants_items_graphical.clear();
    plants_items_to_graphical.clear();
    std::map<uint16_t, uint32_t>::const_iterator i=playerInformations.items.begin();
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
    Player_private_and_public_informations informations=connexionManager->client->get_player_informations();
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
}*/



void OverMapLogic::botFight(const uint16_t &fightId)
{
    //bot fights are per-map in BaseMap; resolve the trainer team from the datapack
    const CATCHCHALLENGER_TYPE_MAPID mapId=ccmap->mapController.currentMap();
    const CatchChallenger::CommonMap &mapData=QtDatapackClientLoader::datapackLoader->getMap(mapId);
    if(mapData.botFights.find(static_cast<uint8_t>(fightId))==mapData.botFights.cend())
    {
        showTip(tr("Bot fight not found").toStdString());
        return;
    }
    const CatchChallenger::BotFight &botFightData=mapData.botFights.at(static_cast<uint8_t>(fightId));
    //transform each datapack BotFightMonster into a real PlayerMonster (with rolled stats)
    std::vector<CatchChallenger::PlayerMonster> team;
    unsigned int index=0;
    while(index<botFightData.monsters.size())
    {
        const CatchChallenger::BotFight::BotFightMonster &m=botFightData.monsters.at(index);
        if(CatchChallenger::CommonDatapack::commonDatapack.has_monster(m.id))
            team.push_back(CatchChallenger::FacilityLib::botFightMonsterToPlayerMonster(
                m,
                CatchChallenger::CommonFightEngine::getStat(
                    CatchChallenger::CommonDatapack::commonDatapack.get_monster(m.id),m.level)));
        index++;
    }
    if(team.empty())
    {
        showTip(tr("Bot fight has no monster").toStdString());
        return;
    }
    //load the enemy team into the engine BEFORE showing the battle so getOtherMonster()
    //is non-null (else the live doNextAction sees an empty enemy and declares an
    //instant bogus win). setBotMonster startTheFight()s and fills botFightMonsters.
    CatchChallenger::Api_protocol_Qt::FightInProgressType fip;
    fip.mapId=mapId;
    fip.botId=static_cast<CATCHCHALLENGER_TYPE_BOTID>(fightId);
    connexionManager->client->setBotMonster(team,fip);
    //remember for the win reward: fightFinished() clears the engine's fightInProgress
    //before battleWin fires, so we can't read the fightId back from the engine then.
    currentBotFightId=fightId;
    currentBotFightMapId=mapId;
    ensureBattle();
    Battle::battle->startBotBattle(mapId,ccmap->mapController.getX(),ccmap->mapController.getY());
    emit setForeground(Battle::battle);
}

void OverMapLogic::ensureBattle()
{
    if(Battle::battle==nullptr)
        Battle::battle=new Battle();
    //idempotent wiring of the battle -> overmap signals (UniqueConnection)
    if(!connect(Battle::battle,&Battle::battleWin,this,&OverMapLogic::battleWon,Qt::UniqueConnection))
        abort();
    if(!connect(Battle::battle,&Battle::battleLose,this,&OverMapLogic::battleFinished,Qt::UniqueConnection))
        abort();
    if(!connect(Battle::battle,&Battle::openBagForBattle,this,&OverMapLogic::openBagForBattle,Qt::UniqueConnection))
        abort();
    if(!connect(Battle::battle,&Battle::openMonsterListForBattle,this,&OverMapLogic::openMonsterListForBattle,Qt::UniqueConnection))
        abort();
    if(!connect(Battle::battle,&Battle::error,this,&OverMapLogic::showTip,Qt::UniqueConnection))
        abort();
    Battle::battle->setVar(connexionManager);
}

void OverMapLogic::battleFinished()
{
    //the fight collision blocked the player on the map (MapVisualiserPlayer::
    //blocked) — release it now the fight is over, like the 800x600 client does,
    //else the player can never move (nor trigger a new fight) again
    ccmap->mapController.unblock();
    //return to the map overlay (the fight replaced the overmap foreground)
    emit setForeground(this);
}

void OverMapLogic::openBagForBattle()
{
    selectObject(ObjectType_UseInFight);
}

void OverMapLogic::openMonsterListForBattle()
{
    selectObject(ObjectType_MonsterToFightKO);
}

void OverMapLogic::wildFightCollision(const CATCHCHALLENGER_TYPE_MAPID &mapIndex, const COORD_TYPE &x, const COORD_TYPE &y)
{
    ensureBattle();
    Battle::battle->startWildBattle(mapIndex,x,y);
    emit setForeground(Battle::battle);
}

void OverMapLogic::botFightCollision(const CATCHCHALLENGER_TYPE_MAPID &mapIndex, const COORD_TYPE &x, const COORD_TYPE &y)
{
    //line-of-sight collision: the signal carries the bot's trigger position; resolve
    //its fightId from the map, then run the SAME path as a dialog fight (botFight()
    //loads the team). Mirror the working 800x600 BaseWindowMap::botFightCollision.
    //The collision engine path already handled the server side, so do NOT requestFight() here.
    const CatchChallenger::CommonMap &mapData=QtDatapackClientLoader::datapackLoader->getMap(mapIndex);
    const std::pair<uint8_t,uint8_t> pos(x,y);
    if(mapData.botsFightTrigger.find(pos)==mapData.botsFightTrigger.cend())
    {
        showTip(tr("Bot triggered but no bot at this place").toStdString());
        return;
    }
    botFight(static_cast<uint16_t>(mapData.botsFightTrigger.at(pos)));
}

void OverMapLogic::battleWon()
{
    //a bot fight win pays cash + marks the trainer beaten (so re-walking the trigger
    //doesn't re-fight). Wild wins have currentBotFightId==0xFFFF -> no reward.
    if(currentBotFightId!=0xFFFF)
    {
        const CatchChallenger::CommonMap &mapData=QtDatapackClientLoader::datapackLoader->getMap(currentBotFightMapId);
        if(mapData.botFights.find(static_cast<uint8_t>(currentBotFightId))!=mapData.botFights.cend())
        {
            const CatchChallenger::BotFight &botFightData=mapData.botFights.at(static_cast<uint8_t>(currentBotFightId));
            if(botFightData.cash>0)
                addCash(botFightData.cash);
            ccmap->mapController.addBeatenBotFight(currentBotFightMapId,
                                                   static_cast<CATCHCHALLENGER_TYPE_BOTID>(currentBotFightId));
        }
        currentBotFightId=0xFFFF;
        currentBotFightMapId=0xFFFF;
    }
    battleFinished();
}

void OverMapLogic::cityCapture(const uint32_t &remainingTime,const uint8_t &type)
{
    if(remainingTime==0)
    {
        nextCityCatchTimer.stop();
        std::cout << "City capture disabled" << std::endl;
        return;//disabled
    }
    {
        const int64_t timerInterval=static_cast<int64_t>(remainingTime)*1000;
        if(timerInterval<0 || timerInterval>INT_MAX)
            std::cerr << "QTimer negative interval at " << __FILE__ << ":" << __LINE__ << " value: " << timerInterval << std::endl;
        nextCityCatchTimer.start(remainingTime*1000);
    }
    nextCatch=QDateTime::fromMSecsSinceEpoch(QDateTime::currentMSecsSinceEpoch()+remainingTime*1000);
    city.capture.frenquency=(CatchChallenger::City::Capture::Frequency)type;
    city.capture.day=(CatchChallenger::City::Capture::Day)QDateTime::currentDateTime().addSecs(remainingTime).date().dayOfWeek();
    city.capture.hour=static_cast<uint8_t>(QDateTime::currentDateTime().addSecs(remainingTime).time().hour());
    city.capture.minute=static_cast<uint8_t>(QDateTime::currentDateTime().addSecs(remainingTime).time().minute());
}

void OverMapLogic::insert_plant(const CATCHCHALLENGER_TYPE_MAPID &mapId, const uint8_t &x, const uint8_t &y, const uint8_t &plant_id, const uint16_t &seconds_to_mature)
{
    if(mapId>=(CATCHCHALLENGER_TYPE_MAPID)QtDatapackClientLoader::datapackLoader->get_mapList().size())
    {
        qDebug() << "MapController::insert_plant() mapId greater than mapList.size()";
        return;
    }
    CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations();
    const std::pair<COORD_TYPE,COORD_TYPE> pos(x,y);
    CatchChallenger::PlayerPlant &plant=playerInformations.mapData[mapId].plants[pos];
    plant.plant=plant_id;
    plant.mature_at=QDateTime::currentMSecsSinceEpoch()/1000+seconds_to_mature;
    cancelAllPlantQuery(mapId,x,y);
}

void OverMapLogic::remove_plant(const CATCHCHALLENGER_TYPE_MAPID &mapId,const uint8_t &x,const uint8_t &y)
{
    if(mapId>=(CATCHCHALLENGER_TYPE_MAPID)QtDatapackClientLoader::datapackLoader->get_mapList().size())
    {
        qDebug() << "MapController::remove_plant() mapId greater than mapList.size()";
        return;
    }
    CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations();
    const std::pair<COORD_TYPE,COORD_TYPE> pos(x,y);
    std::map<uint16_t, CatchChallenger::Player_private_and_public_informations_Map>::iterator mapIt=playerInformations.mapData.find(mapId);
    if(mapIt!=playerInformations.mapData.cend())
        mapIt->second.plants.erase(pos);
    cancelAllPlantQuery(mapId,x,y);
}

void OverMapLogic::cancelAllPlantQuery(const CATCHCHALLENGER_TYPE_MAPID map,const uint8_t x,const uint8_t y)
{
    unsigned int index;
    index=0;
    while(index<seed_in_waiting.size())
    {
        const SeedInWaiting &seedInWaiting=seed_in_waiting.at(index);
        if(seedInWaiting.map==map && seedInWaiting.x==x && seedInWaiting.y==y)
        {
            seed_in_waiting[index].map=0;
            seed_in_waiting[index].x=0;
            seed_in_waiting[index].y=0;
        }
        index++;
    }
    index=0;
    while(index<plant_collect_in_waiting.size())
    {
        const ClientPlantInCollecting &clientPlantInCollecting=plant_collect_in_waiting.at(index);
        if(clientPlantInCollecting.map==map && clientPlantInCollecting.x==x && clientPlantInCollecting.y==y)
        {
            plant_collect_in_waiting[index].map=0;
            plant_collect_in_waiting[index].x=0;
            plant_collect_in_waiting[index].y=0;
        }
        index++;
    }
}

void OverMapLogic::seed_planted(const bool &ok)
{
    removeQuery(QueryType_Seed);
    if(ok)
    {
        /// \todo add to the map here, and don't send on the server
        showTip(tr("Seed correctly planted").toStdString());
        //do the rewards
        {
            const uint16_t &itemId=seed_in_waiting.front().seedItemId;
            if(!QtDatapackClientLoader::datapackLoader->has_itemToPlant(itemId))
            {
                qDebug() << "Item is not a plant";
                emit error(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return;
            }
            const uint8_t &plant=QtDatapackClientLoader::datapackLoader->get_itemToPlant(itemId);
            appendReputationRewards(CatchChallenger::CommonDatapack::commonDatapack.get_plant(plant).rewards.reputation);
        }
    }
    else
    {
        if(seed_in_waiting.front().map!=0)
        {
            ccmap->mapController.remove_plant_full(seed_in_waiting.front().map,seed_in_waiting.front().x,seed_in_waiting.front().y);
            cancelAllPlantQuery(seed_in_waiting.front().map,seed_in_waiting.front().x,seed_in_waiting.front().y);
        }
        add_to_inventory(seed_in_waiting.front().seedItemId,1,false);
        showTip(tr("Seed cannot be planted").toStdString());
    }
    seed_in_waiting.erase(seed_in_waiting.cbegin());
}

void OverMapLogic::plant_collected(const CatchChallenger::Plant_collect &stat)
{
    removeQuery(QueryType_CollectPlant);
    switch(stat)
    {
        case CatchChallenger::Plant_collect_correctly_collected:
            //see to optimise CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer==true and use the internal random number list
            showTip(tr("Plant collected").toStdString());//the item is send by another message with the protocol
        break;
        /*case CatchChallenger::Plant_collect_empty_dirt:
            showTip(tr("Try collect an empty dirt").toStdString());
        break;
        case CatchChallenger::Plant_collect_owned_by_another_player:
            showTip(tr("This plant had been planted recently by another player").toStdString());
            ccmap->mapController.insert_plant_full(plant_collect_in_waiting.front().map,plant_collect_in_waiting.front().x,plant_collect_in_waiting.front().y,
                                             plant_collect_in_waiting.front().plant_id,plant_collect_in_waiting.front().seconds_to_mature);
            cancelAllPlantQuery(plant_collect_in_waiting.front().map,plant_collect_in_waiting.front().x,plant_collect_in_waiting.front().y);
        break;
        case CatchChallenger::Plant_collect_impossible:
            showTip(tr("This plant can't be collected").toStdString());
            ccmap->mapController.insert_plant_full(plant_collect_in_waiting.front().map,plant_collect_in_waiting.front().x,plant_collect_in_waiting.front().y,
                                             plant_collect_in_waiting.front().plant_id,plant_collect_in_waiting.front().seconds_to_mature);
            cancelAllPlantQuery(plant_collect_in_waiting.front().map,plant_collect_in_waiting.front().x,plant_collect_in_waiting.front().y);
        break;*/
        default:
            qDebug() << "OverMapLogic::plant_collected(): unknown return";
        break;
    }
}

/*void OverMapLogic::on_toolButton_quit_plants_clicked()
{
    ui->listPlantList->reset();
    if(inSelection)
    {
        ui->stackedWidget->setCurrentWidget(ui->page_map);
        objectSelection(false,0);
    }
    else
        ui->stackedWidget->setCurrentWidget(ui->page_inventory);
    on_listPlantList_itemSelectionChanged();
}

void OverMapLogic::on_plantUse_clicked()
{
    QList<QListWidgetItem *> items=ui->listPlantList->selectedItems();
    if(items.size()!=1)
        return;
    on_listPlantList_itemActivated(items.first());
}

void OverMapLogic::on_listPlantList_itemActivated(QListWidgetItem *item)
{
    if(plants_items_graphical.find(item)==plants_items_graphical.cend())
    {
        qDebug() << "OverMapLogic::on_inventory_itemActivated(): activated item not found";
        return;
    }
    if(!inSelection)
    {
        qDebug() << "OverMapLogic::on_inventory_itemActivated(): not in selection, use is not done actually";
        return;
    }
    objectSelection(true,CatchChallenger::CommonDatapack::commonDatapack.plants[plants_items_graphical[item]].itemUsed);
}

void OverMapLogic::on_pushButton_interface_crafting_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_crafting);
}

void OverMapLogic::on_toolButton_quit_crafting_clicked()
{
    ui->listCraftingList->reset();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    on_listCraftingList_itemSelectionChanged();
}

void OverMapLogic::on_listCraftingList_itemSelectionChanged()
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    ui->listCraftingMaterials->clear();
    QList<QListWidgetItem *> displayedItems=ui->listCraftingList->selectedItems();
    if(displayedItems.size()!=1)
    {
        ui->labelCraftingImage->setPixmap(QtDatapackClientLoader::datapackLoader->defaultInventoryImage());
        ui->labelCraftingDetails->setText(tr("Select a recipe"));
        ui->craftingUse->setVisible(false);
        return;
    }
    QListWidgetItem *itemMaterials=displayedItems.first();
    const CatchChallenger::CraftingRecipe &content=CatchChallenger::CommonDatapack::commonDatapack.craftingRecipes[crafting_recipes_items_graphical[itemMaterials]];

    qDebug() << "on_listCraftingList_itemSelectionChanged() load the name";
    //load the name
    QString name;
    if(QtDatapackClientLoader::datapackLoader->itemsExtra.find(content.doItemId)!=
            QtDatapackClientLoader::datapackLoader->itemsExtra.cend())
    {
        name=QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra[content.doItemId].name);
        ui->labelCraftingImage->setPixmap(QtDatapackClientLoader::datapackLoader->QtitemsExtra[content.doItemId].image);
    }
    else
    {
        name=tr("Unknow item (%1)").arg(content.doItemId);
        ui->labelCraftingImage->setPixmap(QtDatapackClientLoader::datapackLoader->defaultInventoryImage());
    }
    ui->labelCraftingDetails->setText(tr("Name: <b>%1</b><br /><br />Success: <b>%2%</b><br /><br />Result: <b>%3</b>").arg(name).arg(content.success).arg(content.quantity));

    //load the materials
    bool haveMaterials=true;
    unsigned int index=0;
    QString nameMaterials;
    QListWidgetItem *item;
    uint32_t quantity;
    while(index<content.materials.size())
    {
        //load the material item
        item=new QListWidgetItem();
        if(QtDatapackClientLoader::datapackLoader->itemsExtra.find(content.materials.at(index).item)!=
                QtDatapackClientLoader::datapackLoader->itemsExtra.cend())
        {
            nameMaterials=QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra[content.materials.at(index).item].name);
            item->setIcon(QtDatapackClientLoader::datapackLoader->QtitemsExtra[content.materials.at(index).item].image);
        }
        else
        {
            nameMaterials=tr("Unknow item (%1)").arg(content.materials.at(index).item);
            item->setIcon(QtDatapackClientLoader::datapackLoader->defaultInventoryImage());
        }

        //load the quantity into the inventory
        quantity=0;
        if(playerInformations.items.find(content.materials.at(index).item)!=playerInformations.items.cend())
            quantity=playerInformations.items.at(content.materials.at(index).item);

        //load the display
        item->setText(tr("Needed: %1 %2\nIn the inventory: %3 %4").arg(content.materials.at(index).quantity).arg(nameMaterials).arg(quantity).arg(nameMaterials));
        if(quantity<content.materials.at(index).quantity)
        {
            item->setFont(disableIntoListFont);
            item->setForeground(disableIntoListBrush);
        }

        if(quantity<content.materials.at(index).quantity)
            haveMaterials=false;

        ui->listCraftingMaterials->addItem(item);
        ui->craftingUse->setVisible(haveMaterials);
        index++;
    }
}

void OverMapLogic::on_craftingUse_clicked()
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    //recipeInUsing
    QList<QListWidgetItem *> displayedItems=ui->listCraftingList->selectedItems();
    if(displayedItems.size()!=1)
        return;
    QListWidgetItem *selectedItem=displayedItems.first();
    const CatchChallenger::CraftingRecipe &content=CatchChallenger::CommonDatapack::commonDatapack.craftingRecipes[crafting_recipes_items_graphical[selectedItem]];

    QStringList mIngredients;
    QString mRecipe;
    QString mProduct;
    //load the materials
    unsigned int index=0;
    while(index<content.materials.size())
    {
        if(playerInformations.items.find(content.materials.at(index).item)==playerInformations.items.cend())
            return;
        if(playerInformations.items.at(content.materials.at(index).item)<content.materials.at(index).quantity)
            return;
        uint32_t sub_index=0;
        while(sub_index<content.materials.at(index).quantity)
        {
            mIngredients.push_back(QUrl::fromLocalFile(
                                       QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra[content.materials.at(index).item].imagePath)
                                   ).toEncoded());
            sub_index++;
        }
        index++;
    }
    index=0;
    std::vector<std::pair<uint16_t,uint32_t> > recipeUsage;
    while(index<content.materials.size())
    {
        std::pair<uint16_t,uint32_t> pair;
        pair.first=content.materials.at(index).item;
        pair.second=content.materials.at(index).quantity;
        remove_to_inventory(pair.first,pair.second);
        recipeUsage.push_back(pair);
        index++;
    }
    materialOfRecipeInUsing.push_back(recipeUsage);
    //the product do
    std::pair<uint16_t,uint32_t> pair;
    pair.first=content.doItemId;
    pair.second=content.quantity;
    productOfRecipeInUsing.push_back(pair);
    mProduct=QUrl::fromLocalFile(
                QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra[content.doItemId].imagePath)).toEncoded();
    mRecipe=QUrl::fromLocalFile(
                QString::fromStdString(QtDatapackClientLoader::datapackLoader->itemsExtra[content.itemToLearn].imagePath)).toEncoded();
    //update the UI
    load_inventory();
    load_plant_inventory();
    on_listCraftingList_itemSelectionChanged();
    //send to the network
    client->useRecipe(crafting_recipes_items_graphical.at(selectedItem));
    appendReputationRewards(CatchChallenger::CommonDatapack::commonDatapack.craftingRecipes.at(
                                crafting_recipes_items_graphical.at(selectedItem)).rewards.reputation);
    //create animation widget
    if(animationWidget!=NULL)
        delete animationWidget;
    if(qQuickViewContainer!=NULL)
        delete qQuickViewContainer;
    animationWidget=new QQuickView();
    qQuickViewContainer = QWidget::createWindowContainer(animationWidget);
    qQuickViewContainer->setMinimumSize(QSize(800,600));
    qQuickViewContainer->setMaximumSize(QSize(800,600));
    qQuickViewContainer->setFocusPolicy(Qt::TabFocus);
    ui->verticalLayoutPageAnimation->addWidget(qQuickViewContainer);
    //show the animation
    previousAnimationWidget=ui->stackedWidget->currentWidget();
    ui->stackedWidget->setCurrentWidget(ui->page_animation);
    if(craftingAnimationObject!=NULL)
        delete craftingAnimationObject;
    craftingAnimationObject=new CraftingAnimation(mIngredients,
                                                  mRecipe,mProduct,
                                                  QUrl::fromLocalFile(QString::fromStdString(
              playerBackImagePath)).toEncoded());
    animationWidget->rootContext()->setContextProperty("animationControl",&animationControl);
    animationWidget->rootContext()->setContextProperty("craftingAnimationObject",craftingAnimationObject);
    const QString datapackQmlFile=QString::fromStdString(client->datapackPathBase())+"qml/crafting-animation.qml";
    if(QFile(datapackQmlFile).exists())
        animationWidget->setSource(QUrl::fromLocalFile(datapackQmlFile));
    else
        animationWidget->setSource(QStringLiteral("qrc:/qml/crafting-animation.qml"));
}

void OverMapLogic::on_listCraftingMaterials_itemActivated(QListWidgetItem *item)
{
    Q_UNUSED(item);
    ui->craftingUse->clicked();
}*/

/*void OverMapLogic::recipeUsed(const CatchChallenger::RecipeUsage &recipeUsage)
{
    switch(recipeUsage)
    {
        case CatchChallenger::RecipeUsage_ok:
            materialOfRecipeInUsing.erase(materialOfRecipeInUsing.cbegin());
            add_to_inventory(productOfRecipeInUsing.front().first,productOfRecipeInUsing.front().second);
            productOfRecipeInUsing.erase(productOfRecipeInUsing.cbegin());
            //update the UI
            load_inventory();
            load_plant_inventory();
            on_listCraftingList_itemSelectionChanged();
        break;
        case CatchChallenger::RecipeUsage_impossible:
        {
            unsigned int index=0;
            while(index<materialOfRecipeInUsing.front().size())
            {
                add_to_inventory(materialOfRecipeInUsing.front().at(index).first,
                                 materialOfRecipeInUsing.front().at(index).first,false);
                index++;
            }
            materialOfRecipeInUsing.erase(materialOfRecipeInUsing.cbegin());
            productOfRecipeInUsing.erase(productOfRecipeInUsing.cbegin());
            //update the UI
            load_inventory();
            load_plant_inventory();
            on_listCraftingList_itemSelectionChanged();
        }
        break;
        case RecipeUsage_failed:
            materialOfRecipeInUsing.erase(materialOfRecipeInUsing.cbegin());
            productOfRecipeInUsing.erase(productOfRecipeInUsing.cbegin());
        break;
        default:
        qDebug() << "recipeUsed() unknow code";
        return;
    }
}*/

/*void OverMapLogic::on_listCraftingList_itemActivated(QListWidgetItem *)
{
    if(ui->craftingUse->isVisible())
        on_craftingUse_clicked();
}*/

void OverMapLogic::appendReputationRewards(const std::vector<CatchChallenger::ReputationRewards> &reputationList)
{
    unsigned int index=0;
    while(index<reputationList.size())
    {
        const CatchChallenger::ReputationRewards &reputationRewards=reputationList.at(index);
        appendReputationPoint(CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(reputationRewards.reputationId).name,reputationRewards.point);
        index++;
    }
}

//reputation
void OverMapLogic::appendReputationPoint(const std::string &type,const int32_t &point)
{
    if(point==0)
        return;
    if(!QtDatapackClientLoader::datapackLoader->has_reputationNameToId(type))
    {
        emit error("Unknow reputation: "+type);
        return;
    }
    const uint8_t &reputationId=QtDatapackClientLoader::datapackLoader->get_reputationNameToId(type);
    CatchChallenger::PlayerReputation playerReputation;
    if(connexionManager->client->player_informations.reputation.find(reputationId)!=connexionManager->client->player_informations.reputation.cend())
        playerReputation=connexionManager->client->player_informations.reputation.at(reputationId);
    else
    {
        playerReputation.point=0;
        playerReputation.level=0;
    }
    CatchChallenger::PlayerReputation oldPlayerReputation=playerReputation;
    int32_t old_level=playerReputation.level;
    CatchChallenger::FacilityLib::appendReputationPoint(&playerReputation,point,CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(reputationId));
    if(oldPlayerReputation.level==playerReputation.level && oldPlayerReputation.point==playerReputation.point)
        return;
    if(connexionManager->client->player_informations.reputation.find(reputationId)!=connexionManager->client->player_informations.reputation.cend())
        connexionManager->client->player_informations.reputation[reputationId]=playerReputation;
    else
        connexionManager->client->player_informations.reputation[reputationId]=playerReputation;
    const std::string &reputationCodeName=CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(reputationId).name;
    if(old_level<playerReputation.level)
    {
        if(QtDatapackClientLoader::datapackLoader->has_reputationExtra(reputationCodeName))
            showTip(tr("You have better reputation into %1")
                    .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_reputationExtra(reputationCodeName).name)).toStdString());
        else
            showTip(tr("You have better reputation into %1")
                    .arg("???").toStdString());
    }
    else if(old_level>playerReputation.level)
    {
        if(QtDatapackClientLoader::datapackLoader->has_reputationExtra(reputationCodeName))
            showTip(tr("You have worse reputation into %1")
                    .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_reputationExtra(reputationCodeName).name)).toStdString());
        else
            showTip(tr("You have worse reputation into %1")
                    .arg("???").toStdString());
    }
}

void OverMapLogic::objectSelection(const bool &ok, const uint16_t &itemId, const uint32_t &quantity)
{
    CatchChallenger::Player_private_and_public_informations &playerInformations=
            connexionManager->client->get_player_informations();
    inSelection=false;
    const ObjectType tempWaitedObjectType=waitedObjectType;
    waitedObjectType=ObjectType_All;
    switch(tempWaitedObjectType)
    {
        case ObjectType_ItemEvolutionOnMonster:
        case ObjectType_ItemLearnOnMonster:
        case ObjectType_ItemOnMonster:
        case ObjectType_ItemOnMonsterOutOfFight:
        {
            //itemId is the chosen team POSITION; the item is the one being used
            const uint8_t monsterPosition=static_cast<uint8_t>(itemId);
            if(objectInUsing.empty())
                break;
            const uint16_t item=objectInUsing.back();
            objectInUsing.erase(objectInUsing.cbegin());
            CatchChallenger::CommonDatapack &dp=CatchChallenger::CommonDatapack::commonDatapack;
            if(!ok || monsterPosition>=playerInformations.monsters.size())
            {
                //cancelled or invalid target: give the consumed item back
                if(dp.has_item(item) && dp.get_item(item).consumeAtUse)
                    add_to_inventory(item,1,false);
                break;
            }
            const CatchChallenger::PlayerMonster &monster=playerInformations.monsters.at(monsterPosition);
            const std::string monsterName=QtDatapackClientLoader::datapackLoader->has_monsterExtra(monster.monster)
                    ?QtDatapackClientLoader::datapackLoader->get_monsterExtra(monster.monster).name
                    :std::to_string(monster.monster);
            if(connexionManager->client->useObjectOnMonsterByPosition(item,monsterPosition))
            {
                connexionManager->client->useObjectOnMonsterByPosition(item,monsterPosition);
                showTip(tr("Using <b>%1</b> on <b>%2</b>")
                        .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_itemExtra(item).name))
                        .arg(QString::fromStdString(monsterName)).toStdString());
                //evolution animation/check is wired with the Battle/Animation screen
            }
            else
            {
                showTip(tr("Failed to use <b>%1</b> on <b>%2</b>")
                        .arg(QString::fromStdString(QtDatapackClientLoader::datapackLoader->get_itemExtra(item).name))
                        .arg(QString::fromStdString(monsterName)).toStdString());
                if(dp.has_item(item) && dp.get_item(item).consumeAtUse)
                    add_to_inventory(item,1,false);
            }
        }
        break;
        case ObjectType_Sell:
        {
            //close the picker and return to the map
            setAbove(nullptr);
            if(!ok)
                break;
            if(playerInformations.items.find(itemId)==playerInformations.items.cend())
                break;
            if(playerInformations.items.at(itemId)<quantity)
                break;
            remove_to_inventory(itemId,quantity);
            const uint32_t price=CatchChallenger::CommonDatapack::commonDatapack.get_item(itemId).price/2;
            connexionManager->client->sellObject(itemId,quantity,price);
            //credit the sale locally (the server does the same server-side; the
            //haveSellObject reply is a no-op in this interface)
            addCash(price*quantity);
        }
        break;
        case ObjectType_Seed:
        {
            //came from actionOn(): a SeedInWaiting was pushed for the dirt tile.
            setAbove(nullptr);
            if(seed_in_waiting.empty())
                break;
            if(!ok)
            {
                seed_in_waiting.pop_back();
                break;
            }
            if(!QtDatapackClientLoader::datapackLoader->has_itemToPlant(itemId))
            {
                showTip(tr("This item is not a plant").toStdString());
                seed_in_waiting.pop_back();
                break;
            }
            const uint8_t plantId=QtDatapackClientLoader::datapackLoader->get_itemToPlant(itemId);
            if(!connexionManager->client->haveReputationRequirements(CatchChallenger::CommonDatapack::commonDatapack.get_plant(plantId).requirements.reputation))
            {
                showTip(tr("You don't have the requirements to plant this seed").toStdString());
                seed_in_waiting.pop_back();
                break;
            }
            if(playerInformations.items.find(itemId)==playerInformations.items.cend())
            {
                seed_in_waiting.pop_back();
                break;
            }
            remove_to_inventory(itemId,1);
            const SeedInWaiting seedInWaiting=seed_in_waiting.back();
            seed_in_waiting.back().seedItemId=itemId;
            //SeedInWaiting.map is already a real map id (set in actionOn)
            insert_plant(seedInWaiting.map,seedInWaiting.x,seedInWaiting.y,plantId,
                         static_cast<uint16_t>(CatchChallenger::CommonDatapack::commonDatapack.get_plant(plantId).fruits_seconds));
            emit useSeed(plantId);
        }
        break;
        case ObjectType_MonsterToFight:
        case ObjectType_MonsterToFightKO:
        {
            //switch the active monster in the current fight; itemId is the team position
            if(!ok)
            {
                //a KO switch is mandatory -> re-open the picker; a voluntary one
                //just resumes the fight menu
                if(tempWaitedObjectType==ObjectType_MonsterToFightKO)
                    selectObject(ObjectType_MonsterToFightKO);
                else if(Battle::battle!=nullptr)
                    Battle::battle->doNextAction();
                break;
            }
            const uint8_t pos=static_cast<uint8_t>(itemId);
            if(connexionManager->client->changeOfMonsterInFight(pos))
            {
                connexionManager->client->changeOfMonsterInFightByPosition(pos);
                if(Battle::battle!=nullptr)
                {
                    Battle::battle->init_current_monster_display();
                    Battle::battle->doNextAction();
                }
            }
            else if(tempWaitedObjectType==ObjectType_MonsterToFightKO)
                selectObject(ObjectType_MonsterToFightKO);//invalid pick, must retry
        }
        break;
        case ObjectType_UseInFight:
        {
            //use the picked item on the currently-fighting monster (trap/catch is
            //a later step); itemId is the item
            if(ok)
            {
                const uint8_t pos=connexionManager->client->getCurrentSelectedMonsterNumber();
                if(connexionManager->client->useObjectOnMonsterByPosition(itemId,pos))
                {
                    remove_to_inventory(itemId,1);
                    connexionManager->client->useObjectOnMonsterByPosition(itemId,pos);
                }
            }
            if(Battle::battle!=nullptr)
                Battle::battle->doNextAction();
        }
        break;
        default:
            //Trade item and out-of-fight monster-trade are ported in later steps.
            setAbove(nullptr);
            showTip(tr("This action is not available yet").toStdString());
        break;
    }
}

void OverMapLogic::objectUsed(const CatchChallenger::ObjectUsage &objectUsage)
{
    if(objectInUsing.empty())
    {
        emit error("No object usage to validate");
        return;
    }
    switch(objectUsage)
    {
        case CatchChallenger::ObjectUsage_correctlyUsed:
        {
            const uint16_t item=objectInUsing.front();
            //is crafting recipe
            if(CatchChallenger::CommonDatapack::commonDatapack.has_itemToCraftingRecipe(item))
            {
                //learn the recipe (the Crafting screen re-reads it when next opened);
                //was abort() — reachable now that bag item-use dispatches recipes
                connexionManager->client->addRecipe(CatchChallenger::CommonDatapack::commonDatapack.get_itemToCraftingRecipe(item));
                showTip(tr("You learned a new recipe").toStdString());
            }
            else if(CatchChallenger::CommonDatapack::commonDatapack.has_trap(item))
            {
            }
            else if(CatchChallenger::CommonDatapack::commonDatapack.has_repel(item))
            {
            }
            else
                qDebug() << "OverMapLogic::objectUsed(): unknow object type";
        }
        break;
        case CatchChallenger::ObjectUsage_failedWithConsumption:
        break;
        case CatchChallenger::ObjectUsage_failedWithoutConsumption:
            add_to_inventory(objectInUsing.front());
        break;
        default:
        break;
    }
    objectInUsing.erase(objectInUsing.cbegin());
}

void OverMapLogic::addCash(const uint32_t &cash)
{
    CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations();
    playerInformations.cash+=cash;
}

void OverMapLogic::removeCash(const uint32_t &cash)
{
    CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations();
    playerInformations.cash-=cash;
}
