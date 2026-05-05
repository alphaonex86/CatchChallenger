#include "ScreenTransition.hpp"
#if defined(CATCHCHALLENGER_SOLO) && defined(CATCHCHALLENGER_SOLO)
#include "../../server/qt/InternalServer.hpp"
#include "../libqtcatchchallenger/Settings.hpp"
#include "../libqtcatchchallenger/SoloDatabaseInit.hpp"
#endif
#include <QStandardPaths>
#include "../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../libqtcatchchallenger/CliClientOptions.hpp"
#include "background/CCMap.hpp"
#include "foreground/CharacterList.hpp"
#include "foreground/LoadingScreen.hpp"
#include "foreground/MainScreen.hpp"
#include "foreground/Multi.hpp"
#include "foreground/SubServer.hpp"
#include "foreground/OverMapLogic.hpp"
#include "above/OptionsDialog.hpp"
#include "above/DebugDialog.hpp"
#include "ConnexionManager.hpp"
#include "../libqtcatchchallenger/CliClientOptions.hpp"
#include "../libcatchchallenger/Api_protocol.hpp"
#include <QTimer>
#include <QKeyEvent>
#include "../../general/base/Version.hpp"
#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/CommonSettingsServer.hpp"
#include "../../general/base/CompressionProtocol.hpp"
#ifndef CATCHCHALLENGER_NOAUDIO
#include "AudioGL.hpp"
#endif
#include <iostream>
#include <QOpenGLWidget>
#include <QGuiApplication>
#include <QComboBox>
#ifdef Q_OS_ANDROID
#include <QJniObject>
#include <QJniEnvironment>
#include <QtCore/qnativeinterface.h>

void keep_screen_on(bool on) {
  QNativeInterface::QAndroidApplication::runOnAndroidMainThread([on]{
    QJniObject activity = QJniObject(QNativeInterface::QAndroidApplication::context());
    if (activity.isValid()) {
      QJniObject window =
          activity.callObjectMethod("getWindow", "()Landroid/view/Window;");

      if (window.isValid()) {
        const int FLAG_KEEP_SCREEN_ON = 128;
        if (on) {
          window.callMethod<void>("addFlags", "(I)V", FLAG_KEEP_SCREEN_ON);
        } else {
          window.callMethod<void>("clearFlags", "(I)V", FLAG_KEEP_SCREEN_ON);
        }
      }
    }
    QJniEnvironment env;
    if (env->ExceptionCheck()) {
      env->ExceptionClear();
    }
    return QVariant();
  });
}
#endif

ScreenTransition::ScreenTransition() :
    mScene(new QGraphicsScene(this))
{
    {
        const QString platform=QGuiApplication::platformName();
        if(platform!=QLatin1String("offscreen") && platform!=QLatin1String("minimal")) {
            QOpenGLWidget *context=new QOpenGLWidget();
            setViewport(context);
            setRenderHint(QPainter::Antialiasing,true);
            setRenderHint(QPainter::TextAntialiasing,true);
            setRenderHint(QPainter::SmoothPixmapTransform,false);
        }
        //else use the CPU only
    }
    multiplaySelected=false;
    mousePress=nullptr;
    m_backgroundStack=nullptr;
    m_foregroundStack=nullptr;
    m_aboveStack=nullptr;
    subserver=nullptr;
    characterList=nullptr;
    connexionManager=nullptr;
    ccmap=nullptr;
    overmap=nullptr;
    m=nullptr;
    o=nullptr;
    d=nullptr;
    /*solo=nullptr;*/
    multi=nullptr;
    login=nullptr;
    inGame=false;
    #if defined(CATCHCHALLENGER_SOLO) && defined(CATCHCHALLENGER_SOLO)
    internalServer=nullptr;
    #endif
    setBackground(&b);
    setForeground(&l);
    if(!connect(&l,&LoadingScreen::finished,this,&ScreenTransition::loadingFinished))
        abort();
    #ifndef NOTHREADS
    threadSolo=new QThread(this);
    #endif

    timerUpdateFPS.setSingleShot(true);
    timerUpdateFPS.setInterval(1000);
    timeUpdateFPS.restart();
    frameCounter=0;
    timeRender.restart();
    waitRenderTime=40;
    timerRender.setSingleShot(true);
    if(!connect(&timerRender,&QTimer::timeout,this,&ScreenTransition::render))
        abort();
    if(!connect(&timerUpdateFPS,&QTimer::timeout,this,&ScreenTransition::updateFPS))
        abort();
    timerUpdateFPS.start();

    setScene(mScene);
    mScene->setSceneRect(QRectF(0,0,width(),height()));
    setOptimizationFlags(QGraphicsView::DontAdjustForAntialiasing | QGraphicsView::DontSavePainterState | QGraphicsView::IndirectPainting);
    setBackgroundBrush(Qt::black);
    setFrameStyle(0);
    viewport()->setAttribute(Qt::WA_StaticContents);
    viewport()->setAttribute(Qt::WA_TranslucentBackground);
    viewport()->setAttribute(Qt::WA_NoSystemBackground);
    setViewportUpdateMode(QGraphicsView::NoViewportUpdate);

    #ifdef Q_OS_ANDROID
    keep_screen_on(true);
    #endif

    imageText=new QGraphicsPixmapItem();
    mScene->addItem(imageText);
    imageText->setPos(0,0);
    imageText->setZValue(999);
    // --take-screenshot wants a clean reference image; hide the FPS counter
    // overlay so it doesn't bleed pixels into the saved PNG (the FPS digit
    // varies run-to-run and would defeat pixel-diff regression testing).
    if(!CliClientOptions::takeScreenshotPath.isEmpty())
        imageText->setVisible(false);

    render();
    setTargetFPS(100);
}

ScreenTransition::~ScreenTransition()
{
    #ifndef NOTHREADS
    threadSolo->exit();
    threadSolo->wait();
    threadSolo->deleteLater();
    #endif
}

void ScreenTransition::resizeEvent(QResizeEvent *event)
{
    mScene->setSceneRect(QRectF(0,0,width(),height()));
    (void)event;
}

void ScreenTransition::mousePressEvent(QMouseEvent *event)
{
    /*const QPointF p=mapToScene(event->pos());
    const qreal x=p.x();
    const qreal y=p.y();*/
/*    std::cerr << "void ScreenTransition::mousePressEvent(QGraphicsSceneMouseEvent *event) "
              << std::to_string(x) << "," << std::to_string(y) << std::endl;*/
    /*if(button->boundingRect().contains(x,y))
        button->setPressed(true);*/
    bool callParentClass=false;
    const QPointF &p=mapToScene(event->pos());
    if(mousePress!=nullptr)
    {
        bool temp=true;//don¡t do action if true
        static_cast<ScreenInput *>(mousePress)->mouseReleaseEventXY(p,temp,callParentClass);
        mousePress=nullptr;
    }
    bool temp=false;
    if(m_aboveStack!=nullptr)
    {
        static_cast<ScreenInput *>(m_aboveStack)->mousePressEventXY(p,temp,callParentClass);
        mousePress=m_aboveStack;
    }
    if(!temp && m_foregroundStack!=nullptr)
    {
        static_cast<ScreenInput *>(m_foregroundStack)->mousePressEventXY(p,temp,callParentClass);
        mousePress=m_foregroundStack;
    }
    if(!temp && m_backgroundStack!=nullptr)
    {
        static_cast<ScreenInput *>(m_backgroundStack)->mousePressEventXY(p,temp,callParentClass);
        mousePress=m_backgroundStack;
    }
    if(!temp || callParentClass)
        QGraphicsView::mousePressEvent(event);
}

void ScreenTransition::mouseReleaseEvent(QMouseEvent *event)
{
    /*const QPointF &p=mapToScene(event->pos());
    const qreal x=p.x();
    const qreal y=p.y();*/
    /*std::cerr << "void ScreenTransition::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) "
              << std::to_string(x) << "," << std::to_string(y) << std::endl;*/
    /*if(button->boundingRect().contains(x,y))
        button->emitclicked();
    button->setPressed(false);*/
    bool callParentClass=false;
    const QPointF &p=mapToScene(event->pos());
    mousePress=nullptr;
    bool pressValidated=false;
    if(m_aboveStack!=nullptr)
    {
        m_aboveStack->mouseReleaseEventXY(p,pressValidated,callParentClass);
        mousePress=m_aboveStack;
    }
    if(!pressValidated && m_foregroundStack!=nullptr)
    {
        m_foregroundStack->mouseReleaseEventXY(p,pressValidated,callParentClass);
        mousePress=m_foregroundStack;
    }
    if(!pressValidated && m_backgroundStack!=nullptr)
    {
        m_backgroundStack->mouseReleaseEventXY(p,pressValidated,callParentClass);
        mousePress=m_backgroundStack;
    }
    if(!pressValidated || callParentClass)
        QGraphicsView::mouseReleaseEvent(event);
}

void ScreenTransition::mouseMoveEvent(QMouseEvent *event)
{
    /*const QPointF &p=mapToScene(event->pos());
    const qreal x=p.x();
    const qreal y=p.y();*/
    /*std::cerr << "void ScreenTransition::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) "
              << std::to_string(x) << "," << std::to_string(y) << std::endl;*/
    /*if(button->boundingRect().contains(x,y))
        button->emitclicked();
    button->setPressed(false);*/
    bool callParentClass=false;
    const QPointF &p=mapToScene(event->pos());
    mousePress=nullptr;
    bool pressValidated=false;
    if(m_aboveStack!=nullptr)
    {
        m_aboveStack->mouseMoveEventXY(p,pressValidated,callParentClass);
        mousePress=m_aboveStack;
    }
    else if(m_foregroundStack!=nullptr)
    {
        m_foregroundStack->mouseMoveEventXY(p,pressValidated,callParentClass);
        mousePress=m_foregroundStack;
    }
    if(!pressValidated)
        QGraphicsView::mouseMoveEvent(event);
}

void ScreenTransition::closeEvent(QCloseEvent *event)
{
    std::cout << " ScreenTransition::closeEvent()" << std::endl;
    inGame=false;
    #if defined(CATCHCHALLENGER_SOLO) && defined(CATCHCHALLENGER_SOLO)
    if(internalServer!=nullptr)
    {
        if(!internalServer->isFinished())
        {
            //wait the server need be terminated
            std::cout << "ScreenTransition::closeEvent): internalServer!=nullptr" << std::endl;
            hide();
            if(connexionManager!=nullptr)
                if(connexionManager->client!=nullptr)
                    connexionManager->client->disconnectFromHost();
            ///internalServer->stop();
            event->accept();
            return;
        }
        else
            std::cout << "ScreenTransition::closeEvent: internalServer->isFinished()" << std::endl;
    }
    #endif
    QWidget::closeEvent(event);
    QCoreApplication::quit();
}

void ScreenTransition::setBackground(ScreenInput *widget)
{
    if(m_backgroundStack!=nullptr)
    {
        mScene->clearFocus();
        mScene->removeItem(m_backgroundStack);
    }
    m_backgroundStack=widget;
    if(widget!=nullptr)
    {
        connect(widget,&ScreenInput::setAbove,this,&ScreenTransition::setAbove,Qt::UniqueConnection);
        connect(widget,&ScreenInput::setForeground,this,&ScreenTransition::setForegroundInGame,Qt::UniqueConnection);
        mScene->addItem(m_backgroundStack);
        m_backgroundStack->setZValue(-1);
    }
}

void ScreenTransition::setForeground(ScreenInput *widget)
{
    if(mousePress!=nullptr)
    {
        bool temp=true;//don¡t do action if true
        static_cast<ScreenInput *>(mousePress)->mouseReleaseEventXY(QPointF(0.0,0.0),temp,temp);
        mousePress=nullptr;
    }
    if(ccmap!=nullptr)
        ccmap->keyPressReset();
    if(m_foregroundStack!=nullptr)
    {
        mScene->clearFocus();
        mScene->removeItem(m_foregroundStack);
    }
    m_foregroundStack=widget;
    if(widget!=nullptr)
    {
        connect(widget,&ScreenInput::setAbove,this,&ScreenTransition::setAbove,Qt::UniqueConnection);
        connect(widget,&ScreenInput::setForeground,this,&ScreenTransition::setForegroundInGame,Qt::UniqueConnection);
        mScene->addItem(m_foregroundStack);
    }
}

void ScreenTransition::setForegroundInGame(ScreenInput *widget)
{
    if(!inGame)
        return;
    setForeground(widget);
}

void ScreenTransition::setAbove(ScreenInput *widget)
{
    if(mousePress!=nullptr)
    {
        bool temp=true;//don¡t do action if true
        static_cast<ScreenInput *>(mousePress)->mouseReleaseEventXY(QPointF(0.0,0.0),temp,temp);
        mousePress=nullptr;
    }
    if(ccmap!=nullptr)
        ccmap->keyPressReset();
    if(m_aboveStack!=nullptr)
    {
        mScene->clearFocus();
        mScene->removeItem(m_aboveStack);
    }
    m_aboveStack=widget;
    if(widget!=nullptr)
    {
        connect(widget,&ScreenInput::setAbove,this,&ScreenTransition::setAbove,Qt::UniqueConnection);
        connect(widget,&ScreenInput::setForeground,this,&ScreenTransition::setForegroundInGame,Qt::UniqueConnection);
        mScene->addItem(m_aboveStack);
    }
}

void ScreenTransition::loadingFinished()
{
    if(connexionManager==nullptr)
        toMainScreen();
    else if(connexionManager->client==nullptr)
        toMainScreen();
    else if(!connexionManager->client->getIsLogged())
        toMainScreen();
    else if(!connexionManager->client->getCaracterSelected())
        toSubServer();
    else
        toInGame();
}

void ScreenTransition::toSubServer()
{
    if(subserver==nullptr)
    {
        subserver=new SubServer();
        if(!connect(subserver,&SubServer::connectToSubServer,this,&ScreenTransition::connectToSubServer))
            abort();
        if(!connect(subserver,&SubServer::backMulti,this,&ScreenTransition::toMainScreen))
            abort();
    }
    setForeground(subserver);
}

void ScreenTransition::toInGame()
{
    if(characterList==nullptr)
    {
        characterList=new CharacterList();
        if(!connect(characterList,&CharacterList::backSubServer,this,&ScreenTransition::toSubServer))
            abort();
        if(!connect(characterList,&CharacterList::selectCharacter,this,&ScreenTransition::selectCharacter))
            abort();
    }
    setForeground(characterList);
}

void ScreenTransition::toMainScreen()
{
    QtDatapackClientLoader::datapackLoader=new QtDatapackClientLoader();
    #ifndef CATCHCHALLENGER_NOAUDIO
    if(Audio::audio==nullptr)
    {
        AudioGL* a=new AudioGL();
        Audio::audio=a;
        const QString platform=QGuiApplication::platformName();
        if(platform==QLatin1String("offscreen") || platform==QLatin1String("minimal"))
            a->setVolume(0);
        if(!connect(QCoreApplication::instance(),&QCoreApplication::aboutToQuit,a,&AudioGL::stopCurrentAmbiance,Qt::DirectConnection))
            abort();
    }
    #endif
    if(m==nullptr)
    {
        m=new MainScreen();
        if(!connect(m,&MainScreen::goToOptions,this,&ScreenTransition::openOptions))
            abort();
        if(!connect(m,&MainScreen::goToDebug,this,&ScreenTransition::openDebug))
            abort();
        if(!connect(m,&MainScreen::goToSolo,this,&ScreenTransition::openSolo))
            abort();
        if(!connect(m,&MainScreen::goToMulti,this,&ScreenTransition::openMulti))
            abort();
    }
    setForeground(m);
    setWindowTitle(tr("CatchChallenger %1").arg(QString::fromStdString(CatchChallenger::Version::str)));
    //if --autosolo, auto open the Solo screen (only the first time)
    static bool autoOpenSoloTried=false;
    if(!autoOpenSoloTried && CliClientOptions::autosolo)
    {
        autoOpenSoloTried=true;
        openSolo();
        return;
    }
    //if --host/--port or --server/--port CLI args set, connect directly (no server list entry)
    static bool autoDirectConnectTried=false;
    if(!autoDirectConnectTried && CliClientOptions::port!=0 &&
       (!CliClientOptions::host.isEmpty() || !CliClientOptions::serverName.isEmpty()))
    {
        autoDirectConnectTried=true;
        multiplaySelected=true;
        const QString &h=CliClientOptions::host.isEmpty() ? CliClientOptions::serverName : CliClientOptions::host;
        ConnexionInfo ci;
        ci.host=h;
        ci.port=CliClientOptions::port;
        ci.name=h+QStringLiteral(":")+QString::number(CliClientOptions::port);
        ci.unique_code=h+QStringLiteral("-")+QString::number(CliClientOptions::port);
        ci.isArgument=true;
        const QString login=CliClientOptions::characterName.isEmpty() ? QStringLiteral("test01") : CliClientOptions::characterName;
        const QString pass=login+login;
        connectToServer(ci,login,pass);
        return;
    }
    //if --url CLI arg set (WebSocket direct-connect counterpart of
    //--host/--port), connect directly via ws/wss URL with no server-list
    //entry. The label/unique_code is derived from the URL so the runtime
    //has a stable identity even when the user supplies no friendly name.
    static bool autoDirectWsConnectTried=false;
    if(!autoDirectWsConnectTried && !CliClientOptions::url.isEmpty())
    {
        autoDirectWsConnectTried=true;
        multiplaySelected=true;
        ConnexionInfo ci;
        ci.ws=CliClientOptions::url;
        ci.name=CliClientOptions::url;
        ci.unique_code=CliClientOptions::url;
        ci.port=0;
        ci.isArgument=true;
        const QString login=CliClientOptions::characterName.isEmpty() ? QStringLiteral("test01") : CliClientOptions::characterName;
        const QString pass=login+login;
        connectToServer(ci,login,pass);
        return;
    }
    //if --server CLI arg was set (without --port), open the Multi screen to match by name
    static bool autoOpenMultiTried=false;
    if(!autoOpenMultiTried && !CliClientOptions::serverName.isEmpty())
    {
        autoOpenMultiTried=true;
        openMulti();
    }
}

void ScreenTransition::openOptions()
{
    if(o==nullptr)
    {
        o=new OptionsDialog();
        if(!connect(o,&OptionsDialog::removeAbove,this,&ScreenTransition::removeAbove))
            abort();
    }
    setAbove(o);
}

void ScreenTransition::openDebug()
{
    if(d==nullptr)
    {
        d=new DebugDialog();
        if(!connect(d,&DebugDialog::removeAbove,this,&ScreenTransition::removeAbove))
            abort();
    }
    setAbove(d);
}

void ScreenTransition::removeAbove()
{
    setAbove(nullptr);
}

void ScreenTransition::openSolo()
{
    multiplaySelected=false;
    CommonSettingsServer::commonSettingsServer.mainDatapackCode="[main]";
    CommonSettingsServer::commonSettingsServer.subDatapackCode="[sub]";
    QStringList l=QStandardPaths::standardLocations(QStandardPaths::AppLocalDataLocation);
    if(l.empty())
    {
        errorString(tr("No writable path").toStdString());
        return;
    }
    QString savegamesPath=l.first()+"/solo/";
    if(m!=nullptr)
        m->setError(std::string());
    #if defined(CATCHCHALLENGER_SOLO) && defined(CATCHCHALLENGER_SOLO)
    //locate the new folder and create it
    if(!QDir().mkpath(savegamesPath))
    {
        errorString(tr("Unable to write savegame into: %1").arg(savegamesPath).toStdString());
        return;
    }

    //initialize the db from the embedded SQL schema (with checksum file) if missing,
    //otherwise validate the existing savegame's schema checksum before reusing it.
    {
        const QString dbPath=savegamesPath+"catchchallenger.db.sqlite";
        if(!QFile(dbPath).exists())
        {
            QString initError;
            if(!SoloDatabaseInit::createSavegame(dbPath,&initError))
            {
                std::cerr << "Unable to create savegame db at "
                          << dbPath.toStdString() << ": "
                          << initError.toStdString() << std::endl;
                errorString(QStringLiteral("Unable to create savegame db: %1").arg(initError).toStdString());
                CatchChallenger::FacilityLibGeneral::rmpath(savegamesPath.toStdString());
                return;
            }
        }
        else if(!SoloDatabaseInit::isSavegameValid(dbPath))
        {
            std::cerr << "Savegame schema checksum mismatch for: "
                      << dbPath.toStdString() << std::endl;
            errorString(QStringLiteral("Incompatible version: %1").arg(dbPath).toStdString());
            return;
        }
    }

    if(!Settings::settings->contains("pass"))
    {
        //initialise the pass
        QString pass=QString::fromStdString(CatchChallenger::FacilityLibGeneral::randomPassword("abcdefghijklmnopqurstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890",32));

        //initialise the meta data
        Settings::settings->setValue("pass",pass);
        Settings::settings->sync();
        if(Settings::settings->status()!=QSettings::NoError)
        {
            errorString(QStringLiteral("Unable to write savegame pass into: %1").arg(savegamesPath).toStdString());
            CatchChallenger::FacilityLibGeneral::rmpath(savegamesPath.toStdString());
            return;
        }
    }

    if(internalServer!=nullptr)
    {
        internalServer->deleteLater();
        internalServer=nullptr;
    }
    if(internalServer==nullptr)
    {
        internalServer=new CatchChallenger::InternalServer();
        #ifndef NOTHREADS
        threadSolo->start();
        //internalServer->moveToThread(threadSolo);//-> Timers cannot be stopped from another thread, fixed by using QThread InternalServer::run()
        #endif
    }
    if(!connect(internalServer,&CatchChallenger::InternalServer::is_started,this,&ScreenTransition::is_started,Qt::QueuedConnection))
    {
        std::cerr << "aborted at " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
        abort();
    }
    if(!connect(internalServer,&CatchChallenger::InternalServer::error,this,&ScreenTransition::errorString,Qt::QueuedConnection))
    {
        std::cerr << "aborted at " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
        abort();
    }
    toLoading("Open the local game");

    {
        //std::string datapackPathBase=client->datapackPathBase();
        std::string datapackPathBase=QCoreApplication::applicationDirPath().toStdString()+"/datapack/internal/";
        CatchChallenger::GameServerSettings formatedServerSettings=internalServer->getSettings();

        CommonSettingsServer::commonSettingsServer.waitBeforeConnectAfterKick=0;
        CommonSettingsCommon::commonSettingsCommon.max_character=1;
        CommonSettingsCommon::commonSettingsCommon.min_character=1;

        formatedServerSettings.automatic_account_creation=true;
        formatedServerSettings.max_players=99;//> 1 to allow open to lan
        formatedServerSettings.sendPlayerNumber = true;// to work with open to lan
        //formatedServerSettings.compressionType=CompressionProtocol::CompressionType::None;
        formatedServerSettings.compressionType=CompressionProtocol::CompressionType::Zstandard;// to allow open to lan
        formatedServerSettings.everyBodyIsRoot                                      = true;
        formatedServerSettings.teleportIfMapNotFoundOrOutOfMap                       = true;

        formatedServerSettings.database_login.tryOpenType=CatchChallenger::DatabaseBase::DatabaseType::SQLite;
        formatedServerSettings.database_login.file=(savegamesPath+QStringLiteral("catchchallenger.db.sqlite")).toStdString();
        formatedServerSettings.database_base.tryOpenType=CatchChallenger::DatabaseBase::DatabaseType::SQLite;
        formatedServerSettings.database_base.file=(savegamesPath+QStringLiteral("catchchallenger.db.sqlite")).toStdString();
        formatedServerSettings.database_common.tryOpenType=CatchChallenger::DatabaseBase::DatabaseType::SQLite;
        formatedServerSettings.database_common.file=(savegamesPath+QStringLiteral("catchchallenger.db.sqlite")).toStdString();
        formatedServerSettings.database_server.tryOpenType=CatchChallenger::DatabaseBase::DatabaseType::SQLite;
        formatedServerSettings.database_server.file=(savegamesPath+QStringLiteral("catchchallenger.db.sqlite")).toStdString();
        //formatedServerSettings.mapVisibility.mapVisibilityAlgorithm	= CatchChallenger::MapVisibilityAlgorithmSelection_None;
        formatedServerSettings.mapVisibility.enable=true;// to allow open to lan
        formatedServerSettings.datapack_basePath=datapackPathBase;

        {
            CatchChallenger::GameServerSettings::ProgrammedEvent &event=formatedServerSettings.programmedEventList["day"]["day"];
            event.cycle=60;
            event.offset=0;
            event.value="day";
        }
        {
            CatchChallenger::GameServerSettings::ProgrammedEvent &event=formatedServerSettings.programmedEventList["day"]["night"];
            event.cycle=60;
            event.offset=30;
            event.value="night";
        }
        if(!CliClientOptions::mainDatapackCodeOverride.isEmpty())
            CommonSettingsServer::commonSettingsServer.mainDatapackCode=CliClientOptions::mainDatapackCodeOverride.toStdString();
        else
        {
            const QFileInfoList &list=QDir(QString::fromStdString(datapackPathBase)+"/map/main/").entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name);
            if(list.isEmpty())
            {
                errorString((tr("No main code detected into the current datapack, base, check: ")+QString::fromStdString(datapackPathBase)+"/map/main/").toStdString());
                return;
            }
            CommonSettingsServer::commonSettingsServer.mainDatapackCode=list.at(0).fileName().toStdString();
        }
        QDir mainDir(QString::fromStdString(datapackPathBase)+"map/main/"+
                     QString::fromStdString(CommonSettingsServer::commonSettingsServer.mainDatapackCode)+"/");
        if(!mainDir.exists())
        {
            const QFileInfoList &list=QDir(QString::fromStdString(datapackPathBase)+"/map/main/")
                    .entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name);
            if(list.isEmpty())
            {
                errorString((tr("No main code detected into the current datapack, empty main, check: ")+QString::fromStdString(datapackPathBase)+"/map/main/").toStdString());
                return;
            }
            CommonSettingsServer::commonSettingsServer.mainDatapackCode=list.at(0).fileName().toStdString();
        }

        {
            const QFileInfoList &list=QDir(QString::fromStdString(datapackPathBase)+
                                           "/map/main/"+QString::fromStdString(CommonSettingsServer::commonSettingsServer.mainDatapackCode)+"/sub/")
                    .entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name);
            if(!list.isEmpty())
                CommonSettingsServer::commonSettingsServer.subDatapackCode=list.at(0).fileName().toStdString();
            else
                CommonSettingsServer::commonSettingsServer.subDatapackCode.clear();
        }
        if(!CommonSettingsServer::commonSettingsServer.subDatapackCode.empty())
        {
            QDir subDir(QString::fromStdString(datapackPathBase)+"/map/main/"+
                        QString::fromStdString(CommonSettingsServer::commonSettingsServer.mainDatapackCode)+"/sub/"+
                        QString::fromStdString(CommonSettingsServer::commonSettingsServer.subDatapackCode)+"/");
            if(!subDir.exists())
            {
                const QFileInfoList &list=QDir(QString::fromStdString(datapackPathBase)+"/map/main/"+
                                               QString::fromStdString(CommonSettingsServer::commonSettingsServer.mainDatapackCode)+"/sub/")
                        .entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name);
                if(!list.isEmpty())
                    CommonSettingsServer::commonSettingsServer.subDatapackCode=list.at(0).fileName().toStdString();
                else
                    CommonSettingsServer::commonSettingsServer.subDatapackCode.clear();
            }
        }

        internalServer->setSettings(formatedServerSettings);
    }

    internalServer->start();
    //do after server is init connectToServer(connexionInfo,QString(),QString());
    #endif
}

void ScreenTransition::is_started(bool started)
{
    std::cout << "ScreenTransition::is_started(" << started << ")" << std::endl;
    #if defined(CATCHCHALLENGER_SOLO) && defined(CATCHCHALLENGER_SOLO)
    if(started)
    {
        ConnexionInfo connexionInfo;
        connexionInfo.connexionCounter=0;
        connexionInfo.isCustom=false;
        connexionInfo.port=0;
        connexionInfo.lastConnexion=0;
        connexionInfo.proxyPort=0;
        connectToServer(connexionInfo,"Admin",Settings::settings->value("pass").toString());
        return;
    }
    else
    {
        if(internalServer!=nullptr)
        {
            delete internalServer;
            internalServer=nullptr;
        }
        std::cout << "ScreenTransition::is_started(" << started << ") isVisible(): " << isVisible() << std::endl;
        if(!isVisible())
            QCoreApplication::quit();
    }
    #endif
}

void ScreenTransition::openMulti()
{
    multiplaySelected=true;
    CommonSettingsServer::commonSettingsServer.mainDatapackCode="[main]";
    CommonSettingsServer::commonSettingsServer.subDatapackCode="[sub]";
    if(m!=nullptr)
        m->setError(std::string());
    if(multi==nullptr)
    {
        multi=new Multi();
        if(!connect(multi,&Multi::backMain,this,&ScreenTransition::backMain))
            abort();
        if(!connect(multi,&Multi::connectToServer,this,&ScreenTransition::connectToServer))
            abort();
    }
    setForeground(multi);
}

void ScreenTransition::connectToServer(ConnexionInfo connexionInfo,QString login,QString pass)
{
    Q_UNUSED(connexionInfo);
    Q_UNUSED(login);
    Q_UNUSED(pass);
    setForeground(&l);
    //baseWindow=new CatchChallenger::BaseWindow();
    if(connexionManager!=nullptr)
        delete connexionManager;
    //work around for the resetAll() of protocol
    const std::string mainDatapackCode=CommonSettingsServer::commonSettingsServer.mainDatapackCode;
    const std::string subDatapackCode=CommonSettingsServer::commonSettingsServer.subDatapackCode;
    connexionManager=new ConnexionManager(&l);
    connexionManager->connectToServer(connexionInfo,login,pass);
    CommonSettingsServer::commonSettingsServer.mainDatapackCode=mainDatapackCode;
    CommonSettingsServer::commonSettingsServer.subDatapackCode=subDatapackCode;
    if(!connect(connexionManager,&ConnexionManager::logged,this,&ScreenTransition::logged))
        abort();
    if(!connect(connexionManager,&ConnexionManager::errorString,this,&ScreenTransition::errorString))
        abort();
    if(!connect(connexionManager,&ConnexionManager::disconnectedFromServer,this,&ScreenTransition::disconnectedFromServer,Qt::QueuedConnection))
        abort();
    if(!connect(connexionManager,&ConnexionManager::goToMap,this,&ScreenTransition::goToMap))
        abort();
    l.progression(0,100);
}

/*void ScreenTransition::errorString(std::string error)
{
    setForeground(m);
    m->setError(error);
}*/

void ScreenTransition::toLoading(QString text)
{
    if(m_foregroundStack!=&l)
        l.progression(0,100);
    l.setText(text);
    setBackground(&b);
    setForeground(&l);
}

void ScreenTransition::backMain()
{
    setForeground(m);
}

void ScreenTransition::backSubServer()
{
    if(connexionManager->client->getServerOrdenedList().size()<2)
    {
        if(multiplaySelected && multi!=nullptr)
            setForeground(multi);
        else
            setForeground(m);
    }
    else
        setForeground(subserver);
}

void ScreenTransition::logged(const std::vector<std::vector<CatchChallenger::CharacterEntry> > &characterEntryList)
{
    this->characterEntryList=characterEntryList;
    if(subserver==nullptr)
    {
        subserver=new SubServer();
        if(!connect(subserver,&SubServer::backMulti,this,&ScreenTransition::backMain))
            abort();
        if(!connect(subserver,&SubServer::connectToSubServer,this,&ScreenTransition::connectToSubServer))
            abort();
    }
    setForeground(subserver);
    //after setForeground to be able to change Foreground
    subserver->logged(connexionManager->client->getServerOrdenedList(),connexionManager);
}

void ScreenTransition::connectToSubServer(const int indexSubServer)
{
    if(characterList==nullptr)
    {
        characterList=new CharacterList();
        if(!connect(characterList,&CharacterList::backSubServer,this,&ScreenTransition::backSubServer))
            abort();
        if(!connect(characterList,&CharacterList::selectCharacter,this,&ScreenTransition::selectCharacter))
            abort();
    }
    setForeground(characterList);
    //after setForeground to be able to change Foreground
    characterList->connectToSubServer(indexSubServer,connexionManager,characterEntryList);

    //todo: optimise by prevent create each time
    if(ccmap!=nullptr)
    {
        delete ccmap;
        ccmap=nullptr;
    }
    if(ccmap==nullptr)
    {
        ccmap=new CCMap();
        /*if(!connect(ccmap,&CCMap::backSubServer,this,&ScreenTransition::backSubServer))
            abort();
        if(!connect(ccmap,&CCMap::selectCharacter,this,&ScreenTransition::selectCharacter))
            abort();*/
        if(!connect(ccmap,&CCMap::error,this,&ScreenTransition::errorString))
            abort();
    }
    ccmap->setVar(connexionManager);

    if(overmap!=nullptr)
    {
        delete overmap;
        overmap=nullptr;
    }
    if(overmap==nullptr)
    {
        overmap=new OverMapLogic();
        if(!connect(overmap,&OverMapLogic::error,this,&ScreenTransition::errorString))
            abort();
    }
    overmap->resetAll();
    overmap->setVar(ccmap,connexionManager);
    overmap->connectAllSignals();
}

void ScreenTransition::selectCharacter(const int indexSubServer,const int indexCharacter)
{
    l.reset();
    l.progression(0,100);
    l.setText("Selecting your character...");
    setAbove(nullptr);
    setForeground(&l);
    connexionManager->selectCharacter(indexSubServer,indexCharacter);
}

void ScreenTransition::disconnectedFromServer()
{
    inGame=false;
    #if defined(CATCHCHALLENGER_SOLO) && defined(CATCHCHALLENGER_SOLO)
    if(internalServer!=nullptr)
        internalServer->stop();
    #endif
    //baseWindow->resetAll();
    setBackground(&b);
    setForeground(m);
    setAbove(nullptr);
}

void ScreenTransition::errorString(std::string error)
{
    inGame=false;
    if(connexionManager!=nullptr)
        if(connexionManager->client!=nullptr)
            connexionManager->client->disconnectFromHost();
    #if defined(CATCHCHALLENGER_SOLO) && defined(CATCHCHALLENGER_SOLO)
    if(internalServer!=nullptr)
    {
        std::cerr << "ScreenTransition::errorString(): internalServer!=nullptr" << std::endl;
        internalServer->stop();
    }
    #endif
    if(!error.empty())
    {
        std::cerr << "ScreenTransition::errorString(" << error << ")" << std::endl;
        setBackground(&b);
        setForeground(m);
        setAbove(nullptr);
    }
    if(m!=nullptr)
        m->setError(error);
    // Non-interactive runs (--closewhenonmap / --closewhenonmapafter)
    // can't recover from a connection error: the map will never load,
    // the error widget waits for a click that the test harness will
    // never produce, and under QT_QPA_PLATFORM=offscreen the click
    // can't even fire. Exit with non-zero so the python harness's
    // proc.wait() rc check FAILs the test immediately instead of
    // sitting through its outer 60-120s timeout.
    if(!error.empty() &&
       (CliClientOptions::closeWhenOnMap || CliClientOptions::closeWhenOnMapAfter>0))
    {
        std::cerr << "CliClientOptions: errorString with --closewhenonmap[after], exiting with rc=2" << std::endl;
        QCoreApplication::exit(2);
    }
}

void ScreenTransition::goToServerList()
{
    setBackground(&b);
    //setForeground(baseWindow);
    setAbove(nullptr);
}

void ScreenTransition::goToMap()
{
    inGame=true;
    setBackground(ccmap);
    setForeground(overmap);
    setAbove(nullptr);
    //if --closewhenonmap CLI arg was set, close 1s after spawning
    if(CliClientOptions::closeWhenOnMap)
    {
        std::cerr << "CliClientOptions: --closewhenonmap, exiting in 1s" << std::endl;
        QTimer::singleShot(1000,QCoreApplication::instance(),&QCoreApplication::quit);
    }
    if(CliClientOptions::closeWhenOnMapAfter>0 && !closeWhenOnMapAfterTimer_.isActive())
    {
        std::cerr << "CliClientOptions: --closewhenonmapafter=" << CliClientOptions::closeWhenOnMapAfter
                  << ", toggling direction each 1s, quitting in " << CliClientOptions::closeWhenOnMapAfter << "s" << std::endl;
        closeWhenOnMapAfterRemaining_=CliClientOptions::closeWhenOnMapAfter;
        connect(&closeWhenOnMapAfterTimer_,&QTimer::timeout,this,&ScreenTransition::closeWhenOnMapAfterToggle);
        closeWhenOnMapAfterTimer_.start(1000);
    }
    if(CliClientOptions::dropSendDataAfterOnMap && !CatchChallenger::Api_protocol::dropOutputAfterOnMap)
    {
        std::cerr << "CliClientOptions: --dropsenddataafteronmap, dropping all client->server traffic from now on" << std::endl;
        CatchChallenger::Api_protocol::dropOutputAfterOnMap=true;
    }
    if(!CliClientOptions::takeScreenshotPath.isEmpty())
    {
        // Wait one full event-loop tick + a beat for the first paint cycle
        // to compose all layers (tiles, doors, animation frame 0). The
        // viewport grab below mirrors what the user sees on screen.
        const QString outPath=CliClientOptions::takeScreenshotPath;
        std::cerr << "CliClientOptions: --take-screenshot=" << outPath.toStdString()
                  << ", will save and exit in 500ms" << std::endl;
        QTimer::singleShot(500,this,[this,outPath]{
            // Force one render pass before grabbing so the QGraphicsView's
            // viewport has actually painted the map.
            viewport()->repaint();
            QPixmap shot=viewport()->grab();
            if(!shot.save(outPath))
                std::cerr << "--take-screenshot: failed to save " << outPath.toStdString() << std::endl;
            else
                std::cerr << "--take-screenshot: saved " << outPath.toStdString()
                          << " (" << shot.width() << "x" << shot.height() << ")" << std::endl;
            QCoreApplication::quit();
        });
    }
}

void ScreenTransition::render()
{
    mScene->setSceneRect(QRectF(0,0,width(),height()));
    //mScene->update();
    viewport()->update();
}

void ScreenTransition::paintEvent(QPaintEvent * event)
{
    mScene->setSceneRect(QRectF(0,0,width(),height()));
    timeRender.restart();

    QGraphicsView::paintEvent(event);

    uint32_t elapsed=timeRender.elapsed();
    if(waitRenderTime<=elapsed)
        timerRender.start(1);
    else
        timerRender.start(waitRenderTime-elapsed);

    if(frameCounter<65535)
        frameCounter++;
}

void ScreenTransition::updateFPS()
{
    const unsigned int FPS=(int)(((float)frameCounter)*1000)/timeUpdateFPS.elapsed();
    emit newFPSvalue(FPS);
    // Skip the on-screen FPS overlay rendering when --take-screenshot is on.
    // The pixmap is already hidden in the constructor, so painting into it
    // would just waste CPU; bail before the QPainter dance.
    if(!CliClientOptions::takeScreenshotPath.isEmpty())
    {
        frameCounter=0;
        timeUpdateFPS.restart();
        timerUpdateFPS.start();
        return;
    }
    QImage pix(50,40,QImage::Format_ARGB32_Premultiplied);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setFont(QFont("Times", 12, QFont::Bold));

    const int offset=1;
    p.setPen(QPen(Qt::black));
    p.drawText(pix.rect().x()+offset,pix.rect().y()+offset,pix.rect().width(),pix.rect().height(),Qt::AlignCenter,QString::number(FPS));
    p.drawText(pix.rect().x()-offset,pix.rect().y()+offset,pix.rect().width(),pix.rect().height(),Qt::AlignCenter,QString::number(FPS));
    p.drawText(pix.rect().x()-offset,pix.rect().y()-offset,pix.rect().width(),pix.rect().height(),Qt::AlignCenter,QString::number(FPS));
    p.drawText(pix.rect().x()+offset,pix.rect().y()-offset,pix.rect().width(),pix.rect().height(),Qt::AlignCenter,QString::number(FPS));

    p.setPen(QPen(Qt::white));
    p.drawText(pix.rect(), Qt::AlignCenter,QString::number(FPS));
    imageText->setPixmap(QPixmap::fromImage(pix));

    frameCounter=0;
    timeUpdateFPS.restart();
    timerUpdateFPS.start();
}

void ScreenTransition::setTargetFPS(int targetFPS)
{
    if(targetFPS==0)
        waitRenderTime=0;
    else
    {
        waitRenderTime=static_cast<uint8_t>(static_cast<float>(1000.0)/(float)targetFPS);
        if(waitRenderTime<1)
            waitRenderTime=1;
    }
}

void ScreenTransition::keyPressEvent(QKeyEvent * event)
{
    bool eventTriggerGeneral=false;
    event->setAccepted(false);
    if(m_aboveStack!=nullptr)
        static_cast<ScreenInput *>(m_aboveStack)->keyPressEvent(event,eventTriggerGeneral);
    if(eventTriggerGeneral)
    {
        QGraphicsView::keyPressEvent(event);
        return;
    }
    if(!event->isAccepted() && m_foregroundStack!=nullptr)
        static_cast<ScreenInput *>(m_foregroundStack)->keyPressEvent(event,eventTriggerGeneral);
    if(eventTriggerGeneral)
    {
        QGraphicsView::keyPressEvent(event);
        return;
    }
    if(!event->isAccepted() && m_backgroundStack!=nullptr)
        static_cast<ScreenInput *>(m_backgroundStack)->keyPressEvent(event,eventTriggerGeneral);
    if(eventTriggerGeneral)
    {
        QGraphicsView::keyPressEvent(event);
        return;
    }
    if(!event->isAccepted())
        QGraphicsView::keyPressEvent(event);
}

void ScreenTransition::keyReleaseEvent(QKeyEvent *event)
{
    bool eventTriggerGeneral=false;
    event->setAccepted(false);
    if(m_aboveStack!=nullptr)
        static_cast<ScreenInput *>(m_aboveStack)->keyReleaseEvent(event,eventTriggerGeneral);
    if(eventTriggerGeneral)
    {
        QGraphicsView::keyPressEvent(event);
        return;
    }
    if(!event->isAccepted() && m_foregroundStack!=nullptr)
        static_cast<ScreenInput *>(m_foregroundStack)->keyReleaseEvent(event,eventTriggerGeneral);
    if(eventTriggerGeneral)
    {
        QGraphicsView::keyPressEvent(event);
        return;
    }
    if(!event->isAccepted() && m_backgroundStack!=nullptr)
        static_cast<ScreenInput *>(m_backgroundStack)->keyReleaseEvent(event,eventTriggerGeneral);
    if(eventTriggerGeneral)
    {
        QGraphicsView::keyPressEvent(event);
        return;
    }
    if(!event->isAccepted())
        QGraphicsView::keyReleaseEvent(event);
}

void ScreenTransition::closeWhenOnMapAfterToggle()
{
    closeWhenOnMapAfterRemaining_--;
    if(closeWhenOnMapAfterRemaining_<=0)
    {
        closeWhenOnMapAfterTimer_.stop();
        std::cerr << "CliClientOptions: --closewhenonmapafter time elapsed, exiting" << std::endl;
        QCoreApplication::quit();
        return;
    }
    CatchChallenger::Direction current=ccmap->mapController.getDirection();
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
    std::cerr << "CliClientOptions: --closewhenonmapafter direction " << (int)next
              << " (" << closeWhenOnMapAfterRemaining_ << "s remaining)" << std::endl;
    {
        QKeyEvent press(QEvent::KeyPress, key, Qt::NoModifier);
        QKeyEvent release(QEvent::KeyRelease, key, Qt::NoModifier);
        ccmap->mapController.keyPressEvent(&press);
        ccmap->mapController.keyReleaseEvent(&release);
    }
    if(connexionManager!=nullptr && connexionManager->client!=nullptr && connexionManager->client->getCaracterSelected())
        connexionManager->client->send_player_direction(next);
}
