#include "SimpleSoloServer.h"
#include "ui_SimpleSoloServer.h"
#include <QStandardPaths>
#include "../../general/base/CommonSettingsServer.h"
#include "../../general/base/CommonSettingsCommon.h"
#include "../base/ClientVariable.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::SimpleSoloServer)
{
    ui->setupUi(this);
    solowindow=new SoloWindow(this,QCoreApplication::applicationDirPath()+QStringLiteral("/datapack/"),QStandardPaths::writableLocation(QStandardPaths::DataLocation)+QStringLiteral("/savegames/"),true);
    if(!connect(solowindow,&SoloWindow::play,this,&MainWindow::play))
    {
        std::cerr << "aborted at " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
        abort();
    }
    internalServer=NULL;
    socket=NULL;
    client=NULL;
    baseWindow=NULL;

    ui->stackedWidget->addWidget(solowindow);
    ui->stackedWidget->setCurrentWidget(solowindow);
    solowindow->setOnlySolo();
    //solowindow->show();
    setWindowTitle(QStringLiteral("CatchChallenger"));

    #ifndef CATCHCHALLENGER_NOAUDIO
    vlcPlayer=NULL;
    if(Audio::audio.vlcInstance!=NULL)
    {
        const QString &soundFile=QCoreApplication::applicationDirPath()+QStringLiteral(CATCHCHALLENGER_CLIENT_MUSIC_LOADING);
        if(QFile(soundFile).exists())
        {
            const QString &musicPath=QDir::toNativeSeparators(soundFile);
            // Create a new Media
            libvlc_media_t *vlcMedia = libvlc_media_new_path(Audio::audio.vlcInstance, musicPath.toUtf8().constData());
            if(vlcMedia!=NULL)
            {
                // Create a new libvlc player
                vlcPlayer = libvlc_media_player_new_from_media(vlcMedia);
                if(vlcPlayer!=NULL)
                {
                    // Get event manager for the player instance
                    libvlc_event_manager_t *manager = libvlc_media_player_event_manager(vlcPlayer);
                    // Attach the event handler to the media player error's events
                    libvlc_event_attach(manager,libvlc_MediaPlayerEncounteredError,MainWindow::vlceventStatic,this);
                    libvlc_event_attach(manager,libvlc_MediaPlayerEndReached,MainWindow::vlceventStatic,this);
                    // Release the media
                    libvlc_media_release(vlcMedia);
                    // And start playback
                    libvlc_media_player_play(vlcPlayer);
                    Audio::audio.addPlayer(vlcPlayer);
                    //audio
                    if(!connect(this,&MainWindow::audioLoopRestart,this,&MainWindow::audioLoop,Qt::QueuedConnection))
                    {
                        std::cerr << "!connect(this,&MainWindow::audioLoopRestart,this,&MainWindow::audioLoop,Qt::QueuedConnection)" << std::endl;
                        abort();
                    }
                }
                else
                {
                    qDebug() << "problem with vlc media player";
                    const char * string=libvlc_errmsg();
                    if(string!=NULL)
                        qDebug() << string;
                }
            }
            else
            {
                qDebug() << "problem with vlc media" << musicPath;
                const char * string=libvlc_errmsg();
                if(string!=NULL)
                    qDebug() << string;
            }
        }
        else
            qDebug() << "The loading file sound not exists: " << soundFile;
    }
    else
    {
        qDebug() << "no vlc instance";
        const char * string=libvlc_errmsg();
        if(string!=NULL)
            qDebug() << string;
    }
    #endif
    /* already done into another part: #ifdef CATCHCHALLENGER_GITCOMMIT
    ui->version->setText(QStringLiteral(CATCHCHALLENGER_VERSION)+QStringLiteral(" - ")+QStringLiteral(CATCHCHALLENGER_GITCOMMIT));
    #else
    ui->version->setText(QStringLiteral(CATCHCHALLENGER_VERSION));
    #endif*/
}

MainWindow::~MainWindow()
{
    #ifndef CATCHCHALLENGER_NOAUDIO
    if(vlcPlayer!=NULL)
    {
        libvlc_media_player_stop(vlcPlayer);
        Audio::audio.removePlayer(vlcPlayer);
    }
    #endif
    if(internalServer!=NULL)
    {
        delete internalServer;
        internalServer=NULL;
    }
    delete ui;
    if(socket!=NULL)
    {
        socket->abort();
        socket->deleteLater();
    }
    baseWindow->deleteLater();
    baseWindow=NULL;
}

void MainWindow::play(const QString &savegamesPath)
{
    QSettings metaData(savegamesPath+QStringLiteral("metadata.conf"),QSettings::IniFormat);
    if(internalServer!=NULL)
    {
        internalServer->deleteLater();
        internalServer=NULL;
    }
    if(socket!=NULL)
    {
        socket->deleteLater();
        socket=NULL;
    }
    if(client!=NULL)
    {
        client->deleteLater();
        client=NULL;
    }
    if(baseWindow!=NULL)
    {
        baseWindow->deleteLater();
        baseWindow=NULL;
    }
    if(internalServer==NULL)
    {
        internalServer=new CatchChallenger::InternalServer(metaData);
        if(!connect(internalServer,&CatchChallenger::InternalServer::is_started,this,&MainWindow::is_started,Qt::QueuedConnection))
        {
            std::cerr << "aborted at " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
            abort();
        }
        if(!connect(internalServer,&CatchChallenger::InternalServer::error,this,&MainWindow::serverErrorStd,Qt::QueuedConnection))
        {
            std::cerr << "aborted at " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
            abort();
        }

        socket=new CatchChallenger::ConnectedSocket(new CatchChallenger::QFakeSocket());
        client=new CatchChallenger::Api_client_virtual(socket);
        if(!connect(client,               &CatchChallenger::Api_protocol::protocol_is_good,   this,&MainWindow::protocol_is_good))
        {
            std::cerr << "aborted at " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
            abort();
        }
        if(!connect(client,               &CatchChallenger::Api_protocol::disconnected,       this,&MainWindow::disconnected))
        {
            std::cerr << "aborted at " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
            abort();
        }
        if(!connect(client,               &CatchChallenger::Api_protocol::message,            this,&MainWindow::message))
        {
            std::cerr << "aborted at " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
            abort();
        }
        if(!connect(socket,                                                 &CatchChallenger::ConnectedSocket::stateChanged,    this,&MainWindow::stateChanged))
        {
            std::cerr << "aborted at " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
            abort();
        }
        baseWindow=new CatchChallenger::BaseWindow();
        baseWindow->setMultiPlayer(false,client);
        baseWindow->connectAllSignals();
        baseWindow->setMinimumSize(800,600);
        if(!connect(baseWindow,&CatchChallenger::BaseWindow::newError,this,&MainWindow::newError,Qt::QueuedConnection))
        {
            std::cerr << "aborted at " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
            abort();
        }

        ui->stackedWidget->addWidget(baseWindow);
        if(!connect(baseWindow,&CatchChallenger::BaseWindow::gameIsLoaded,this,&MainWindow::gameIsLoaded))
        {
            std::cerr << "aborted at " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
            abort();
        }
    }

    sendSettings(internalServer,savegamesPath);

    internalServer->start();
    ui->stackedWidget->setCurrentWidget(baseWindow);
    timeLaunched=QDateTime::currentDateTimeUtc().toTime_t();
    if(!metaData.contains(QStringLiteral("pass")))
    {
        QMessageBox::critical(NULL,tr("Error"),tr("Unable to load internal value"));
        return;
    }
    launchedGamePath=savegamesPath;
    haveLaunchedGame=true;
    pass=metaData.value(QStringLiteral("pass")).toString();

    baseWindow->serverIsLoading();
}

bool MainWindow::sendSettings(CatchChallenger::InternalServer * internalServer,const QString &savegamesPath)
{
    CatchChallenger::GameServerSettings formatedServerSettings=internalServer->getSettings();

    CommonSettingsServer::commonSettingsServer.waitBeforeConnectAfterKick=0;
    CommonSettingsCommon::commonSettingsCommon.max_character=1;
    CommonSettingsCommon::commonSettingsCommon.min_character=1;

    formatedServerSettings.automatic_account_creation=true;
    formatedServerSettings.max_players=1;
    formatedServerSettings.sendPlayerNumber = false;
    formatedServerSettings.compressionType=CatchChallenger::CompressionType_None;
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
    formatedServerSettings.mapVisibility.mapVisibilityAlgorithm	= CatchChallenger::MapVisibilityAlgorithmSelection_None;
    formatedServerSettings.datapack_basePath=client->datapackPathBase().toStdString();

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
    settings.beginGroup("content");
    if(settings.contains("mainDatapackCode"))
        CommonSettingsServer::commonSettingsServer.mainDatapackCode=settings.value("mainDatapackCode","[main]").toString().toStdString();
    else
    {
        const QFileInfoList &list=QDir(client->datapackPathBase()+"/map/main/").entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name);
        if(list.isEmpty())
        {
            QMessageBox::critical(this,tr("Error"),tr("No main code detected into the current datapack"));
            return false;
        }
        settings.setValue("mainDatapackCode",list.at(0).fileName());
        CommonSettingsServer::commonSettingsServer.mainDatapackCode=list.at(0).fileName().toStdString();
    }
    QDir mainDir(client->datapackPathBase()+"map/main/"+QString::fromStdString(CommonSettingsServer::commonSettingsServer.mainDatapackCode)+"/");
    if(!mainDir.exists())
    {
        const QFileInfoList &list=QDir(client->datapackPathBase()+"/map/main/").entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name);
        if(list.isEmpty())
        {
            QMessageBox::critical(this,tr("Error"),tr("No main code detected into the current datapack"));
            return false;
        }
        settings.setValue("mainDatapackCode",list.at(0).fileName());
        CommonSettingsServer::commonSettingsServer.mainDatapackCode=list.at(0).fileName().toStdString();
    }

    if(settings.contains("subDatapackCode"))
        CommonSettingsServer::commonSettingsServer.subDatapackCode=settings.value("subDatapackCode","").toString().toStdString();
    else
    {
        const QFileInfoList &list=QDir(client->datapackPathBase()+"/map/main/"+QString::fromStdString(CommonSettingsServer::commonSettingsServer.mainDatapackCode)+"/sub/").entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name);
        if(!list.isEmpty())
        {
            settings.setValue("subDatapackCode",list.at(0).fileName());
            CommonSettingsServer::commonSettingsServer.subDatapackCode=list.at(0).fileName().toStdString();
        }
        else
            CommonSettingsServer::commonSettingsServer.subDatapackCode.clear();
    }
    if(!CommonSettingsServer::commonSettingsServer.subDatapackCode.empty())
    {
        QDir subDir(client->datapackPathBase()+"/map/main/"+QString::fromStdString(CommonSettingsServer::commonSettingsServer.mainDatapackCode)+"/sub/"+QString::fromStdString(CommonSettingsServer::commonSettingsServer.subDatapackCode)+"/");
        if(!subDir.exists())
        {
            const QFileInfoList &list=QDir(client->datapackPathBase()+"/map/main/"+QString::fromStdString(CommonSettingsServer::commonSettingsServer.mainDatapackCode)+"/sub/").entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name);
            if(!list.isEmpty())
            {
                settings.setValue("subDatapackCode",list.at(0).fileName());
                CommonSettingsServer::commonSettingsServer.subDatapackCode=list.at(0).fileName().toStdString();
            }
            else
                CommonSettingsServer::commonSettingsServer.subDatapackCode.clear();
        }
    }
    settings.endGroup();

    internalServer->setSettings(formatedServerSettings);
    return true;
}

void MainWindow::protocol_is_good()
{
    client->setDatapackPath(QCoreApplication::applicationDirPath()+"/datapack/");
    client->tryLogin(QStringLiteral("admin"),pass);
}

void MainWindow::disconnected(QString reason)
{
    QMessageBox::information(this,tr("Disconnected"),tr("Disconnected by the reason: %1").arg(reason));
    haveShowDisconnectionReason=true;
    resetAll();
}

void MainWindow::message(QString message)
{
    qDebug() << message;
}

void MainWindow::stateChanged(QAbstractSocket::SocketState socketState)
{
    if(socketState==QAbstractSocket::UnconnectedState)
    {
        if(client!=NULL)
            if(client->stage()==CatchChallenger::Api_client_real::StageConnexion::Stage2)
                return;
        const QByteArray &data=socket->readAll();
        Q_UNUSED(data);
        //client->rea
        if(!isVisible() && internalServer==NULL)
        {
            QCoreApplication::quit();
            return;
        }
        /*never be wrong, single player
        if(client!=NULL && client->protocolWrong())
            QMessageBox::about(this,tr("Quit"),tr("The server have closed the connexion"));*/
        if(internalServer!=NULL)
            internalServer->stop();
        resetAll();
    }
    if(baseWindow!=NULL)
    {
        baseWindow->stateChanged(socketState);
        if(socketState==QAbstractSocket::ConnectedState)
            client->sendProtocol();
    }
}

void MainWindow::serverError(const QString &error)
{
    QMessageBox::critical(NULL,tr("Error"),tr("The engine is closed due to: %1").arg(error));
    resetAll();
}

void MainWindow::serverErrorStd(const std::string &error)
{
    QMessageBox::critical(NULL,tr("Error"),tr("The engine is closed due to: %1").arg(QString::fromStdString(error)));
    resetAll();
}

void MainWindow::is_started(bool started)
{
    if(!started)
    {
        if(!isVisible())
            QCoreApplication::quit();
        else
            resetAll();
    }
    else
    {
        baseWindow->serverIsReady();
        socket->connectToHost(QStringLiteral("localhost"),9999);
    }
}

void MainWindow::saveTime()
{
    if(internalServer==NULL)
        return;
    //save the time
    if(haveLaunchedGame)
    {
        bool settingOk=false;
        QSettings metaData(launchedGamePath+QStringLiteral("metadata.conf"),QSettings::IniFormat);
        if(metaData.isWritable())
        {
            if(metaData.status()==QSettings::NoError)
            {
                QString locaction=baseWindow->lastLocation();
                const QString &mapPath=QString::fromStdString(internalServer->getSettings().datapack_basePath)+DATAPACK_BASE_PATH_MAPMAIN+QString::fromStdString(CommonSettingsServer::commonSettingsServer.mainDatapackCode)+"/";//internalServer->getSettings().mainDatapackCode
                if(locaction.startsWith(mapPath))
                    locaction.remove(0,mapPath.size());
                if(!locaction.isEmpty())
                    metaData.setValue(QStringLiteral("location"),locaction);
                uint64_t current_date_time=QDateTime::currentDateTimeUtc().toTime_t();
                if(current_date_time>timeLaunched)
                    metaData.setValue("time_played",metaData.value("time_played").toUInt()+(uint32_t)(current_date_time-timeLaunched));
                settingOk=true;
            }
            else
                qDebug() << "Settings error: " << metaData.status();
        }
        solowindow->updateSavegameList();
        if(!settingOk)
        {
            QMessageBox::critical(NULL,tr("Error"),tr("Unable to save internal value at game stopping"));
            return;
        }
        haveLaunchedGame=false;
    }
}

void MainWindow::resetAll()
{
    if(client!=NULL)
        client->resetAll();
    ui->stackedWidget->setCurrentWidget(solowindow);
    if(internalServer!=NULL)
        internalServer->stop();
    saveTime();
}

void MainWindow::newError(QString error,QString detailedError)
{
    qDebug() << detailedError.toLocal8Bit();
    if(client!=NULL)
        client->tryDisconnect();
    QMessageBox::critical(this,tr("Error"),error);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    event->ignore();
    hide();
    if(socket!=NULL || internalServer!=NULL)
    {
        if(internalServer!=NULL)
        {
            if(internalServer->isListen())
                internalServer->stop();
            else
                QCoreApplication::quit();
        }
        else
            QCoreApplication::quit();
        if(socket!=NULL)
            socket->disconnectFromHost();
        if(socket!=NULL)
            socket->abort();
    }
    else
        QCoreApplication::quit();
}

void MainWindow::gameIsLoaded()
{
    #ifndef CATCHCHALLENGER_NOAUDIO
    if(vlcPlayer!=NULL)
        libvlc_media_player_stop(vlcPlayer);
    #endif
    this->setWindowTitle(QStringLiteral("CatchChallenger - %1").arg(client->getPseudo()));
}

#ifndef CATCHCHALLENGER_NOAUDIO
void MainWindow::vlceventStatic(const libvlc_event_t *event, void *ptr)
{
    static_cast<MainWindow *>(ptr)->vlcevent(event);
}

void MainWindow::vlcevent(const libvlc_event_t *event)
{
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
            emit audioLoopRestart(vlcPlayer);
        break;
        default:
            qDebug() << "vlc event: " << event->type;
        break;
    }
}

void MainWindow::audioLoop(void *player)
{
    libvlc_media_player_t * const vlcPlayer=static_cast<libvlc_media_player_t *>(player);
    libvlc_media_player_stop(vlcPlayer);
    libvlc_media_player_play(vlcPlayer);
}
#endif
