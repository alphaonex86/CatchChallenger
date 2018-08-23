#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "../../../general/base/FacilityLib.h"
#include "../ClientVariable.h"
#include "../../fight/interface/ClientFightEngine.h"
#include "../../../general/base/CommonDatapack.h"
#include "../../../general/base/CommonDatapackServerSpec.h"
#include "../../../general/base/CommonSettingsServer.h"
#include "../../../general/base/CommonSettingsCommon.h"
#include "DatapackClientLoader.h"
#include "MapController.h"
#include "Chat.h"
#include "WithAnotherPlayer.h"
#include "../LanguagesSelect.h"
#include "../Options.h"
#ifndef CATCHCHALLENGER_NOAUDIO
#include "../Audio.h"
#endif

#include <QListWidgetItem>
#include <QBuffer>
#include <QInputDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QRegularExpression>
#include <QtQml>
#include <QComboBox>
#include <iostream>
#ifndef CATCHCHALLENGER_NOAUDIO
#include <vlc/vlc.h>
#endif

//do buy queue
//do sell queue

using namespace CatchChallenger;

std::string BaseWindow::text_type="type";
std::string BaseWindow::text_lang="lang";
std::string BaseWindow::text_en="en";
std::string BaseWindow::text_text="text";
QFile BaseWindow::debugFile;
uint8_t BaseWindow::debugFileStatus=0;

// bug if init here
QIcon BaseWindow::icon_server_list_star1;
QIcon BaseWindow::icon_server_list_star2;
QIcon BaseWindow::icon_server_list_star3;
QIcon BaseWindow::icon_server_list_star4;
QIcon BaseWindow::icon_server_list_star5;
QIcon BaseWindow::icon_server_list_star6;
QIcon BaseWindow::icon_server_list_stat1;
QIcon BaseWindow::icon_server_list_stat2;
QIcon BaseWindow::icon_server_list_stat3;
QIcon BaseWindow::icon_server_list_stat4;
QIcon BaseWindow::icon_server_list_bug;
std::vector<QIcon> BaseWindow::icon_server_list_color;

BaseWindow::BaseWindow() :
    ui(new Ui::BaseWindowUI)
{
    qRegisterMetaType<CatchChallenger::Chat_type>("CatchChallenger::Chat_type");
    qRegisterMetaType<CatchChallenger::Player_type>("CatchChallenger::Player_type");
    qRegisterMetaType<CatchChallenger::Player_private_and_public_informations>("CatchChallenger::Player_private_and_public_informations");
    qRegisterMetaType<std::vector<ServerFromPoolForDisplay *> >("std::vector<ServerFromPoolForDisplay *>");
    qRegisterMetaType<MapVisualiserPlayer::BlockedOn>("MapVisualiserPlayer::BlockedOn");

    qRegisterMetaType<Chat_type>("Chat_type");
    qRegisterMetaType<Player_type>("Player_type");
    qRegisterMetaType<Player_private_and_public_informations>("Player_private_and_public_informations");

    qRegisterMetaType<std::unordered_map<uint32_t,uint32_t> >("std::unordered_map<uint32_t,uint32_t>");
    qRegisterMetaType<std::unordered_map<uint32_t,uint32_t> >("CatchChallenger::Plant_collect");
    qRegisterMetaType<std::vector<ItemToSellOrBuy> >("std::vector<ItemToSell>");
    qRegisterMetaType<std::vector<std::pair<uint8_t,uint8_t> > >("std::vector<std::pair<uint8_t,uint8_t> >");
    qRegisterMetaType<Skill::AttackReturn>("Skill::AttackReturn");
    qRegisterMetaType<std::vector<uint32_t> >("std::vector<uint32_t>");
    qRegisterMetaType<std::vector<std::vector<CharacterEntry> > >("std::vector<std::vector<CharacterEntry> >");
    qRegisterMetaType<std::vector<MarketMonster> >("std::vector<MarketMonster>");

    qRegisterMetaType<std::unordered_map<uint16_t,uint16_t> >("std::unordered_map<uint16_t,uint16_t>");
    qRegisterMetaType<std::unordered_map<uint16_t,uint32_t> >("std::unordered_map<uint16_t,uint32_t>");
    qRegisterMetaType<std::unordered_map<uint8_t,uint32_t> >("std::unordered_map<uint8_t,uint32_t>");
    qRegisterMetaType<std::unordered_map<uint8_t,uint16_t> >("std::unordered_map<uint8_t,uint16_t>");
    qRegisterMetaType<std::unordered_map<uint8_t,uint32_t> >("std::unordered_map<uint8_t,uint32_t>");

    qRegisterMetaType<std::map<uint16_t,uint16_t> >("std::map<uint16_t,uint16_t>");
    qRegisterMetaType<std::map<uint16_t,uint32_t> >("std::map<uint16_t,uint32_t>");
    qRegisterMetaType<std::map<uint8_t,uint32_t> >("std::map<uint8_t,uint32_t>");
    qRegisterMetaType<std::map<uint8_t,uint16_t> >("std::map<uint8_t,uint16_t>");
    qRegisterMetaType<std::map<uint8_t,uint32_t> >("std::map<uint8_t,uint32_t>");
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<uint8_t>("uint8_t");
    qRegisterMetaType<uint16_t>("uint16_t");
    qRegisterMetaType<uint32_t>("uint32_t");
    qRegisterMetaType<uint64_t>("uint64_t");
    qRegisterMetaType<std::vector<std::string> >("std::vector<std::string>");
    qRegisterMetaType<std::vector<char> >("std::vector<char>");
    qRegisterMetaType<std::vector<uint32_t> >("std::vector<uint32_t>");

    qmlRegisterUncreatableType<EvolutionControl>("EvolutionControl", 1, 0, "EvolutionControl","");
    qmlRegisterUncreatableType<AnimationControl>("AnimationControl", 2, 0, "AnimationControl","");

    socketState=QAbstractSocket::UnconnectedState;

    mapController=new MapController(true,false,true);
    chat=new Chat(mapController);
    mapController->fightEngine=&fightEngine;
    client=NULL;
    ProtocolParsing::initialiseTheVariable();
    ui->setupUi(this);
    animationWidget=NULL;
    qQuickViewContainer=NULL;
    monsterBeforeMoveForChangeInWaiting=false;
    //Chat::chat=new Chat(ui->page_map);
    escape=false;
    multiplayer=false;
    movie=NULL;
    newProfile=NULL;
    lastStepUsed=0;
    datapackFileSize=0;
    craftingAnimationObject=NULL;
    #ifndef CATCHCHALLENGER_NOAUDIO
    currentAmbiance.manager=NULL;
    currentAmbiance.player=NULL;
    #endif

    #ifdef CATCHCHALLENGER_VERSION_ULTIMATE
    ui->label_ultimate->setVisible(false);
    #endif
    {
        const QList<QByteArray> &supportedImageFormats=QImageReader::supportedImageFormats();
        int index=0;
        while(index<supportedImageFormats.size())
        {
            this->supportedImageFormats.insert(QString(supportedImageFormats.at(index)).toStdString());
            index++;
        }
    }

    checkQueryTime.start(200);
    if(!connect(&checkQueryTime,&QTimer::timeout,   this,&BaseWindow::detectSlowDown))
        abort();
    updateRXTXTimer.start(1000);
    updateRXTXTime.restart();

    tip_timeout.setInterval(TIMETODISPLAY_TIP);
    gain_timeout.setInterval(500);
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
    botFightTimer.setSingleShot(true);
    botFightTimer.setInterval(1000);

    disableIntoListFont.setItalic(true);
    disableIntoListBrush=QBrush(QColor(200,20,20));

    //connect the datapack loader
    if(!connect(&DatapackClientLoader::datapackLoader,  &DatapackClientLoader::datapackParsed,                  this,                                   &BaseWindow::datapackParsed,Qt::QueuedConnection))
        abort();
    if(!connect(&DatapackClientLoader::datapackLoader,  &DatapackClientLoader::datapackParsedMainSub,           this,                                   &BaseWindow::datapackParsedMainSub,Qt::QueuedConnection))
        abort();
    if(!connect(&DatapackClientLoader::datapackLoader,  &DatapackClientLoader::datapackChecksumError,           this,                                   &BaseWindow::datapackChecksumError,Qt::QueuedConnection))
        abort();
    if(!connect(this,                                   &BaseWindow::parseDatapack,                             &DatapackClientLoader::datapackLoader,  &DatapackClientLoader::parseDatapack,Qt::QueuedConnection))
        abort();
    if(!connect(this,                                   &BaseWindow::parseDatapackMainSub,                      &DatapackClientLoader::datapackLoader,  &DatapackClientLoader::parseDatapackMainSub,Qt::QueuedConnection))
        abort();
    if(!connect(&DatapackClientLoader::datapackLoader,  &DatapackClientLoader::datapackParsed,                  mapController,           &MapController::datapackParsed,Qt::QueuedConnection))
        abort();
    if(!connect(this,                                   &BaseWindow::datapackParsedMainSubMap,                  mapController,        &MapController::datapackParsedMainSub,Qt::QueuedConnection))
        abort();

    //render, logical part into Map_Client
    if(!connect(mapController,&MapController::send_player_direction, this,&BaseWindow::send_player_direction,Qt::QueuedConnection))
        abort();
    if(!connect(mapController,&MapController::stopped_in_front_of,   this,&BaseWindow::stopped_in_front_of,Qt::QueuedConnection))
        abort();
    if(!connect(mapController,&MapController::actionOn,              this,&BaseWindow::actionOn,Qt::QueuedConnection))
        abort();
    if(!connect(mapController,&MapController::actionOnNothing,       this,&BaseWindow::actionOnNothing,Qt::QueuedConnection))
        abort();
    if(!connect(mapController,&MapController::blockedOn,             this,&BaseWindow::blockedOn,Qt::QueuedConnection))
        abort();
    if(!connect(mapController,&MapController::error,                 this,&BaseWindow::error))
        abort();
    if(!connect(mapController,&MapController::errorWithTheCurrentMap,this,&BaseWindow::errorWithTheCurrentMap))
        abort();
    if(!connect(mapController,&MapController::repelEffectIsOver,     this,&BaseWindow::repelEffectIsOver))
        abort();
    if(!connect(mapController,&MapController::teleportConditionNotRespected, this,&BaseWindow::teleportConditionNotRespected,Qt::QueuedConnection))
        abort();

    #ifndef CATCHCHALLENGER_NOAUDIO
    //audio
    /*if(!connect(this,&BaseWindow::audioLoopRestart,&BaseWindow::audioLoop,Qt::QueuedConnection))
        abort();*/
    #endif

    //fight
    if(!connect(mapController,   &MapController::wildFightCollision,     this,&BaseWindow::wildFightCollision))
        abort();
    if(!connect(mapController,   &MapController::botFightCollision,      this,&BaseWindow::botFightCollision))
        abort();
    if(!connect(mapController,   &MapController::currentMapLoaded,       this,&BaseWindow::currentMapLoaded))
        abort();
    if(!connect(mapController,   &MapController::pathFindingNotFound,    this,&BaseWindow::pathFindingNotFound))
        abort();
    if(!connect(&moveFightMonsterBottomTimer,   &QTimer::timeout,                       this,&BaseWindow::moveFightMonsterBottom))
        abort();
    if(!connect(&moveFightMonsterTopTimer,      &QTimer::timeout,                       this,&BaseWindow::moveFightMonsterTop))
        abort();
    if(!connect(&moveFightMonsterBothTimer,     &QTimer::timeout,                       this,&BaseWindow::moveFightMonsterBoth))
        abort();
    if(!connect(&displayAttackTimer,            &QTimer::timeout,                       this,&BaseWindow::displayAttack))
        abort();
    if(!connect(&displayExpTimer,               &QTimer::timeout,                       this,&BaseWindow::displayExperienceGain))
        abort();
    if(!connect(&displayTrapTimer,              &QTimer::timeout,                       this,&BaseWindow::displayTrap))
        abort();
    if(!connect(&doNextActionTimer,             &QTimer::timeout,                       this,&BaseWindow::doNextAction))
        abort();
    if(!connect(&botFightTimer,                 &QTimer::timeout,                       this,&BaseWindow::botFightFullDiffered))
        abort();
    if(!connect(ui->stackedWidget,              &QStackedWidget::currentChanged,        this,&BaseWindow::pageChanged))
        abort();

    if(!connect(&fightEngine,&ClientFightEngine::newError,  this,&BaseWindow::newError))
        abort();
    if(!connect(&fightEngine,&ClientFightEngine::error,     this,&BaseWindow::error))
        abort();
    if(!connect(&fightEngine,&ClientFightEngine::message,     this,&BaseWindow::message))
        abort();
    /*if(!connect(&fightEngine,&ClientFightEngine::errorFightEngine,     this,&BaseWindow::stderror))
        abort();*/

    if(!connect(&updateRXTXTimer,&QTimer::timeout,          this,&BaseWindow::updateRXTX))
        abort();

    if(!connect(&tip_timeout,&QTimer::timeout,              this,&BaseWindow::tipTimeout))
        abort();
    if(!connect(&gain_timeout,&QTimer::timeout,             this,&BaseWindow::gainTimeout))
        abort();
    if(!connect(&nextCityCatchTimer,&QTimer::timeout,     this,&BaseWindow::cityCaptureUpdateTime))
        abort();
    if(!connect(&updater_page_zonecatch,&QTimer::timeout, this,&BaseWindow::updatePageZoneCatch))
        abort();
    if(!connect(&animationControl,&AnimationControl::animationFinished,this,&BaseWindow::animationFinished,Qt::QueuedConnection))
        abort();

    renderFrame = new QFrame(ui->page_map);
    renderFrame->setObjectName(QString::fromUtf8("renderFrame"));
    renderFrame->setMinimumSize(QSize(600, 572));
    QVBoxLayout *renderLayout = new QVBoxLayout(renderFrame);
    renderLayout->setSpacing(0);
    renderLayout->setContentsMargins(0, 0, 0, 0);
    renderLayout->setObjectName(QString::fromUtf8("renderLayout"));
    renderLayout->addWidget(mapController);
    renderFrame->setGeometry(QRect(0, 0, 800, 516));
    renderFrame->lower();
    renderFrame->lower();
    renderFrame->lower();
    ui->labelFightTrap->hide();
    nextCityCatchTimer.setSingleShot(true);
    updater_page_zonecatch.setSingleShot(false);

    chat->setGeometry(QRect(0, 0, 250, 400));

    resetAll();
    loadSettings();

    mapController->setFocus();

    /// \todo able to cancel quest
    ui->cancelQuest->hide();
    loadSoundSettings();
    //qInstallMessageHandler(&BaseWindow::customMessageHandler);

    #ifdef CATCHCHALLENGER_VERSION_ULTIMATE
    {
        QFile file(":/images/interface/repeatable.png");
        if(file.open(QIODevice::ReadOnly))
        {
            imagesInterfaceRepeatableString=QStringLiteral("<img src=\"data:image/png;base64,%1\" alt=\"Repeatable\" title=\"Repeatable\" />").arg(QString(file.readAll().toBase64())).toStdString();
            file.close();
        }
    }
    {
        QFile file(":/images/interface/in-progress.png");
        if(file.open(QIODevice::ReadOnly))
        {
            imagesInterfaceInProgressString=QStringLiteral("<img src=\"data:image/png;base64,%1\" alt=\"In progress\" title=\"In progress\" />").arg(QString(file.readAll().toBase64())).toStdString();
            file.close();
        }
    }
    #endif

    #ifndef CATCHCHALLENGER_NOAUDIO
    Audio::audio.setVolume(Options::options.getAudioVolume());
    #endif
    #ifdef CATCHCHALLENGER_NOAUDIO
    qDebug() << "CATCHCHALLENGER_NOAUDIO def, audio disabled";
    #endif
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
    #ifndef CATCHCHALLENGER_NOAUDIO
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
    #endif
    if(movie!=NULL)
        delete movie;
    delete ui;
    if(mapController!=NULL)
    {
        delete mapController;
        mapController=NULL;
    }
    else
        abort();
}

void BaseWindow::connectAllSignals()
{
    if(client==NULL)
    {
        std::cerr << "BaseWindow::connectAllSignals(), unable to connect the signals: client==NULL" << std::endl;
        return;
    }
    mapController->connectAllSignals(client);

    //for bug into solo player: Api_client_real -> Api_protocol
    if(!connect(client,&CatchChallenger::Api_client_real::QtlastReplyTime,      this,&BaseWindow::lastReplyTime,    Qt::QueuedConnection))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::Qtprotocol_is_good,   this,&BaseWindow::protocol_is_good, Qt::QueuedConnection))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::Qtdisconnected,          this,&BaseWindow::disconnected,     Qt::QueuedConnection))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QtnewError,           this,&BaseWindow::newError,         Qt::QueuedConnection))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::Qtmessage,          this,&BaseWindow::message,          Qt::QueuedConnection))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QtnotLogged,          this,&BaseWindow::notLogged,        Qt::QueuedConnection))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::Qtlogged,             this,&BaseWindow::logged,           Qt::QueuedConnection))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QthaveTheDatapack,    this,&BaseWindow::haveTheDatapack,  Qt::QueuedConnection))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QthaveTheDatapackMainSub,    this,&BaseWindow::haveTheDatapackMainSub,  Qt::QueuedConnection))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QthaveDatapackMainSubCode,    this,&BaseWindow::haveDatapackMainSubCode,  Qt::QueuedConnection))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QtclanActionFailed,   this,&BaseWindow::clanActionFailed, Qt::QueuedConnection))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QtclanActionSuccess,  this,&BaseWindow::clanActionSuccess,Qt::QueuedConnection))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QtclanDissolved,      this,&BaseWindow::clanDissolved,    Qt::QueuedConnection))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QtclanInformations,   this,&BaseWindow::clanInformations, Qt::QueuedConnection))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QtclanInvite,         this,&BaseWindow::clanInvite,       Qt::QueuedConnection))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QtcityCapture,        this,&BaseWindow::cityCapture,      Qt::QueuedConnection))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QtsetEvents,          this,&BaseWindow::setEvents,        Qt::QueuedConnection))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QtnewEvent,           this,&BaseWindow::newEvent,         Qt::QueuedConnection))
        abort();

    if(!connect(client,&CatchChallenger::Api_client_real::QtcaptureCityYourAreNotLeader,                this,&BaseWindow::captureCityYourAreNotLeader,              Qt::QueuedConnection))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QtcaptureCityYourLeaderHaveStartInOtherCity,  this,&BaseWindow::captureCityYourLeaderHaveStartInOtherCity,Qt::QueuedConnection))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QtcaptureCityPreviousNotFinished,             this,&BaseWindow::captureCityPreviousNotFinished,           Qt::QueuedConnection))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QtcaptureCityStartBattle,                     this,&BaseWindow::captureCityStartBattle,                   Qt::QueuedConnection))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QtcaptureCityStartBotFight,                   this,&BaseWindow::captureCityStartBotFight,                 Qt::QueuedConnection))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QtcaptureCityDelayedStart,                    this,&BaseWindow::captureCityDelayedStart,                  Qt::QueuedConnection))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QtcaptureCityWin,                             this,&BaseWindow::captureCityWin,                           Qt::QueuedConnection))
        abort();

    //connect the map controler
    if(!connect(client,&CatchChallenger::Api_client_real::Qthave_current_player_info,this,&BaseWindow::have_character_position,Qt::QueuedConnection))
        abort();

    //inventory
    if(!connect(client,&CatchChallenger::Api_client_real::Qthave_inventory,     this,&BaseWindow::have_inventory))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::Qtadd_to_inventory,   this,&BaseWindow::add_to_inventory_slot))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::Qtremove_to_inventory,this,&BaseWindow::remove_to_inventory_slot))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QtmonsterCatch,       this,&BaseWindow::monsterCatch))
        abort();

    //character
    if(!connect(client,&CatchChallenger::Api_client_real::QtnewCharacterId,     this,&BaseWindow::newCharacterId))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QthaveCharacter,     this,&BaseWindow::haveCharacter))
        abort();

    //chat
    if(!connect(client,&CatchChallenger::Api_client_real::Qtnew_chat_text,  chat,&Chat::new_chat_text))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::Qtnew_system_text,chat,&Chat::new_system_text))
        abort();

    //plants
    if(!connect(this,&BaseWindow::useSeed,              client,&CatchChallenger::Api_client_real::useSeed))
        abort();
    if(!connect(this,&BaseWindow::collectMaturePlant,   client,&CatchChallenger::Api_client_real::collectMaturePlant))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::Qtinsert_plant,   this,&BaseWindow::insert_plant))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::Qtremove_plant,   this,&BaseWindow::remove_plant))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::Qtseed_planted,   this,&BaseWindow::seed_planted))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::Qtplant_collected,this,&BaseWindow::plant_collected))
        abort();
    //crafting
    if(!connect(client,&CatchChallenger::Api_client_real::QtrecipeUsed,     this,&BaseWindow::recipeUsed))
        abort();
    //trade
    if(!connect(client,&CatchChallenger::Api_client_real::QttradeRequested,             this,&BaseWindow::tradeRequested))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QttradeAcceptedByOther,       this,&BaseWindow::tradeAcceptedByOther))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QttradeCanceledByOther,       this,&BaseWindow::tradeCanceledByOther))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QttradeFinishedByOther,       this,&BaseWindow::tradeFinishedByOther))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QttradeValidatedByTheServer,  this,&BaseWindow::tradeValidatedByTheServer))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QttradeAddTradeCash,          this,&BaseWindow::tradeAddTradeCash))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QttradeAddTradeObject,        this,&BaseWindow::tradeAddTradeObject))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QttradeAddTradeMonster,       this,&BaseWindow::tradeAddTradeMonster))
        abort();
    //inventory
    if(!connect(client,&CatchChallenger::Api_client_real::QtobjectUsed,                 this,&BaseWindow::objectUsed))
        abort();
    //shop
    if(!connect(client,&CatchChallenger::Api_client_real::QthaveShopList,               this,&BaseWindow::haveShopList))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QthaveSellObject,             this,&BaseWindow::haveSellObject))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QthaveBuyObject,              this,&BaseWindow::haveBuyObject))
        abort();
    //factory
    if(!connect(client,&CatchChallenger::Api_client_real::QthaveFactoryList,            this,&BaseWindow::haveFactoryList))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QthaveSellFactoryObject,      this,&BaseWindow::haveSellFactoryObject))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QthaveBuyFactoryObject,       this,&BaseWindow::haveBuyFactoryObject))
        abort();
    //battle
    if(!connect(client,&CatchChallenger::Api_client_real::QtbattleRequested,            this,&BaseWindow::battleRequested))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QtbattleAcceptedByOther,      this,&BaseWindow::battleAcceptedByOther))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QtbattleCanceledByOther,      this,&BaseWindow::battleCanceledByOther))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QtsendBattleReturn,           this,&BaseWindow::sendBattleReturn))
        abort();
    //market
    if(!connect(client,&CatchChallenger::Api_client_real::QtmarketList,                 this,&BaseWindow::marketList))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QtmarketBuy,                  this,&BaseWindow::marketBuy))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QtmarketBuyMonster,           this,&BaseWindow::marketBuyMonster))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QtmarketPut,                  this,&BaseWindow::marketPut))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QtmarketGetCash,              this,&BaseWindow::marketGetCash))
        abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QtmarketWithdrawCanceled,     this,&BaseWindow::marketWithdrawCanceled))
       abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QtmarketWithdrawObject,       this,&BaseWindow::marketWithdrawObject))
       abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QtmarketWithdrawMonster,      this,&BaseWindow::marketWithdrawMonster))
       abort();
    //datapack
    if(!connect(client,&CatchChallenger::Api_client_real::QtdatapackSizeBase,this,&BaseWindow::datapackSize))
       abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QtgatewayCacheUpdate,this,&BaseWindow::gatewayCacheUpdate))
       abort();
    #ifdef CATCHCHALLENGER_MULTI
    if(!connect(static_cast<CatchChallenger::Api_client_real*>(client),&CatchChallenger::Api_client_real::newDatapackFileBase,            this,&BaseWindow::newDatapackFile))
       abort();
    if(!connect(static_cast<CatchChallenger::Api_client_real*>(client),&CatchChallenger::Api_client_real::newDatapackFileMain,            this,&BaseWindow::newDatapackFile))
       abort();
    if(!connect(static_cast<CatchChallenger::Api_client_real*>(client),&CatchChallenger::Api_client_real::newDatapackFileSub,             this,&BaseWindow::newDatapackFile))
       abort();
    if(!connect(static_cast<CatchChallenger::Api_client_real*>(client),&CatchChallenger::Api_client_real::progressingDatapackFileBase,    this,&BaseWindow::progressingDatapackFile))
       abort();
    if(!connect(static_cast<CatchChallenger::Api_client_real*>(client),&CatchChallenger::Api_client_real::progressingDatapackFileMain,    this,&BaseWindow::progressingDatapackFile))
       abort();
    if(!connect(static_cast<CatchChallenger::Api_client_real*>(client),&CatchChallenger::Api_client_real::progressingDatapackFileSub,     this,&BaseWindow::progressingDatapackFile))
       abort();
    #endif

    if(!connect(this,&BaseWindow::destroyObject,client,&CatchChallenger::Api_client_real::destroyObject))
       abort();
    if(!connect(client,&CatchChallenger::Api_client_real::QtteleportTo,this,&BaseWindow::teleportTo,Qt::QueuedConnection))
       abort();
    if(!connect(client,&CatchChallenger::Api_client_real::Qtnumber_of_player,this,&BaseWindow::number_of_player))
       abort();
    if(!connect(client,&CatchChallenger::Api_client_real::Qtrandom_seeds,&fightEngine,&ClientFightEngine::newRandomNumber))
       abort();

    if(!connect(client,&CatchChallenger::Api_client_real::Qtinsert_player,              this,&BaseWindow::insert_player,Qt::QueuedConnection))
       abort();
}

void BaseWindow::tradeRequested(const std::string &pseudo,const uint8_t &skinInt)
{
    WithAnotherPlayer withAnotherPlayerDialog(this,WithAnotherPlayer::WithAnotherPlayerType_Trade,getFrontSkin(skinInt),pseudo);
    withAnotherPlayerDialog.exec();
    if(!withAnotherPlayerDialog.actionIsAccepted())
    {
        client->tradeRefused();
        return;
    }
    client->tradeAccepted();
    tradeAcceptedByOther(pseudo,skinInt);
}

void BaseWindow::battleRequested(const std::string &pseudo, const uint8_t &skinInt)
{
    if(fightEngine.isInFight())
    {
        qDebug() << "already in fight";
        client->battleRefused();
        return;
    }
    WithAnotherPlayer withAnotherPlayerDialog(this,WithAnotherPlayer::WithAnotherPlayerType_Battle,getFrontSkin(skinInt),pseudo);
    withAnotherPlayerDialog.exec();
    if(!withAnotherPlayerDialog.actionIsAccepted())
    {
        client->battleRefused();
        return;
    }
    client->battleAccepted();
}

std::string BaseWindow::lastLocation() const
{
    return mapController->lastLocation();
}

std::unordered_map<uint16_t, PlayerQuest> BaseWindow::getQuests() const
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    return playerInformations.quests;
}

uint16_t BaseWindow::getActualBotId() const
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

void BaseWindow::message(std::string message) const
{
    qDebug() << QString::fromStdString(message);
}

void BaseWindow::stdmessage(std::string message) const
{
    qDebug() << QString::fromStdString(message);
}

void BaseWindow::number_of_player(uint16_t number,uint16_t max)
{
    if(max>=65534)
        ui->frame_main_display_interface_player->hide();
    else
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
}

void BaseWindow::on_toolButton_interface_quit_clicked()
{
    client->tryDisconnect();
}

void BaseWindow::on_toolButton_quit_interface_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_map);
}

void BaseWindow::on_pushButton_interface_trainer_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_player);
}

void BaseWindow::add_to_inventory_slot(const std::unordered_map<uint16_t,uint32_t> &items)
{
    add_to_inventory(items);
}

void BaseWindow::add_to_inventory(const uint16_t &item,const uint32_t &quantity,const bool &showGain)
{
    std::vector<std::pair<uint16_t,uint32_t> > items;
    items.push_back(std::pair<uint16_t,uint32_t>(item,quantity));
    add_to_inventory(items,showGain);
}

void BaseWindow::add_to_inventory(const std::vector<std::pair<uint16_t,uint32_t> > &items,const bool &showGain)
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

void BaseWindow::add_to_inventory(const std::unordered_map<uint16_t,uint32_t> &items,const bool &showGain)
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
            if(DatapackClientLoader::datapackLoader.itemsExtra.find(item)!=DatapackClientLoader::datapackLoader.itemsExtra.cend())
            {
                image=DatapackClientLoader::datapackLoader.itemsExtra.at(item).image;
                name=DatapackClientLoader::datapackLoader.itemsExtra.at(item).name;
            }
            else
            {
                image=DatapackClientLoader::datapackLoader.defaultInventoryImage();
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
        BaseWindow::showGain();
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

void BaseWindow::remove_to_inventory(const uint16_t &itemId,const uint32_t &quantity)
{
    std::unordered_map<uint16_t,uint32_t> items;
    items[itemId]=quantity;
    remove_to_inventory(items);
}

void BaseWindow::remove_to_inventory_slot(const std::unordered_map<uint16_t,uint32_t> &items)
{
    remove_to_inventory(items);
}

void BaseWindow::remove_to_inventory(const std::unordered_map<uint16_t,uint32_t> &items)
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

void BaseWindow::error(std::string error)
{
    emit newError(tr("Error with the protocol").toStdString(),error);
}

void BaseWindow::stderror(const std::string &error)
{
    emit newError(tr("Error with the protocol").toStdString(),error);
}

void BaseWindow::errorWithTheCurrentMap()
{
    if(socketState!=QAbstractSocket::ConnectedState)
        return;
    client->tryDisconnect();
    resetAll();
    QMessageBox::critical(this,tr("Map error"),tr("The current map into the datapack is in error (not found, read failed, wrong format, corrupted, ...)\nReport the bug to the datapack maintainer."));
}

void BaseWindow::repelEffectIsOver()
{
    showTip(tr("The repel effect is over").toStdString());
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
    if(DatapackClientLoader::datapackLoader.itemsExtra.find(items_graphical.at(item))==
            DatapackClientLoader::datapackLoader.itemsExtra.cend())
    {
        ui->inventoryInformation->setVisible(false);
        ui->inventoryUse->setVisible(false);
        ui->inventoryDestroy->setVisible(!inSelection);
        ui->inventory_image->setPixmap(DatapackClientLoader::datapackLoader.defaultInventoryImage());
        ui->inventory_name->setText(tr("Unknown name"));
        ui->inventory_description->setText(tr("Unknown description"));
        return;
    }
    const DatapackClientLoader::ItemExtra &content=DatapackClientLoader::datapackLoader.itemsExtra.at(items_graphical.at(item));
    ui->inventoryDestroy->setVisible(!inSelection);
    ui->inventory_image->setPixmap(content.image);
    ui->inventory_name->setText(QString::fromStdString(content.name));
    ui->inventory_description->setText(QString::fromStdString(content.description));

    ui->inventoryInformation->setVisible(!inSelection &&
                                         /* is a plant */
                                         DatapackClientLoader::datapackLoader.itemToPlants.find(items_graphical.at(item))!=
                                         DatapackClientLoader::datapackLoader.itemToPlants.cend()
                                         );
    bool isRecipe=false;
    {
        /* is a recipe */
        isRecipe=CatchChallenger::CommonDatapack::commonDatapack.itemToCrafingRecipes
                .find(items_graphical.at(item))!=
                CatchChallenger::CommonDatapack::commonDatapack.itemToCrafingRecipes.cend();
        if(isRecipe)
        {
            const uint16_t &recipeId=CatchChallenger::CommonDatapack::commonDatapack.itemToCrafingRecipes.at(items_graphical.at(item));
            const CrafingRecipe &recipe=CatchChallenger::CommonDatapack::commonDatapack.crafingRecipes.at(recipeId);
            if(!haveReputationRequirements(recipe.requirements.reputation))
            {
                std::string string;
                unsigned int index=0;
                while(index<recipe.requirements.reputation.size())
                {
                    string+=reputationRequirementsToText(recipe.requirements.reputation.at(index));
                    index++;
                }
                ui->inventory_description->setText(ui->inventory_description->text()+"<br />"+
                                                   tr("<span style=\"color:#D50000\">Don't meet the requirements: %1</span>")
                                                   .arg(QString::fromStdString(string))
                                                   );
                isRecipe=false;
            }
        }
    }
    ui->inventoryUse->setVisible(inSelection ||
                                 isRecipe
                                 ||
                                 /* is a repel */
                                 CatchChallenger::CommonDatapack::commonDatapack.items.repel.find(items_graphical.at(item))!=
            CatchChallenger::CommonDatapack::commonDatapack.items.repel.cend()
                                 ||
                                 /* is a item with monster effect */
                                 CatchChallenger::CommonDatapack::commonDatapack.items.monsterItemEffect.find(items_graphical.at(item))!=
            CatchChallenger::CommonDatapack::commonDatapack.items.monsterItemEffect.cend()
                                 ||
                                 /* is a item with monster effect out of fight */
                                 (CatchChallenger::CommonDatapack::commonDatapack.items.monsterItemEffectOutOfFight.find(items_graphical.at(item))!=
            CatchChallenger::CommonDatapack::commonDatapack.items.monsterItemEffectOutOfFight.cend() && !fightEngine.isInFight())
                                 ||
                                 /* is a evolution item */
                                 CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.find(items_graphical.at(item))!=
            CatchChallenger::CommonDatapack::commonDatapack.items.evolutionItem.cend()
                                 ||
                                 /* is a evolution item */
                                 (CatchChallenger::CommonDatapack::commonDatapack.items.itemToLearn.find(items_graphical.at(item))!=
            CatchChallenger::CommonDatapack::commonDatapack.items.itemToLearn.cend() && !fightEngine.isInFight())
                                         );
}

void BaseWindow::tipTimeout()
{
    ui->tip->setVisible(false);
}

void BaseWindow::gainTimeout()
{
    unsigned int index=0;
    while(index<add_to_inventoryGainTime.size())
    {
        if(add_to_inventoryGainTime.at(index).elapsed()>TIMETODISPLAY_GAIN)
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
        if(add_to_inventoryGainExtraTime.at(index).elapsed()>TIMETODISPLAY_GAIN)
        {
            add_to_inventoryGainExtraTime.erase(add_to_inventoryGainExtraTime.cbegin()+index);
            add_to_inventoryGainExtraList.erase(add_to_inventoryGainExtraList.cbegin()+index);
        }
        else
            index++;
    }
    if(add_to_inventoryGainTime.empty() && add_to_inventoryGainExtraTime.empty())
        ui->gain->setVisible(false);
    else
    {
        gain_timeout.start();
        composeAndDisplayGain();
    }
}

void BaseWindow::showTip(const std::string &tip)
{
    ui->tip->setVisible(true);
    ui->tip->setText(QString::fromStdString(tip));
    tip_timeout.start();
}

void BaseWindow::showPlace(const std::string &place)
{
    add_to_inventoryGainExtraList.push_back(place);
    add_to_inventoryGainExtraTime.push_back(QTime::currentTime());
    ui->gain->setVisible(true);
    composeAndDisplayGain();
    gain_timeout.start();
}

void BaseWindow::showGain()
{
    gainTimeout();
    ui->gain->setVisible(true);
    composeAndDisplayGain();
    gain_timeout.start();
}

void BaseWindow::composeAndDisplayGain()
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
    ui->gain->setText(QString::fromStdString(text));
}

void BaseWindow::addQuery(const QueryType &queryType)
{
    if(queryList.size()==0)
        ui->persistant_tip->setVisible(true);
    queryList.push_back(queryType);
    updateQueryList();
}

void BaseWindow::removeQuery(const QueryType &queryType)
{
    vectorremoveOne(queryList,queryType);
    if(queryList.size()==0)
        ui->persistant_tip->setVisible(false);
    else
        updateQueryList();
}

void BaseWindow::updateQueryList()
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
    ui->persistant_tip->setText(queryStringList.join("<br />"));
}

void BaseWindow::on_toolButton_quit_options_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_map);
}

void BaseWindow::newEvent(const uint8_t &event,const uint8_t &event_value)
{
    if(this->events.at(event)==event_value)
    {
        std::cerr << "event " << event << "already set on " << event_value << std::endl;
        return;
    }
    forcedEvent(event,event_value);
}

void BaseWindow::forcedEvent(const uint8_t &event,const uint8_t &event_value)
{
    if(!mapController->currentMapIsLoaded())
        return;
    const std::string &type=mapController->currentMapType();
    this->events[event]=event_value;
    //color
    {
        if(DatapackClientLoader::datapackLoader.visualCategories.find(type)!=
                DatapackClientLoader::datapackLoader.visualCategories.cend())
        {
            const std::vector<DatapackClientLoader::VisualCategory::VisualCategoryCondition> &conditions=
                    DatapackClientLoader::datapackLoader.visualCategories.at(type).conditions;
            unsigned int index=0;
            while(index<conditions.size())
            {
                const DatapackClientLoader::VisualCategory::VisualCategoryCondition &condition=conditions.at(index);
                if(condition.event<events.size())
                {
                    if(events.at(condition.event)==condition.eventValue)
                    {
                        mapController->setColor(condition.color,15000);
                        break;
                    }
                }
                else
                    qDebug() << QStringLiteral("event for condition out of range: %1 for %2 event(s)").arg(condition.event).arg(events.size());
                index++;
            }
            if(index==conditions.size())
                mapController->setColor(DatapackClientLoader::datapackLoader.visualCategories.at(type).defaultColor,15000);
        }
        else
            mapController->setColor(Qt::transparent,15000);
    }
}

//network
void BaseWindow::updateRXTX()
{
    if(client==NULL)
        return;
    int updateRXTXTimeElapsed=updateRXTXTime.elapsed();
    if(updateRXTXTimeElapsed==0)
        return;
    uint64_t RXSize=client->getRXSize();
    uint64_t TXSize=client->getTXSize();
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

void BaseWindow::appendReputationRewards(const std::vector<ReputationRewards> &reputationList)
{
    unsigned int index=0;
    while(index<reputationList.size())
    {
        const ReputationRewards &reputationRewards=reputationList.at(index);
        appendReputationPoint(CommonDatapack::commonDatapack.reputation.at(reputationRewards.reputationId).name,reputationRewards.point);
        index++;
    }
    show_reputation();
}

bool BaseWindow::haveReputationRequirements(const std::vector<ReputationRequirements> &reputationList) const
{
    unsigned int index=0;
    while(index<reputationList.size())
    {
        const CatchChallenger::ReputationRequirements &reputation=reputationList.at(index);
        if(client->player_informations.reputation.find(reputation.reputationId)!=client->player_informations.reputation.cend())
        {
            const PlayerReputation &playerReputation=client->player_informations.reputation.at(reputation.reputationId);
            if(!reputation.positif)
            {
                if(-reputation.level<playerReputation.level)
                {
                    emit message(QStringLiteral("reputation.level(%1)<playerReputation.level(%2)")
                                 .arg(reputation.level).arg(playerReputation.level).toStdString());
                    return false;
                }
            }
            else
            {
                if(reputation.level>playerReputation.level || playerReputation.point<0)
                {
                    emit message(QStringLiteral("reputation.level(%1)>playerReputation.level(%2) || playerReputation.point(%3)<0")
                                 .arg(reputation.level).arg(playerReputation.level).arg(playerReputation.point).toStdString());
                    return false;
                }
            }
        }
        else
            if(!reputation.positif)//default level is 0, but required level is negative
            {
                emit message(QStringLiteral("reputation.level(%1)<0 and no reputation.type=%2").arg(reputation.level).arg(
                                 QString::fromStdString(CommonDatapack::commonDatapack.reputation.at(reputation.reputationId).name)
                                 ).toStdString());
                return false;
            }
        index++;
    }
    return true;
}

bool BaseWindow::nextStepQuest(const Quest &quest)
{
    CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations();
    #ifdef DEBUG_CLIENT_QUEST
    qDebug() << "drop quest step requirement for: " << quest.id;
    #endif
    if(playerInformations.quests.find(quest.id)==playerInformations.quests.cend())
    {
        qDebug() << "step out of range for: " << quest.id;
        return false;
    }
    uint8_t step=playerInformations.quests.at(quest.id).step;
    if(step<=0 || step>quest.steps.size())
    {
        qDebug() << "step out of range for: " << quest.id;
        return false;
    }
    const CatchChallenger::Quest::StepRequirements &requirements=quest.steps.at(step-1).requirements;
    unsigned int index=0;
    while(index<requirements.items.size())
    {
        const CatchChallenger::Quest::Item &item=requirements.items.at(index);
        std::unordered_map<uint16_t,uint32_t> items;
        items[item.item]=item.quantity;
        remove_to_inventory(items);
        index++;
    }
    playerInformations.quests[quest.id].step++;
    if(playerInformations.quests.at(quest.id).step>quest.steps.size())
    {
        #ifdef DEBUG_CLIENT_QUEST
        qDebug() << "finish the quest: " << quest.id;
        #endif
        playerInformations.quests[quest.id].step=0;
        playerInformations.quests[quest.id].finish_one_time=true;
        index=0;
        while(index<quest.rewards.reputation.size())
        {
            appendReputationPoint(CommonDatapack::commonDatapack.reputation.at(
                                      quest.rewards.reputation.at(index).reputationId).name,
                                  quest.rewards.reputation.at(index).point);
            index++;
        }
        show_reputation();
        index=0;
        while(index<quest.rewards.allow.size())
        {
            playerInformations.allow.insert(quest.rewards.allow.at(index));
            index++;
        }
    }
    return true;
}

//reputation
void BaseWindow::appendReputationPoint(const std::string &type,const int32_t &point)
{
    if(point==0)
        return;
    if(DatapackClientLoader::datapackLoader.reputationNameToId.find(type)==DatapackClientLoader::datapackLoader.reputationNameToId.cend())
    {
        emit error("Unknow reputation: "+type);
        return;
    }
    const uint8_t &reputationId=DatapackClientLoader::datapackLoader.reputationNameToId.at(type);
    PlayerReputation playerReputation;
    if(client->player_informations.reputation.find(reputationId)!=client->player_informations.reputation.cend())
        playerReputation=client->player_informations.reputation.at(reputationId);
    else
    {
        playerReputation.point=0;
        playerReputation.level=0;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_REPUTATION
    emit message(QStringLiteral("Reputation %1 at level: %2 with point: %3").arg(type).arg(playerReputation.level).arg(playerReputation.point));
    #endif
    PlayerReputation oldPlayerReputation=playerReputation;
    int32_t old_level=playerReputation.level;
    FacilityLib::appendReputationPoint(&playerReputation,point,CommonDatapack::commonDatapack.reputation.at(reputationId));
    if(oldPlayerReputation.level==playerReputation.level && oldPlayerReputation.point==playerReputation.point)
        return;
    if(client->player_informations.reputation.find(reputationId)!=client->player_informations.reputation.cend())
        client->player_informations.reputation[reputationId]=playerReputation;
    else
        client->player_informations.reputation[reputationId]=playerReputation;
    const std::string &reputationCodeName=CommonDatapack::commonDatapack.reputation.at(reputationId).name;
    if(old_level<playerReputation.level)
    {
        if(DatapackClientLoader::datapackLoader.reputationExtra.find(reputationCodeName)!=
                DatapackClientLoader::datapackLoader.reputationExtra.cend())
            showTip(tr("You have better reputation into %1")
                    .arg(QString::fromStdString(DatapackClientLoader::datapackLoader.reputationExtra.at(reputationCodeName).name)).toStdString());
        else
            showTip(tr("You have better reputation into %1")
                    .arg("???").toStdString());
    }
    else if(old_level>playerReputation.level)
    {
        if(DatapackClientLoader::datapackLoader.reputationExtra.find(reputationCodeName)!=
                DatapackClientLoader::datapackLoader.reputationExtra.cend())
            showTip(tr("You have worse reputation into %1")
                    .arg(QString::fromStdString(DatapackClientLoader::datapackLoader.reputationExtra.at(reputationCodeName).name)).toStdString());
        else
            showTip(tr("You have worse reputation into %1")
                    .arg("???").toStdString());
    }
    #ifdef DEBUG_MESSAGE_CLIENT_REPUTATION
    emit message(QStringLiteral("New reputation %1 at level: %2 with point: %3").arg(type).arg(playerReputation.level).arg(playerReputation.point));
    #endif
}

std::string BaseWindow::parseHtmlToDisplay(const std::string &htmlContent)
{
    QString newContent=QString::fromStdString(htmlContent);
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
    return newContent.toStdString();
}

void BaseWindow::objectUsed(const ObjectUsage &objectUsage)
{
    if(objectInUsing.empty())
    {
        emit error("No object usage to validate");
        return;
    }
    switch(objectUsage)
    {
        case ObjectUsage_correctlyUsed:
        {
            const uint16_t item=objectInUsing.front();
            //is crafting recipe
            if(CatchChallenger::CommonDatapack::commonDatapack.itemToCrafingRecipes.find(item)!=
                    CatchChallenger::CommonDatapack::commonDatapack.itemToCrafingRecipes.cend())
            {
                client->addRecipe(CatchChallenger::CommonDatapack::commonDatapack.itemToCrafingRecipes.at(item));
                load_crafting_inventory();
            }
            else if(CommonDatapack::commonDatapack.items.trap.find(item)!=CommonDatapack::commonDatapack.items.trap.cend())
            {
            }
            else if(CatchChallenger::CommonDatapack::commonDatapack.items.repel.find(item)!=
                    CatchChallenger::CommonDatapack::commonDatapack.items.repel.cend())
            {
            }
            else
                qDebug() << "BaseWindow::objectUsed(): unknow object type";
        }
        break;
        case ObjectUsage_failedWithConsumption:
        break;
        case ObjectUsage_failedWithoutConsumption:
            add_to_inventory(objectInUsing.front());
        break;
        default:
        break;
    }
    objectInUsing.erase(objectInUsing.cbegin());
}

void BaseWindow::on_inventoryDestroy_clicked()
{
    Player_private_and_public_informations &playerInformations=client->get_player_informations();
    qDebug() << "on_inventoryDestroy_clicked()";
    const QList<QListWidgetItem *> &items=ui->inventory->selectedItems();
    if(items.size()!=1)
        return;
    const uint16_t itemId=items_graphical.at(items.first());
    if(playerInformations.items.find(itemId)==playerInformations.items.cend())
        return;
    uint32_t quantity=playerInformations.items.at(itemId);
    if(quantity>1)
    {
        bool ok;
        uint32_t quantity_temp=QInputDialog::getInt(this,tr("Destroy"),tr("Quantity to destroy"),quantity,1,quantity,1,&ok);
        if(!ok)
            return;
        quantity=quantity_temp;
    }
    QMessageBox::StandardButton button;
    if(DatapackClientLoader::datapackLoader.itemsExtra.find(itemId)!=DatapackClientLoader::datapackLoader.itemsExtra.cend())
        button=QMessageBox::question(this,tr("Destroy"),tr("Are you sure you want to destroy %1 %2?")
                                     .arg(quantity).arg(QString::fromStdString(DatapackClientLoader::datapackLoader.itemsExtra.at(itemId).name))
                                     ,QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes);
    else
        button=QMessageBox::question(this,tr("Destroy"),tr("Are you sure you want to destroy %1 unknow item (id: %2)?")
                                     .arg(quantity).arg(itemId)
                                     ,QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes);
    if(button!=QMessageBox::Yes)
        return;
    if(playerInformations.items.find(itemId)==playerInformations.items.cend())
        return;
    if(playerInformations.items.at(itemId)<quantity)
        quantity=playerInformations.items.at(itemId);
    emit destroyObject(itemId,quantity);
    remove_to_inventory(itemId,quantity);
    load_inventory();
    load_plant_inventory();
}

uint32_t BaseWindow::itemQuantity(const uint16_t &itemId) const
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    if(playerInformations.items.find(itemId)!=playerInformations.items.cend())
        return playerInformations.items.at(itemId);
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
    if(items_graphical.find(item)==items_graphical.cend())
    {
        qDebug() << "on_inventoryInformation_clicked() item not found here";
        return;
    }
    const uint16_t itemFound=items_graphical.at(item);
    if(DatapackClientLoader::datapackLoader.itemToPlants.find(itemFound)!=
            DatapackClientLoader::datapackLoader.itemToPlants.cend())
    {
        if(plants_items_to_graphical.find(DatapackClientLoader::datapackLoader.itemToPlants.at(itemFound))==plants_items_to_graphical.cend())
        {
            qDebug() << QStringLiteral("on_inventoryInformation_clicked() is not into plant list: item: %1, plant: %2")
                        .arg(itemFound).arg(DatapackClientLoader::datapackLoader.itemToPlants.at(itemFound));
            return;
        }
        ui->listPlantList->reset();
        ui->stackedWidget->setCurrentWidget(ui->page_plants);
        plants_items_to_graphical.at(DatapackClientLoader::datapackLoader.itemToPlants.at(itemFound))->setSelected(true);
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

void BaseWindow::addCash(const uint32_t &cash)
{
    Player_private_and_public_informations &playerInformations=client->get_player_informations();
    playerInformations.cash+=cash;
    ui->player_informations_cash->setText(QStringLiteral("%1$").arg(playerInformations.cash));
    ui->shopCash->setText(tr("Cash: %1$").arg(playerInformations.cash));
    ui->tradePlayerCash->setMaximum(static_cast<uint32_t>(playerInformations.cash));
}

void BaseWindow::removeCash(const uint32_t &cash)
{
    Player_private_and_public_informations &playerInformations=client->get_player_informations();
    playerInformations.cash-=cash;
    ui->player_informations_cash->setText(QStringLiteral("%1$").arg(playerInformations.cash));
    ui->shopCash->setText(tr("Cash: %1$").arg(playerInformations.cash));
    ui->tradePlayerCash->setMaximum(static_cast<uint32_t>(playerInformations.cash));
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
    client->addTradeCash(ui->tradePlayerCash->value()-ui->tradePlayerCash->minimum());
    ui->tradePlayerCash->setMinimum(ui->tradePlayerCash->value());
}

void BaseWindow::on_tradeCancel_clicked()
{
    client->tradeCanceled();

    //return the pending stuff
    {
        //item
        add_to_inventory(tradeCurrentObjects,false);
        tradeCurrentObjects.clear();

        //monster
        while(!tradeCurrentMonsters.empty())
        {
            fightEngine.insertPlayerMonster(tradeCurrentMonstersPosition.front(),tradeCurrentMonsters.front());
            tradeCurrentMonstersPosition.erase(tradeCurrentMonstersPosition.cbegin());
            tradeCurrentMonsters.erase(tradeCurrentMonsters.cbegin());
        }
    }

    ui->stackedWidget->setCurrentWidget(ui->page_map);
}

void BaseWindow::on_tradeValidate_clicked()
{
    client->tradeFinish();
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
    const uint8_t &currentRow=static_cast<uint8_t>(ui->monsterList->row(selectedMonsters.first()));
    if(currentRow<=0)
        return;
    if(!fightEngine.moveUpMonster(currentRow))
        return;
    const bool updateMonsterTile=(currentRow==1);
    client->monsterMoveUp(currentRow+1);
    QListWidgetItem * item=ui->monsterList->takeItem(currentRow);
    ui->monsterList->insertItem(currentRow-1,item);
    ui->monsterList->item(currentRow)->setSelected(false);
    ui->monsterList->item(currentRow-1)->setSelected(true);
    ui->monsterList->setCurrentRow(currentRow-1);
    on_monsterList_itemSelectionChanged();
    if(updateMonsterTile)
        mapController->updatePlayerMonsterTile(fightEngine.monsterByPosition(0)->monster);
}

void BaseWindow::on_monsterListMoveDown_clicked()
{
    QList<QListWidgetItem *> selectedMonsters=ui->monsterList->selectedItems();
    if(selectedMonsters.size()!=1)
        return;
    const int &currentRow=ui->monsterList->row(selectedMonsters.first());
    if(currentRow<0)
        return;
    if(currentRow>=(ui->monsterList->count()-1))
        return;
    if(!fightEngine.moveDownMonster(static_cast<uint8_t>(currentRow)))
        return;
    const bool updateMonsterTile=(currentRow==0);
    client->monsterMoveDown(static_cast<uint8_t>(currentRow+1));
    QListWidgetItem * item=ui->monsterList->takeItem(currentRow);
    ui->monsterList->insertItem(currentRow+1,item);
    ui->monsterList->item(currentRow)->setSelected(false);
    ui->monsterList->item(currentRow+1)->setSelected(true);
    ui->monsterList->setCurrentRow(currentRow+1);
    on_monsterList_itemSelectionChanged();
    if(updateMonsterTile)
        mapController->updatePlayerMonsterTile(fightEngine.monsterByPosition(0)->monster);
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
    const std::vector<PlayerMonster> &playerMonster=fightEngine.getPlayerMonster();
    if(playerMonster.size()<=1)
    {
        ui->monsterListMoveUp->setEnabled(false);
        ui->monsterListMoveDown->setEnabled(false);
        return;
    }
    unsigned int row=ui->monsterList->row(selectedMonsters.first());
    ui->monsterListMoveUp->setEnabled(row>0);
    ui->monsterListMoveDown->setEnabled(row<(playerMonster.size()-1));
}

void BaseWindow::teleportConditionNotRespected(const std::string &text)
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

void BaseWindow::lastReplyTime(const uint32_t &time)
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
    if(client==NULL)
        return;
    uint32_t queryCount=0;
    worseQueryTime=0;
    std::vector<std::string> middleQueryList;
    const std::map<uint8_t,uint64_t> &values=client->getQuerySendTimeList();
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
    if(queryCount>0)
        ui->labelQueryList->setText(
                    tr("Running query: %1 (%3 and %4), query with worse time: %2ms")
                    .arg(queryCount)
                    .arg(worseQueryTime)
                    .arg(QString::fromStdString(stringimplode(client->getQueryRunningList(),';')))
                    .arg(QString::fromStdString(stringimplode(middleQueryList,";")))
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
        //qDebug() << "The last query was slow:" << lastReplyTimeValue << ">" << TIMEINMSTOBESLOW << " && " << lastReplyTimeSince.elapsed() << "<" << TIMETOSHOWTHETURTLE;
        if(ui->labelSlow->isVisible())
            return;
        ui->labelSlow->setToolTip(tr("The last query was slow (%1ms)").arg(lastReplyTimeValue));
        ui->labelSlow->show();
        return;
    }
    if(worseQueryTime>TIMEINMSTOBESLOW)
    {
        if(ui->labelSlow->isVisible())
            return;
        ui->labelSlow->setToolTip(tr("Remain query in suspend (%1ms ago)").arg(worseQueryTime));
        ui->labelSlow->show();
        return;
    }
    if(!ui->labelSlow->isVisible())
        return;
    ui->labelSlow->hide();
}

void BaseWindow::on_serverList_activated(const QModelIndex &index)
{
    Q_UNUSED(index);
    on_serverListSelect_clicked();
}

void BaseWindow::on_serverListSelect_clicked()
{
    const QList<QTreeWidgetItem *> &selectedItems=ui->serverList->selectedItems();
    if(selectedItems.size()!=1)
        return;

    const QTreeWidgetItem * const selectedItem=selectedItems.at(0);
    serverSelected=selectedItem->data(99,99).toUInt();
    const std::vector<ServerFromPoolForDisplay> &serverOrdenedList=client->getServerOrdenedList();
    if(serverSelected<(int)serverOrdenedList.size())
        updateConnectingStatus();
    else
        error("BaseWindow::on_serverListSelect_clicked(), wrong serverSelected internal data");
}

void CatchChallenger::BaseWindow::on_toolButton_quit_encyclopedia_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_map);
}

void CatchChallenger::BaseWindow::on_checkBoxEncyclopediaMonsterKnown_toggled(bool checked)
{
    Q_UNUSED(checked);
    const Player_private_and_public_informations &informations=client->get_player_informations();
    ui->listWidgetEncyclopediaMonster->clear();
    ui->labelEncyclopediaMonster->setText("");
    if(informations.encyclopedia_monster!=NULL)
    {
        bool firstFound=false;
        std::vector<uint16_t> keys;
        for(const auto n : DatapackClientLoader::datapackLoader.monsterExtra)
            keys.push_back(n.first);
        qSort(keys.begin(),keys.end());
        uint16_t max=keys.back();
        while(max>0 && !(informations.encyclopedia_monster[max/8] & (1<<(7-max%8))))
            max--;
        uint32_t i=0;
        while (i<max)
        {
            const uint16_t &monsterId=keys.at(i);
            const DatapackClientLoader::MonsterExtra &monsterExtra=DatapackClientLoader::datapackLoader.monsterExtra.at(monsterId);
            QListWidgetItem *item=new QListWidgetItem();
            const uint16_t &bitgetUp=monsterId;
            if(informations.encyclopedia_monster[bitgetUp/8] & (1<<(7-bitgetUp%8)))
            {
                item->setText(QString::fromStdString(monsterExtra.name));
                item->setData(99,monsterId);
                item->setIcon(QIcon(monsterExtra.thumb));
                firstFound=true;
            }
            else
            {
                item->setTextColor(QColor(100,100,100));
                item->setText(tr("Unknown"));
                item->setData(99,0);
                item->setIcon(QIcon(":/images/monsters/default/small.png"));
                if(ui->checkBoxEncyclopediaMonsterKnown->isChecked())
                    firstFound=false;
            }
            if(firstFound)
                ui->listWidgetEncyclopediaMonster->addItem(item);
            ++i;
        }
    }
}

void CatchChallenger::BaseWindow::on_checkBoxEncyclopediaItemKnown_toggled(bool checked)
{
    Q_UNUSED(checked);
    const Player_private_and_public_informations &informations=client->get_player_informations();
    ui->listWidgetEncyclopediaItem->clear();
    ui->labelEncyclopediaItem->setText("");
    if(informations.encyclopedia_item!=NULL)
    {
        bool firstFound=false;
        std::vector<uint16_t> keys;
        for(const auto n : DatapackClientLoader::datapackLoader.itemsExtra)
            keys.push_back(n.first);
        qSort(keys.begin(),keys.end());
        uint16_t max=keys.back();
        while(max>0 && !(informations.encyclopedia_item[max/8] & (1<<(7-max%8))))
            max--;
        uint32_t i=0;
        while (i<max)
        {
            const uint16_t &itemId=keys.at(i);
            const DatapackClientLoader::ItemExtra &itemsExtra=DatapackClientLoader::datapackLoader.itemsExtra.at(itemId);
            QListWidgetItem *item=new QListWidgetItem();
            const uint16_t &bitgetUp=itemId;
            if(informations.encyclopedia_item[bitgetUp/8] & (1<<(7-bitgetUp%8)))
            {
                item->setText(QString::fromStdString(itemsExtra.name));
                item->setData(99,itemId);
                item->setIcon(QIcon(itemsExtra.image));
                firstFound=true;
            }
            else
            {
                item->setTextColor(QColor(100,100,100));
                item->setText(tr("Unknown"));
                item->setData(99,0);
                //item->setIcon(QIcon(":/images/monsters/default/small.png"));
                if(ui->checkBoxEncyclopediaItemKnown->isChecked())
                    firstFound=false;
            }
            if(firstFound)
                ui->listWidgetEncyclopediaItem->addItem(item);
            ++i;
        }
    }
}

void CatchChallenger::BaseWindow::on_toolButtonAdmin_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_admin);
    ui->listNearPlayer->clear();
    {
        const Player_private_and_public_informations &player_private_and_public_informations=client->get_player_informations_ro();
        QListWidgetItem *item=new QListWidgetItem();
        item->setText(QString::fromStdString(player_private_and_public_informations.public_informations.pseudo));
        item->setData(99,QString::fromStdString(player_private_and_public_informations.public_informations.pseudo));
        item->setIcon(getFrontSkin(player_private_and_public_informations.public_informations.skinId));
        ui->listNearPlayer->addItem(item);
    }
    if(multiplayer)
    {
        const std::unordered_map<uint16_t,MapControllerMP::OtherPlayer> &playerList=mapController->getOtherPlayerList();

        for( const auto& n : playerList ) {
                const MapControllerMP::OtherPlayer &player = n.second;

                QListWidgetItem *item=new QListWidgetItem();
                item->setText(QString::fromStdString(player.informations.pseudo));
                item->setData(99,QString::fromStdString(player.informations.pseudo));
                item->setIcon(getFrontSkin(player.informations.skinId));
                ui->listNearPlayer->addItem(item);
        }
    }
}

void CatchChallenger::BaseWindow::on_toolButton_quit_admin_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_map);
}

char CatchChallenger::BaseWindow::my_toupper(char ch)
{
    return std::toupper(static_cast<unsigned char>(ch));
}

std::string CatchChallenger::BaseWindow::str_toupper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                // static_cast<int(*)(int)>(std::toupper)         // wrong
                // [](int c){ return std::toupper(c); }           // wrong
                // [](char c){ return std::toupper(c); }          // wrong
                   [](unsigned char c){ return std::toupper(c); } // correct
                  );
    return s;
}

void CatchChallenger::BaseWindow::on_itemFilterAdmin_returnPressed()
{
    ui->listAllItem->clear();
    const std::string &text=ui->itemFilterAdmin->text().toUpper().toStdString();
    for (const auto &n : DatapackClientLoader::datapackLoader.itemsExtra)
    {
        const uint16_t &itemId=n.first;
        const DatapackClientLoader::ItemExtra &itemsExtra=n.second;
        const std::string up=str_toupper(itemsExtra.name);
        if(text.empty() || up.find(text) != std::string::npos || std::to_string(itemId).find(text) != std::string::npos)
        {
            QListWidgetItem *item=new QListWidgetItem();
            item->setText(QString::fromStdString(itemsExtra.name));
            item->setData(99,itemId);
            item->setIcon(QIcon(itemsExtra.image));
            ui->listAllItem->addItem(item);
        }
    }
}

void CatchChallenger::BaseWindow::on_playerGiveAdmin_clicked()
{
    QList<QListWidgetItem *> players=ui->listNearPlayer->selectedItems();
    if(players.size()!=1)
        return;
    QList<QListWidgetItem *> items=ui->listAllItem->selectedItems();
    if(items.size()!=1)
        return;
    client->sendChatText(CatchChallenger::Chat_type_local,"/give "+items.first()->data(99).toString().toStdString()+
                         " "+players.first()->data(99).toString().toStdString());
    ui->stackedWidget->setCurrentWidget(ui->page_map);
}

void CatchChallenger::BaseWindow::on_listNearPlayer_itemActivated(QListWidgetItem *item)
{
    item->setSelected(true);
    ui->listAllItem->setFocus();
}

void CatchChallenger::BaseWindow::on_listAllItem_itemActivated(QListWidgetItem *item)
{
    item->setSelected(true);
    on_playerGiveAdmin_clicked();
}
