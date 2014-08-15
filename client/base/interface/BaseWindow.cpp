#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "../../general/base/FacilityLib.h"
#include "../ClientVariable.h"
#include "../../fight/interface/ClientFightEngine.h"
#include "../../../general/base/CommonDatapack.h"
#include "DatapackClientLoader.h"
#include "MapController.h"
#include "Chat.h"
#include "WithAnotherPlayer.h"
#include "GetPrice.h"
#include "../LanguagesSelect.h"
#include "../Options.h"
#include "../Audio.h"

#include <QListWidgetItem>
#include <QBuffer>
#include <QInputDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QRegularExpression>
#include <QScriptValue>
#include <QScriptEngine>
#include <QtQml>
#include <QComboBox>
#include <vlc/vlc.h>

//do buy queue
//do sell queue

using namespace CatchChallenger;

BaseWindow* BaseWindow::baseWindow=NULL;

QString BaseWindow::text_type=QLatin1Literal("type");
QString BaseWindow::text_lang=QLatin1Literal("lang");
QString BaseWindow::text_en=QLatin1Literal("en");
QString BaseWindow::text_text=QLatin1Literal("text");
QFile BaseWindow::debugFile;
quint8 BaseWindow::debugFileStatus=0;

BaseWindow::BaseWindow() :
    ui(new Ui::BaseWindowUI)
{
    qRegisterMetaType<CatchChallenger::Chat_type>("CatchChallenger::Chat_type");
    qRegisterMetaType<CatchChallenger::Player_type>("CatchChallenger::Player_type");
    qRegisterMetaType<CatchChallenger::Player_private_and_public_informations>("CatchChallenger::Player_private_and_public_informations");
    qRegisterMetaType<QHash<quint32,quint32> >("QHash<quint32,quint32>");
    qRegisterMetaType<QHash<quint32,quint32> >("CatchChallenger::Plant_collect");
    qRegisterMetaType<QList<ItemToSellOrBuy> >("QList<ItemToSell>");
    qRegisterMetaType<QList<QPair<quint8,quint8> > >("QList<QPair<quint8,quint8> >");
    qRegisterMetaType<Skill::AttackReturn>("Skill::AttackReturn");
    qRegisterMetaType<QList<quint32> >("QList<quint32>");
    qmlRegisterUncreatableType<EvolutionControl>("EvolutionControl", 1, 0, "EvolutionControl","");
    qmlRegisterUncreatableType<AnimationControl>("AnimationControl", 2, 0, "AnimationControl","");

    socketState=QAbstractSocket::UnconnectedState;

    MapController::mapController=new MapController(true,false,true,false);
    if(CatchChallenger::Api_client_real::client!=NULL)
        MapController::mapController->setDatapackPath(CatchChallenger::Api_client_real::client->datapackPath());
    ProtocolParsing::initialiseTheVariable();
    ui->setupUi(this);
    animationWidget=NULL;
    qQuickViewContainer=NULL;
    monsterBeforeMoveForChangeInWaiting=false;
    Chat::chat=new Chat(ui->page_map);
    escape=false;
    multiplayer=false;
    movie=NULL;
    newProfile=NULL;
    lastStepUsed=0;
    craftingAnimationObject=NULL;
    #ifdef CATCHCHALLENGER_VERSION_ULTIMATE
    ui->label_ultimate->setVisible(false);
    #endif
    {
        const QList<QByteArray> &supportedImageFormats=QImageReader::supportedImageFormats();
        int index=0;
        while(index<supportedImageFormats.size())
        {
            this->supportedImageFormats << QString(supportedImageFormats.at(index));
            index++;
        }
    }

    checkQueryTime.start(200);
    connect(&checkQueryTime,&QTimer::timeout,   this,&BaseWindow::detectSlowDown);
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
    displayExpTimer.setSingleShot(true);
    displayExpTimer.setInterval(20);
    displayTrapTimer.setSingleShot(true);
    displayTrapTimer.setInterval(50);
    doNextActionTimer.setSingleShot(true);
    doNextActionTimer.setInterval(1500);

    connect(this,&BaseWindow::sendsetMultiPlayer,Chat::chat,&Chat::setVisible,Qt::QueuedConnection);

    //connect the datapack loader
    connect(&DatapackClientLoader::datapackLoader,  &DatapackClientLoader::datapackParsed,  this,                                   &BaseWindow::datapackParsed,Qt::QueuedConnection);
    connect(&DatapackClientLoader::datapackLoader,  &DatapackClientLoader::datapackChecksumError,  this,                            &BaseWindow::datapackChecksumError,Qt::QueuedConnection);
    connect(this,                                   &BaseWindow::parseDatapack,             &DatapackClientLoader::datapackLoader,  &DatapackClientLoader::parseDatapack,Qt::QueuedConnection);
    connect(&DatapackClientLoader::datapackLoader,  &DatapackClientLoader::datapackParsed,  MapController::mapController,           &MapController::datapackParsed,Qt::QueuedConnection);

    //render, logical part into Map_Client
    connect(MapController::mapController,&MapController::stopped_in_front_of,   this,&BaseWindow::stopped_in_front_of);
    connect(MapController::mapController,&MapController::actionOn,              this,&BaseWindow::actionOn);
    connect(MapController::mapController,&MapController::actionOnNothing,       this,&BaseWindow::actionOnNothing);
    connect(MapController::mapController,&MapController::blockedOn,             this,&BaseWindow::blockedOn);
    connect(MapController::mapController,&MapController::error,                 this,&BaseWindow::error);
    connect(MapController::mapController,&MapController::errorWithTheCurrentMap,this,&BaseWindow::errorWithTheCurrentMap);
    connect(MapController::mapController,&MapController::repelEffectIsOver,     this,&BaseWindow::repelEffectIsOver);
    connect(MapController::mapController,&MapController::send_player_direction, this,&BaseWindow::send_player_direction,Qt::QueuedConnection);
    connect(MapController::mapController,&MapController::teleportConditionNotRespected, this,&BaseWindow::teleportConditionNotRespected,Qt::QueuedConnection);

    //fight
    connect(MapController::mapController,   &MapController::wildFightCollision,     this,&BaseWindow::wildFightCollision);
    connect(MapController::mapController,   &MapController::botFightCollision,      this,&BaseWindow::botFightCollision);
    connect(MapController::mapController,   &MapController::currentMapLoaded,       this,&BaseWindow::currentMapLoaded);
    connect(&moveFightMonsterBottomTimer,   &QTimer::timeout,                       this,&BaseWindow::moveFightMonsterBottom);
    connect(&moveFightMonsterTopTimer,      &QTimer::timeout,                       this,&BaseWindow::moveFightMonsterTop);
    connect(&moveFightMonsterBothTimer,     &QTimer::timeout,                       this,&BaseWindow::moveFightMonsterBoth);
    connect(&displayAttackTimer,            &QTimer::timeout,                       this,&BaseWindow::displayAttack);
    connect(&displayExpTimer,               &QTimer::timeout,                       this,&BaseWindow::displayExperienceGain);
    connect(&displayTrapTimer,              &QTimer::timeout,                       this,&BaseWindow::displayTrap);
    connect(&doNextActionTimer,             &QTimer::timeout,                       this,&BaseWindow::doNextAction);
    connect(ui->stackedWidget,              &QStackedWidget::currentChanged,        this,&BaseWindow::pageChanged);


    connect(&CatchChallenger::ClientFightEngine::fightEngine,&ClientFightEngine::newError,  this,&BaseWindow::newError);
    connect(&CatchChallenger::ClientFightEngine::fightEngine,&ClientFightEngine::error,     this,&BaseWindow::error);
    connect(&CatchChallenger::ClientFightEngine::fightEngine,&ClientFightEngine::errorFightEngine,     this,&BaseWindow::error);

    connect(&updateRXTXTimer,&QTimer::timeout,          this,&BaseWindow::updateRXTX);

    connect(&tip_timeout,&QTimer::timeout,              this,&BaseWindow::tipTimeout);
    connect(&gain_timeout,&QTimer::timeout,             this,&BaseWindow::gainTimeout);
    connect(&nextCityCatchTimer,&QTimer::timeout,     this,&BaseWindow::cityCaptureUpdateTime);
    connect(&updater_page_zonecatch,&QTimer::timeout, this,&BaseWindow::updatePageZoneCatch);
    connect(&animationControl,&AnimationControl::animationFinished,this,&BaseWindow::animationFinished,Qt::QueuedConnection);

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
    nextCityCatchTimer.setSingleShot(true);
    updater_page_zonecatch.setSingleShot(false);

    Chat::chat->setGeometry(QRect(0, 0, 250, 400));

    resetAll();
    loadSettings();

    MapController::mapController->setFocus();

    /// \todo able to cancel quest
    ui->cancelQuest->hide();
    loadSoundSettings();
    qInstallMessageHandler(&BaseWindow::customMessageHandler);
}

BaseWindow::~BaseWindow()
{
    /*if(craftingAnimationObject!=NULL)
    {
        craftingAnimationObject->deleteLater();
        craftingAnimationObject=NULL;
    }*/
    debugFile.flush();
    if(newProfile!=NULL)
    {
        delete newProfile;
        newProfile=NULL;
    }
    while(!ambianceList.isEmpty())
    {
        libvlc_media_player_stop(ambianceList.first().player);
        libvlc_media_player_release(ambianceList.first().player);
        ambianceList.removeFirst();
    }
    if(movie!=NULL)
        delete movie;
    delete ui;
    delete MapController::mapController;
    MapController::mapController=NULL;
    delete Chat::chat;
    Chat::chat=NULL;
}

void BaseWindow::connectAllSignals()
{
    MapController::mapController->connectAllSignals();

    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::lastReplyTime,      this,&BaseWindow::lastReplyTime,    Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::protocol_is_good,   this,&BaseWindow::protocol_is_good, Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_protocol::disconnected,          this,&BaseWindow::disconnected,     Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::newError,           this,&BaseWindow::newError,         Qt::QueuedConnection);
    //connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::message,          this,&BaseWindow::message,          Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::notLogged,          this,&BaseWindow::notLogged,        Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::logged,             this,&BaseWindow::logged,           Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::haveTheDatapack,    this,&BaseWindow::haveTheDatapack,  Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::newError,           this,&BaseWindow::newError,         Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::clanActionFailed,   this,&BaseWindow::clanActionFailed, Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::clanActionSuccess,  this,&BaseWindow::clanActionSuccess,Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::clanDissolved,      this,&BaseWindow::clanDissolved,    Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::clanInformations,   this,&BaseWindow::clanInformations, Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::clanInvite,         this,&BaseWindow::clanInvite,       Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::cityCapture,        this,&BaseWindow::cityCapture,      Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::setEvents,          this,&BaseWindow::setEvents,        Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::newEvent,           this,&BaseWindow::newEvent,         Qt::QueuedConnection);

    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::captureCityYourAreNotLeader,                this,&BaseWindow::captureCityYourAreNotLeader,              Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::captureCityYourLeaderHaveStartInOtherCity,  this,&BaseWindow::captureCityYourLeaderHaveStartInOtherCity,Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::captureCityPreviousNotFinished,             this,&BaseWindow::captureCityPreviousNotFinished,           Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::captureCityStartBattle,                     this,&BaseWindow::captureCityStartBattle,                   Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::captureCityStartBotFight,                   this,&BaseWindow::captureCityStartBotFight,                 Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::captureCityDelayedStart,                    this,&BaseWindow::captureCityDelayedStart,                  Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::captureCityWin,                             this,&BaseWindow::captureCityWin,                           Qt::QueuedConnection);

    //connect the map controler
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::have_current_player_info,this,&BaseWindow::have_current_player_info,Qt::QueuedConnection);

    //inventory
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::have_inventory,     this,&BaseWindow::have_inventory);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::add_to_inventory,   this,&BaseWindow::add_to_inventory_slot);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::remove_to_inventory,this,&BaseWindow::remove_to_inventory_slot);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::monsterCatch,       this,&BaseWindow::monsterCatch);

    //character
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::newCharacterId,     this,&BaseWindow::newCharacterId);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::haveCharacter,     this,&BaseWindow::have_current_player_info);

    //chat
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::new_chat_text,  Chat::chat,&Chat::new_chat_text);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::new_system_text,Chat::chat,&Chat::new_system_text);

    //plants
    connect(this,&BaseWindow::useSeed,              CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::useSeed);
    connect(this,&BaseWindow::collectMaturePlant,   CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::collectMaturePlant);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::insert_plant,   this,&BaseWindow::insert_plant);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::remove_plant,   this,&BaseWindow::remove_plant);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::seed_planted,   this,&BaseWindow::seed_planted);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::plant_collected,this,&BaseWindow::plant_collected);
    //crafting
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::recipeUsed,     this,&BaseWindow::recipeUsed);
    //trade
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::tradeRequested,             this,&BaseWindow::tradeRequested);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::tradeAcceptedByOther,       this,&BaseWindow::tradeAcceptedByOther);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::tradeCanceledByOther,       this,&BaseWindow::tradeCanceledByOther);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::tradeFinishedByOther,       this,&BaseWindow::tradeFinishedByOther);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::tradeValidatedByTheServer,  this,&BaseWindow::tradeValidatedByTheServer);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::tradeAddTradeCash,          this,&BaseWindow::tradeAddTradeCash);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::tradeAddTradeObject,        this,&BaseWindow::tradeAddTradeObject);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::tradeAddTradeMonster,       this,&BaseWindow::tradeAddTradeMonster);
    //inventory
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::objectUsed,                 this,&BaseWindow::objectUsed);
    //shop
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::haveShopList,               this,&BaseWindow::haveShopList);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::haveSellObject,             this,&BaseWindow::haveSellObject);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::haveBuyObject,              this,&BaseWindow::haveBuyObject);
    //factory
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::haveFactoryList,            this,&BaseWindow::haveFactoryList);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::haveSellFactoryObject,      this,&BaseWindow::haveSellFactoryObject);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::haveBuyFactoryObject,       this,&BaseWindow::haveBuyFactoryObject);
    //battle
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::battleRequested,            this,&BaseWindow::battleRequested);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::battleAcceptedByOther,      this,&BaseWindow::battleAcceptedByOther);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::battleCanceledByOther,      this,&BaseWindow::battleCanceledByOther);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::sendBattleReturn,           this,&BaseWindow::sendBattleReturn);
    //market
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::marketList,                 this,&BaseWindow::marketList);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::marketBuy,                  this,&BaseWindow::marketBuy);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::marketBuyMonster,           this,&BaseWindow::marketBuyMonster);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::marketPut,                  this,&BaseWindow::marketPut);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::marketGetCash,              this,&BaseWindow::marketGetCash);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::marketWithdrawCanceled,     this,&BaseWindow::marketWithdrawCanceled);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::marketWithdrawObject,       this,&BaseWindow::marketWithdrawObject);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::marketWithdrawMonster,      this,&BaseWindow::marketWithdrawMonster);
    //datapack
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::datapackSize,this,&BaseWindow::datapackSize);
    #ifdef CATCHCHALLENGER_MULTI
    connect(static_cast<CatchChallenger::Api_client_real*>(CatchChallenger::Api_client_real::client),&CatchChallenger::Api_client_real::newDatapackFile,            this,&BaseWindow::newDatapackFile);
    connect(static_cast<CatchChallenger::Api_client_real*>(CatchChallenger::Api_client_real::client),&CatchChallenger::Api_client_real::progressingDatapackFile,this,&BaseWindow::progressingDatapackFile);
    #endif

    connect(this,&BaseWindow::destroyObject,CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::destroyObject);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::teleportTo,this,&BaseWindow::teleportTo,Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::number_of_player,this,&BaseWindow::number_of_player);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::random_seeds,&ClientFightEngine::fightEngine,&ClientFightEngine::newRandomNumber);

    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::insert_player,              this,&BaseWindow::insert_player,Qt::QueuedConnection);
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

    const QPixmap skin(getFrontSkinPath(skinInt));
    if(!skin.isNull())
    {
        //reset the other player info
        ui->tradePlayerImage->setVisible(true);
        ui->tradePlayerPseudo->setVisible(true);
        ui->tradeOtherImage->setVisible(true);
        ui->tradeOtherPseudo->setVisible(true);
        ui->tradeOtherImage->setPixmap(skin);
        ui->tradeOtherPseudo->setText(pseudo);
    }
    else
    {
        ui->tradePlayerImage->setVisible(false);
        ui->tradePlayerPseudo->setVisible(false);
        ui->tradeOtherImage->setVisible(false);
        ui->tradeOtherPseudo->setVisible(false);
    }
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
    tradeEvolutionMonsters=tradeOtherMonsters;
    load_monsters();
    tradeOtherObjects.clear();
    tradeCurrentObjects.clear();
    tradeCurrentMonsters.clear();
    tradeOtherMonsters.clear();
    checkEvolution();
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
    QHashIterator<quint16,quint32> i(tradeOtherObjects);
    while (i.hasNext()) {
        i.next();
        ui->tradeOtherItems->addItem(itemToGraphic(i.key(),i.value()));
    }
}

void BaseWindow::tradeUpdateCurrentObject()
{
    ui->tradePlayerItems->clear();
    QHashIterator<quint16,quint32> i(tradeCurrentObjects);
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

QString BaseWindow::lastLocation() const
{
    return MapController::mapController->lastLocation();
}

QHash<quint16, PlayerQuest> BaseWindow::getQuests() const
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

void BaseWindow::message(QString message) const
{
    qDebug() << message;
}

void BaseWindow::number_of_player(quint16 number,quint16 max)
{
    ui->frame_main_display_interface_player->show();
    QString stringMax;
    if(max>1000)
        stringMax=QStringLiteral("%1K").arg(max/1000);
    else
        stringMax=QString::number(max);
    QString stringNumber;
    if(number>1000)
        stringNumber=QStringLiteral("%1K").arg(number/1000);
    else
        stringNumber=QString::number(number);
    ui->label_interface_number_of_player->setText(QStringLiteral("%1/%2").arg(stringNumber).arg(stringMax));
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
        case ObjectType_MonsterToTradeToMarket:
        case ObjectType_MonsterToLearn:
        case ObjectType_MonsterToFight:
        case ObjectType_MonsterToFightKO:
        case ObjectType_ItemOnMonster:
        case ObjectType_ItemOnMonsterOutOfFight:
        case ObjectType_ItemEvolutionOnMonster:
        case ObjectType_ItemLearnOnMonster:
            ui->selectMonster->setVisible(true);
            ui->stackedWidget->setCurrentWidget(ui->page_monster);
            load_monsters();
        break;
        case ObjectType_SellToMarket:
        case ObjectType_All:
        case ObjectType_Trade:
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
        default:
            emit error("unknown selection type");
        return;
    }
}

void BaseWindow::objectSelection(const bool &ok, const quint16 &itemId, const quint32 &quantity)
{
    inSelection=false;
    ObjectType tempWaitedObjectType=waitedObjectType;
    waitedObjectType=ObjectType_All;
    switch(tempWaitedObjectType)
    {
        case ObjectType_ItemEvolutionOnMonster:
        case ObjectType_ItemLearnOnMonster:
        case ObjectType_ItemOnMonster:
        case ObjectType_ItemOnMonsterOutOfFight:
        {
            const quint32 monsterUniqueId=itemId;
            const quint32 item=objectInUsing.last();
            objectInUsing.removeLast();
            if(!ok)
            {
                ui->stackedWidget->setCurrentWidget(ui->page_inventory);
                ui->inventoryUse->setText(tr("Select"));
                if(CatchChallenger::CommonDatapack::commonDatapack.items.item.contains(item))
                    if(CatchChallenger::CommonDatapack::commonDatapack.items.item[item].consumeAtUse)
                        add_to_inventory(item,1,false);
                break;
            }
            if(CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.contains(item))
            {
                const PlayerMonster &monster=*ClientFightEngine::fightEngine.monsterById(monsterUniqueId);
                const Monster &monsterInformations=CommonDatapack::commonDatapack.monsters.value(monster.monster);
                const DatapackClientLoader::MonsterExtra &monsterInformationsExtra=DatapackClientLoader::datapackLoader.monsterExtra.value(monster.monster);
                idMonsterEvolution=0;
                const Monster &monsterInformationsEvolution=CommonDatapack::commonDatapack.monsters.value(CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.value(item).value(monster.monster));
                const DatapackClientLoader::MonsterExtra &monsterInformationsEvolutionExtra=DatapackClientLoader::datapackLoader.monsterExtra.value(CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.value(item).value(monster.monster));
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
                ui->stackedWidget->setCurrentWidget(ui->page_animation);
                previousAnimationWidget=ui->page_map;
                if(baseMonsterEvolution!=NULL)
                    delete baseMonsterEvolution;
                if(targetMonsterEvolution!=NULL)
                    delete targetMonsterEvolution;
                baseMonsterEvolution=new QmlMonsterGeneralInformations(monsterInformations,monsterInformationsExtra);
                targetMonsterEvolution=new QmlMonsterGeneralInformations(monsterInformationsEvolution,monsterInformationsEvolutionExtra);
                if(evolutionControl!=NULL)
                    delete evolutionControl;
                evolutionControl=new EvolutionControl(monsterInformations,monsterInformationsExtra,monsterInformationsEvolution,monsterInformationsEvolutionExtra);
                animationWidget->rootContext()->setContextProperty("animationControl",&animationControl);
                animationWidget->rootContext()->setContextProperty("evolutionControl",evolutionControl);
                animationWidget->rootContext()->setContextProperty("canBeCanceled",QVariant(false));
                animationWidget->rootContext()->setContextProperty("itemEvolution",QUrl::fromLocalFile(DatapackClientLoader::datapackLoader.itemsExtra.value(item).imagePath));
                animationWidget->rootContext()->setContextProperty("baseMonsterEvolution",baseMonsterEvolution);
                animationWidget->rootContext()->setContextProperty("targetMonsterEvolution",targetMonsterEvolution);
                const QString datapackQmlFile=CatchChallenger::Api_client_real::client->datapackPath()+"qml/evolution-animation.qml";
                if(QFile(datapackQmlFile).exists())
                    animationWidget->setSource(QUrl::fromLocalFile(datapackQmlFile));
                else
                    animationWidget->setSource(QStringLiteral("qrc:/qml/evolution-animation.qml"));
                CatchChallenger::Api_client_real::client->useObjectOnMonster(item,monsterUniqueId);
                ClientFightEngine::fightEngine.useObjectOnMonster(item,monsterUniqueId);
                load_monsters();
                return;
            }
            else
            {
                ui->stackedWidget->setCurrentWidget(ui->page_inventory);
                ui->inventoryUse->setText(tr("Select"));
                if(ClientFightEngine::fightEngine.useObjectOnMonster(item,monsterUniqueId))
                {
                    showTip(tr("Using %1 on %2").arg(DatapackClientLoader::datapackLoader.itemsExtra.value(item).name).arg(DatapackClientLoader::datapackLoader.monsterExtra.value(monsterUniqueId).name));
                    CatchChallenger::Api_client_real::client->useObjectOnMonster(item,monsterUniqueId);
                    load_monsters();
                }
                else
                {
                    showTip(tr("Failed to use %1 on %2").arg(DatapackClientLoader::datapackLoader.itemsExtra.value(item).name).arg(DatapackClientLoader::datapackLoader.monsterExtra.value(monsterUniqueId).name));
                    if(CatchChallenger::CommonDatapack::commonDatapack.items.item.contains(item))
                        if(CatchChallenger::CommonDatapack::commonDatapack.items.item[item].consumeAtUse)
                            add_to_inventory(item,1,false);
                }
            }
        }
        break;
        case ObjectType_Sell:
        {
            ui->stackedWidget->setCurrentWidget(ui->page_map);
            ui->inventoryUse->setText(tr("Select"));
            if(!ok)
                break;
            if(!items.contains(itemId))
            {
                qDebug() << "item id is not into the inventory";
                break;
            }
            if(items.value(itemId)<quantity)
            {
                qDebug() << "item id have not the quantity";
                break;
            }
            remove_to_inventory(itemId,quantity);
            ItemToSellOrBuy tempItem;
            tempItem.object=itemId;
            tempItem.quantity=quantity;
            tempItem.price=CatchChallenger::CommonDatapack::commonDatapack.items.item.value(itemId).price/2;
            itemsToSell << tempItem;
            CatchChallenger::Api_client_real::client->sellObject(shopId,tempItem.object,tempItem.quantity,tempItem.price);
            load_inventory();
            load_plant_inventory();
        }
        break;
        case ObjectType_SellToMarket:
        {
            ui->inventoryUse->setText(tr("Select"));
            ui->stackedWidget->setCurrentWidget(ui->page_market);
            if(!ok)
                break;
            if(!items.contains(itemId))
            {
                qDebug() << "item id is not into the inventory";
                break;
            }
            if(items.value(itemId)<quantity)
            {
                qDebug() << "item id have not the quantity";
                break;
            }
            quint32 suggestedPrice=50;
            if(CommonDatapack::commonDatapack.items.item.contains(itemId))
                suggestedPrice=CommonDatapack::commonDatapack.items.item.value(itemId).price;
            GetPrice getPrice(this,suggestedPrice);
            getPrice.exec();
            if(!getPrice.isOK())
                break;
            CatchChallenger::Api_client_real::client->putMarketObject(itemId,quantity,getPrice.price());
            marketPutCashInSuspend=getPrice.price();
            remove_to_inventory(itemId,quantity);
            QPair<quint16,quint32> pair;
            pair.first=itemId;
            pair.second=quantity;
            marketPutObjectInSuspendList << pair;
            load_inventory();
            load_plant_inventory();
        }
        break;
        case ObjectType_Trade:
            ui->inventoryUse->setText(tr("Select"));
            ui->stackedWidget->setCurrentWidget(ui->page_trade);
            if(!ok)
                break;
            if(!items.contains(itemId))
            {
                qDebug() << "item id is not into the inventory";
                break;
            }
            if(items.value(itemId)<quantity)
            {
                qDebug() << "item id have not the quantity";
                break;
            }
            CatchChallenger::Api_client_real::client->addObject(itemId,quantity);
            items[itemId]-=quantity;
            if(items.value(itemId)==0)
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
            ui->stackedWidget->setCurrentWidget(ui->page_map);
            ui->inventoryUse->setText(tr("Select"));
            load_monsters();
            if(!ok)
                return;
            ui->stackedWidget->setCurrentWidget(ui->page_learn);
            monsterToLearn=itemId;
            if(!showLearnSkill(monsterToLearn))
            {
                newError(tr("Internal error"),"Unable to load the right monster");
                return;
            }
        }
        break;
        case ObjectType_MonsterToFight:
        case ObjectType_MonsterToFightKO:
        {
            ui->inventoryUse->setText(tr("Select"));
            ui->stackedWidget->setCurrentWidget(ui->page_battle);
            load_monsters();
            if(!ok)
                return;
            resetPosition(true,false,true);
            //do copie here because the call of changeOfMonsterInFight apply the skill/buff effect
            const PlayerMonster *tempMonster=ClientFightEngine::fightEngine.monsterById(itemId);
            if(tempMonster==NULL)
            {
                qDebug() << "Monster not found";
                return;
            }
            PlayerMonster copiedMonster=*tempMonster;
            if(!ClientFightEngine::fightEngine.changeOfMonsterInFight(itemId))
                return;
            CatchChallenger::Api_client_real::client->changeOfMonsterInFight(itemId);
            PlayerMonster * playerMonster=ClientFightEngine::fightEngine.getCurrentMonster();
            init_current_monster_display(&copiedMonster);
            ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageEnter);
            if(DatapackClientLoader::datapackLoader.monsterExtra.contains(playerMonster->monster))
            {
                ui->labelFightEnter->setText(tr("Go %1").arg(DatapackClientLoader::datapackLoader.monsterExtra.value(playerMonster->monster).name));
                ui->labelFightMonsterBottom->setPixmap(DatapackClientLoader::datapackLoader.monsterExtra.value(playerMonster->monster).back.scaled(160,160));
            }
            else
            {
                ui->labelFightEnter->setText(tr("You change of monster"));
                ui->labelFightMonsterBottom->setPixmap(QPixmap(":/images/monsters/default/back.png"));
            }
            ui->pushButtonFightEnterNext->setVisible(false);
            moveType=MoveType_Enter;
            battleStep=BattleStep_Presentation;
            monsterBeforeMoveForChangeInWaiting=true;
            moveFightMonsterBottom();
        }
        break;
        case ObjectType_MonsterToTradeToMarket:
        {
            ui->inventoryUse->setText(tr("Select"));
            ui->stackedWidget->setCurrentWidget(ui->page_market);
            load_monsters();
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
                    GetPrice getPrice(this,15000);
                    getPrice.exec();
                    if(!getPrice.isOK())
                        break;
                    marketPutMonsterList << playerMonster.at(index);
                    marketPutMonsterPlaceList << index;
                    ClientFightEngine::fightEngine.removeMonster(itemId);
                    CatchChallenger::Api_client_real::client->putMarketMonster(itemId,getPrice.price());
                    marketPutCashInSuspend=getPrice.price();
                    break;
                }
                index++;
            }
        }
        break;
        case ObjectType_MonsterToTrade:
        {
            ui->inventoryUse->setText(tr("Select"));
            load_monsters();
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
                    item->setText(DatapackClientLoader::datapackLoader.monsterExtra.value(tradeCurrentMonsters.last().monster).name);
                    item->setToolTip(tr("Level: %1").arg(tradeCurrentMonsters.last().level));
                    item->setIcon(DatapackClientLoader::datapackLoader.monsterExtra.value(tradeCurrentMonsters.last().monster).front);
                    ui->tradePlayerMonsters->addItem(item);
                    break;
                }
                index++;
            }
        }
        break;
        case ObjectType_Seed:
        {
            if(havePlant(&MapController::mapController->getMap(MapController::mapController->current_map)->logicalMap,MapController::mapController->getX(),MapController::mapController->getY())>=0)
            {
                qDebug() << "Too slow to select a seed, have plant now";
                showTip(tr("Sorry, but now the dirt is not free to plant"));
                return;
            }
            ui->stackedWidget->setCurrentWidget(ui->page_map);
            ui->inventoryUse->setText(tr("Select"));
            ui->plantUse->setVisible(false);
            if(!ok)
                break;
            if(!items.contains(itemId))
            {
                qDebug() << "item id is not into the inventory";
                break;
            }
            if(!haveReputationRequirements(CatchChallenger::CommonDatapack::commonDatapack.crafingRecipes.value(CatchChallenger::CommonDatapack::commonDatapack.itemToCrafingRecipes.value(itemId)).requirements.reputation))
            {
                qDebug() << "You don't have the requirements to plant the seed";
                showTip(tr("You don't have the requirements to plant the seed"));
                return;
            }
            remove_to_inventory(itemId);
            SeedInWaiting seedInWaiting;
            seedInWaiting.map=MapController::mapController->current_map;
            seedInWaiting.x=MapController::mapController->getX();
            seedInWaiting.y=MapController::mapController->getY();
            seedInWaiting.seed=itemId;
            seed_in_waiting << seedInWaiting;
            const quint8 &plant=DatapackClientLoader::datapackLoader.itemToPlants.value(itemId);
            insert_plant(MapController::mapController->getMap(MapController::mapController->current_map)->logicalMap.id,seedInWaiting.x,seedInWaiting.y,plant,CommonDatapack::commonDatapack.plants.value(plant).fruits_seconds);
            addQuery(QueryType_Seed);
            if(DatapackClientLoader::datapackLoader.itemToPlants.contains(itemId))
            {
                load_plant_inventory();
                load_inventory();
                qDebug() << QStringLiteral("send seed for: %1").arg(plant);
                emit useSeed(plant);
            }
            else
                qDebug() << QStringLiteral("seed not found for item: %1").arg(itemId);
        }
        break;
        case ObjectType_UseInFight:
        {
            ui->inventoryUse->setText(tr("Select"));
            ui->stackedWidget->setCurrentWidget(ui->page_battle);
            load_inventory();
            if(!ok)
                break;
            if(!items.contains(itemId))
            {
                qDebug() << "item id is not into the inventory";
                break;
            }
            if(items.value(itemId)<quantity)
            {
                qDebug() << "item id have not the quantity";
                break;
            }
            remove_to_inventory(itemId);
            if(CatchChallenger::ClientFightEngine::fightEngine.isInFightWithWild() && CommonDatapack::commonDatapack.items.trap.contains(itemId))
                useTrap(itemId);
            else if(CommonDatapack::commonDatapack.items.monsterItemEffect.contains(itemId))
            {
                const quint32 &monsterUniqueId=ClientFightEngine::fightEngine.getCurrentMonster()->monster;
                CatchChallenger::Api_client_real::client->useObjectOnMonster(itemId,monsterUniqueId);
                ClientFightEngine::fightEngine.useObjectOnMonster(itemId,monsterUniqueId);
            }
            else
                error("Select a buggy object");
        }
        break;
        default:
        qDebug() << "waitedObjectType is unknow";
        return;
    }
    waitedObjectType=ObjectType_All;
}

void BaseWindow::add_to_inventory_slot(const QHash<quint16,quint32> &items)
{
    add_to_inventory(items);
}

void BaseWindow::add_to_inventory(const quint32 &item,const quint32 &quantity,const bool &showGain)
{
    QList<QPair<quint16,quint32> > items;
    items << QPair<quint16,quint32>(item,quantity);
    add_to_inventory(items,showGain);
}

void BaseWindow::add_to_inventory(const QList<QPair<quint16,quint32> > &items,const bool &showGain)
{
    int index=0;
    QHash<quint16,quint32> tempHash;
    while(index<items.size())
    {
        tempHash[items.at(index).first]=items.at(index).second;
        index++;
    }
    add_to_inventory(tempHash,showGain);
}

void BaseWindow::add_to_inventory(const QHash<quint16,quint32> &items,const bool &showGain)
{
    if(items.empty())
        return;
    if(showGain)
    {
        QString html=tr("You have obtained: ");
        QStringList objects;
        QHashIterator<quint16,quint32> i(items);
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
                image=DatapackClientLoader::datapackLoader.itemsExtra.value(i.key()).image;
                name=DatapackClientLoader::datapackLoader.itemsExtra.value(i.key()).name;
            }
            else
            {
                image=DatapackClientLoader::datapackLoader.defaultInventoryImage();
                name=QStringLiteral("id: %1").arg(i.key());
            }

            image=image.scaled(24,24);
            QByteArray byteArray;
            QBuffer buffer(&byteArray);
            image.save(&buffer, "PNG");
            if(objects.size()<16)
            {
                if(i.value()>1)
                    objects << QStringLiteral("<b>%2x</b> %3 <img src=\"data:image/png;base64,%1\" />").arg(QString(byteArray.toBase64())).arg(i.value()).arg(name);
                else
                    objects << QStringLiteral("%2 <img src=\"data:image/png;base64,%1\" />").arg(QString(byteArray.toBase64())).arg(name);
            }
        }
        if(objects.size()==16)
            objects << "...";
        html+=objects.join(", ");
        BaseWindow::showGain(html);
    }
    else
    {
        //add without show
        QHashIterator<quint16,quint32> i(items);
        while (i.hasNext()) {
            i.next();
            //add really to the list
            if(this->items.contains(i.key()))
                this->items[i.key()]+=i.value();
            else
                this->items[i.key()]=i.value();
        }
    }

    load_inventory();
    load_plant_inventory();
    on_listCraftingList_itemSelectionChanged();
}

void BaseWindow::remove_to_inventory(const quint32 &itemId,const quint32 &quantity)
{
    QHash<quint16,quint32> items;
    items[itemId]=quantity;
    remove_to_inventory(items);
}

void BaseWindow::remove_to_inventory_slot(const QHash<quint16,quint32> &items)
{
    remove_to_inventory(items);
}

void BaseWindow::remove_to_inventory(const QHash<quint16,quint32> &items)
{
    QHashIterator<quint16,quint32> i(items);
    while (i.hasNext()) {
        i.next();

        //add really to the list
        if(this->items.contains(i.key()))
        {
            if(this->items.value(i.key())<=i.value())
                this->items.remove(i.key());
            else
                this->items[i.key()]-=i.value();
        }
    }
    load_inventory();
    load_plant_inventory();
}

void BaseWindow::error(QString error)
{
    emit newError(tr("Error with the protocol"),error);
}

void BaseWindow::errorWithTheCurrentMap()
{
    if(socketState!=QAbstractSocket::ConnectedState)
        return;
    CatchChallenger::Api_client_real::client->tryDisconnect();
    resetAll();
    QMessageBox::critical(this,tr("Map error"),tr("The current map into the datapack is in error (not found, read failed, wrong format, corrupted, ...)\nReport the bug to the datapack maintainer."));
}

void BaseWindow::repelEffectIsOver()
{
    showTip(tr("The repel effect is over"));
}

void BaseWindow::send_player_direction(const CatchChallenger::Direction &the_direction)
{
    Q_UNUSED(the_direction);
    ui->IG_dialog->setVisible(false);
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
    if(!DatapackClientLoader::datapackLoader.itemsExtra.contains(items_graphical.value(item)))
    {
        ui->inventoryInformation->setVisible(false);
        ui->inventoryUse->setVisible(false);
        ui->inventoryDestroy->setVisible(!inSelection);
        ui->inventory_image->setPixmap(DatapackClientLoader::datapackLoader.defaultInventoryImage());
        ui->inventory_name->setText(tr("Unknown name"));
        ui->inventory_description->setText(tr("Unknown description"));
        return;
    }
    const DatapackClientLoader::ItemExtra &content=DatapackClientLoader::datapackLoader.itemsExtra.value(items_graphical.value(item));
    ui->inventoryDestroy->setVisible(!inSelection);
    ui->inventory_image->setPixmap(content.image);
    ui->inventory_name->setText(content.name);
    ui->inventory_description->setText(content.description);

    ui->inventoryInformation->setVisible(!inSelection &&
                                         /* is a plant */
                                         DatapackClientLoader::datapackLoader.itemToPlants.contains(items_graphical.value(item))
                                         );
    bool isRecipe=false;
    {
        /* is a recipe */
        isRecipe=CatchChallenger::CommonDatapack::commonDatapack.itemToCrafingRecipes.contains(items_graphical.value(item));
        if(isRecipe)
        {
            if(!haveReputationRequirements(CatchChallenger::CommonDatapack::commonDatapack.crafingRecipes.value(CatchChallenger::CommonDatapack::commonDatapack.itemToCrafingRecipes.value(items_graphical.value(item))).requirements.reputation))
            {
                ui->inventory_description->setText(ui->inventory_description->text()+"<br />"+tr("Don't meet the requirements"));
                isRecipe=false;
            }
        }
    }
    ui->inventoryUse->setVisible(inSelection ||
                                 isRecipe
                                 ||
                                 /* is a repel */
                                 CatchChallenger::CommonDatapack::commonDatapack.items.repel.contains(items_graphical.value(item))
                                 ||
                                 /* is a item with monster effect */
                                 CatchChallenger::CommonDatapack::commonDatapack.items.monsterItemEffect.contains(items_graphical.value(item))
                                 ||
                                 /* is a item with monster effect out of fight */
                                 (CatchChallenger::CommonDatapack::commonDatapack.items.monsterItemEffectOutOfFight.contains(items_graphical.value(item)) && !ClientFightEngine::fightEngine.isInFight())
                                 ||
                                 /* is a evolution item */
                                 CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.contains(items_graphical.value(item))
                                 ||
                                 /* is a evolution item */
                                 (CatchChallenger::CommonDatapack::commonDatapack.items.itemToLearn.contains(items_graphical.value(item)) && !ClientFightEngine::fightEngine.isInFight())
                                         );
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

void BaseWindow::showPlace(const QString &place)
{
    ui->gain->setVisible(true);
    ui->gain->setText(place);
    gain_timeout.start();
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
                queryStringList << tr("Unknown action...");
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
            if(map->plantList.at(index)->x==x && map->plantList.at(index)->y==y)
            {
                quint64 current_time=QDateTime::currentMSecsSinceEpoch()/1000;
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

bool BaseWindow::stopped_in_front_of_check_bot(CatchChallenger::Map_client *map, quint8 x, quint8 y)
{
    if(!map->bots.contains(QPair<quint8,quint8>(x,y)))
        return false;
    showTip(tr("To interact with the bot press <i><b>Enter</b></i>"));
    return true;
}

//return -1 if not found, else the index
qint32 BaseWindow::havePlant(CatchChallenger::Map_client *map, quint8 x, quint8 y) const
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

void BaseWindow::actionOnNothing()
{
    ui->IG_dialog->setVisible(false);
}

void BaseWindow::actionOn(Map_client *map, quint8 x, quint8 y)
{
    if(ui->IG_dialog->isVisible())
        ui->IG_dialog->setVisible(false);
    if(actionOnCheckBot(map,x,y))
        return;
    else if(CatchChallenger::MoveOnTheMap::isDirt(*map,x,y))
    {
        int index=havePlant(map,x,y);
        if(index>=0)
        {
            quint64 current_time=QDateTime::currentMSecsSinceEpoch()/1000;
            if(map->plantList.at(index)->mature_at<=current_time)
            {
                ClientPlantInCollecting clientPlantInCollecting;
                clientPlantInCollecting.map=map->map_file;
                clientPlantInCollecting.plant_id=map->plantList.at(index)->plant_id;
                clientPlantInCollecting.seconds_to_mature=0;
                clientPlantInCollecting.x=map->plantList.at(index)->x;
                clientPlantInCollecting.y=map->plantList.at(index)->y;
                plant_collect_in_waiting << clientPlantInCollecting;
                addQuery(QueryType_CollectPlant);
                emit collectMaturePlant();
            }
            else
                showTip(tr("This plant is growing and can't be collected"));
        }
        else
            selectObject(ObjectType_Seed);
        return;
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

bool BaseWindow::actionOnCheckBot(CatchChallenger::Map_client *map, quint8 x, quint8 y)
{
    if(!map->bots.contains(QPair<quint8,quint8>(x,y)))
        return false;
    actualBot=map->bots.value(QPair<quint8,quint8>(x,y));
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
    actualBot=map->bots.value(QPair<quint8,quint8>(x,y));
    isInQuest=false;
    if(!actualBot.step.contains(step))
    {
        newError(tr("Internal error"),"Bot trigged but no step found");
        return;
    }
    if(actualBot.step.value(step).attribute(BaseWindow::text_type)=="fight")
    {
        if(!actualBot.step.value(step).hasAttribute("fightid"))
        {
            showTip(tr("Bot step missing data error, repport this error please"));
            return;
        }
        bool ok;
        quint32 fightId=actualBot.step.value(step).attribute("fightid").toUInt(&ok);
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
                libvlc_media_player_stop(ambianceList.first().player);
                libvlc_media_player_release(ambianceList.first().player);
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
                libvlc_media_player_stop(ambianceList.first().player);
                libvlc_media_player_release(ambianceList.first().player);
                ambianceList.removeFirst();
            }
            if(!noSound)
            {
                if(Audio::audio.vlcInstance)
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
                    }
                }
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

void BaseWindow::newEvent(const quint8 &event,const quint8 &event_value)
{
    if(this->events.at(event)==event_value)
        return;
    if(!MapController::mapController->currentMapIsLoaded())
        return;
    const QString &type=MapController::mapController->currentMapType();
    this->events[event]=event_value;
    //color
    {
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
                        MapController::mapController->setColor(condition.color,15000);
                        break;
                    }
                }
                else
                    qDebug() << QStringLiteral("event for condition out of range: %1 for %2 event(s)").arg(condition.event).arg(events.size());
                index++;
            }
            if(index==conditions.size())
                MapController::mapController->setColor(DatapackClientLoader::datapackLoader.visualCategories.value(type).defaultColor,15000);
        }
        else
            MapController::mapController->setColor(Qt::transparent,15000);
    }
}

//network
void BaseWindow::updateRXTX()
{
    if(CatchChallenger::Api_client_real::client==NULL)
        return;
    int updateRXTXTimeElapsed=updateRXTXTime.elapsed();
    if(updateRXTXTimeElapsed==0)
        return;
    quint64 RXSize=CatchChallenger::Api_client_real::client->getRXSize();
    quint64 TXSize=CatchChallenger::Api_client_real::client->getTXSize();
    if(previousRXSize>RXSize)
        previousRXSize=RXSize;
    if(previousTXSize>TXSize)
        previousTXSize=TXSize;
    double RXSpeed=(RXSize-previousRXSize)*1000/updateRXTXTimeElapsed;
    double TXSpeed=(TXSize-previousTXSize)*1000/updateRXTXTimeElapsed;
    if(RXSpeed<1024)
        ui->labelInput->setText(QStringLiteral("%1B/s").arg(RXSpeed,0,'g',0));
    else if(RXSpeed<10240)
        ui->labelInput->setText(QStringLiteral("%1KB/s").arg(RXSpeed/1024,0,'g',1));
    else
        ui->labelInput->setText(QStringLiteral("%1KB/s").arg(RXSpeed/1024,0,'g',0));
    if(TXSpeed<1024)
        ui->labelOutput->setText(QStringLiteral("%1B/s").arg(TXSpeed,0,'g',0));
    else if(TXSpeed<10240)
        ui->labelOutput->setText(QStringLiteral("%1KB/s").arg(TXSpeed/1024,0,'g',1));
    else
        ui->labelOutput->setText(QStringLiteral("%1KB/s").arg(TXSpeed/1024,0,'g',0));
    #ifdef DEBUG_CLIENT_NETWORK_USAGE
    if(RXSpeed>0 && TXSpeed>0)
        qDebug() << QStringLiteral("received: %1B/s, transmited: %2B/s").arg(RXSpeed).arg(TXSpeed);
    else
    {
        if(RXSpeed>0)
            qDebug() << QStringLiteral("received: %1B/s").arg(RXSpeed);
        if(TXSpeed>0)
            qDebug() << QStringLiteral("transmited: %1B/s").arg(TXSpeed);
    }
    #endif
    updateRXTXTime.restart();
    previousRXSize=RXSize;
    previousTXSize=TXSize;
}

bool BaseWindow::haveNextStepQuestRequirements(const CatchChallenger::Quest &quest) const
{
    #ifdef DEBUG_CLIENT_QUEST
    qDebug() << QStringLiteral("haveNextStepQuestRequirements for quest: %1").arg(questId);
    #endif
    if(!quests.contains(quest.id))
    {
        qDebug() << "step out of range for: " << quest.id;
        return false;
    }
    quint8 step=quests.value(quest.id).step;
    if(step<=0 || step>quest.steps.size())
    {
        qDebug() << "step out of range for: " << quest.id;
        return false;
    }
    #ifdef DEBUG_CLIENT_QUEST
    qDebug() << QStringLiteral("haveNextStepQuestRequirements for quest: %1, step: %2").arg(questId).arg(step);
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
        if(informations.quests.value(quest.id).step!=0)
        {
            #ifdef DEBUG_CLIENT_QUEST
            qDebug() << "can start the quest because is already running: " << questId;
            #endif
            return false;
        }
        if(informations.quests.value(quest.id).finish_one_time && !quest.repeatable)
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
        if(!informations.quests.value(questId).finish_one_time)
        {
            #ifdef DEBUG_CLIENT_QUEST
            qDebug() << "quest never finished: " << questId;
            #endif
            return false;
        }
        index++;
    }
    return haveReputationRequirements(quest.requirements.reputation);
}

void BaseWindow::appendReputationRewards(const QList<ReputationRewards> &reputationList)
{
    int index=0;
    while(index<reputationList.size())
    {
        const ReputationRewards &reputationRewards=reputationList.at(index);
        appendReputationPoint(CommonDatapack::commonDatapack.reputation.at(reputationRewards.reputationId).name,reputationRewards.point);
        index++;
    }
    show_reputation();
}

bool BaseWindow::haveReputationRequirements(const QList<ReputationRequirements> &reputationList) const
{
    int index=0;
    while(index<reputationList.size())
    {
        const CatchChallenger::ReputationRequirements &reputation=reputationList.at(index);
        if(CatchChallenger::Api_client_real::client->player_informations.reputation.contains(reputation.reputationId))
        {
            const PlayerReputation &playerReputation=CatchChallenger::Api_client_real::client->player_informations.reputation.value(reputation.reputationId);
            if(!reputation.positif)
            {
                if(-reputation.level<playerReputation.level)
                {
                    emit message(QStringLiteral("reputation.level(%1)<playerReputation.level(%2)").arg(reputation.level).arg(playerReputation.level));
                    return false;
                }
            }
            else
            {
                if(reputation.level>playerReputation.level || playerReputation.point<0)
                {
                    emit message(QStringLiteral("reputation.level(%1)>playerReputation.level(%2) || playerReputation.point(%3)<0").arg(reputation.level).arg(playerReputation.level).arg(playerReputation.point));
                    return false;
                }
            }
        }
        else
            if(!reputation.positif)//default level is 0, but required level is negative
            {
                emit message(QStringLiteral("reputation.level(%1)<0 and no reputation.type=%2").arg(reputation.level).arg(CommonDatapack::commonDatapack.reputation.at(reputation.reputationId).name));
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
    quint8 step=quests.value(quest.id).step;
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
        QHash<quint16,quint32> items;
        items[item.item]=item.quantity;
        remove_to_inventory(items);
        index++;
    }
    quests[quest.id].step++;
    if(quests.value(quest.id).step>quest.steps.size())
    {
        #ifdef DEBUG_CLIENT_QUEST
        qDebug() << "finish the quest: " << quest.id;
        #endif
        quests[quest.id].step=0;
        quests[quest.id].finish_one_time=true;
        index=0;
        while(index<quest.rewards.reputation.size())
        {
            appendReputationPoint(CommonDatapack::commonDatapack.reputation.at(quest.rewards.reputation.value(index).reputationId).name,quest.rewards.reputation.value(index).point);
            index++;
        }
        show_reputation();
        index=0;
        while(index<quest.rewards.allow.size())
        {
            allow << quest.rewards.allow.value(index);
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
    if(!DatapackClientLoader::datapackLoader.reputationNameToId.contains(type))
    {
        emit error(QStringLiteral("Unknow reputation: %1").arg(type));
        return;
    }
    const quint16 &reputatioId=DatapackClientLoader::datapackLoader.reputationNameToId.value(type);
    PlayerReputation playerReputation;
    if(CatchChallenger::Api_client_real::client->player_informations.reputation.contains(reputatioId))
        playerReputation=CatchChallenger::Api_client_real::client->player_informations.reputation.value(reputatioId);
    else
    {
        playerReputation.point=0;
        playerReputation.level=0;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_REPUTATION
    emit message(QStringLiteral("Reputation %1 at level: %2 with point: %3").arg(type).arg(playerReputation.level).arg(playerReputation.point));
    #endif
    PlayerReputation oldPlayerReputation=playerReputation;
    FacilityLib::appendReputationPoint(&playerReputation,point,CommonDatapack::commonDatapack.reputation.at(reputatioId));
    if(oldPlayerReputation.level==playerReputation.level && oldPlayerReputation.point==playerReputation.point)
        return;
    if(CatchChallenger::Api_client_real::client->player_informations.reputation.contains(reputatioId))
        CatchChallenger::Api_client_real::client->player_informations.reputation[reputatioId]=playerReputation;
    else
        CatchChallenger::Api_client_real::client->player_informations.reputation.insert(reputatioId,playerReputation);
    #ifdef DEBUG_MESSAGE_CLIENT_REPUTATION
    emit message(QStringLiteral("New reputation %1 at level: %2 with point: %3").arg(type).arg(playerReputation.level).arg(playerReputation.point));
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
        const CatchChallenger::Quest &currentQuest=CatchChallenger::CommonDatapack::commonDatapack.quests.value(questId);
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
                if(quests.value(botQuests.at(index)).step==0)
                {
                    if(currentQuest.repeatable)
                    {
                        if(quests.value(botQuests.at(index)).finish_one_time)
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
                    QList<quint16> bots=currentQuest.steps.at(quests.value(questId).step-1).bots;
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
    QHashIterator<quint16, PlayerQuest> i(quests);
    while (i.hasNext()) {
        i.next();
        if(!botQuests.contains(i.key()) && i.value().step>0)
        {
            CatchChallenger::Quest currentQuest=CatchChallenger::CommonDatapack::commonDatapack.quests.value(i.key());
            QList<quint16> bots=currentQuest.steps.at(i.value().step-1).bots;
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
        const CatchChallenger::Quest &currentQuest=CatchChallenger::CommonDatapack::commonDatapack.quests.value(questId);
        if(!quests.contains(botQuests.at(index)))
        {
            //quest not started
            if(haveStartQuestRequirement(currentQuest))
            {
                oneEntry.first=questId;
                if(DatapackClientLoader::datapackLoader.questsExtra.contains(questId))
                    oneEntry.second=DatapackClientLoader::datapackLoader.questsExtra.value(questId).name;
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
                if(quests.value(botQuests.at(index)).step==0)
                {
                    if(currentQuest.repeatable)
                    {
                        if(quests.value(botQuests.at(index)).finish_one_time)
                        {
                            //quest already done but repeatable
                            if(haveStartQuestRequirement(currentQuest))
                            {
                                oneEntry.first=questId;
                                if(DatapackClientLoader::datapackLoader.questsExtra.contains(questId))
                                    oneEntry.second=DatapackClientLoader::datapackLoader.questsExtra.value(questId).name;
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
                    QList<quint16> bots=currentQuest.steps.at(quests.value(questId).step-1).bots;
                    if(bots.contains(botId))
                    {
                        oneEntry.first=questId;
                        if(DatapackClientLoader::datapackLoader.questsExtra.contains(questId))
                            oneEntry.second=tr("%1 (in progress)").arg(DatapackClientLoader::datapackLoader.questsExtra.value(questId).name);
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
    QHashIterator<quint16, PlayerQuest> i(quests);
    while (i.hasNext()) {
        i.next();
        if(!botQuests.contains(i.key()) && i.value().step>0)
        {
            CatchChallenger::Quest currentQuest=CatchChallenger::CommonDatapack::commonDatapack.quests.value(i.key());
            QList<quint16> bots=currentQuest.steps.at(i.value().step-1).bots;
            if(bots.contains(botId))
            {
                //in progress, but not the starting bot
                oneEntry.first=i.key();
                oneEntry.second=tr("%1 (in progress)").arg(DatapackClientLoader::datapackLoader.questsExtra.value(i.key()).name);
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
    lastStepUsed=step;
    isInQuest=false;
    if(!actualBot.step.contains(step))
    {
        showTip(tr("Error into the bot, repport this error please"));
        return;
    }
    if(actualBot.step.value(step).attribute(BaseWindow::text_type)==BaseWindow::text_text)
    {
        const QString &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
        QDomElement text = actualBot.step.value(step).firstChildElement(BaseWindow::text_text);
        if(!language.isEmpty() && language!=BaseWindow::text_en)
            while(!text.isNull())
            {
                if(text.hasAttribute(BaseWindow::text_lang) && text.attribute(BaseWindow::text_lang)==language)
                {
                    QString textToShow=text.text();
                    textToShow=parseHtmlToDisplay(textToShow);
                    ui->IG_dialog_text->setText(textToShow);
                    ui->IG_dialog_name->setText(actualBot.name);
                    ui->IG_dialog->setVisible(true);
                    return;
                }
                text = text.nextSiblingElement(BaseWindow::text_text);
            }
        text = actualBot.step.value(step).firstChildElement(BaseWindow::text_text);
        while(!text.isNull())
        {
            if(!text.hasAttribute(BaseWindow::text_lang) || text.attribute(BaseWindow::text_lang)==BaseWindow::text_en)
            {
                QString textToShow=text.text();
                textToShow=parseHtmlToDisplay(textToShow);
                ui->IG_dialog_text->setText(textToShow);
                ui->IG_dialog_name->setText(actualBot.name);
                ui->IG_dialog->setVisible(true);
                return;
            }
            text = text.nextSiblingElement(BaseWindow::text_text);
        }
        showTip(tr("Bot text not found, repport this error please"));
        return;
    }
    else if(actualBot.step.value(step).attribute(BaseWindow::text_type)==QStringLiteral("shop"))
    {
        if(!actualBot.step.value(step).hasAttribute(QStringLiteral("shop")))
        {
            showTip(tr("Shop called but missing informations"));
            return;
        }
        bool ok;
        shopId=actualBot.step.value(step).attribute("shop").toUInt(&ok);
        if(!ok)
        {
            showTip(tr("Shop called but wrong id"));
            return;
        }
        if(actualBot.properties.contains(QStringLiteral("skin")))
        {
            QPixmap skin=getFrontSkinPath(actualBot.properties.value(QStringLiteral("skin")));
            if(!skin.isNull())
            {
                ui->shopSellerImage->setPixmap(skin.scaled(160,160));
                ui->shopSellerImage->setVisible(true);
            }
            else
                ui->shopSellerImage->setVisible(false);
        }
        else
            ui->shopSellerImage->setVisible(false);
        ui->stackedWidget->setCurrentWidget(ui->page_shop);
        ui->shopItemList->clear();
        on_shopItemList_itemSelectionChanged();
        ui->shopDescription->setText(tr("Waiting the shop content"));
        ui->shopBuy->setVisible(false);
        qDebug() << "goToBotStep(), client->getShopList(shopId): " << shopId;
        CatchChallenger::Api_client_real::client->getShopList(shopId);
        return;
    }
    else if(actualBot.step.value(step).attribute(BaseWindow::text_type)==QStringLiteral("sell"))
    {
        if(!actualBot.step.value(step).hasAttribute(QStringLiteral("shop")))
        {
            showTip(tr("Shop called but missing informations"));
            return;
        }
        bool ok;
        shopId=actualBot.step.value(step).attribute(QStringLiteral("shop")).toUInt(&ok);
        if(!ok)
        {
            showTip(tr("Shop called but wrong id"));
            return;
        }
        if(actualBot.properties.contains(QStringLiteral("skin")))
        {
            QPixmap skin=getFrontSkinPath(actualBot.properties.value(QStringLiteral("skin")));
            if(!skin.isNull())
            {
                ui->shopSellerImage->setPixmap(skin.scaled(160,160));
                ui->shopSellerImage->setVisible(true);
            }
            else
                ui->shopSellerImage->setVisible(false);
        }
        else
            ui->shopSellerImage->setVisible(false);
        waitToSell=true;
        selectObject(ObjectType_Sell);
        return;
    }
    else if(actualBot.step.value(step).attribute(BaseWindow::text_type)==QStringLiteral("learn"))
    {
        selectObject(ObjectType_MonsterToLearn);
        return;
    }
    else if(actualBot.step.value(step).attribute(BaseWindow::text_type)==QStringLiteral("heal"))
    {
        ClientFightEngine::fightEngine.healAllMonsters();
        CatchChallenger::Api_client_real::client->heal();
        load_monsters();
        showTip(tr("You are healed"));
        return;
    }
    else if(actualBot.step.value(step).attribute(BaseWindow::text_type)==QStringLiteral("quests"))
    {
        QString textToShow;
        const QList<QPair<quint32,QString> > &quests=BaseWindow::getQuestList(actualBot.botId);
        if(step==1)
        {
            if(quests.size()==1)
            {
                const QPair<quint32,QString> &quest=quests.at(0);
                if(DatapackClientLoader::datapackLoader.questsExtra.value(quest.first).autostep)
                {
                    on_IG_dialog_text_linkActivated(QString("quest_%1").arg(quest.first));
                    return;
                }
            }
        }
        if(quests.isEmpty())
        {
            if(actualBot.step.contains(step+1))
                on_IG_dialog_text_linkActivated("next");
            else
                textToShow+=tr("No quests at the moment or you don't meat the requirements");
        }
        else
        {
            textToShow+=QStringLiteral("<ul>");
            int index=0;
            while(index<quests.size())
            {
                const QPair<quint32,QString> &quest=quests.at(index);
                textToShow+=QStringLiteral("<li><a href=\"quest_%1\">%2</a></li>").arg(quest.first).arg(quest.second);
                index++;
            }
            if(quests.isEmpty())
                textToShow+=tr("No quests at the moment or you don't meat the requirements");
            textToShow+=QStringLiteral("</ul>");
        }
        ui->IG_dialog_text->setText(textToShow);
        ui->IG_dialog_name->setText(actualBot.name);
        ui->IG_dialog->setVisible(true);
        return;
    }
    else if(actualBot.step.value(step).attribute(BaseWindow::text_type)==QStringLiteral("clan"))
    {
        QString textToShow;
        if(clan==0)
        {
            if(allow.contains(ActionAllow_Clan))
                textToShow=QStringLiteral("<center><a href=\"clan_create\">%1</a></center>").arg(tr("Clan create"));
            else
                textToShow=QStringLiteral("<center>You can't create your clan</center>");
        }
        else
            textToShow=QStringLiteral("<center>%1</center>").arg(tr("You are already into a clan. Use the clan dongle into the player information."));
        ui->IG_dialog_text->setText(textToShow);
        ui->IG_dialog_name->setText(actualBot.name);
        ui->IG_dialog->setVisible(true);
        return;
    }
    else if(actualBot.step.value(step).attribute(BaseWindow::text_type)==QStringLiteral("warehouse"))
    {
        monster_to_withdraw.clear();
        monster_to_deposit.clear();
        change_warehouse_items.clear();
        temp_warehouse_cash=0;
        QPixmap pixmap;
        if(actualBot.properties.contains(QStringLiteral("skin")))
        {
            ui->warehousePlayerImage->setVisible(true);
            ui->warehousePlayerPseudo->setVisible(true);
            ui->warehouseBotImage->setVisible(true);
            ui->warehouseBotPseudo->setVisible(true);
            pixmap=getFrontSkinPath(actualBot.properties.value(QStringLiteral("skin")));
            if(pixmap.isNull())
            {
                ui->warehousePlayerImage->setVisible(false);
                ui->warehousePlayerPseudo->setVisible(false);
                ui->warehouseBotImage->setVisible(false);
                ui->warehouseBotPseudo->setVisible(false);
            }
            else
                ui->warehouseBotImage->setPixmap(pixmap);
        }
        else
        {
            ui->warehousePlayerImage->setVisible(false);
            ui->warehousePlayerPseudo->setVisible(false);
            ui->warehouseBotImage->setVisible(false);
            ui->warehouseBotPseudo->setVisible(false);
            pixmap=QPixmap(QStringLiteral(":/images/player_default/front.png"));
        }
        ui->stackedWidget->setCurrentWidget(ui->page_warehouse);
        updateTheWareHouseContent();
        return;
    }
    else if(actualBot.step.value(step).attribute(BaseWindow::text_type)==QStringLiteral("market"))
    {
        ui->marketMonster->clear();
        ui->marketObject->clear();
        ui->marketOwnMonster->clear();
        ui->marketOwnObject->clear();
        ui->marketWithdraw->setVisible(false);
        ui->marketStat->setText(tr("In waiting of market list"));
        CatchChallenger::Api_client_real::client->getMarketList();
        ui->stackedWidget->setCurrentWidget(ui->page_market);
        return;
    }
    else if(actualBot.step.value(step).attribute(BaseWindow::text_type)==QStringLiteral("industry"))
    {
        if(!actualBot.step.value(step).hasAttribute(QStringLiteral("industry")))
        {
            showTip(tr("Industry called but missing informations"));
            return;
        }

        bool ok;
        factoryId=actualBot.step.value(step).attribute(QStringLiteral("industry")).toUInt(&ok);
        if(!ok)
        {
            showTip(tr("Industry called but wrong id"));
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
        if(actualBot.properties.contains(QStringLiteral("skin")))
        {
            QPixmap skin=getFrontSkinPath(actualBot.properties.value(QStringLiteral("skin")));
            if(!skin.isNull())
            {
                ui->factoryBotImage->setPixmap(skin.scaled(80,80));
                ui->factoryBotImage->setVisible(true);
            }
            else
                ui->factoryBotImage->setVisible(false);
        }
        else
            ui->factoryBotImage->setVisible(false);
        ui->stackedWidget->setCurrentWidget(ui->page_factory);
        CatchChallenger::Api_client_real::client->getFactoryList(factoryId);
        return;
    }
    else if(actualBot.step.value(step).attribute(BaseWindow::text_type)==QStringLiteral("zonecapture"))
    {
        if(!actualBot.step.value(step).hasAttribute(QStringLiteral("zone")))
        {
            showTip(tr("Missing attribute for the step"));
            return;
        }
        if(clan==0)
        {
            showTip(tr("You can't try capture if you are not in a clan"));
            return;
        }
        QString zone=actualBot.step.value(step).attribute(QStringLiteral("zone"));
        if(DatapackClientLoader::datapackLoader.zonesExtra.contains(zone))
        {
            zonecatchName=DatapackClientLoader::datapackLoader.zonesExtra.value(zone).name;
            ui->zonecaptureWaitText->setText(tr("You are waiting to capture %1").arg(QStringLiteral("<b>%1</b>").arg(zonecatchName)));
        }
        else
        {
            zonecatchName.clear();
            ui->zonecaptureWaitText->setText(tr("You are waiting to capture a zone"));
        }
        updater_page_zonecatch.start(1000);
        nextCatchOnScreen=nextCatch;
        zonecatch=true;
        ui->stackedWidget->setCurrentWidget(ui->page_zonecatch);
        CatchChallenger::Api_client_real::client->waitingForCityCapture(false);
        updatePageZoneCatch();
        return;
    }
    else if(actualBot.step.value(step).attribute(BaseWindow::text_type)==QStringLiteral("script"))
    {
        QScriptEngine engine;
        QString contents = actualBot.step.value(step).text();
        contents=QStringLiteral("function getTextEntryPoint()\n{\n")+contents+QStringLiteral("\n}");
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

        QScriptValue getTextEntryPoint = engine.globalObject().property(QStringLiteral("getTextEntryPoint"));
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
            .arg(returnValue.property(QStringLiteral("lineNumber")).toInt32())
            .arg(returnValue.toString()));
            return;
        }
        qDebug() << QStringLiteral("textEntryPoint:") << textEntryPoint;
        return;
    }
    else if(actualBot.step.value(step).attribute(BaseWindow::text_type)==QStringLiteral("fight"))
    {
        if(!actualBot.step.value(step).hasAttribute(QStringLiteral("fightid")))
        {
            showTip(tr("Bot step missing data error, repport this error please"));
            return;
        }
        bool ok;
        quint32 fightId=actualBot.step.value(step).attribute(QStringLiteral("fightid")).toUInt(&ok);
        if(!ok)
        {
            showTip(tr("Bot step wrong data type error, repport this error please"));
            return;
        }
        if(!CommonDatapack::commonDatapack.botFights.contains(fightId))
        {
            showTip(tr("Bot fight not found"));
            return;
        }
        if(MapController::mapController->haveBeatBot(fightId))
        {
            if(actualBot.step.contains(step+1))
                goToBotStep(step+1);
            else
                showTip(tr("Already beaten!"));
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
    const quint32 &itemId=items_graphical.value(item);
    if(inSelection)
    {
        quint32 tempQuantitySelected;
        bool ok=true;
        switch(waitedObjectType)
        {
            case ObjectType_Sell:
            case ObjectType_Trade:
            case ObjectType_SellToMarket:
                if(items.value(itemId)>1)
                    tempQuantitySelected=QInputDialog::getInt(this,tr("Quantity"),tr("Select a quantity"),1,1,items.value(itemId),1,&ok);
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
        if(!items.contains(itemId))
        {
            objectSelection(false);
            return;
        }
        if(items.value(itemId)<tempQuantitySelected)
        {
            objectSelection(false);
            return;
        }
        quint32 objectItem=itemId;
        objectSelection(true,objectItem,tempQuantitySelected);
        return;
    }

    //is crafting recipe
    if(CatchChallenger::CommonDatapack::commonDatapack.itemToCrafingRecipes.contains(itemId))
    {
        Player_private_and_public_informations informations=CatchChallenger::Api_client_real::client->get_player_informations();
        if(informations.recipes.contains(CatchChallenger::CommonDatapack::commonDatapack.itemToCrafingRecipes.value(itemId)))
        {
            QMessageBox::information(this,tr("Information"),tr("You already know this recipe"));
            return;
        }
        objectInUsing << itemId;
        if(CommonDatapack::commonDatapack.items.item.value(objectInUsing.last()).consumeAtUse)
            remove_to_inventory(objectInUsing.last());
        CatchChallenger::Api_client_real::client->useObject(objectInUsing.last());
    }
    //it's repel
    else if(CatchChallenger::CommonDatapack::commonDatapack.items.repel.contains(itemId))
    {
        MapController::mapController->addRepelStep(CatchChallenger::CommonDatapack::commonDatapack.items.repel.value(itemId));
        objectInUsing << itemId;
        if(CommonDatapack::commonDatapack.items.item.value(objectInUsing.last()).consumeAtUse)
            remove_to_inventory(objectInUsing.last());
        CatchChallenger::Api_client_real::client->useObject(objectInUsing.last());
    }
    //it's object with monster effect
    else if(CatchChallenger::CommonDatapack::commonDatapack.items.monsterItemEffect.contains(itemId))
    {
        objectInUsing << itemId;
        if(CommonDatapack::commonDatapack.items.item.value(objectInUsing.last()).consumeAtUse)
            remove_to_inventory(objectInUsing.last());
        selectObject(ObjectType_ItemOnMonster);
    }
    //it's object with monster effect but offline
    else if(CatchChallenger::CommonDatapack::commonDatapack.items.monsterItemEffectOutOfFight.contains(itemId))
    {
        objectInUsing << itemId;
        if(CommonDatapack::commonDatapack.items.item.value(objectInUsing.last()).consumeAtUse)
            remove_to_inventory(objectInUsing.last());
        selectObject(ObjectType_ItemOnMonster);
    }
    else if(CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.contains(itemId))
    {
        objectInUsing << itemId;
        if(CommonDatapack::commonDatapack.items.item.value(objectInUsing.last()).consumeAtUse)
            remove_to_inventory(objectInUsing.last());
        selectObject(ObjectType_ItemEvolutionOnMonster);
    }
    else if(CatchChallenger::CommonDatapack::commonDatapack.items.itemToLearn.contains(itemId))
    {
        objectInUsing << itemId;
        if(CommonDatapack::commonDatapack.items.item.value(objectInUsing.last()).consumeAtUse)
            remove_to_inventory(objectInUsing.last());
        selectObject(ObjectType_ItemLearnOnMonster);
    }
    else
        qDebug() << "BaseWindow::on_inventory_itemActivated(): unknow object type";
}

void BaseWindow::objectUsed(const ObjectUsage &objectUsage)
{
    if(objectInUsing.isEmpty())
    {
        emit error("No object usage to validate");
        return;
    }
    switch(objectUsage)
    {
        case ObjectUsage_correctlyUsed:
        //is crafting recipe
        if(CatchChallenger::CommonDatapack::commonDatapack.itemToCrafingRecipes.contains(objectInUsing.first()))
        {
            CatchChallenger::Api_client_real::client->addRecipe(CatchChallenger::CommonDatapack::commonDatapack.itemToCrafingRecipes.value(objectInUsing.first()));
            load_crafting_inventory();
        }
        else if(CommonDatapack::commonDatapack.items.trap.contains(objectInUsing.first()))
        {
        }
        else if(CatchChallenger::CommonDatapack::commonDatapack.items.repel.contains(objectInUsing.first()))
        {
        }
        else
            qDebug() << "BaseWindow::objectUsed(): unknow object type";

        break;
        case ObjectUsage_failed:
        break;
        case ObjectUsage_impossible:
            add_to_inventory(objectInUsing.first());
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
    quint32 itemId=items_graphical.value(items.first());
    if(!this->items.contains(itemId))
        return;
    quint32 quantity=this->items.value(itemId);
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
        button=QMessageBox::question(this,tr("Destroy"),tr("Are you sure you want to destroy %1 %2?").arg(quantity).arg(DatapackClientLoader::datapackLoader.itemsExtra.value(itemId).name),QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes);
    else
        button=QMessageBox::question(this,tr("Destroy"),tr("Are you sure you want to destroy %1 unknow item (id: %2)?").arg(quantity).arg(itemId),QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes);
    if(button!=QMessageBox::Yes)
        return;
    if(!this->items.contains(itemId))
        return;
    if(this->items.value(itemId)<quantity)
        quantity=this->items.value(itemId);
    emit destroyObject(itemId,quantity);
    remove_to_inventory(itemId,quantity);
    load_inventory();
    load_plant_inventory();
}

quint32 BaseWindow::itemQuantity(const quint32 &itemId) const
{
    if(items.contains(itemId))
        return items.value(itemId);
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
    if(DatapackClientLoader::datapackLoader.itemToPlants.contains(items_graphical.value(item)))
    {
        if(!plants_items_to_graphical.contains(DatapackClientLoader::datapackLoader.itemToPlants.value(items_graphical.value(item))))
        {
            qDebug() << QStringLiteral("on_inventoryInformation_clicked() is not into plant list: item: %1, plant: %2").arg(items_graphical.value(item)).arg(DatapackClientLoader::datapackLoader.itemToPlants.value(items_graphical.value(item)));
            return;
        }
        ui->listPlantList->reset();
        ui->stackedWidget->setCurrentWidget(ui->page_plants);
        plants_items_to_graphical.value(DatapackClientLoader::datapackLoader.itemToPlants.value(items_graphical.value(item)))->setSelected(true);
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
                showTip(QStringLiteral("Unable to open the url: %1").arg(link));
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
                showTip(QStringLiteral("Unable to open the link: %1").arg(link));
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
            if(DatapackClientLoader::datapackLoader.questsExtra.contains(questId))
                if(DatapackClientLoader::datapackLoader.questsExtra.value(questId).autostep)
                {
                    int index=0;
                    bool ok;
                    do
                    {
                        ok=tryValidateQuestStep();
                        index++;
                    } while(ok && index<99);
                    if(index==99)
                    {
                        emit error(QString("Infinity loop into autostep for quest: %1").arg(questId));
                        return;
                    }
                }
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
            tryValidateQuestStep();
            index++;
            continue;
        }
        else if(link=="next" && lastStepUsed>=1)
        {
            goToBotStep(lastStepUsed+1);
            index++;
            continue;
        }
        quint8 step=link.toUShort(&ok);
        if(!ok)
        {
            showTip(QStringLiteral("Unable to open the link: %1").arg(link));
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

bool BaseWindow::tryValidateQuestStep(bool silent)
{
    if(!CatchChallenger::CommonDatapack::commonDatapack.quests.contains(questId))
    {
        if(!silent)
            showTip(tr("Quest not found"));
        return false;
    }

    if(!quests.contains(questId))
    {
        if(CatchChallenger::CommonDatapack::commonDatapack.quests.value(questId).steps.at(0).bots.contains(actualBot.botId) && haveStartQuestRequirement(CatchChallenger::CommonDatapack::commonDatapack.quests.value(questId)))
        {
            CatchChallenger::Api_client_real::client->startQuest(questId);
            startQuest(CatchChallenger::CommonDatapack::commonDatapack.quests.value(questId));
            updateDisplayedQuests();
            return true;
        }
        else
        {
            if(!silent)
                showTip(tr("You don't have the requirement to start this quest"));
            return false;
        }
    }
    else if(quests.value(questId).step==0)
    {
        if(CatchChallenger::CommonDatapack::commonDatapack.quests.value(questId).steps.at(0).bots.contains(actualBot.botId) && haveStartQuestRequirement(CatchChallenger::CommonDatapack::commonDatapack.quests.value(questId)))
        {
            CatchChallenger::Api_client_real::client->startQuest(questId);
            startQuest(CatchChallenger::CommonDatapack::commonDatapack.quests.value(questId));
            updateDisplayedQuests();
            return true;
        }
        else
        {
            if(!silent)
                showTip(tr("You don't have the requirement to start this quest"));
            return false;
        }
    }
    if(!haveNextStepQuestRequirements(CatchChallenger::CommonDatapack::commonDatapack.quests.value(questId)))
    {
        if(!silent)
            showTip(tr("You don't have the requirement to continue this quest"));
        return false;
    }
    if(quests.value(questId).step>=(CatchChallenger::CommonDatapack::commonDatapack.quests.value(questId).steps.size()))
    {
        if(!silent)
            showTip(tr("You have finish the quest <b>%1</b>").arg(DatapackClientLoader::datapackLoader.questsExtra.value(questId).name));
        CatchChallenger::Api_client_real::client->finishQuest(questId);
        nextStepQuest(CatchChallenger::CommonDatapack::commonDatapack.quests.value(questId));
        updateDisplayedQuests();
        return true;
    }
    if(!CatchChallenger::CommonDatapack::commonDatapack.quests.value(questId).steps.at(quests.value(questId).step).bots.contains(actualBot.botId))
    {
        if(!silent)
            showTip(tr("You need talk to another bot"));
        return false;
    }
    CatchChallenger::Api_client_real::client->nextQuestStep(questId);
    nextStepQuest(CatchChallenger::CommonDatapack::commonDatapack.quests.value(questId));
    updateDisplayedQuests();
    return true;
}

void BaseWindow::getTextEntryPoint()
{
    if(!isInQuest)
    {
        showTip(QStringLiteral("Internal error: Is not in quest"));
        return;
    }
    QScriptEngine engine;

    const QString &client_logic=CatchChallenger::Api_client_real::client->datapackPath()+"/"+DATAPACK_BASE_PATH_QUESTS+"/"+QString::number(questId)+"/client_logic.js";
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
        PlayerQuest quest=quests.value(questId);
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
        else if(haveNextStepQuestRequirements(CatchChallenger::CommonDatapack::commonDatapack.quests.value(questId)))
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
    quint32 textEntryPoint=(quint32)returnValue.toInt32();
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
        qDebug() << QStringLiteral("No quest text for this quest: %1").arg(questId);
        showTip(tr("No quest text for this quest"));
        return;
    }
    if(!DatapackClientLoader::datapackLoader.questsText.value(questId).text.contains(textId))
    {
        qDebug() << "No quest text entry point";
        showTip(tr("No quest text entry point"));
        return;
    }

    QString textToShow=parseHtmlToDisplay(DatapackClientLoader::datapackLoader.questsText.value(questId).text.value(textId));
    ui->IG_dialog_text->setText(textToShow);
    ui->IG_dialog_name->setText(actualBot.name);
    ui->IG_dialog->setVisible(true);
}

void BaseWindow::addCash(const quint32 &cash)
{
    this->cash+=cash;
    ui->player_informations_cash->setText(QStringLiteral("%1$").arg(this->cash));
    ui->shopCash->setText(tr("Cash: %1$").arg(this->cash));
    ui->tradePlayerCash->setMaximum(this->cash);
}

void BaseWindow::removeCash(const quint32 &cash)
{
    this->cash-=cash;
    ui->player_informations_cash->setText(QStringLiteral("%1$").arg(this->cash));
    ui->shopCash->setText(tr("Cash: %1$").arg(this->cash));
    ui->tradePlayerCash->setMaximum(this->cash);
}

void BaseWindow::on_pushButton_interface_monsters_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_monster);
}

void BaseWindow::on_toolButton_monster_list_quit_clicked()
{
    if(inSelection)
    {
        switch(waitedObjectType)
        {
            case ObjectType_MonsterToFightKO:
            ui->stackedWidgetFightBottomBar->setCurrentWidget(ui->stackedWidgetFightBottomBarPageMain);
            objectSelection(false);
            break;
            case ObjectType_ItemOnMonster:
            case ObjectType_ItemOnMonsterOutOfFight:
            case ObjectType_ItemEvolutionOnMonster:
            case ObjectType_ItemLearnOnMonster:
            case ObjectType_MonsterToTrade:
            case ObjectType_MonsterToLearn:
            case ObjectType_MonsterToFight:
            objectSelection(false);
            return;
            default:
            break;
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

void BaseWindow::on_tradeAddMonster_clicked()
{
    selectObject(ObjectType_MonsterToTrade);
}

void BaseWindow::on_selectMonster_clicked()
{
    QList<QListWidgetItem *> selectedMonsters=ui->monsterList->selectedItems();
    if(selectedMonsters.size()!=1)
        return;
    on_monsterList_itemActivated(selectedMonsters.first());
}

void BaseWindow::on_close_IG_dialog_clicked()
{
    isInQuest=false;
    lastStepUsed=0;
}

void BaseWindow::on_pushButtonFightBag_clicked()
{
    selectObject(ObjectType_UseInFight);
}

void BaseWindow::on_monsterDetailsQuit_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_monster);
}

void BaseWindow::on_monsterListMoveUp_clicked()
{
    QList<QListWidgetItem *> selectedMonsters=ui->monsterList->selectedItems();
    if(selectedMonsters.size()!=1)
        return;
    int	currentRow=ui->monsterList->row(selectedMonsters.first());
    if(currentRow<=0)
        return;
    if(!CatchChallenger::ClientFightEngine::fightEngine.moveUpMonster(currentRow))
        return;
    CatchChallenger::Api_client_real::client->monsterMoveUp(currentRow+1);
    QListWidgetItem * item=ui->monsterList->takeItem(currentRow);
    ui->monsterList->insertItem(currentRow-1,item);
    ui->monsterList->item(currentRow)->setSelected(false);
    ui->monsterList->item(currentRow-1)->setSelected(true);
    ui->monsterList->setCurrentRow(currentRow-1);
    on_monsterList_itemSelectionChanged();
}

void BaseWindow::on_monsterListMoveDown_clicked()
{
    QList<QListWidgetItem *> selectedMonsters=ui->monsterList->selectedItems();
    if(selectedMonsters.size()!=1)
        return;
    int	currentRow=ui->monsterList->row(selectedMonsters.first());
    if(currentRow<0)
        return;
    if(currentRow>=(ui->monsterList->count()-1))
        return;
    if(!CatchChallenger::ClientFightEngine::fightEngine.moveDownMonster(currentRow))
        return;
    CatchChallenger::Api_client_real::client->monsterMoveDown(currentRow+1);
    QListWidgetItem * item=ui->monsterList->takeItem(currentRow);
    ui->monsterList->insertItem(currentRow+1,item);
    ui->monsterList->item(currentRow)->setSelected(false);
    ui->monsterList->item(currentRow+1)->setSelected(true);
    ui->monsterList->setCurrentRow(currentRow+1);
    on_monsterList_itemSelectionChanged();
}

void BaseWindow::on_monsterList_itemSelectionChanged()
{
    QList<QListWidgetItem *> selectedMonsters=ui->monsterList->selectedItems();
    if(selectedMonsters.size()!=1)
    {
        ui->monsterListMoveUp->setEnabled(false);
        ui->monsterListMoveDown->setEnabled(false);
        return;
    }
    const QList<PlayerMonster> &playerMonster=CatchChallenger::ClientFightEngine::fightEngine.getPlayerMonster();
    if(playerMonster.size()<=1)
    {
        ui->monsterListMoveUp->setEnabled(false);
        ui->monsterListMoveDown->setEnabled(false);
        return;
    }
    int row=ui->monsterList->row(selectedMonsters.first());
    ui->monsterListMoveUp->setEnabled(row>0);
    ui->monsterListMoveDown->setEnabled(row<(playerMonster.size()-1));
}

void BaseWindow::teleportConditionNotRespected(const QString &text)
{
    showTip(text);
}

void BaseWindow::changeDeviceIndex(int device)
{
    if(device==-1)
        return;
    /*const QStringList &outputDeviceNames=soundEngine.outputDeviceNames();
    if(device<outputDeviceNames.size())
    {
        Options::options.setDeviceIndex(device);
        soundEngine.setOutputDeviceName(outputDeviceNames.at(device));
    }*/
}

void BaseWindow::lastReplyTime(const quint32 &time)
{
    ui->labelLastReplyTime->setText(tr("Last reply time: %1ms").arg(time));
    if(lastReplyTimeValue<=TIMEINMSTOBESLOW || lastReplyTimeSince.elapsed()>TIMETOSHOWTHETURTLE)
    {
        lastReplyTimeValue=time;
        lastReplyTimeSince.restart();
    }
    else
    {
        if(time>TIMEINMSTOBESLOW)
        {
            lastReplyTimeValue=time;
            lastReplyTimeSince.restart();
        }
    }
    updateTheTurtle();
}

void BaseWindow::detectSlowDown()
{
    if(CatchChallenger::Api_client_real::client==NULL)
        return;
    quint32 queryCount=0;
    worseQueryTime=0;
    QStringList middleQueryList;
    const QMap<quint8,QTime> &values=CatchChallenger::Api_client_real::client->getQuerySendTimeList();
    queryCount+=values.size();
    QMapIterator<quint8,QTime> i(values);
    while (i.hasNext()) {
        i.next();
        middleQueryList << QString::number(i.key());
        const quint32 &time=i.value().elapsed();
        if(time>worseQueryTime)
            worseQueryTime=time;
    }
    if(queryCount>0)
        ui->labelQueryList->setText(
                    tr("Running query: %1 (%3 and %4), query with worse time: %2ms")
                    .arg(queryCount)
                    .arg(worseQueryTime)
                    .arg(CatchChallenger::Api_client_real::client->getQueryRunningList().join(QStringLiteral(";")))
                    .arg(middleQueryList.join(QStringLiteral(";")))
                    );
    else
        ui->labelQueryList->setText(tr("No query running"));
    updateTheTurtle();
}

void BaseWindow::updateTheTurtle()
{
    if(!multiplayer)
    {
        if(!ui->labelSlow->isVisible())
            return;
        ui->labelSlow->hide();
        return;
    }
    if(lastReplyTimeValue>TIMEINMSTOBESLOW && lastReplyTimeSince.elapsed()<TIMETOSHOWTHETURTLE)
    {
        if(ui->labelSlow->isVisible())
            return;
        ui->labelSlow->setToolTip(tr("The last query was slow"));
        ui->labelSlow->show();
        return;
    }
    if(worseQueryTime>TIMEINMSTOBESLOW)
    {
        if(ui->labelSlow->isVisible())
            return;
        ui->labelSlow->setToolTip(tr("Remain query in suspend"));
        ui->labelSlow->show();
        return;
    }
    if(!ui->labelSlow->isVisible())
        return;
    ui->labelSlow->hide();
}
