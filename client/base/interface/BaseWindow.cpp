#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "../../general/base/FacilityLib.h"
#include "../ClientVariable.h"
#include "../fight/interface/ClientFightEngine.h"
#include "../../../general/base/CommonDatapack.h"
#include "DatapackClientLoader.h"
#include "MapController.h"
#include "Chat.h"
#include "WithAnotherPlayer.h"

#include <QListWidgetItem>
#include <QBuffer>
#include <QInputDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QRegularExpression>
#include <QScriptValue>
#include <QScriptEngine>

//do buy queue
//do sell queue

using namespace CatchChallenger;

BaseWindow* BaseWindow::baseWindow=NULL;

BaseWindow::BaseWindow() :
    ui(new Ui::BaseWindowUI)
{
    qRegisterMetaType<CatchChallenger::Chat_type>("CatchChallenger::Chat_type");
    qRegisterMetaType<CatchChallenger::Player_type>("CatchChallenger::Player_type");
    qRegisterMetaType<CatchChallenger::Player_private_and_public_informations>("CatchChallenger::Player_private_and_public_informations");
    qRegisterMetaType<QHash<quint32,quint32> >("QHash<quint32,quint32>");
    qRegisterMetaType<QHash<quint32,quint32> >("CatchChallenger::Plant_collect");
    qRegisterMetaType<QList<ItemToSellOrBuy> >("QList<ItemToSell>");
    qRegisterMetaType<Skill::AttackReturn>("Skill::AttackReturn");

    socketState=QAbstractSocket::UnconnectedState;

    MapController::mapController=new MapController(true,false,true,false);
    ProtocolParsing::initialiseTheVariable();
    ui->setupUi(this);
    Chat::chat=new Chat(ui->page_map);
    escape=false;
    movie=NULL;

    updateRXTXTimer.start(1000);
    updateRXTXTime.restart();

    tip_timeout.setInterval(TIMETODISPLAY_TIP);
    gain_timeout.setInterval(TIMETODISPLAY_GAIN);
    tip_timeout.setSingleShot(true);
    gain_timeout.setSingleShot(true);

    moveFightMonsterBottomTimer.setSingleShot(true);
    moveFightMonsterBottomTimer.setInterval(20);
    moveFightMonsterTopTimer.setSingleShot(true);
    moveFightMonsterTopTimer.setInterval(20);
    moveFightMonsterBothTimer.setSingleShot(true);
    moveFightMonsterBothTimer.setInterval(20);
    displayAttackTimer.setSingleShot(true);
    displayAttackTimer.setInterval(50);
    displayTrapTimer.setSingleShot(true);
    displayTrapTimer.setInterval(50);
    doNextActionTimer.setSingleShot(true);
    doNextActionTimer.setInterval(1500);

    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::protocol_is_good,this,&BaseWindow::protocol_is_good,Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_protocol::disconnected,this,&BaseWindow::disconnected,Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::error,this,&BaseWindow::error,Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::message,this,&BaseWindow::message,Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::notLogged,this,&BaseWindow::notLogged,Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::logged,this,&BaseWindow::logged,Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::haveTheDatapack,this,&BaseWindow::haveTheDatapack,Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::newError,this,&BaseWindow::newError,Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::clanActionFailed,this,&BaseWindow::clanActionFailed,Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::clanActionSuccess,this,&BaseWindow::clanActionSuccess,Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::clanDissolved,this,&BaseWindow::clanDissolved,Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::clanInformations,this,&BaseWindow::clanInformations,Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::clanInvite,this,&BaseWindow::clanInvite,Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::cityCapture,this,&BaseWindow::cityCapture,Qt::QueuedConnection);

    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::captureCityYourAreNotLeader,this,&BaseWindow::captureCityYourAreNotLeader,Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::captureCityYourLeaderHaveStartInOtherCity,this,&BaseWindow::captureCityYourLeaderHaveStartInOtherCity,Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::captureCityPreviousNotFinished,this,&BaseWindow::captureCityPreviousNotFinished,Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::captureCityStartBattle,this,&BaseWindow::captureCityStartBattle,Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::captureCityStartBotFight,this,&BaseWindow::captureCityStartBotFight,Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::captureCityDelayedStart,this,&BaseWindow::captureCityDelayedStart,Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::captureCityWin,this,&BaseWindow::captureCityWin,Qt::QueuedConnection);

    //connect the map controler
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::have_current_player_info,this,&BaseWindow::have_current_player_info,Qt::QueuedConnection);

    //inventory
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::have_inventory,this,&BaseWindow::have_inventory);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::add_to_inventory,this,&BaseWindow::add_to_inventory_slot);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::remove_to_inventory,this,&BaseWindow::remove_to_inventory);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::monsterCatch,this,&BaseWindow::monsterCatch);

    //chat
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::new_chat_text,Chat::chat,&Chat::new_chat_text);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::new_system_text,Chat::chat,&Chat::new_system_text);
    connect(this,&BaseWindow::sendsetMultiPlayer,Chat::chat,&Chat::setVisible,Qt::QueuedConnection);

    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::number_of_player,this,&BaseWindow::number_of_player);

    //connect the datapack loader
    connect(&DatapackClientLoader::datapackLoader,&DatapackClientLoader::datapackParsed,this,&BaseWindow::datapackParsed,Qt::QueuedConnection);
    connect(this,&BaseWindow::parseDatapack,&DatapackClientLoader::datapackLoader,&DatapackClientLoader::parseDatapack,Qt::QueuedConnection);
    connect(&DatapackClientLoader::datapackLoader,&DatapackClientLoader::datapackParsed,MapController::mapController,&MapController::datapackParsed,Qt::QueuedConnection);

    //render, logical part into Map_Client
    connect(MapController::mapController,&MapController::stopped_in_front_of,this,&BaseWindow::stopped_in_front_of);
    connect(MapController::mapController,&MapController::actionOn,this,&BaseWindow::actionOn);
    connect(MapController::mapController,&MapController::blockedOn,this,&BaseWindow::blockedOn);
    connect(MapController::mapController,&MapController::error,this,&BaseWindow::error);
    connect(MapController::mapController,&MapController::errorWithTheCurrentMap,this,&BaseWindow::errorWithTheCurrentMap);

    //fight
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::random_seeds,&ClientFightEngine::fightEngine,&ClientFightEngine::newRandomNumber);
    connect(MapController::mapController,&MapController::wildFightCollision,this,&BaseWindow::wildFightCollision);
    connect(MapController::mapController,&MapController::botFightCollision,this,&BaseWindow::botFightCollision);
    connect(MapController::mapController,&MapController::currentMapLoaded,this,&BaseWindow::currentMapLoaded);
    connect(&moveFightMonsterBottomTimer,&QTimer::timeout,this,&BaseWindow::moveFightMonsterBottom);
    connect(&moveFightMonsterTopTimer,&QTimer::timeout,this,&BaseWindow::moveFightMonsterTop);
    connect(&moveFightMonsterBothTimer,&QTimer::timeout,this,&BaseWindow::moveFightMonsterBoth);
    connect(&displayAttackTimer,&QTimer::timeout,this,&BaseWindow::displayAttack);
    connect(&displayTrapTimer,&QTimer::timeout,this,&BaseWindow::displayTrap);
    connect(&doNextActionTimer,&QTimer::timeout,this,&BaseWindow::doNextAction);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::teleportTo,this,&BaseWindow::teleportTo,Qt::QueuedConnection);

    //plants
    connect(this,&BaseWindow::useSeed,CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::useSeed);
    connect(this,&BaseWindow::collectMaturePlant,CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::collectMaturePlant);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::seed_planted,this,&BaseWindow::seed_planted);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::plant_collected,this,&BaseWindow::plant_collected);
    //crafting
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::recipeUsed,this,&BaseWindow::recipeUsed);
    //trade
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::tradeRequested,this,&BaseWindow::tradeRequested);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::tradeAcceptedByOther,this,&BaseWindow::tradeAcceptedByOther);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::tradeCanceledByOther,this,&BaseWindow::tradeCanceledByOther);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::tradeFinishedByOther,this,&BaseWindow::tradeFinishedByOther);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::tradeValidatedByTheServer,this,&BaseWindow::tradeValidatedByTheServer);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::tradeAddTradeCash,this,&BaseWindow::tradeAddTradeCash);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::tradeAddTradeObject,this,&BaseWindow::tradeAddTradeObject);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::tradeAddTradeMonster,this,&BaseWindow::tradeAddTradeMonster);
    //inventory
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::objectUsed,this,&BaseWindow::objectUsed);
    //shop
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::haveShopList,this,&BaseWindow::haveShopList);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::haveSellObject,this,&BaseWindow::haveSellObject);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::haveBuyObject,this,&BaseWindow::haveBuyObject);
    //factory
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::haveFactoryList,this,&BaseWindow::haveFactoryList);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::haveSellFactoryObject,this,&BaseWindow::haveSellFactoryObject);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::haveBuyFactoryObject,this,&BaseWindow::haveBuyFactoryObject);
    //battle
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::battleRequested,this,&BaseWindow::battleRequested);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::battleAcceptedByOther,this,&BaseWindow::battleAcceptedByOther);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::battleCanceledByOther,this,&BaseWindow::battleCanceledByOther);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::sendBattleReturn,this,&BaseWindow::sendBattleReturn);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::sendBattleReturn,this,&BaseWindow::sendBattleReturn);

    connect(&CatchChallenger::ClientFightEngine::fightEngine,&ClientFightEngine::newError,this,&BaseWindow::newError);
    connect(&CatchChallenger::ClientFightEngine::fightEngine,&ClientFightEngine::error,this,&BaseWindow::error);

    connect(this,&BaseWindow::destroyObject,CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::destroyObject);
    connect(&updateRXTXTimer,&QTimer::timeout,this,&BaseWindow::updateRXTX);

    connect(&tip_timeout,&QTimer::timeout,this,&BaseWindow::tipTimeout);
    connect(&gain_timeout,&QTimer::timeout,this,&BaseWindow::gainTimeout);
    connect(&nextCityCaptureTimer,&QTimer::timeout,this,&BaseWindow::cityCaptureUpdateTime);
    connect(&updater_page_zonecapture,&QTimer::timeout,this,&BaseWindow::updatePageZonecapture);

    MapController::mapController->setDatapackPath(CatchChallenger::Api_client_real::client->get_datapack_base_name());

    renderFrame = new QFrame(ui->page_map);
    renderFrame->setObjectName(QString::fromUtf8("renderFrame"));
    renderFrame->setMinimumSize(QSize(600, 572));
    QVBoxLayout *renderLayout = new QVBoxLayout(renderFrame);
    renderLayout->setSpacing(0);
    renderLayout->setContentsMargins(0, 0, 0, 0);
    renderLayout->setObjectName(QString::fromUtf8("renderLayout"));
    renderLayout->addWidget(MapController::mapController);
    renderFrame->setGeometry(QRect(0, 0, 800, 516));
    renderFrame->lower();
    renderFrame->lower();
    renderFrame->lower();
    ui->labelFightTrap->hide();
    nextCityCaptureTimer.setSingleShot(true);
    updater_page_zonecapture.setSingleShot(false);

    Chat::chat->setGeometry(QRect(0, 0, 250, 400));

    resetAll();
    loadSettings();

    MapController::mapController->setFocus();

    CatchChallenger::Api_client_real::client->startReadData();
    /// \todo able to cancel quest
    ui->cancelQuest->hide();

    audioReadThread.start(QThread::IdlePriority);
}

BaseWindow::~BaseWindow()
{
    while(!ambiance.isEmpty())
    {
        delete ambiance.first();
        ambiance.removeFirst();
    }
    audioReadThread.quit();
    audioReadThread.wait();
    if(movie!=NULL)
        delete movie;
    delete ui;
    delete MapController::mapController;
    delete Chat::chat;
}

void BaseWindow::tradeRequested(const QString &pseudo,const quint8 &skinInt)
{
    WithAnotherPlayer withAnotherPlayerDialog(this,WithAnotherPlayer::WithAnotherPlayerType_Trade,getFrontSkin(skinInt),pseudo);
    withAnotherPlayerDialog.exec();
    if(!withAnotherPlayerDialog.actionIsAccepted())
    {
        CatchChallenger::Api_client_real::client->tradeRefused();
        return;
    }
    CatchChallenger::Api_client_real::client->tradeAccepted();
    tradeAcceptedByOther(pseudo,skinInt);
}

void BaseWindow::tradeAcceptedByOther(const QString &pseudo,const quint8 &skinInt)
{
    ui->stackedWidget->setCurrentWidget(ui->page_trade);
    tradeOtherStat=TradeOtherStat_InWait;
    //reset the current player info
    ui->tradePlayerCash->setMinimum(0);
    ui->tradePlayerCash->setValue(0);
    ui->tradePlayerItems->clear();
    ui->tradePlayerMonsters->clear();
    ui->tradePlayerCash->setEnabled(true);
    ui->tradePlayerItems->setEnabled(true);
    ui->tradePlayerMonsters->setEnabled(true);
    ui->tradeAddItem->setEnabled(true);
    ui->tradeAddMonster->setEnabled(true);
    ui->tradeValidate->setEnabled(true);

    QPixmap otherFrontImage=getFrontSkin(skinInt);

    //reset the other player info
    ui->tradeOtherImage->setPixmap(otherFrontImage);
    ui->tradeOtherPseudo->setText(pseudo);
    ui->tradeOtherCash->setValue(0);
    ui->tradeOtherItems->clear();
    ui->tradeOtherMonsters->clear();
    ui->tradeOtherStat->setText(tr("The other player have not validation their selection"));
}

void BaseWindow::tradeCanceledByOther()
{
    if(ui->stackedWidget->currentWidget()!=ui->page_trade)
        return;
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    showTip(tr("The other player have canceled your trade request"));
    addCash(ui->tradePlayerCash->value());
    add_to_inventory(tradeCurrentObjects,false);
    CatchChallenger::ClientFightEngine::fightEngine.addPlayerMonster(tradeCurrentMonsters);
    load_monsters();
    tradeOtherObjects.clear();
    tradeCurrentObjects.clear();
    tradeCurrentMonsters.clear();
    tradeOtherMonsters.clear();
}

void BaseWindow::tradeFinishedByOther()
{
    tradeOtherStat=TradeOtherStat_Accepted;
    if(!ui->tradeValidate->isEnabled())
        ui->stackedWidget->setCurrentWidget(ui->page_map);
    else
        ui->tradeOtherStat->setText(tr("The other player have validated the selection"));
}

void BaseWindow::tradeValidatedByTheServer()
{
    if(ui->stackedWidget->currentWidget()==ui->page_trade)
        ui->stackedWidget->setCurrentWidget(ui->page_map);
    showTip(tr("Your trade is successfull"));
    add_to_inventory(tradeOtherObjects);
    addCash(ui->tradeOtherCash->value());
    CatchChallenger::ClientFightEngine::fightEngine.addPlayerMonster(tradeOtherMonsters);
    load_monsters();
    tradeOtherObjects.clear();
    tradeCurrentObjects.clear();
    tradeCurrentMonsters.clear();
    tradeOtherMonsters.clear();
}

void BaseWindow::tradeAddTradeCash(const quint64 &cash)
{
    ui->tradeOtherCash->setValue(ui->tradeOtherCash->value()+cash);
}

void BaseWindow::tradeAddTradeObject(const quint32 &item,const quint32 &quantity)
{
    if(tradeOtherObjects.contains(item))
        tradeOtherObjects[item]+=quantity;
    else
        tradeOtherObjects[item]=quantity;
    ui->tradeOtherItems->clear();
    QHashIterator<quint32,quint32> i(tradeOtherObjects);
    while (i.hasNext()) {
        i.next();
        ui->tradeOtherItems->addItem(itemToGraphic(i.key(),i.value()));
    }
}

void BaseWindow::tradeUpdateCurrentObject()
{
    ui->tradePlayerItems->clear();
    QHashIterator<quint32,quint32> i(tradeCurrentObjects);
    while (i.hasNext()) {
        i.next();
        ui->tradePlayerItems->addItem(itemToGraphic(i.key(),i.value()));
    }
}

void BaseWindow::battleRequested(const QString &pseudo, const quint8 &skinInt)
{
    if(CatchChallenger::ClientFightEngine::fightEngine.isInFight())
    {
        qDebug() << "already in fight";
        CatchChallenger::Api_client_real::client->battleRefused();
        return;
    }
    WithAnotherPlayer withAnotherPlayerDialog(this,WithAnotherPlayer::WithAnotherPlayerType_Battle,getFrontSkin(skinInt),pseudo);
    withAnotherPlayerDialog.exec();
    if(!withAnotherPlayerDialog.actionIsAccepted())
    {
        CatchChallenger::Api_client_real::client->battleRefused();
        return;
    }
    CatchChallenger::Api_client_real::client->battleAccepted();
}

void BaseWindow::battleAcceptedByOther(const QString &pseudo,const quint8 &skinId,const QList<quint8> &stat,const quint8 &monsterPlace,const PublicPlayerMonster &publicPlayerMonster)
{
    BattleInformations battleInformations;
    battleInformations.pseudo=pseudo;
    battleInformations.skinId=skinId;
    battleInformations.stat=stat;
    battleInformations.monsterPlace=monsterPlace;
    battleInformations.publicPlayerMonster=publicPlayerMonster;
    battleInformationsList << battleInformations;
    if(battleInformationsList.size()>1 || !botFightList.isEmpty())
        return;
    battleAcceptedByOtherFull(battleInformations);
}

void BaseWindow::battleAcceptedByOtherFull(const BattleInformations &battleInformations)
{
    if(CatchChallenger::ClientFightEngine::fightEngine.isInFight())
    {
        qDebug() << "already in fight";
        CatchChallenger::Api_client_real::client->battleRefused();
        return;
    }
    battleType=BattleType_OtherPlayer;
    ui->stackedWidget->setCurrentWidget(ui->page_battle);

    //skinFolderList=CatchChallenger::FacilityLib::skinIdList(CatchChallenger::Api_client_real::client->get_datapack_base_name()+DATAPACK_BASE_PATH_SKIN);
    QPixmap otherFrontImage=getFrontSkin(battleInformations.skinId);

    //reset the other player info
    ui->labelFightMonsterTop->setPixmap(otherFrontImage);
    //ui->battleOtherPseudo->setText(lastBattleQuery.first().first);
    ui->frameFightTop->hide();
    ui->frameFightBottom->hide();
    ui->labelFightMonsterBottom->setPixmap(playerBackImage);
    ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
    ui->labelFightEnter->setText(tr("%1 wish fight with you").arg(battleInformations.pseudo));
    ui->pushButtonFightEnterNext->hide();

    resetPosition(true);
    moveType=MoveType_Enter;
    battleStep=BattleStep_Presentation;
    moveFightMonsterBoth();
    CatchChallenger::ClientFightEngine::fightEngine.setBattleMonster(battleInformations.stat,battleInformations.monsterPlace,battleInformations.publicPlayerMonster);
}

void BaseWindow::battleCanceledByOther()
{
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    showTip(tr("The other player have canceled the battle"));
    load_monsters();
}

QString BaseWindow::lastLocation() const
{
    return MapController::mapController->lastLocation();
}

QHash<quint32, PlayerQuest> BaseWindow::getQuests() const
{
    return quests;
}

quint8 BaseWindow::getActualBotId() const
{
    return actualBot.botId;
}

void BaseWindow::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
    break;
    default:
    break;
    }
}

void BaseWindow::message(QString message)
{
    qDebug() << message;
}

void BaseWindow::number_of_player(quint16 number,quint16 max)
{
    ui->frame_main_display_interface_player->show();
    ui->label_interface_number_of_player->setText(QString("%1/%2").arg(number).arg(max));
}

void BaseWindow::on_toolButton_interface_quit_clicked()
{
    CatchChallenger::Api_client_real::client->tryDisconnect();
}

void BaseWindow::on_toolButton_quit_interface_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_map);
}

void BaseWindow::on_pushButton_interface_trainer_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_player);
}

//return ok, itemId
void BaseWindow::selectObject(const ObjectType &objectType)
{
    inSelection=true;
    waitedObjectType=objectType;
    switch(objectType)
    {
        case ObjectType_Seed:
            ui->stackedWidget->setCurrentWidget(ui->page_plants);
            on_listPlantList_itemSelectionChanged();
        break;
        case ObjectType_Sell:
            ui->stackedWidget->setCurrentWidget(ui->page_shop);
            displaySellList();
        break;
        case ObjectType_MonsterToTrade:
        case ObjectType_MonsterToLearn:
            ui->selectMonster->setVisible(true);
            ui->stackedWidget->setCurrentWidget(ui->page_monster);
        break;
        case ObjectType_All:
        case ObjectType_Trade:
        default:
            ui->inventoryUse->setText(tr("Select"));
            ui->inventoryUse->setVisible(true);
            ui->stackedWidget->setCurrentWidget(ui->page_inventory);
            on_listCraftingList_itemSelectionChanged();
        break;
        case ObjectType_UseInFight:
            ui->inventoryUse->setText(tr("Select"));
            ui->inventoryUse->setVisible(true);
            ui->stackedWidget->setCurrentWidget(ui->page_inventory);
            load_inventory();
        break;
    }
}

void BaseWindow::objectSelection(const bool &ok, const quint32 &itemId, const quint32 &quantity)
{
    inSelection=false;
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    ui->inventoryUse->setText(tr("Select"));
    switch(waitedObjectType)
    {
        case ObjectType_Sell:
            if(!ok)
                break;
            if(!items.contains(itemId))
            {
                qDebug() << "item id is not into the inventory";
                break;
            }
            if(items[itemId]<quantity)
            {
                qDebug() << "item id have not the quantity";
                break;
            }
            items[itemId]-=quantity;
            if(items[itemId]==0)
                items.remove(itemId);
            ItemToSellOrBuy tempItem;
            tempItem.object=itemId;
            tempItem.quantity=quantity;
            tempItem.price=CatchChallenger::CommonDatapack::commonDatapack.items.item[itemId].price/2;
            itemsToSell << tempItem;
            CatchChallenger::Api_client_real::client->sellObject(shopId,tempItem.object,tempItem.quantity,tempItem.price);
            load_inventory();
            load_plant_inventory();
        break;
        case ObjectType_Trade:
            ui->stackedWidget->setCurrentWidget(ui->page_trade);
            if(!ok)
                break;
            if(!items.contains(itemId))
            {
                qDebug() << "item id is not into the inventory";
                break;
            }
            if(items[itemId]<quantity)
            {
                qDebug() << "item id have not the quantity";
                break;
            }
            CatchChallenger::Api_client_real::client->addObject(itemId,quantity);
            items[itemId]-=quantity;
            if(items[itemId]==0)
                items.remove(itemId);
            if(tradeCurrentObjects.contains(itemId))
                tradeCurrentObjects[itemId]+=quantity;
            else
                tradeCurrentObjects[itemId]=quantity;
            load_inventory();
            load_plant_inventory();
            tradeUpdateCurrentObject();
        break;
        case ObjectType_MonsterToLearn:
        {
            if(!ok)
            {
                ui->stackedWidget->setCurrentWidget(ui->page_map);
                return;
            }
            ui->stackedWidget->setCurrentWidget(ui->page_learn);
            monsterToLearn=itemId;
            if(!showLearnSkill(monsterToLearn))
            {
                newError(tr("Internal error"),"Unable to load the right monster");
                return;
            }
        }
        break;
        case ObjectType_MonsterToTrade:
        {
            if(waitedObjectType==ObjectType_MonsterToLearn)
            {
                ui->stackedWidget->setCurrentWidget(ui->page_learn);
                monsterToLearn=itemId;
                return;
            }
            ui->stackedWidget->setCurrentWidget(ui->page_trade);
            if(!ok)
                break;
            QList<PlayerMonster> playerMonster=ClientFightEngine::fightEngine.getPlayerMonster();
            if(playerMonster.size()<=1)
            {
                QMessageBox::warning(this,tr("Warning"),tr("You can't trade your last monster"));
                break;
            }
            if(!ClientFightEngine::fightEngine.remainMonstersToFight(itemId))
            {
                QMessageBox::warning(this,tr("Warning"),tr("You don't have more monster valid"));
                break;
            }
            //get the right monster
            int index=0;
            while(index<playerMonster.size())
            {
                if(playerMonster.at(index).id==itemId)
                {
                    tradeCurrentMonsters << playerMonster.at(index);
                    ClientFightEngine::fightEngine.removeMonster(itemId);
                    CatchChallenger::Api_client_real::client->addMonster(itemId);
                    QListWidgetItem *item=new QListWidgetItem();
                    item->setText(DatapackClientLoader::datapackLoader.monsterExtra[tradeCurrentMonsters.last().monster].name);
                    item->setToolTip(tr("Level: %1").arg(tradeCurrentMonsters.last().level));
                    item->setIcon(DatapackClientLoader::datapackLoader.monsterExtra[tradeCurrentMonsters.last().monster].front);
                    ui->tradePlayerMonsters->addItem(item);
                    break;
                }
                index++;
            }
            load_monsters();
        }
        break;
        case ObjectType_Seed:
            ui->plantUse->setVisible(false);
            if(!ok)
                break;
            if(!items.contains(itemId))
            {
                qDebug() << "item id is not into the inventory";
                break;
            }
            items[itemId]--;
            if(items[itemId]==0)
                items.remove(itemId);
            seed_in_waiting=itemId;
            addQuery(QueryType_Seed);
            seedWait=true;
            if(DatapackClientLoader::datapackLoader.itemToPlants.contains(itemId))
            {
                load_plant_inventory();
                load_inventory();
                qDebug() << QString("send seed for: %1").arg(DatapackClientLoader::datapackLoader.itemToPlants[itemId]);
                emit useSeed(DatapackClientLoader::datapackLoader.itemToPlants[itemId]);
            }
            else
                qDebug() << QString("seed not found for item: %1").arg(itemId);
        break;
        case ObjectType_UseInFight:
        {
            ui->stackedWidget->setCurrentWidget(ui->page_battle);
            if(!ok)
                break;
            if(!items.contains(itemId))
            {
                qDebug() << "item id is not into the inventory";
                break;
            }
            if(items[itemId]<quantity)
            {
                qDebug() << "item id have not the quantity";
                break;
            }
            items[itemId]-=quantity;
            if(items[itemId]==0)
                items.remove(itemId);
            useTrap(itemId);
        }
        break;
        default:
        qDebug() << "waitedObjectType is unknow";
        return;
    }
    waitedObjectType=ObjectType_All;
}

void BaseWindow::add_to_inventory_slot(const QHash<quint32,quint32> &items)
{
    add_to_inventory(items);
}

void BaseWindow::add_to_inventory(const QHash<quint32,quint32> &items,const bool &showGain)
{
    if(items.empty())
        return;
    if(showGain)
    {
        QString html=tr("You have obtained: ");
        QStringList objects;
        QHashIterator<quint32,quint32> i(items);
        while (i.hasNext()) {
            i.next();

            //add really to the list
            if(this->items.contains(i.key()))
                this->items[i.key()]+=i.value();
            else
                this->items[i.key()]=i.value();

            QPixmap image;
            QString name;
            if(DatapackClientLoader::datapackLoader.itemsExtra.contains(i.key()))
            {
                image=DatapackClientLoader::datapackLoader.itemsExtra[i.key()].image;
                name=DatapackClientLoader::datapackLoader.itemsExtra[i.key()].name;
            }
            else
            {
                image=DatapackClientLoader::datapackLoader.defaultInventoryImage();
                name=QString("id: %1").arg(i.key());
            }

            image=image.scaled(24,24);
            QByteArray byteArray;
            QBuffer buffer(&byteArray);
            image.save(&buffer, "PNG");
            if(objects.size()<16)
            {
                if(i.value()>1)
                    objects << QString("<b>%2x</b> %3 <img src=\"data:image/png;base64,%1\" />").arg(QString(byteArray.toBase64())).arg(i.value()).arg(name);
                else
                    objects << QString("%2 <img src=\"data:image/png;base64,%1\" />").arg(QString(byteArray.toBase64())).arg(name);
            }
        }
        if(objects.size()==16)
            objects << "...";
        html+=objects.join(", ");
        BaseWindow::showGain(html);
    }

    load_inventory();
    load_plant_inventory();
    on_listCraftingList_itemSelectionChanged();
}

void BaseWindow::remove_to_inventory(const QHash<quint32,quint32> &items)
{
    QHashIterator<quint32,quint32> i(items);
    while (i.hasNext()) {
        i.next();

        //add really to the list
        if(this->items.contains(i.key()))
        {
            if(this->items[i.key()]<=i.value())
                this->items.remove(i.key());
            else
                this->items[i.key()]-=i.value();
        }
    }
    load_inventory();
    load_plant_inventory();
}

void BaseWindow::newError(QString error,QString detailedError)
{
    qDebug() << detailedError.toLocal8Bit();
    if(socketState!=QAbstractSocket::ConnectedState)
        return;
    CatchChallenger::Api_client_real::client->tryDisconnect();
    resetAll();
    QMessageBox::critical(this,tr("Error"),error);
}

void BaseWindow::error(QString error)
{
    newError(tr("Error with the protocol"),error);
}

void BaseWindow::errorWithTheCurrentMap()
{
    if(socketState!=QAbstractSocket::ConnectedState)
        return;
    CatchChallenger::Api_client_real::client->tryDisconnect();
    resetAll();
    QMessageBox::critical(this,tr("Map error"),tr("The current map into the datapack is in error (not found, read failed, wrong format, corrupted, ...)\nReport the bug to the datapack maintainer."));
}

void BaseWindow::on_pushButton_interface_bag_clicked()
{
    if(inSelection)
    {
        qDebug() << "BaseWindow::on_pushButton_interface_bag_clicked() in selection, can't click here";
        return;
    }
    ui->stackedWidget->setCurrentWidget(ui->page_inventory);
}

void BaseWindow::on_toolButton_quit_inventory_clicked()
{
    ui->inventory->reset();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    if(inSelection)
        objectSelection(false);
    on_inventory_itemSelectionChanged();
}

void BaseWindow::on_inventory_itemSelectionChanged()
{
    QList<QListWidgetItem *> items=ui->inventory->selectedItems();
    if(items.size()!=1)
    {
        ui->inventory_image->setPixmap(DatapackClientLoader::datapackLoader.defaultInventoryImage());
        ui->inventory_name->setText("");
        ui->inventory_description->setText(tr("Select an object"));
        ui->inventoryInformation->setVisible(false);
        ui->inventoryUse->setVisible(false);
        ui->inventoryDestroy->setVisible(false);
        return;
    }
    QListWidgetItem *item=items.first();
    if(!DatapackClientLoader::datapackLoader.itemsExtra.contains(items_graphical[item]))
    {
        ui->inventoryInformation->setVisible(false);
        ui->inventoryUse->setVisible(false);
        ui->inventoryDestroy->setVisible(!inSelection);
        ui->inventory_image->setPixmap(DatapackClientLoader::datapackLoader.defaultInventoryImage());
        ui->inventory_name->setText(tr("Unknow name"));
        ui->inventory_description->setText(tr("Unknow description"));
        return;
    }

    const DatapackClientLoader::ItemExtra &content=DatapackClientLoader::datapackLoader.itemsExtra[items_graphical[item]];
    ui->inventoryInformation->setVisible(!inSelection &&
                                         /* is a plant */
                                         DatapackClientLoader::datapackLoader.itemToPlants.contains(items_graphical[item])
                                         );
    ui->inventoryUse->setVisible(inSelection ||
                                         /* is a recipe */
                                         CatchChallenger::CommonDatapack::commonDatapack.itemToCrafingRecipes.contains(items_graphical[item])
                                         );
    ui->inventoryDestroy->setVisible(!inSelection);
    ui->inventory_image->setPixmap(content.image);
    ui->inventory_name->setText(content.name);
    ui->inventory_description->setText(content.description);
}

void BaseWindow::tipTimeout()
{
    ui->tip->setVisible(false);
}

void BaseWindow::gainTimeout()
{
    ui->gain->setVisible(false);
}

void BaseWindow::showTip(const QString &tip)
{
    ui->tip->setVisible(true);
    ui->tip->setText(tip);
    tip_timeout.start();
}

void BaseWindow::showGain(const QString &gain)
{
    ui->gain->setVisible(true);
    ui->gain->setText(gain);
    gain_timeout.start();
}

void BaseWindow::addQuery(const QueryType &queryType)
{
    if(queryList.size()==0)
        ui->persistant_tip->setVisible(true);
    queryList << queryType;
    updateQueryList();
}

void BaseWindow::removeQuery(const QueryType &queryType)
{
    queryList.removeOne(queryType);
    if(queryList.size()==0)
        ui->persistant_tip->setVisible(false);
    else
        updateQueryList();
}

void BaseWindow::updateQueryList()
{
    QStringList queryStringList;
    int index=0;
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
                queryStringList << tr("Unknow action...");
            break;
        }
        index++;
    }
    ui->persistant_tip->setText(queryStringList.join("<br />"));
}

void BaseWindow::on_toolButton_quit_options_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_map);
}

void BaseWindow::stopped_in_front_of(CatchChallenger::Map_client *map, quint8 x, quint8 y)
{
    if(stopped_in_front_of_check_bot(map,x,y))
        return;
    else if(CatchChallenger::MoveOnTheMap::isDirt(*map,x,y))
    {
        int index=0;
        while(index<map->plantList.size())
        {
            if(map->plantList.at(index).x==x && map->plantList.at(index).y==y)
            {
                quint64 current_time=QDateTime::currentMSecsSinceEpoch()/1000;
                if(map->plantList.at(index).mature_at<current_time)
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
            CatchChallenger::Map * current_map=map;
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

bool BaseWindow::stopped_in_front_of_check_bot(CatchChallenger::Map_client *map, quint8 x, quint8 y)
{
    if(!map->bots.contains(QPair<quint8,quint8>(x,y)))
        return false;
    showTip(tr("To interact with the bot press <i><b>Enter</b></i>"));
    return true;
}

void BaseWindow::actionOn(Map_client *map, quint8 x, quint8 y)
{
    if(actionOnCheckBot(map,x,y))
        return;
    else if(CatchChallenger::MoveOnTheMap::isDirt(*map,x,y))
    {
        int index=0;
        while(index<map->plantList.size())
        {
            if(map->plantList.at(index).x==x && map->plantList.at(index).y==y)
            {
                if(collectWait)
                {
                    showTip(tr("Wait to finish to collect the previous plant"));
                    return;
                }
                quint64 current_time=QDateTime::currentMSecsSinceEpoch()/1000;
                if(map->plantList.at(index).mature_at<current_time)
                {
                    collectWait=true;
                    addQuery(QueryType_CollectPlant);
                    emit collectMaturePlant();
                }
                else
                    showTip(tr("This plant is growing and can't be collected"));
                return;
            }
            index++;
        }
        if(seedWait)
        {
            showTip(tr("Wait to finish to plant the previous seed"));
            return;
        }
        selectObject(ObjectType_Seed);
        return;
    }
    else
    {
        //check bot with border
        CatchChallenger::Map * current_map=map;
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

bool BaseWindow::actionOnCheckBot(CatchChallenger::Map_client *map, quint8 x, quint8 y)
{
    if(!map->bots.contains(QPair<quint8,quint8>(x,y)))
        return false;
    actualBot=map->bots[QPair<quint8,quint8>(x,y)];
    isInQuest=false;
    goToBotStep(1);
    return true;
}

void BaseWindow::botFightCollision(CatchChallenger::Map_client *map, quint8 x, quint8 y)
{
    if(!map->bots.contains(QPair<quint8,quint8>(x,y)))
    {
        newError(tr("Internal error"),"Bot trigged but no bot at this place");
        return;
    }
    quint8 step=1;
    actualBot=map->bots[QPair<quint8,quint8>(x,y)];
    isInQuest=false;
    if(!actualBot.step.contains(step))
    {
        newError(tr("Internal error"),"Bot trigged but no step found");
        return;
    }
    if(actualBot.step[step].attribute("type")=="fight")
    {
        if(!actualBot.step[step].hasAttribute("fightid"))
        {
            showTip(tr("Bot step missing data error, repport this error please"));
            return;
        }
        bool ok;
        quint32 fightId=actualBot.step[step].attribute("fightid").toUInt(&ok);
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
        case MapVisualiserPlayer::BlockedOn_Grass:
        case MapVisualiserPlayer::BlockedOn_Wather:
        case MapVisualiserPlayer::BlockedOn_Cave:
        case MapVisualiserPlayer::BlockedOn_Fight:
            qDebug() << "You can't enter to the fight zone if you are not able to fight";
            showTip(tr("You can't enter to the fight zone if you are not able to fight"));
        break;
        case MapVisualiserPlayer::BlockedOn_RandomNumber:
            qDebug() << "You can't enter to the fight zone, because have not random number";
            showTip(tr("You can't enter to the fight zone, because have not random number"));
        break;
    }
}

void BaseWindow::currentMapLoaded()
{
    qDebug() << "BaseWindow::currentMapLoaded(): map: " << MapController::mapController->currentMap() << " with type: " << MapController::mapController->currentMapType();
    QString type=MapController::mapController->currentMapType();
    if(!DatapackClientLoader::datapackLoader.audioAmbiance.contains(type))
    {
        while(!ambiance.isEmpty())
        {
            ambiance.first()->stop();
            delete ambiance.first();
            ambiance.removeFirst();
        }
        return;
    }
    QString file=DatapackClientLoader::datapackLoader.audioAmbiance[type];
    while(!ambiance.isEmpty())
    {
        if(ambiance.first()->getFilePath()==file)
            return;
        ambiance.first()->stop();
        delete ambiance.first();
        ambiance.removeFirst();
    }
    ambiance << new QOggSimplePlayer(file,&audioReadThread);
    ambiance.last()->start();
}

//network
void BaseWindow::updateRXTX()
{
    quint64 RXSize=CatchChallenger::Api_client_real::client->getRXSize();
    quint64 TXSize=CatchChallenger::Api_client_real::client->getTXSize();
    if(previousRXSize>RXSize)
        previousRXSize=RXSize;
    if(previousTXSize>TXSize)
        previousTXSize=TXSize;
    double RXSpeed=(RXSize-previousRXSize)*1000/updateRXTXTime.elapsed();
    double TXSpeed=(TXSize-previousTXSize)*1000/updateRXTXTime.elapsed();
    if(RXSpeed<1024)
        ui->labelInput->setText(QString("%1B/s").arg(RXSpeed,0,'g',0));
    else if(RXSpeed<10240)
        ui->labelInput->setText(QString("%1KB/s").arg(RXSpeed/1024,0,'g',1));
    else
        ui->labelInput->setText(QString("%1KB/s").arg(RXSpeed/1024,0,'g',0));
    if(TXSpeed<1024)
        ui->labelOutput->setText(QString("%1B/s").arg(TXSpeed,0,'g',0));
    else if(TXSpeed<10240)
        ui->labelOutput->setText(QString("%1KB/s").arg(TXSpeed/1024,0,'g',1));
    else
        ui->labelOutput->setText(QString("%1KB/s").arg(TXSpeed/1024,0,'g',0));
    #ifdef DEBUG_CLIENT_NETWORK_USAGE
    if(RXSpeed>0 && TXSpeed>0)
        qDebug() << QString("received: %1B/s, transmited: %2B/s").arg(RXSpeed).arg(TXSpeed);
    else
    {
        if(RXSpeed>0)
            qDebug() << QString("received: %1B/s").arg(RXSpeed);
        if(TXSpeed>0)
            qDebug() << QString("transmited: %1B/s").arg(TXSpeed);
    }
    #endif
    updateRXTXTime.restart();
    previousRXSize=RXSize;
    previousTXSize=TXSize;
}

bool BaseWindow::haveNextStepQuestRequirements(const CatchChallenger::Quest &quest) const
{
    #ifdef DEBUG_CLIENT_QUEST
    qDebug() << QString("haveNextStepQuestRequirements for quest: %1").arg(questId);
    #endif
    if(!quests.contains(quest.id))
    {
        qDebug() << "step out of range for: " << quest.id;
        return false;
    }
    quint8 step=quests[quest.id].step;
    if(step<=0 || step>quest.steps.size())
    {
        qDebug() << "step out of range for: " << quest.id;
        return false;
    }
    #ifdef DEBUG_CLIENT_QUEST
    qDebug() << QString("haveNextStepQuestRequirements for quest: %1, step: %2").arg(questId).arg(step);
    #endif
    const CatchChallenger::Quest::StepRequirements &requirements=quest.steps.at(step-1).requirements;
    int index=0;
    while(index<requirements.items.size())
    {
        const CatchChallenger::Quest::Item &item=requirements.items.at(index);
        if(itemQuantity(item.item)<item.quantity)
        {
            #ifdef DEBUG_CLIENT_QUEST
            qDebug() << "quest requirement, have not the quantity for the item: " << item.item;
            #endif
            return false;
        }
        index++;
    }
    index=0;
    while(index<requirements.fightId.size())
    {
        const quint32 &fightId=requirements.fightId.at(index);
        if(!MapController::mapController->haveBeatBot(fightId))
        {
            #ifdef DEBUG_CLIENT_QUEST
            qDebug() << "quest requirement, have not beat the bot: " << fightId;
            #endif
            return false;
        }
        index++;
    }
    return true;
}

bool BaseWindow::haveStartQuestRequirement(const CatchChallenger::Quest &quest) const
{
    #ifdef DEBUG_CLIENT_QUEST
    qDebug() << "check quest requirement for: " << quest.id;
    #endif
    Player_private_and_public_informations informations=CatchChallenger::Api_client_real::client->get_player_informations();
    if(quests.contains(quest.id))
    {
        if(informations.quests[quest.id].step!=0)
        {
            #ifdef DEBUG_CLIENT_QUEST
            qDebug() << "can start the quest because is already running: " << questId;
            #endif
            return false;
        }
        if(informations.quests[quest.id].finish_one_time && !quest.repeatable)
        {
            #ifdef DEBUG_CLIENT_QUEST
            qDebug() << "done one time and no repeatable: " << questId;
            #endif
            return false;
        }
    }
    int index=0;
    while(index<quest.requirements.quests.size())
    {
        const quint32 &questId=quest.requirements.quests.at(index);
        if(!quests.contains(questId))
        {
            #ifdef DEBUG_CLIENT_QUEST
            qDebug() << "have never started the quest: " << questId;
            #endif
            return false;
        }
        if(!informations.quests[questId].finish_one_time)
        {
            #ifdef DEBUG_CLIENT_QUEST
            qDebug() << "quest never finished: " << questId;
            #endif
            return false;
        }
        index++;
    }
    index=0;
    while(index<quest.requirements.reputation.size())
    {
        const CatchChallenger::Quest::ReputationRequirements &reputation=quest.requirements.reputation.at(index);
        if(informations.reputation.contains(reputation.type))
        {
            const PlayerReputation &playerReputation=informations.reputation[reputation.type];
            if(!reputation.positif)
            {
                if(-reputation.level<playerReputation.level)
                {
                    #ifdef DEBUG_CLIENT_QUEST
                    qDebug() << "reputation.level(" << reputation.level << ")<playerReputation.level(" << playerReputation.level << ")";
                    #endif
                    return false;
                }
            }
            else
            {
                if(reputation.level>playerReputation.level || playerReputation.point<0)
                {
                    #ifdef DEBUG_CLIENT_QUEST
                    qDebug() << "reputation.level(" << reputation.level << ")>playerReputation.level(" << playerReputation.level << ") || playerReputation.point(" << playerReputation.point << ")<0";
                    #endif
                    return false;
                }
            }
        }
        else
            if(!reputation.positif)//default level is 0, but required level is negative
            {
                #ifdef DEBUG_CLIENT_QUEST
                qDebug() << "reputation.level(" << reputation.level << ")<0 and no reputation.type=" << reputation.type;
                #endif
                return false;
            }
        index++;
    }
    return true;
}

bool BaseWindow::nextStepQuest(const Quest &quest)
{
    #ifdef DEBUG_CLIENT_QUEST
    qDebug() << "drop quest step requirement for: " << quest.id;
    #endif
    if(!quests.contains(quest.id))
    {
        qDebug() << "step out of range for: " << quest.id;
        return false;
    }
    quint8 step=quests[quest.id].step;
    if(step<=0 || step>quest.steps.size())
    {
        qDebug() << "step out of range for: " << quest.id;
        return false;
    }
    const CatchChallenger::Quest::StepRequirements &requirements=quest.steps.at(step-1).requirements;
    int index=0;
    while(index<requirements.items.size())
    {
        const CatchChallenger::Quest::Item &item=requirements.items.at(index);
        QHash<quint32,quint32> items;
        items[item.item]=item.quantity;
        remove_to_inventory(items);
        index++;
    }
    quests[quest.id].step++;
    if(quests[quest.id].step>quest.steps.size())
    {
        #ifdef DEBUG_CLIENT_QUEST
        qDebug() << "finish the quest: " << quest.id;
        #endif
        quests[quest.id].step=0;
        quests[quest.id].finish_one_time=true;
        index=0;
        while(index<quest.rewards.reputation.size())
        {
            appendReputationPoint(quest.rewards.reputation[index].type,quest.rewards.reputation[index].point);
            index++;
        }
        index=0;
        while(index<quest.rewards.allow.size())
        {
            allow << quest.rewards.allow[index];
            index++;
        }
    }
    return true;
}

//reputation
void BaseWindow::appendReputationPoint(const QString &type,const qint32 &point)
{
    if(point==0)
        return;
    if(!CatchChallenger::CommonDatapack::commonDatapack.reputation.contains(type))
    {
        emit error(QString("Unknow reputation: %1").arg(type));
        return;
    }
    PlayerReputation playerReputation;
    playerReputation.point=0;
    playerReputation.level=0;
    if(CatchChallenger::Api_client_real::client->player_informations.reputation.contains(type))
        playerReputation=CatchChallenger::Api_client_real::client->player_informations.reputation[type];
    #ifdef DEBUG_MESSAGE_CLIENT_REPUTATION
    emit message(QString("Reputation %1 at level: %2 with point: %3").arg(type).arg(playerReputation.level).arg(playerReputation.point));
    #endif
    playerReputation.point+=point;
    do
    {
        if(playerReputation.level<0 && playerReputation.point>0)
        {
            playerReputation.level++;
            playerReputation.point+=CatchChallenger::CommonDatapack::commonDatapack.reputation[type].reputation_negative.at(-playerReputation.level);
            continue;
        }
        if(playerReputation.level>0 && playerReputation.point<0)
        {
            playerReputation.level--;
            playerReputation.point+=CatchChallenger::CommonDatapack::commonDatapack.reputation[type].reputation_negative.at(playerReputation.level);
            continue;
        }
        if(playerReputation.level<=0 && playerReputation.point<CatchChallenger::CommonDatapack::commonDatapack.reputation[type].reputation_negative.at(-playerReputation.level))
        {
            if((-playerReputation.level)<CatchChallenger::CommonDatapack::commonDatapack.reputation[type].reputation_negative.size())
            {
                playerReputation.point-=CatchChallenger::CommonDatapack::commonDatapack.reputation[type].reputation_negative.at(-playerReputation.level);
                playerReputation.level--;
            }
            else
            {
                #ifdef DEBUG_MESSAGE_CLIENT_REPUTATION
                emit message(QString("Reputation %1 at level max: %2").arg(type).arg(playerReputation.level));
                #endif
                playerReputation.point=CatchChallenger::CommonDatapack::commonDatapack.reputation[type].reputation_negative.at(-playerReputation.level);
            }
            continue;
        }
        if(playerReputation.level>=0 && playerReputation.point<CatchChallenger::CommonDatapack::commonDatapack.reputation[type].reputation_positive.at(playerReputation.level))
        {
            if(playerReputation.level<CatchChallenger::CommonDatapack::commonDatapack.reputation[type].reputation_negative.size())
            {
                playerReputation.point-=CatchChallenger::CommonDatapack::commonDatapack.reputation[type].reputation_negative.at(playerReputation.level);
                playerReputation.level++;
            }
            else
            {
                #ifdef DEBUG_MESSAGE_CLIENT_REPUTATION
                emit message(QString("Reputation %1 at level max: %2").arg(type).arg(playerReputation.level));
                #endif
                playerReputation.point=CatchChallenger::CommonDatapack::commonDatapack.reputation[type].reputation_negative.at(playerReputation.level);
            }
            continue;
        }
    } while(false);
    CatchChallenger::Api_client_real::client->player_informations.reputation[type]=playerReputation;
    #ifdef DEBUG_MESSAGE_CLIENT_REPUTATION
    emit message(QString("New reputation %1 at level: %2 with point: %3").arg(type).arg(playerReputation.level).arg(playerReputation.point));
    #endif
}

bool BaseWindow::startQuest(const Quest &quest)
{
    if(!quests.contains(quest.id))
    {
        quests[quest.id].step=1;
        quests[quest.id].finish_one_time=false;
    }
    else
        quests[quest.id].step=1;
    return true;
}

bool BaseWindow::botHaveQuest(const quint32 &botId)
{
    #ifdef DEBUG_CLIENT_QUEST
    qDebug() << "check bot quest for: " << botId;
    #endif
    //do the not started quest here
    QList<quint32> botQuests=DatapackClientLoader::datapackLoader.botToQuestStart.values(botId);
    int index=0;
    while(index<botQuests.size())
    {
        const quint8 &questId=botQuests.at(index);
        const CatchChallenger::Quest &currentQuest=CatchChallenger::CommonDatapack::commonDatapack.quests[questId];
        if(!quests.contains(botQuests.at(index)))
        {
            //quest not started
            if(haveStartQuestRequirement(currentQuest))
                return true;
            else
                {}//have not the requirement
        }
        else
        {
            if(!CatchChallenger::CommonDatapack::commonDatapack.quests.contains(botQuests.at(index)))
                qDebug() << "internal bug: have quest registred, but no quest found with this id";
            else
            {
                if(quests[botQuests.at(index)].step==0)
                {
                    if(currentQuest.repeatable)
                    {
                        if(quests[botQuests.at(index)].finish_one_time)
                        {
                            //quest already done but repeatable
                            if(haveStartQuestRequirement(currentQuest))
                                return true;
                            else
                                {}//have not the requirement
                        }
                        else
                            {}//bug: can't be !finish_one_time && currentQuest.steps==0
                    }
                    else
                        {}//quest already done
                }
                else
                {
                    QList<quint32> bots=currentQuest.steps.at(quests[questId].step-1).bots;
                    if(bots.contains(botId))
                        return true;//in progress
                    else
                        {}//Need got to another bot to progress, this it's just the starting bot
                }
            }
        }
        index++;
    }
    //do the started quest here
    QHashIterator<quint32, PlayerQuest> i(quests);
    while (i.hasNext()) {
        i.next();
        if(!botQuests.contains(i.key()) && i.value().step>0)
        {
            CatchChallenger::Quest currentQuest=CatchChallenger::CommonDatapack::commonDatapack.quests[i.key()];
            QList<quint32> bots=currentQuest.steps.at(i.value().step-1).bots;
            if(bots.contains(botId))
                return true;//in progress, but not the starting bot
            else
                {}//it's another bot
        }
    }
    return false;
}

QList<QPair<quint32,QString> > BaseWindow::getQuestList(const quint32 &botId)
{
    QList<QPair<quint32,QString> > entryList;
    QPair<quint32,QString> oneEntry;
    //do the not started quest here
    QList<quint32> botQuests=DatapackClientLoader::datapackLoader.botToQuestStart.values(botId);
    int index=0;
    while(index<botQuests.size())
    {
        const quint8 &questId=botQuests.at(index);
        const CatchChallenger::Quest &currentQuest=CatchChallenger::CommonDatapack::commonDatapack.quests[questId];
        if(!quests.contains(botQuests.at(index)))
        {
            //quest not started
            if(haveStartQuestRequirement(currentQuest))
            {
                oneEntry.first=questId;
                if(DatapackClientLoader::datapackLoader.questsExtra.contains(questId))
                    oneEntry.second=DatapackClientLoader::datapackLoader.questsExtra[questId].name;
                else
                {
                    qDebug() << "internal bug: quest extra not found";
                    oneEntry.second="???";
                }
                entryList << oneEntry;
            }
            else
                {}//have not the requirement
        }
        else
        {
            if(!CatchChallenger::CommonDatapack::commonDatapack.quests.contains(botQuests.at(index)))
                qDebug() << "internal bug: have quest registred, but no quest found with this id";
            else
            {
                if(quests[botQuests.at(index)].step==0)
                {
                    if(currentQuest.repeatable)
                    {
                        if(quests[botQuests.at(index)].finish_one_time)
                        {
                            //quest already done but repeatable
                            if(haveStartQuestRequirement(currentQuest))
                            {
                                oneEntry.first=questId;
                                if(DatapackClientLoader::datapackLoader.questsExtra.contains(questId))
                                    oneEntry.second=DatapackClientLoader::datapackLoader.questsExtra[questId].name;
                                else
                                {
                                    qDebug() << "internal bug: quest extra not found";
                                    oneEntry.second="???";
                                }
                                entryList << oneEntry;
                            }
                            else
                                {}//have not the requirement
                        }
                        else
                            {}//bug: can't be !finish_one_time && currentQuest.steps==0
                    }
                    else
                        {}//quest already done
                }
                else
                {
                    QList<quint32> bots=currentQuest.steps.at(quests[questId].step-1).bots;
                    if(bots.contains(botId))
                    {
                        oneEntry.first=questId;
                        if(DatapackClientLoader::datapackLoader.questsExtra.contains(questId))
                            oneEntry.second=tr("%1 (in progress)").arg(DatapackClientLoader::datapackLoader.questsExtra[questId].name);
                        else
                        {
                            qDebug() << "internal bug: quest extra not found";
                            oneEntry.second=tr("??? (in progress)");
                        }
                        entryList << oneEntry;
                    }
                    else
                        {}//Need got to another bot to progress, this it's just the starting bot
                }
            }
        }
        index++;
    }
    //do the started quest here
    QHashIterator<quint32, PlayerQuest> i(quests);
    while (i.hasNext()) {
        i.next();
        if(!botQuests.contains(i.key()) && i.value().step>0)
        {
            CatchChallenger::Quest currentQuest=CatchChallenger::CommonDatapack::commonDatapack.quests[i.key()];
            QList<quint32> bots=currentQuest.steps.at(i.value().step-1).bots;
            if(bots.contains(botId))
            {
                //in progress, but not the starting bot
                oneEntry.first=i.key();
                oneEntry.second=tr("%1 (in progress)").arg(DatapackClientLoader::datapackLoader.questsExtra[i.key()].name);
                entryList << oneEntry;
            }
            else
                {}//it's another bot
        }
    }
    return entryList;
}

//bot
void BaseWindow::goToBotStep(const quint8 &step)
{
    isInQuest=false;
    if(!actualBot.step.contains(step))
    {
        showTip(tr("Error into the bot, repport this error please"));
        return;
    }
    if(actualBot.step[step].attribute("type")=="text")
    {
        QDomElement text = actualBot.step[step].firstChildElement("text");
        while(!text.isNull())
        {
            if(text.hasAttribute("lang") && text.attribute("lang")!="en")
            {
                //not enlish, skip for now
            }
            else
            {
                QString textToShow=text.text();
                textToShow=parseHtmlToDisplay(textToShow);
                ui->IG_dialog_text->setText(textToShow);
                ui->IG_dialog->setVisible(true);
                return;
            }
            text = actualBot.step[step].nextSiblingElement("text");
        }
        showTip(tr("Bot text not found, repport this error please"));
        return;
    }
    else if(actualBot.step[step].attribute("type")=="shop")
    {
        if(CatchChallenger::Api_client_real::client->getHaveShopAction())
        {
            showTip(tr("Already in shop action"));
            return;
        }
        if(!actualBot.step[step].hasAttribute("shop"))
        {
            showTip(tr("The shop call, but missing informations"));
            return;
        }
        bool ok;
        shopId=actualBot.step[step].attribute("shop").toUInt(&ok);
        if(!ok)
        {
            showTip(tr("The shop call, but wrong shop id"));
            return;
        }
        QPixmap pixmap;
        if(actualBot.properties.contains("skin"))
            pixmap=getFrontSkin(actualBot.properties["skin"]);
        else
            pixmap=QPixmap(":/images/player_default/front.png");
        pixmap=pixmap.scaled(160,160);
        ui->shopSellerImage->setPixmap(pixmap);
        ui->stackedWidget->setCurrentWidget(ui->page_shop);
        ui->shopItemList->clear();
        on_shopItemList_itemSelectionChanged();
        ui->shopDescription->setText(tr("Waiting the shop content"));
        ui->shopBuy->setVisible(false);
        qDebug() << "goToBotStep(), client->getShopList(shopId): " << shopId;
        CatchChallenger::Api_client_real::client->getShopList(shopId);
        ui->shopCash->setText(tr("Cash: %1").arg(cash));
        return;
    }
    else if(actualBot.step[step].attribute("type")=="sell")
    {
        if(!actualBot.step[step].hasAttribute("shop"))
        {
            showTip(tr("The shop call, but missing informations"));
            return;
        }
        bool ok;
        shopId=actualBot.step[step].attribute("shop").toUInt(&ok);
        if(!ok)
        {
            showTip(tr("The shop call, but wrong shop id"));
            return;
        }
        QPixmap pixmap;
        if(actualBot.properties.contains("skin"))
            pixmap=getFrontSkin(actualBot.properties["skin"]);
        else
            pixmap=QPixmap(":/images/player_default/front.png");
        pixmap=pixmap.scaled(160,160);
        ui->shopSellerImage->setPixmap(pixmap);
        if(CatchChallenger::Api_client_real::client->getHaveShopAction())
        {
            showTip(tr("Already in shop action"));
            return;
        }
        waitToSell=true;
        selectObject(ObjectType_Sell);
        return;
    }
    else if(actualBot.step[step].attribute("type")=="learn")
    {
        selectObject(ObjectType_MonsterToLearn);
        return;
    }
    else if(actualBot.step[step].attribute("type")=="heal")
    {
        ClientFightEngine::fightEngine.healAllMonsters();
        CatchChallenger::Api_client_real::client->heal();
        load_monsters();
        showTip(tr("You are healed"));
        return;
    }
    else if(actualBot.step[step].attribute("type")=="quests")
    {
        QString textToShow;
        textToShow+="<ul>";
        QList<QPair<quint32,QString> > quests=BaseWindow::getQuestList(actualBot.botId);
        int index=0;
        while(index<quests.size())
        {
            QPair<quint32,QString> quest=quests.at(index);
            textToShow+=QString("<a href=\"quest_%1\">%2</a>").arg(quest.first).arg(quest.second);
            index++;
        }
        textToShow+="</ul>";
        ui->IG_dialog_text->setText(textToShow);
        ui->IG_dialog->setVisible(true);
        return;
    }
    else if(actualBot.step[step].attribute("type")=="clan")
    {
        QString textToShow;
        if(clan==0)
        {
            if(allow.contains(ActionAllow_Clan))
                textToShow=QString("<center><a href=\"clan_create\">%1</a></center>").arg(tr("Clan create"));
            else
                textToShow=QString("<center>You can't create your clan</center>");
        }
        else
            textToShow=QString("<center>%1</center>").arg(tr("You are already into a clan. Use the clan dongle into the player information."));
        ui->IG_dialog_text->setText(textToShow);
        ui->IG_dialog->setVisible(true);
        return;
    }
    else if(actualBot.step[step].attribute("type")=="warehouse")
    {
        monster_to_withdraw.clear();
        monster_to_deposit.clear();
        change_warehouse_items.clear();
        temp_warehouse_cash=0;
        QPixmap pixmap;
        if(actualBot.properties.contains("skin"))
            pixmap=getFrontSkin(actualBot.properties["skin"]);
        else
            pixmap=QPixmap(":/images/player_default/front.png");
        ui->warehouseBotImage->setPixmap(pixmap);
        ui->stackedWidget->setCurrentWidget(ui->page_warehouse);
        updateTheWareHouseContent();
        return;
    }
    else if(actualBot.step[step].attribute("type")=="industry")
    {
        if(CatchChallenger::Api_client_real::client->getHaveFactoryAction())
        {
            showTip(tr("Already in shop action"));
            return;
        }
        if(!actualBot.step[step].hasAttribute("factory_id"))
        {
            showTip(tr("The shop call, but missing informations"));
            return;
        }

        bool ok;
        factoryId=actualBot.step[step].attribute("factory_id").toUInt(&ok);
        if(!ok)
        {
            showTip(tr("The shop call, but wrong shop id"));
            return;
        }
        if(!CommonDatapack::commonDatapack.industriesLink.contains(factoryId))
        {
            showTip(tr("The factory is not found"));
            return;
        }
        ui->factoryResources->clear();
        ui->factoryProducts->clear();
        ui->factoryStatus->setText(tr("Waiting of status"));
        QPixmap pixmap;
        if(actualBot.properties.contains("skin"))
            pixmap=getFrontSkin(actualBot.properties["skin"]);
        else
            pixmap=QPixmap(":/images/player_default/front.png");
        ui->factoryBotImage->setPixmap(pixmap);
        ui->stackedWidget->setCurrentWidget(ui->page_factory);
        CatchChallenger::Api_client_real::client->getFactoryList(factoryId);
        return;
    }
    else if(actualBot.step[step].attribute("type")=="zonecapture")
    {
        if(!actualBot.step[step].hasAttribute("zone"))
        {
            showTip(tr("Missing attribute for the step"));
            return;
        }
        if(clan==0)
        {
            showTip(tr("You can't try capture if you are not in a clan"));
            return;
        }
        QString zone=actualBot.step[step].attribute("zone");
        if(DatapackClientLoader::datapackLoader.zonesExtra.contains(zone))
        {
            zonecaptureName=DatapackClientLoader::datapackLoader.zonesExtra[zone].name;
            ui->zonecaptureWaitText->setText(tr("You are waiting to capture %1").arg(QString("<b>%1</b>").arg(zonecaptureName)));
        }
        else
        {
            zonecaptureName.clear();
            ui->zonecaptureWaitText->setText(tr("You are waiting to capture a zone"));
        }
        updater_page_zonecapture.start(1000);
        nextCaptureOnScreen=nextCapture;
        zonecapture=true;
        ui->stackedWidget->setCurrentWidget(ui->page_zonecapture);
        CatchChallenger::Api_client_real::client->waitingForCityCapture(false);
        updatePageZonecapture();
        return;
    }
    else if(actualBot.step[step].attribute("type")=="script")
    {
        QScriptEngine engine;
        QString contents = actualBot.step[step].text();
        contents="function getTextEntryPoint()\n{\n"+contents+"\n}";
        QScriptValue result = engine.evaluate(contents);
        if (result.isError()) {
            qDebug() << "script error:" << QString::fromLatin1("%1: %2")
                        .arg(result.property("lineNumber").toInt32())
                        .arg(result.toString());
            showTip(QString::fromLatin1("%1: %2")
            .arg(result.property("lineNumber").toInt32())
            .arg(result.toString()));
            return;
        }

        QScriptValue getTextEntryPoint = engine.globalObject().property("getTextEntryPoint");
        if(getTextEntryPoint.isError())
        {
            qDebug() << "script error:" << QString::fromLatin1("%1: %2")
                        .arg(getTextEntryPoint.property("lineNumber").toInt32())
                        .arg(getTextEntryPoint.toString());
            showTip(QString::fromLatin1("%1: %2")
            .arg(getTextEntryPoint.property("lineNumber").toInt32())
            .arg(getTextEntryPoint.toString()));
            return;
        }
        QScriptValue returnValue=getTextEntryPoint.call();
        quint32 textEntryPoint=returnValue.toNumber();
        if(returnValue.isError())
        {
            qDebug() << "script error:" << QString::fromLatin1("%1: %2")
                        .arg(returnValue.property("lineNumber").toInt32())
                        .arg(returnValue.toString());
            showTip(QString::fromLatin1("%1: %2")
            .arg(returnValue.property("lineNumber").toInt32())
            .arg(returnValue.toString()));
            return;
        }
        qDebug() << "textEntryPoint:" << textEntryPoint;
        return;
    }
    else if(actualBot.step[step].attribute("type")=="fight")
    {
        if(!actualBot.step[step].hasAttribute("fightid"))
        {
            showTip(tr("Bot step missing data error, repport this error please"));
            return;
        }
        bool ok;
        quint32 fightId=actualBot.step[step].attribute("fightid").toUInt(&ok);
        if(!ok)
        {
            showTip(tr("Bot step wrong data type error, repport this error please"));
            return;
        }
        CatchChallenger::Api_client_real::client->requestFight(fightId);
        botFight(fightId);
        return;
    }
    else
    {
        showTip(tr("Bot step type error, repport this error please"));
        return;
    }
}

QString BaseWindow::parseHtmlToDisplay(const QString &htmlContent)
{
    QString newContent=htmlContent;
    #ifdef NOREMOTE
    QRegularExpression remote(QRegularExpression::escape("<span class=\"remote\">")+".*"+QRegularExpression::escape("</span>"));
    remote.setPatternOptions(QRegularExpression::InvertedGreedinessOption);
    newContent.remove(remote);
    #endif
    if(!botHaveQuest(actualBot.botId))//if have not quest
    {
        QRegularExpression quest(QRegularExpression::escape("<span class=\"quest\">")+".*"+QRegularExpression::escape("</span>"));
        quest.setPatternOptions(QRegularExpression::InvertedGreedinessOption);
        newContent.remove(quest);
    }
    newContent.replace("href=\"http","style=\"color:#BB9900;\" href=\"http",Qt::CaseInsensitive);
    newContent.replace(QRegularExpression("(href=\"http[^>]+>[^<]+)</a>"),"\\1 <img src=\":/images/link.png\" alt=\"\" /></a>");
    return newContent;
}

void BaseWindow::on_inventory_itemActivated(QListWidgetItem *item)
{
    if(!items_graphical.contains(item))
    {
        qDebug() << "BaseWindow::on_inventory_itemActivated(): activated item not found";
        return;
    }
    if(inSelection)
    {
        quint32 tempQuantitySelected;
        bool ok=true;
        switch(waitedObjectType)
        {
            case ObjectType_Sell:
            case ObjectType_Trade:
                if(items[items_graphical[item]]>1)
                    tempQuantitySelected=QInputDialog::getInt(this,tr("Quantity"),tr("Select a quantity"),1,1,items[items_graphical[item]],1,&ok);
                else
                    tempQuantitySelected=1;
            break;
            default:
                tempQuantitySelected=1;
            break;
        }
        if(!ok)
        {
            objectSelection(false);
            return;
        }
        if(!items.contains(items_graphical[item]))
        {
            objectSelection(false);
            return;
        }
        if(items[items_graphical[item]]<tempQuantitySelected)
        {
            objectSelection(false);
            return;
        }
        objectSelection(true,items_graphical[item],tempQuantitySelected);
        return;
    }

    //is crafting recipe
    if(CatchChallenger::CommonDatapack::commonDatapack.itemToCrafingRecipes.contains(items_graphical[item]))
    {
        Player_private_and_public_informations informations=CatchChallenger::Api_client_real::client->get_player_informations();
        if(informations.recipes.contains(CatchChallenger::CommonDatapack::commonDatapack.itemToCrafingRecipes[items_graphical[item]]))
        {
            QMessageBox::information(this,tr("Information"),tr("You already know this recipe"));
            return;
        }
        objectInUsing << items_graphical[item];
        items[items_graphical[item]]--;
        if(items[items_graphical[item]]==0)
            items.remove(items_graphical[item]);
        CatchChallenger::Api_client_real::client->useObject(items_graphical[item]);
        load_inventory();
    }
    else
        qDebug() << "BaseWindow::on_inventory_itemActivated(): unknow object type";
}

void BaseWindow::objectUsed(const ObjectUsage &objectUsage)
{
    switch(objectUsage)
    {
        case ObjectUsage_correctlyUsed:
        //is crafting recipe
        if(CatchChallenger::CommonDatapack::commonDatapack.itemToCrafingRecipes.contains(objectInUsing.first()))
        {
            CatchChallenger::Api_client_real::client->addRecipe(CatchChallenger::CommonDatapack::commonDatapack.itemToCrafingRecipes[objectInUsing.first()]);
            load_crafting_inventory();
        }
        else
            qDebug() << "BaseWindow::objectUsed(): unknow object type";
        break;
        case ObjectUsage_failed:
        break;
        case ObjectUsage_impossible:
            if(items.contains(objectInUsing.first()))
                items[objectInUsing.first()]++;
            else
                items[objectInUsing.first()]=1;
            load_inventory();
        break;
        default:
        break;
    }
    objectInUsing.removeFirst();
}

void BaseWindow::on_inventoryDestroy_clicked()
{
    qDebug() << "on_inventoryDestroy_clicked()";
    QList<QListWidgetItem *> items=ui->inventory->selectedItems();
    if(items.size()!=1)
        return;
    quint32 itemId=items_graphical[items.first()];
    if(!this->items.contains(itemId))
        return;
    quint32 quantity=this->items[itemId];
    if(quantity>1)
    {
        bool ok;
        quint32 quantity_temp=QInputDialog::getInt(this,tr("Destroy"),tr("Quantity to destroy"),quantity,1,quantity,1,&ok);
        if(!ok)
            return;
        quantity=quantity_temp;
    }
    QMessageBox::StandardButton button;
    if(DatapackClientLoader::datapackLoader.itemsExtra.contains(itemId))
        button=QMessageBox::question(this,tr("Destroy"),tr("Are you sure you want to destroy %1 %2?").arg(quantity).arg(DatapackClientLoader::datapackLoader.itemsExtra[itemId].name),QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes);
    else
        button=QMessageBox::question(this,tr("Destroy"),tr("Are you sure you want to destroy %1 unknow item (id: %2)?").arg(quantity).arg(itemId),QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes);
    if(button!=QMessageBox::Yes)
        return;
    if(!this->items.contains(itemId))
        return;
    if(this->items[itemId]<quantity)
        quantity=this->items[itemId];
    emit destroyObject(itemId,quantity);
    if(this->items[itemId]<=quantity)
        this->items.remove(itemId);
    else
        this->items[itemId]-=quantity;
    load_inventory();
    load_plant_inventory();
}

quint32 BaseWindow::itemQuantity(const quint32 &itemId) const
{
    if(items.contains(itemId))
        return items[itemId];
    return 0;
}

void BaseWindow::on_inventoryUse_clicked()
{
    QList<QListWidgetItem *> items=ui->inventory->selectedItems();
    if(items.size()!=1)
        return;
    on_inventory_itemActivated(items.first());
}

void BaseWindow::on_inventoryInformation_clicked()
{
    QList<QListWidgetItem *> items=ui->inventory->selectedItems();
    if(items.size()!=1)
    {
        qDebug() << "on_inventoryInformation_clicked() should not be accessible here";
        return;
    }
    QListWidgetItem *item=items.first();
    if(!items_graphical.contains(item))
    {
        qDebug() << "on_inventoryInformation_clicked() item not found here";
        return;
    }
    if(DatapackClientLoader::datapackLoader.itemToPlants.contains(items_graphical[item]))
    {
        if(!plants_items_to_graphical.contains(DatapackClientLoader::datapackLoader.itemToPlants[items_graphical[item]]))
        {
            qDebug() << QString("on_inventoryInformation_clicked() is not into plant list: item: %1, plant: %2").arg(items_graphical[item]).arg(DatapackClientLoader::datapackLoader.itemToPlants[items_graphical[item]]);
            return;
        }
        ui->listPlantList->reset();
        ui->stackedWidget->setCurrentWidget(ui->page_plants);
        plants_items_to_graphical[DatapackClientLoader::datapackLoader.itemToPlants[items_graphical[item]]]->setSelected(true);
        on_listPlantList_itemSelectionChanged();
    }
    else
    {
        qDebug() << "on_inventoryInformation_clicked() information on unknow object type";
        return;
    }
}

void BaseWindow::on_toolButtonOptions_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_options);
}

void BaseWindow::on_IG_dialog_text_linkActivated(const QString &rawlink)
{
    ui->IG_dialog->setVisible(false);
    QStringList stringList=rawlink.split(";");
    int index=0;
    while(index<stringList.size())
    {
        const QString &link=stringList.at(index);
        qDebug() << "parsed link to use: " << link;
        if(link.startsWith("http://") || link.startsWith("https://"))
        {
            if(!QDesktopServices::openUrl(QUrl(link)))
                showTip(QString("Unable to open the url: %1").arg(link));
            index++;
            continue;
        }
        bool ok;
        if(link.startsWith("quest_"))
        {
            QString tempLink=link;
            tempLink.remove("quest_");
            quint32 questId=tempLink.toUShort(&ok);
            if(!ok)
            {
                showTip(QString("Unable to open the link: %1").arg(link));
                index++;
                continue;
            }
            if(!CatchChallenger::CommonDatapack::commonDatapack.quests.contains(questId))
            {
                showTip(tr("Quest not found"));
                index++;
                continue;
            }
            isInQuest=true;
            this->questId=questId;
            getTextEntryPoint();
            index++;
            continue;
        }
        else if(link=="clan_create")
        {
            bool ok;
            QString text = QInputDialog::getText(this,tr("Give the clan name"),tr("Clan name:"),QLineEdit::Normal,QString(), &ok);
            if(ok && !text.isEmpty())
            {
                actionClan << ActionClan_Create;
                CatchChallenger::Api_client_real::client->createClan(text);
            }
            index++;
            continue;
        }
        else if(link=="close")
            return;
        else if(link=="next_quest_step" && isInQuest)
        {
            nextQuestStep();
            index++;
            continue;
        }
        quint8 step=link.toUShort(&ok);
        if(!ok)
        {
            showTip(QString("Unable to open the link: %1").arg(link));
            index++;
            continue;
        }
        if(isInQuest)
        {
            showQuestText(step);
            index++;
            continue;
        }
        goToBotStep(step);
        index++;
    }
}

void BaseWindow::nextQuestStep()
{
    if(!CatchChallenger::CommonDatapack::commonDatapack.quests.contains(questId))
    {
        showTip(tr("Quest not found"));
        return;
    }

    if(!quests.contains(questId))
    {
        if(haveStartQuestRequirement(CatchChallenger::CommonDatapack::commonDatapack.quests[questId]))
        {
            CatchChallenger::Api_client_real::client->startQuest(questId);
            startQuest(CatchChallenger::CommonDatapack::commonDatapack.quests[questId]);
            updateDisplayedQuests();
        }
        else
            showTip(tr("You don't have the requirement to start this quest"));
        return;
    }
    else if(quests[questId].step==0)
    {
        if(haveStartQuestRequirement(CatchChallenger::CommonDatapack::commonDatapack.quests[questId]))
        {
            CatchChallenger::Api_client_real::client->startQuest(questId);
            startQuest(CatchChallenger::CommonDatapack::commonDatapack.quests[questId]);
            updateDisplayedQuests();
        }
        else
            showTip(tr("You don't have the requirement to start this quest"));
        return;
    }
    if(!haveNextStepQuestRequirements(CatchChallenger::CommonDatapack::commonDatapack.quests[questId]))
    {
        showTip(tr("You don't have the requirement to continue this quest"));
        return;
    }
    if(quests[questId].step>=(CatchChallenger::CommonDatapack::commonDatapack.quests[questId].steps.size()))
    {
        showTip(tr("You have finish the quest <b>%1</b>").arg(DatapackClientLoader::datapackLoader.questsExtra[questId].name));
        CatchChallenger::Api_client_real::client->finishQuest(questId);
        nextStepQuest(CatchChallenger::CommonDatapack::commonDatapack.quests[questId]);
        updateDisplayedQuests();
        return;
    }
    CatchChallenger::Api_client_real::client->nextQuestStep(questId);
    nextStepQuest(CatchChallenger::CommonDatapack::commonDatapack.quests[questId]);
    updateDisplayedQuests();
}

void BaseWindow::getTextEntryPoint()
{
    if(!isInQuest)
    {
        showTip(QString("Internal error: Is not in quest"));
        return;
    }
    QScriptEngine engine;

    QString client_logic=CatchChallenger::Api_client_real::client->get_datapack_base_name()+"/"+DATAPACK_BASE_PATH_QUESTS+"/"+QString::number(questId)+"/client_logic.js";
    if(!QFile(client_logic).exists())
    {
        showTip(tr("Client file missing"));
        qDebug() << "client_logic file is missing:" << client_logic;
        return;
    }

    QFile scriptFile(client_logic);
    scriptFile.open(QIODevice::ReadOnly);
    QTextStream stream(&scriptFile);
    QString contents = stream.readAll();
    contents="function getTextEntryPoint()\n{\n"+contents+"\n}";
    quint8 currentQuestStepVar;
    bool haveNextStepQuestRequirementsVar;
    bool finishOneTimeVar;
    scriptFile.close();
    if(!quests.contains(questId))
    {
        contents.replace("currentQuestStep()","0");
        contents.replace("currentBot()","0");
        contents.replace("finishOneTime()","false");
        contents.replace("haveQuestStepRequirements()","false");//bug if use that's
        currentQuestStepVar=0;
        haveNextStepQuestRequirementsVar=false;
        finishOneTimeVar=false;
    }
    else
    {
        PlayerQuest quest=quests[questId];
        contents.replace("currentQuestStep()",QString::number(quest.step));
        contents.replace("currentBot()",QString::number(actualBot.botId));
        if(quest.finish_one_time)
            contents.replace("finishOneTime()","true");
        else
            contents.replace("finishOneTime()","false");
        if(quest.step<=0)
        {
            contents.replace("haveQuestStepRequirements()","false");
            haveNextStepQuestRequirementsVar=false;
        }
        else if(haveNextStepQuestRequirements(CatchChallenger::CommonDatapack::commonDatapack.quests[questId]))
        {
            contents.replace("haveQuestStepRequirements()","true");
            haveNextStepQuestRequirementsVar=true;
        }
        else
        {
            contents.replace("haveQuestStepRequirements()","false");
            haveNextStepQuestRequirementsVar=false;
        }
        currentQuestStepVar=quest.step;
        finishOneTimeVar=quest.finish_one_time;
    }
    #ifdef DEBUG_CLIENT_QUEST
    qDebug() << "currentQuestStep:" << currentQuestStepVar << ", haveNextStepQuestRequirements:" << haveNextStepQuestRequirementsVar << ", finishOneTime:" << finishOneTimeVar << ", contents:" << contents;
    #endif

    QScriptValue result = engine.evaluate(contents, client_logic);
    if (result.isError()) {
        qDebug() << "script error:" << QString::fromLatin1("%0:%1: %2")
                    .arg(client_logic)
                    .arg(result.property("lineNumber").toInt32())
                    .arg(result.toString());
        showTip(QString::fromLatin1("%0:%1: %2")
        .arg(client_logic)
        .arg(result.property("lineNumber").toInt32())
        .arg(result.toString()));
        return;
    }

    QScriptValue getTextEntryPoint = engine.globalObject().property("getTextEntryPoint");
    if(getTextEntryPoint.isError())
    {
        qDebug() << "script error:" << QString::fromLatin1("%0:%1: %2")
                    .arg(client_logic)
                    .arg(getTextEntryPoint.property("lineNumber").toInt32())
                    .arg(getTextEntryPoint.toString());
        showTip(QString::fromLatin1("%0:%1: %2")
        .arg(client_logic)
        .arg(getTextEntryPoint.property("lineNumber").toInt32())
        .arg(getTextEntryPoint.toString()));
        return;
    }
    QScriptValue returnValue=getTextEntryPoint.call();
    quint32 textEntryPoint=returnValue.toNumber();
    if(returnValue.isError())
    {
        qDebug() << "script error:" << QString::fromLatin1("%0:%1: %2")
                    .arg(client_logic)
                    .arg(returnValue.property("lineNumber").toInt32())
                    .arg(returnValue.toString());
        showTip(QString::fromLatin1("%0:%1: %2")
        .arg(client_logic)
        .arg(returnValue.property("lineNumber").toInt32())
        .arg(returnValue.toString()));
        return;
    }
    qDebug() << "textEntryPoint:" << textEntryPoint;
    showQuestText(textEntryPoint);

    Q_UNUSED(currentQuestStepVar);
    Q_UNUSED(haveNextStepQuestRequirementsVar);
    Q_UNUSED(finishOneTimeVar);
}

void BaseWindow::showQuestText(const quint32 &textId)
{
    if(!DatapackClientLoader::datapackLoader.questsText.contains(questId))
    {
        qDebug() << QString("No quest text for this quest: %1").arg(questId);
        showTip(tr("No quest text for this quest"));
        return;
    }
    if(!DatapackClientLoader::datapackLoader.questsText[questId].text.contains(textId))
    {
        qDebug() << "No quest text entry point";
        showTip(tr("No quest text entry point"));
        return;
    }

    QString textToShow=parseHtmlToDisplay(DatapackClientLoader::datapackLoader.questsText[questId].text[textId]);
    ui->IG_dialog_text->setText(textToShow);
    ui->IG_dialog->setVisible(true);
}

void BaseWindow::on_toolButton_quit_shop_clicked()
{
    waitToSell=false;
    inSelection=false;
    ui->stackedWidget->setCurrentWidget(ui->page_map);
}

void BaseWindow::on_shopItemList_itemActivated(QListWidgetItem *item)
{
    if(CatchChallenger::Api_client_real::client->getHaveShopAction())
        return;
    if(!waitToSell)
    {
        if(cash<itemsIntoTheShop[shop_items_graphical[item]].price)
        {
            QMessageBox::information(this,tr("Buy"),tr("You have not the cash to buy this item"));
            return;
        }
        bool ok=true;
        if(cash/itemsIntoTheShop[shop_items_graphical[item]].price>1)
            tempQuantityForBuy=QInputDialog::getInt(this,tr("Buy"),tr("Quantity to buy"),1,1,cash/itemsIntoTheShop[shop_items_graphical[item]].price,1,&ok);
        else
            tempQuantityForBuy=1;
        if(!ok)
            return;
        tempItemForBuy=shop_items_graphical[item];
        CatchChallenger::Api_client_real::client->buyObject(shopId,tempItemForBuy,tempQuantityForBuy,itemsIntoTheShop[tempItemForBuy].price);
        ui->stackedWidget->setCurrentWidget(ui->page_map);
        tempCashForBuy=itemsIntoTheShop[tempItemForBuy].price*tempQuantityForBuy;
        removeCash(tempCashForBuy);
        showTip(tr("Buying the object..."));
    }
    else
    {
        if(!items.contains(shop_items_graphical[item]))
            return;
        bool ok=true;
        if(items[shop_items_graphical[item]]>1)
            tempQuantityForSell=QInputDialog::getInt(this,tr("Sell"),tr("Quantity to sell"),1,1,items[shop_items_graphical[item]],1,&ok);
        else
            tempQuantityForSell=1;
        if(!ok)
            return;
        if(!items.contains(shop_items_graphical[item]))
            return;
        if(items[shop_items_graphical[item]]<tempQuantityForSell)
            return;
        objectSelection(true,shop_items_graphical[item],tempQuantityForSell);
        ui->stackedWidget->setCurrentWidget(ui->page_map);
        showTip(tr("Selling the object..."));
    }
}

void BaseWindow::on_shopItemList_itemSelectionChanged()
{
    QList<QListWidgetItem *> items=ui->shopItemList->selectedItems();
    if(items.size()!=1)
    {
        ui->shopName->setText("");
        ui->shopDescription->setText(tr("Select an object"));
        ui->shopBuy->setVisible(false);
        ui->shopImage->setPixmap(QPixmap(":/images/inventory/unknow-object.png"));
        return;
    }
    ui->shopBuy->setVisible(true);
    QListWidgetItem *item=items.first();
    if(!DatapackClientLoader::datapackLoader.itemsExtra.contains(shop_items_graphical[item]))
    {
        ui->shopImage->setPixmap(DatapackClientLoader::datapackLoader.defaultInventoryImage());
        ui->shopName->setText(tr("Unknow name"));
        ui->shopDescription->setText(tr("Unknow description"));
        return;
    }
    const DatapackClientLoader::ItemExtra &content=DatapackClientLoader::datapackLoader.itemsExtra[shop_items_graphical[item]];

    ui->shopImage->setPixmap(content.image);
    ui->shopName->setText(content.name);
    ui->shopDescription->setText(content.description);
}

void BaseWindow::on_shopBuy_clicked()
{
    QList<QListWidgetItem *> items=ui->shopItemList->selectedItems();
    if(items.size()!=1)
    {
        qDebug() << "on_shopBuy_clicked() no correct selection";
        return;
    }
    on_shopItemList_itemActivated(items.first());
}

void BaseWindow::haveShopList(const QList<ItemToSellOrBuy> &items)
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::haveShopList()";
    #endif
    QFont MissingQuantity;
    MissingQuantity.setItalic(true);
    ui->shopItemList->clear();
    itemsIntoTheShop.clear();
    shop_items_graphical.clear();
    shop_items_to_graphical.clear();
    int index=0;
    while(index<items.size())
    {
        itemsIntoTheShop[items.at(index).object]=items.at(index);
        QListWidgetItem *item=new QListWidgetItem();
        shop_items_to_graphical[items.at(index).object]=item;
        shop_items_graphical[item]=items.at(index).object;
        if(DatapackClientLoader::datapackLoader.itemsExtra.contains(items.at(index).object))
        {
            item->setIcon(DatapackClientLoader::datapackLoader.itemsExtra[items.at(index).object].image);
            if(items.at(index).quantity==0)
                item->setText(tr("%1\nPrice: %2$").arg(DatapackClientLoader::datapackLoader.itemsExtra[items.at(index).object].name).arg(items.at(index).price));
            else
                item->setText(tr("%1 at %2$\nQuantity: %3").arg(DatapackClientLoader::datapackLoader.itemsExtra[items.at(index).object].name).arg(items.at(index).price).arg(items.at(index).quantity));
        }
        else
        {
            item->setIcon(DatapackClientLoader::datapackLoader.defaultInventoryImage());
            if(items.at(index).quantity==0)
                item->setText(tr("Item %1\nPrice: %2$").arg(items.at(index).object).arg(items.at(index).price));
            else
                item->setText(tr("Item %1 at %2$\nQuantity: %3").arg(items.at(index).object).arg(items.at(index).price).arg(items.at(index).quantity));
        }
        if(items.at(index).price>cash)
        {
            item->setFont(MissingQuantity);
            item->setForeground(QBrush(QColor(200,20,20)));
        }
        ui->shopItemList->addItem(item);
        index++;
    }
    ui->shopBuy->setText(tr("Buy"));
    on_shopItemList_itemSelectionChanged();
}

void BaseWindow::displaySellList()
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::displaySellList()";
    #endif
    ui->shopItemList->clear();
    itemsIntoTheShop.clear();
    shop_items_graphical.clear();
    shop_items_to_graphical.clear();
    QHashIterator<quint32,quint32> i(items);
    while (i.hasNext()) {
        i.next();
        if(DatapackClientLoader::datapackLoader.itemsExtra.contains(i.key()))
        {
            QListWidgetItem *item=new QListWidgetItem();
            shop_items_to_graphical[i.key()]=item;
            shop_items_graphical[item]=i.key();
            item->setIcon(DatapackClientLoader::datapackLoader.itemsExtra[i.key()].image);
            if(i.value()>1)
                item->setText(tr("%1\nPrice: %2$, quantity: %3")
                        .arg(DatapackClientLoader::datapackLoader.itemsExtra[i.key()].name)
                        .arg(CatchChallenger::CommonDatapack::commonDatapack.items.item[i.key()].price/2)
                        .arg(i.value())
                        );
            else
                item->setText(tr("%1\nPrice: %2$")
                        .arg(DatapackClientLoader::datapackLoader.itemsExtra[i.key()].name)
                        .arg(CatchChallenger::CommonDatapack::commonDatapack.items.item[i.key()].price/2)
                        );
            ui->shopItemList->addItem(item);
        }
    }
    ui->shopBuy->setText(tr("Sell"));
    on_shopItemList_itemSelectionChanged();
}

void BaseWindow::haveBuyObject(const BuyStat &stat,const quint32 &newPrice)
{
    QHash<quint32,quint32> items;
    switch(stat)
    {
        case BuyStat_Done:
            items[tempItemForBuy]=tempQuantityForBuy;
            add_to_inventory(items);
        break;
        case BuyStat_BetterPrice:
            if(newPrice==0)
            {
                qDebug() << "haveSellObject() Can't buy at 0$!";
                return;
            }
            addCash(tempCashForBuy);
            removeCash(newPrice*tempQuantityForBuy);
            items[tempItemForBuy]=tempQuantityForBuy;
            add_to_inventory(items);
        break;
        case BuyStat_HaveNotQuantity:
            addCash(tempCashForBuy);
            showTip(tr("Sorry but have not the quantity of this item"));
        break;
        case BuyStat_PriceHaveChanged:
            addCash(tempCashForBuy);
            showTip(tr("Sorry but now the price is worse"));
        break;
        default:
            qDebug() << "haveBuyObject(stat) have unknow value";
        break;
    }
}

void BaseWindow::haveSellObject(const SoldStat &stat,const quint32 &newPrice)
{
    waitToSell=false;
    switch(stat)
    {
        case SoldStat_Done:
            addCash(itemsToSell.first().price*itemsToSell.first().quantity);
            showTip(tr("Item sold"));
        break;
        case SoldStat_BetterPrice:
            if(newPrice==0)
            {
                qDebug() << "haveSellObject() the price 0$ can't be better price!";
                return;
            }
            addCash(newPrice*itemsToSell.first().quantity);
            showTip(tr("Item sold at better price"));
        break;
        case SoldStat_WrongQuantity:
            if(items.contains(itemsToSell.first().object))
                items[itemsToSell.first().object]+=itemsToSell.first().quantity;
            else
                items[itemsToSell.first().object]=itemsToSell.first().quantity;
            load_inventory();
            load_plant_inventory();
            showTip(tr("Sorry but have not the quantity of this item"));
        break;
        case SoldStat_PriceHaveChanged:
            if(items.contains(itemsToSell.first().object))
                items[itemsToSell.first().object]+=itemsToSell.first().quantity;
            else
                items[itemsToSell.first().object]=itemsToSell.first().quantity;
            load_inventory();
            load_plant_inventory();
            showTip(tr("Sorry but now the price is worse"));
        break;
        default:
            qDebug() << "haveBuyObject(stat) have unknow value";
        break;
    }
    itemsToSell.removeFirst();
}

void BaseWindow::addCash(const quint32 &cash)
{
    this->cash+=cash;
    ui->player_informations_cash->setText(QString("%1$").arg(this->cash));
    ui->tradePlayerCash->setMaximum(this->cash);
}

void BaseWindow::removeCash(const quint32 &cash)
{
    this->cash-=cash;
    ui->player_informations_cash->setText(QString("%1$").arg(this->cash));
    ui->tradePlayerCash->setMaximum(this->cash);
}

void BaseWindow::on_pushButton_interface_monsters_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_monster);
}

void BaseWindow::on_toolButton_monster_list_quit_clicked()
{
    if(waitedObjectType==ObjectType_MonsterToTrade || waitedObjectType==ObjectType_MonsterToLearn || waitedObjectType==ObjectType_UseInFight)
    {
        if(inSelection)
        {
            objectSelection(false);
            return;
        }
    }
    ui->stackedWidget->setCurrentWidget(ui->page_map);
}

void BaseWindow::on_tradePlayerCash_editingFinished()
{
    if(ui->tradePlayerCash->value()==ui->tradePlayerCash->minimum())
        return;
    CatchChallenger::Api_client_real::client->addTradeCash(ui->tradePlayerCash->value()-ui->tradePlayerCash->minimum());
    ui->tradePlayerCash->setMinimum(ui->tradePlayerCash->value());
}

void BaseWindow::on_toolButton_bioscan_quit_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_map);
}

void BaseWindow::on_tradeCancel_clicked()
{
    CatchChallenger::Api_client_real::client->tradeCanceled();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
}

void BaseWindow::on_tradeValidate_clicked()
{
    CatchChallenger::Api_client_real::client->tradeFinish();
    if(tradeOtherStat==TradeOtherStat_Accepted)
        ui->stackedWidget->setCurrentWidget(ui->page_map);
    else
    {
        ui->tradePlayerCash->setEnabled(false);
        ui->tradePlayerItems->setEnabled(false);
        ui->tradePlayerMonsters->setEnabled(false);
        ui->tradeAddItem->setEnabled(false);
        ui->tradeAddMonster->setEnabled(false);
        ui->tradeValidate->setEnabled(false);
    }
}

void BaseWindow::on_tradeAddItem_clicked()
{
    selectObject(ObjectType_Trade);
}

void CatchChallenger::BaseWindow::on_tradeAddMonster_clicked()
{
    selectObject(ObjectType_MonsterToTrade);
}

void CatchChallenger::BaseWindow::on_selectMonster_clicked()
{
    QList<QListWidgetItem *> selectedMonsters=ui->monsterList->selectedItems();
    if(selectedMonsters.size()!=1)
        return;
    on_monsterList_itemActivated(selectedMonsters.first());
}

void CatchChallenger::BaseWindow::on_monsterList_itemActivated(QListWidgetItem *item)
{
    if(!monsters_items_graphical.contains(item))
        return;
    objectSelection(true,monsters_items_graphical[item]);
}

void CatchChallenger::BaseWindow::on_close_IG_dialog_clicked()
{
    isInQuest=false;
}

void CatchChallenger::BaseWindow::on_warehouseWithdrawCash_clicked()
{
    bool ok=true;
    int i;
    if((warehouse_cash-temp_warehouse_cash)==1)
        i = 1;
    else
        i = QInputDialog::getInt(this, tr("Withdraw"),tr("Amount cash to withdraw:"), 0, 0, warehouse_cash-temp_warehouse_cash, 1, &ok);
    if(!ok || i<=0)
        return;
    temp_warehouse_cash+=i;
    updateTheWareHouseContent();
}

void CatchChallenger::BaseWindow::on_warehouseDepositCash_clicked()
{
    bool ok=true;
    int i;
    if((cash+temp_warehouse_cash)==1)
        i = 1;
    else
        i = QInputDialog::getInt(this, tr("Deposite"),tr("Amount cash to deposite:"), 0, 0, cash+temp_warehouse_cash, 1, &ok);
    if(!ok || i<=0)
        return;
    temp_warehouse_cash-=i;
    updateTheWareHouseContent();
}

void CatchChallenger::BaseWindow::on_warehouseWithdrawItem_clicked()
{
    QList<QListWidgetItem *> itemList=ui->warehousePlayerStoredInventory->selectedItems();
    if(itemList.size()!=1)
        return;
    on_warehousePlayerStoredInventory_itemActivated(itemList.first());
}

void CatchChallenger::BaseWindow::on_warehouseDepositItem_clicked()
{
    QList<QListWidgetItem *> itemList=ui->warehousePlayerInventory->selectedItems();
    if(itemList.size()!=1)
        return;
    on_warehousePlayerInventory_itemActivated(itemList.first());
}

void CatchChallenger::BaseWindow::on_warehouseWithdrawMonster_clicked()
{
    QList<QListWidgetItem *> itemList=ui->warehousePlayerStoredMonster->selectedItems();
    if(itemList.size()!=1)
        return;
    on_warehousePlayerStoredMonster_itemActivated(itemList.first());
}

void CatchChallenger::BaseWindow::on_warehouseDepositMonster_clicked()
{
    QList<QListWidgetItem *> itemList=ui->warehousePlayerMonster->selectedItems();
    if(itemList.size()!=1)
        return;
    on_warehousePlayerMonster_itemActivated(itemList.first());
}

void CatchChallenger::BaseWindow::on_warehousePlayerInventory_itemActivated(QListWidgetItem *item)
{
    quint32 quantity=0;
    quint32 id=item->data(99).toUInt();
    if(items.contains(id))
        quantity+=items[id];
    if(change_warehouse_items.contains(id))
        if(change_warehouse_items[id]>0)
            quantity+=change_warehouse_items[id];
    bool ok=true;
    int i;
    if(quantity==1)
        i = 1;
    else
        i = QInputDialog::getInt(this, tr("Deposite"),tr("Amount %1 to deposite:").arg(DatapackClientLoader::datapackLoader.itemsExtra[id].name), 0, 0, quantity, 1, &ok);
    if(!ok || i<=0)
        return;
    if(change_warehouse_items.contains(id))
        change_warehouse_items[id]-=i;
    else
        change_warehouse_items[id]=-i;
    if(change_warehouse_items[id]==0)
        change_warehouse_items.remove(id);
    updateTheWareHouseContent();
}

void CatchChallenger::BaseWindow::on_warehousePlayerStoredInventory_itemActivated(QListWidgetItem *item)
{
    quint32 quantity=0;
    quint32 id=item->data(99).toUInt();
    if(items.contains(id))
        quantity+=items[id];
    if(change_warehouse_items.contains(id))
        if(change_warehouse_items[id]<0)
            quantity-=change_warehouse_items[id];
    bool ok=true;
    int i;
    if(quantity==1)
        i = 1;
    else
        i = QInputDialog::getInt(this, tr("Withdraw"),tr("Amount %1 to withdraw:").arg(DatapackClientLoader::datapackLoader.itemsExtra[id].name), 0, 0, quantity, 1, &ok);
    if(!ok || i<=0)
        return;
    if(change_warehouse_items.contains(id))
        change_warehouse_items[id]+=i;
    else
        change_warehouse_items[id]=i;
    if(change_warehouse_items[id]==0)
        change_warehouse_items.remove(id);
    updateTheWareHouseContent();
}

void CatchChallenger::BaseWindow::on_warehousePlayerMonster_itemActivated(QListWidgetItem *item)
{
    quint32 id=item->data(99).toUInt();
    bool remain_valid_monster=false;
    int index=0;
    QList<PlayerMonster> warehouseMonsterOnPlayerList=warehouseMonsterOnPlayer();
    while(index<warehouseMonsterOnPlayerList.size())
    {
        const PlayerMonster &monster=warehouseMonsterOnPlayerList.at(index);
        if(monster.id!=id && monster.egg_step==0 && monster.hp>0)
        {
            remain_valid_monster=true;
            break;
        }
        index++;
    }
    if(!remain_valid_monster)
    {
        QMessageBox::warning(this,tr("Error"),tr("You can't deposite your last alive monster!"));
        return;
    }
    monster_to_withdraw.removeOne(id);
    monster_to_deposit <<  id;
    updateTheWareHouseContent();
}

void CatchChallenger::BaseWindow::on_warehousePlayerStoredMonster_itemActivated(QListWidgetItem *item)
{
    quint32 id=item->data(99).toUInt();
    QList<PlayerMonster> warehouseMonsterOnPlayerList=warehouseMonsterOnPlayer();
    if(warehouseMonsterOnPlayerList.size()>CATCHCHALLENGER_MONSTER_MAX_WEAR_ON_PLAYER)
    {
        QMessageBox::warning(this,tr("Error"),tr("You can't wear more monster!"));
        return;
    }
    monster_to_deposit.removeOne(id);
    monster_to_withdraw <<  id;
    updateTheWareHouseContent();
}

QList<PlayerMonster> CatchChallenger::BaseWindow::warehouseMonsterOnPlayer() const
{
    QList<PlayerMonster> warehouseMonsterOnPlayerList;
    {
        const QList<PlayerMonster> &playerMonster=CatchChallenger::ClientFightEngine::fightEngine.getPlayerMonster();
        int index=0;
        int size=playerMonster.size();
        while(index<size)
        {
            const PlayerMonster &monster=playerMonster.at(index);
            if(CatchChallenger::CommonDatapack::commonDatapack.monsters.contains(monster.monster) && !monster_to_deposit.contains(monster.id))
                warehouseMonsterOnPlayerList << monster;
            index++;
        }
    }
    {
        int index=0;
        int size=warehouse_playerMonster.size();
        while(index<size)
        {
            const PlayerMonster &monster=warehouse_playerMonster.at(index);
            if(CatchChallenger::CommonDatapack::commonDatapack.monsters.contains(monster.monster) && monster_to_withdraw.contains(monster.id))
                warehouseMonsterOnPlayerList << monster;
            index++;
        }
    }
    return warehouseMonsterOnPlayerList;
}

void CatchChallenger::BaseWindow::on_toolButton_quit_warehouse_clicked()
{
    monster_to_withdraw.clear();
    monster_to_deposit.clear();
    change_warehouse_items.clear();
    warehouse_cash=0;
    ui->stackedWidget->setCurrentWidget(ui->page_map);
}

void CatchChallenger::BaseWindow::on_warehouseValidate_clicked()
{
    {
        QList<QPair<quint32,qint32> > change_warehouse_items_list;
        QHash<quint32,qint32>::const_iterator i = change_warehouse_items.constBegin();
        while (i != change_warehouse_items.constEnd()) {
            change_warehouse_items_list << QPair<quint32,qint32>(i.key(),i.value());
            ++i;
        }
        CatchChallenger::Api_client_real::client->wareHouseStore(temp_warehouse_cash,change_warehouse_items_list,monster_to_withdraw,monster_to_deposit);
    }
    //validate the change here
    cash+=temp_warehouse_cash;
    ui->player_informations_cash->setText(QString("%1$").arg(this->cash));
    ui->tradePlayerCash->setMaximum(this->cash);
    warehouse_cash-=temp_warehouse_cash;
    {
        QHash<quint32,qint32>::const_iterator i = change_warehouse_items.constBegin();
        while (i != change_warehouse_items.constEnd()) {
            if(i.value()>0)
            {
                if(items.contains(i.key()))
                    items[i.key()]+=i.value();
                else
                    items[i.key()]=i.value();
                warehouse_items[i.key()]-=i.value();
                if(warehouse_items[i.key()]==0)
                    warehouse_items.remove(i.key());
            }
            if(i.value()<0)
            {
                items[i.key()]+=i.value();
                if(items[i.key()]==0)
                    items.remove(i.key());
                if(warehouse_items.contains(i.key()))
                    warehouse_items[i.key()]-=i.value();
                else
                    warehouse_items[i.key()]=-i.value();
            }
            ++i;
        }
        load_inventory();
        load_plant_inventory();
    }
    {
        QList<PlayerMonster> playerMonsterToWithdraw;
        int index=0;
        while(index<monster_to_withdraw.size())
        {
            int sub_index=0;
            while(sub_index<warehouse_playerMonster.size())
            {
                if(warehouse_playerMonster.at(sub_index).id==monster_to_withdraw.at(index))
                {
                    playerMonsterToWithdraw << warehouse_playerMonster.at(sub_index);
                    warehouse_playerMonster.removeAt(sub_index);
                    break;
                }
                sub_index++;
            }
            index++;
        }
        CatchChallenger::ClientFightEngine::fightEngine.addPlayerMonster(playerMonsterToWithdraw);
    }
    {
        int index=0;
        while(index<monster_to_deposit.size())
        {
            const QList<PlayerMonster> &playerMonster=CatchChallenger::ClientFightEngine::fightEngine.getPlayerMonster();
            int sub_index=0;
            while(sub_index<warehouse_playerMonster.size())
            {
                if(playerMonster.at(sub_index).id==monster_to_deposit.at(index))
                {
                    warehouse_playerMonster << playerMonster.at(sub_index);
                    CatchChallenger::ClientFightEngine::fightEngine.removeMonster(monster_to_deposit.at(index));
                    break;
                }
                sub_index++;
            }
            index++;
        }
    }
    load_monsters();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
}

void CatchChallenger::BaseWindow::on_pushButtonFightBag_clicked()
{
    selectObject(ObjectType_UseInFight);
}

void CatchChallenger::BaseWindow::clanActionSuccess(const quint32 &clanId)
{
    switch(actionClan.first())
    {
        case ActionClan_Create:
            if(clan==0)
            {
                clan=clanId;
                clan_leader=true;
            }
            updateClanDisplay();
            showTip(tr("The clan is created"));
        break;
        case ActionClan_Leave:
        case ActionClan_Dissolve:
            clan=0;
            updateClanDisplay();
            showTip(tr("You are leaved the clan"));
        break;
        case ActionClan_Invite:
            showTip(tr("You have correctly invited the player"));
        break;
        case ActionClan_Eject:
            showTip(tr("You have correctly ejected the player from clan"));
        break;
        default:
        newError(tr("Internal error"),"ActionClan unknown");
        return;
    }
    actionClan.removeFirst();
}

void CatchChallenger::BaseWindow::clanActionFailed()
{
    switch(actionClan.first())
    {
        case ActionClan_Create:
            updateClanDisplay();
        break;
        case ActionClan_Leave:
        case ActionClan_Dissolve:
        break;
        case ActionClan_Invite:
            showTip(tr("You have failed to invite the player"));
        break;
        case ActionClan_Eject:
            showTip(tr("You have failed to eject the player from clan"));
        break;
        default:
        newError(tr("Internal error"),"ActionClan unknown");
        return;
    }
    actionClan.removeFirst();
}

void CatchChallenger::BaseWindow::clanDissolved()
{
    haveClanInformations=false;
    clanName.clear();
    clan=0;
    updateClanDisplay();
}

void CatchChallenger::BaseWindow::updateClanDisplay()
{
    ui->tabWidgetTrainerCard->setTabEnabled(4,clan!=0);
    ui->clanGrouBoxNormal->setVisible(!clan_leader);
    ui->clanGrouBoxLeader->setVisible(clan_leader);
    ui->clanGrouBoxInformations->setVisible(haveClanInformations);
    if(haveClanInformations)
    {
        if(clanName.isEmpty())
            ui->clanName->setText(tr("Your clan"));
        else
            ui->clanName->setText(clanName);
    }
    if(Chat::chat!=NULL)
        Chat::chat->setClan(clan!=0);
}

void CatchChallenger::BaseWindow::on_clanActionLeave_clicked()
{
    actionClan << ActionClan_Leave;
    CatchChallenger::Api_client_real::client->leaveClan();
}

void CatchChallenger::BaseWindow::on_clanActionDissolve_clicked()
{
    actionClan << ActionClan_Dissolve;
    CatchChallenger::Api_client_real::client->dissolveClan();
}

void CatchChallenger::BaseWindow::on_clanActionInvite_clicked()
{
    bool ok;
    QString text = QInputDialog::getText(this,tr("Give the player pseudo"),tr("Player pseudo to invite:"),QLineEdit::Normal,QString(), &ok);
    if(ok && !text.isEmpty())
    {
        actionClan << ActionClan_Invite;
        CatchChallenger::Api_client_real::client->inviteClan(text);
    }
}

void CatchChallenger::BaseWindow::on_clanActionEject_clicked()
{
    bool ok;
    QString text = QInputDialog::getText(this,tr("Give the player pseudo"),tr("Player pseudo to invite:"),QLineEdit::Normal,QString(), &ok);
    if(ok && !text.isEmpty())
    {
        actionClan << ActionClan_Eject;
        CatchChallenger::Api_client_real::client->ejectClan(text);
    }
}

void CatchChallenger::BaseWindow::clanInformations(const QString &name)
{
    haveClanInformations=true;
    clanName=name;
    updateClanDisplay();
}

void CatchChallenger::BaseWindow::clanInvite(const quint32 &clanId,const QString &name)
{
    QMessageBox::StandardButton button=QMessageBox::question(this,tr("Invite"),tr("The clan %1 invite you to become a member. Do you accept?").arg(QString("<b>%1</b>").arg(name)));
    CatchChallenger::Api_client_real::client->inviteAccept(button==QMessageBox::Yes);
    if(button==QMessageBox::Yes)
    {
        this->clan=clanId;
        this->clan_leader=false;
        haveClanInformations=false;
        updateClanDisplay();
    }
}

void CatchChallenger::BaseWindow::cityCaptureUpdateTime()
{
    if(city.capture.frenquency==City::Capture::Frequency_week)
        nextCapture=QDateTime::fromMSecsSinceEpoch(QDateTime::currentMSecsSinceEpoch()+24*3600*7*1000);
    else
        nextCapture=FacilityLib::nextCaptureTime(city);
    nextCityCaptureTimer.start(nextCapture.toMSecsSinceEpoch()-QDateTime::currentMSecsSinceEpoch());
}

void CatchChallenger::BaseWindow::updatePageZonecapture()
{
    if(QDateTime::currentMSecsSinceEpoch()<nextCaptureOnScreen.toMSecsSinceEpoch())
    {
        int sec=(nextCaptureOnScreen.toMSecsSinceEpoch()-QDateTime::currentMSecsSinceEpoch())/1000+1;
        QString timeText;
        if(sec>3600*24*365*50)
            timeText="Time player: bug";
        else if(sec>=3600*24*10)
            timeText=QObject::tr("%n day(s)","",sec/(3600*24));
        else if(sec>=3600*24)
            timeText=QObject::tr("%n day(s) and %1","",sec/(3600*24)).arg(QObject::tr("%n hour(s)","",(sec%(3600*24))/3600));
        else if(sec>=3600)
            timeText=QObject::tr("%n hour(s) and %1","",sec/3600).arg(QObject::tr("%n minute(s)","",(sec%3600)/60));
        else if(sec>=60)
            timeText=QObject::tr("%n minute(s) and %1","",sec/60).arg(QObject::tr("%n second(s)","",sec%60));
        else
            timeText=QObject::tr("%n second(s)","",sec);
        ui->zonecaptureWaitTime->setText(tr("Remaining time: %1").arg(QString("<b>%1</b>").arg(timeText)));
        ui->zonecaptureCancel->setVisible(true);
    }
    else
    {
        ui->zonecaptureCancel->setVisible(false);
        ui->zonecaptureWaitTime->setText("<i>"+tr("In waiting of players list")+"</i>");
        updater_page_zonecapture.stop();
    }
}

void CatchChallenger::BaseWindow::on_zonecaptureCancel_clicked()
{
    updater_page_zonecapture.stop();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    CatchChallenger::Api_client_real::client->waitingForCityCapture(true);
    zonecapture=false;
}

void CatchChallenger::BaseWindow::captureCityYourAreNotLeader()
{
    updater_page_zonecapture.stop();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    showTip(tr("You are not a clan leader to start a city capture"));
    zonecapture=false;
}

void CatchChallenger::BaseWindow::captureCityYourLeaderHaveStartInOtherCity(const QString &zone)
{
    updater_page_zonecapture.stop();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    if(DatapackClientLoader::datapackLoader.zonesExtra.contains(zone))
        showTip(tr("Your clan leader have start a caputre for another city: %1").arg(QString("<b>%1</b>").arg(DatapackClientLoader::datapackLoader.zonesExtra[zone].name)));
    else
        showTip(tr("Your clan leader have start a caputre for another city"));
    zonecapture=false;
}

void CatchChallenger::BaseWindow::captureCityPreviousNotFinished()
{
    updater_page_zonecapture.stop();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    showTip(tr("Previous capture of this city is not finished"));
    zonecapture=false;
}

void CatchChallenger::BaseWindow::captureCityStartBattle(const quint16 &player_count,const quint16 &clan_count)
{
    ui->zonecaptureCancel->setVisible(false);
    ui->zonecaptureWaitTime->setText("<i>"+tr("%1 and %2 in wainting to capture the city").arg("<b>"+tr("%n player(s)","",player_count)+"</b>").arg("<b>"+tr("%n clan(s)","",clan_count)+"</b>")+"</i>");
    updater_page_zonecapture.stop();
}

void CatchChallenger::BaseWindow::captureCityStartBotFight(const quint16 &player_count,const quint16 &clan_count,const quint32 &fightId)
{
    ui->zonecaptureCancel->setVisible(false);
    ui->zonecaptureWaitTime->setText("<i>"+tr("%1 and %2 in wainting to capture the city").arg("<b>"+tr("%n player(s)","",player_count)+"</b>").arg("<b>"+tr("%n clan(s)","",clan_count)+"</b>")+"</i>");
    updater_page_zonecapture.stop();
    botFight(fightId);
}

void CatchChallenger::BaseWindow::captureCityDelayedStart(const quint16 &player_count,const quint16 &clan_count)
{
    ui->zonecaptureCancel->setVisible(false);
    ui->zonecaptureWaitTime->setText("<i>"+tr("In waiting fight.")+" "+tr("%1 and %2 in wainting to capture the city").arg("<b>"+tr("%n player(s)","",player_count)+"</b>").arg("<b>"+tr("%n clan(s)","",clan_count)+"</b>")+"</i>");
    updater_page_zonecapture.stop();
}

void CatchChallenger::BaseWindow::captureCityWin()
{
    updater_page_zonecapture.stop();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    if(!zonecaptureName.isEmpty())
        showTip(tr("Your clan win the city: %1").arg(QString("<b>%1</b>").arg(zonecaptureName)));
    else
        showTip(tr("Your clan win the city"));
    zonecapture=false;
}

void CatchChallenger::BaseWindow::on_factoryBuy_clicked()
{
    QList<QListWidgetItem *> selectedItems=ui->factoryProducts->selectedItems();
    if(selectedItems.size()!=1)
        return;
    on_factoryProducts_itemActivated(selectedItems.first());
}

void CatchChallenger::BaseWindow::on_factoryProducts_itemActivated(QListWidgetItem *item)
{
    quint32 quantity=1;
    quint32 id=item->data(99).toUInt();
    quint32 price=item->data(98).toUInt();
    if(!item->text().isEmpty())
        quantity=item->text().toUInt();
    if(cash<price)
    {
        QMessageBox::warning(this,tr("No cash"),tr("No bash to buy this item"));
        return;
    }
    int i=1;
    if(cash>=(price*2) && quantity>1)
    {
        bool ok;
        i = QInputDialog::getInt(this, tr("Deposite"),tr("Amount %1 to deposite:").arg(DatapackClientLoader::datapackLoader.itemsExtra[id].name), 0, 0, quantity, 1, &ok);
        if(!ok || i<=0)
            return;
    }
    quantity-=i;
    if(quantity<1)
        delete item;
    else if(quantity==1)
        item->setText(QString());
    else
        item->setText(QString::number(quantity));
    tempQuantityForBuy=quantity;
    tempItemForBuy=id;
    tempCashForBuy=i*price;
    removeCash(tempCashForBuy);
    CatchChallenger::Api_client_real::client->buyFactoryObject(factoryId,id,i,price);
}

void CatchChallenger::BaseWindow::on_factorySell_clicked()
{
    QList<QListWidgetItem *> selectedItems=ui->factoryResources->selectedItems();
    if(selectedItems.size()!=1)
        return;
    on_factoryResources_itemActivated(selectedItems.first());
}

void CatchChallenger::BaseWindow::on_factoryResources_itemActivated(QListWidgetItem *item)
{
    quint32 quantity=1;
    quint32 id=item->data(99).toUInt();
    quint32 price=item->data(98).toUInt();
    if(!item->text().isEmpty())
        quantity=item->text().toUInt();
    if(!items.contains(id))
    {
        QMessageBox::warning(this,tr("No item"),tr("You have not the item to sell"));
        return;
    }
    int i=1;
    if(items[id]>1 && quantity>1)
    {
        bool ok;
        i = QInputDialog::getInt(this, tr("Deposite"),tr("Amount %1 to deposite:").arg(DatapackClientLoader::datapackLoader.itemsExtra[id].name), 0, 0, quantity, 1, &ok);
        if(!ok || i<=0)
            return;
    }
    quantity-=i;
    if(quantity<1)
        delete item;
    else if(quantity==1)
        item->setText(QString());
    else
        item->setText(QString::number(quantity));
    ItemToSellOrBuy tempItem;
    tempItem.object=id;
    tempItem.quantity=i;
    tempItem.price=price;
    itemsToSell << tempItem;
    items[id]-=i;
    if(items[id]==0)
        items.remove(id);
    CatchChallenger::Api_client_real::client->sellFactoryObject(factoryId,id,i,price);
}

void BaseWindow::haveBuyFactoryObject(const BuyStat &stat,const quint32 &newPrice)
{
    QHash<quint32,quint32> items;
    switch(stat)
    {
        case BuyStat_Done:
            items[tempItemForBuy]=tempQuantityForBuy;
            add_to_inventory(items);
        break;
        case BuyStat_BetterPrice:
            if(newPrice==0)
            {
                qDebug() << "haveBuyFactoryObject() Can't buy at 0$!";
                return;
            }
            addCash(tempCashForBuy);
            removeCash(newPrice*tempQuantityForBuy);
            items[tempItemForBuy]=tempQuantityForBuy;
            add_to_inventory(items);
        break;
        case BuyStat_HaveNotQuantity:
            addCash(tempCashForBuy);
            showTip(tr("Sorry but have not the quantity of this item"));
        break;
        case BuyStat_PriceHaveChanged:
            addCash(tempCashForBuy);
            showTip(tr("Sorry but now the price is worse"));
        break;
        default:
            qDebug() << "haveBuyFactoryObject(stat) have unknow value";
        break;
    }
}

void BaseWindow::haveSellFactoryObject(const SoldStat &stat,const quint32 &newPrice)
{
    waitToSell=false;
    switch(stat)
    {
        case SoldStat_Done:
            addCash(itemsToSell.first().price*itemsToSell.first().quantity);
            showTip(tr("Item sold"));
        break;
        case SoldStat_BetterPrice:
            if(newPrice==0)
            {
                qDebug() << "haveSellFactoryObject() the price 0$ can't be better price!";
                return;
            }
            addCash(newPrice*itemsToSell.first().quantity);
            showTip(tr("Item sold at better price"));
        break;
        case SoldStat_WrongQuantity:
            if(items.contains(itemsToSell.first().object))
                items[itemsToSell.first().object]+=itemsToSell.first().quantity;
            else
                items[itemsToSell.first().object]=itemsToSell.first().quantity;
            load_inventory();
            load_plant_inventory();
            showTip(tr("Sorry but have not the quantity of this item"));
        break;
        case SoldStat_PriceHaveChanged:
            if(items.contains(itemsToSell.first().object))
                items[itemsToSell.first().object]+=itemsToSell.first().quantity;
            else
                items[itemsToSell.first().object]=itemsToSell.first().quantity;
            load_inventory();
            load_plant_inventory();
            showTip(tr("Sorry but now the price is worse"));
        break;
        default:
            qDebug() << "haveSellFactoryObject(stat) have unknow value";
        break;
    }
    itemsToSell.removeFirst();
}

void BaseWindow::haveFactoryList(const QList<ItemToSellOrBuy> &resources,const QList<ItemToSellOrBuy> &products)
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::haveFactoryList()";
    #endif
    QFont MissingQuantity;
    MissingQuantity.setItalic(true);
    int index;
    ui->factoryResources->clear();
    index=0;
    while(index<resources.size())
    {
        QListWidgetItem *item=new QListWidgetItem();
        item->setData(99,resources.at(index).object);
        item->setData(98,resources.at(index).price);
        if(resources.at(index).quantity>1)
            item->setText(QString::number(resources.at(index).quantity));
        if(DatapackClientLoader::datapackLoader.itemsExtra.contains(resources.at(index).object))
        {
            item->setIcon(DatapackClientLoader::datapackLoader.itemsExtra[resources.at(index).object].image);
            if(resources.at(index).quantity==0)
                item->setToolTip(tr("%1\nPrice: %2$").arg(DatapackClientLoader::datapackLoader.itemsExtra[resources.at(index).object].name).arg(resources.at(index).price));
            else
                item->setToolTip(tr("%1 at %2$\nQuantity: %3").arg(DatapackClientLoader::datapackLoader.itemsExtra[resources.at(index).object].name).arg(resources.at(index).price).arg(resources.at(index).quantity));
        }
        else
        {
            item->setIcon(DatapackClientLoader::datapackLoader.defaultInventoryImage());
            if(resources.at(index).quantity==0)
                item->setToolTip(tr("Item %1\nPrice: %2$").arg(resources.at(index).object).arg(resources.at(index).price));
            else
                item->setToolTip(tr("Item %1 at %2$\nQuantity: %3").arg(resources.at(index).object).arg(resources.at(index).price).arg(resources.at(index).quantity));
        }
        if(!items.contains(resources.at(index).object))
        {
            item->setFont(MissingQuantity);
            item->setForeground(QBrush(QColor(200,20,20)));
        }
        ui->factoryResources->addItem(item);
        index++;
    }
    ui->factoryProducts->clear();
    index=0;
    while(index<products.size())
    {
        QListWidgetItem *item=new QListWidgetItem();
        item->setData(99,products.at(index).object);
        item->setData(98,products.at(index).price);
        if(products.at(index).quantity>1)
            item->setText(QString::number(products.at(index).quantity));
        if(DatapackClientLoader::datapackLoader.itemsExtra.contains(products.at(index).object))
        {
            item->setIcon(DatapackClientLoader::datapackLoader.itemsExtra[products.at(index).object].image);
            if(products.at(index).quantity==0)
                item->setToolTip(tr("%1\nPrice: %2$").arg(DatapackClientLoader::datapackLoader.itemsExtra[products.at(index).object].name).arg(products.at(index).price));
            else
                item->setToolTip(tr("%1 at %2$\nQuantity: %3").arg(DatapackClientLoader::datapackLoader.itemsExtra[products.at(index).object].name).arg(products.at(index).price).arg(products.at(index).quantity));
        }
        else
        {
            item->setIcon(DatapackClientLoader::datapackLoader.defaultInventoryImage());
            if(products.at(index).quantity==0)
                item->setToolTip(tr("Item %1\nPrice: %2$").arg(products.at(index).object).arg(products.at(index).price));
            else
                item->setToolTip(tr("Item %1 at %2$\nQuantity: %3").arg(products.at(index).object).arg(products.at(index).price).arg(products.at(index).quantity));
        }
        if(products.at(index).price>cash)
        {
            item->setFont(MissingQuantity);
            item->setForeground(QBrush(QColor(200,20,20)));
        }
        ui->factoryProducts->addItem(item);
        index++;
    }
    ui->factoryStatus->setText(tr("Have the factory list"));
}

void CatchChallenger::BaseWindow::on_factoryQuit_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_map);
}
