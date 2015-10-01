#include "SimpleSoloServer.h"
#include "ui_SimpleSoloServer.h"
#include <QStandardPaths>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::SimpleSoloServer)
{
    ui->setupUi(this);
    solowindow=new SoloWindow(this,QCoreApplication::applicationDirPath()+QStringLiteral("/datapack/"),QStandardPaths::writableLocation(QStandardPaths::DataLocation)+QStringLiteral("/savegames/"),true);
    connect(solowindow,&SoloWindow::play,this,&MainWindow::play);

    socket=new CatchChallenger::ConnectedSocket(new CatchChallenger::QFakeSocket());
    CatchChallenger::Api_client_real::client=new CatchChallenger::Api_client_virtual(socket,QCoreApplication::applicationDirPath()+QStringLiteral("/datapack/"));
    internalServer=new CatchChallenger::InternalServer();
    connect(internalServer,&CatchChallenger::InternalServer::is_started,this,&MainWindow::is_started,Qt::QueuedConnection);
    connect(internalServer,&CatchChallenger::InternalServer::error,this,&MainWindow::serverError,Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,               &CatchChallenger::Api_protocol::protocol_is_good,   this,&MainWindow::protocol_is_good);
    connect(CatchChallenger::Api_client_real::client,               &CatchChallenger::Api_protocol::disconnected,       this,&MainWindow::disconnected);
    connect(CatchChallenger::Api_client_real::client,               &CatchChallenger::Api_protocol::message,            this,&MainWindow::message);
    connect(socket,                                                 &CatchChallenger::ConnectedSocket::stateChanged,    this,&MainWindow::stateChanged);
    CatchChallenger::BaseWindow::baseWindow=new CatchChallenger::BaseWindow();
    CatchChallenger::BaseWindow::baseWindow->connectAllSignals();
    CatchChallenger::BaseWindow::baseWindow->setMultiPlayer(false);
    connect(CatchChallenger::BaseWindow::baseWindow,&CatchChallenger::BaseWindow::newError,this,&MainWindow::newError,Qt::QueuedConnection);
    CatchChallenger::BaseWindow::baseWindow->setMinimumSize(800,600);
    ui->stackedWidget->addWidget(CatchChallenger::BaseWindow::baseWindow);
    ui->stackedWidget->addWidget(solowindow);
    ui->stackedWidget->setCurrentWidget(solowindow);
    solowindow->setOnlySolo();
    //solowindow->show();
    setWindowTitle(QStringLiteral("CatchChallenger"));

    vlcPlayer=NULL;
    if(Audio::audio.vlcInstance!=NULL)
    {
        if(QFile(QCoreApplication::applicationDirPath()+QStringLiteral("/music/loading.ogg")).exists())
        {
            const QString &musicPath=QDir::toNativeSeparators(QCoreApplication::applicationDirPath()+QStringLiteral("/music/loading.ogg"));
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
                    libvlc_event_attach(manager,libvlc_MediaPlayerEncounteredError,MainWindow::vlcevent,this);
                    // Release the media
                    libvlc_media_release(vlcMedia);
                    libvlc_media_add_option(vlcMedia, "input-repeat=-1");
                    // And start playback
                    libvlc_media_player_play(vlcPlayer);
                    Audio::audio.addPlayer(vlcPlayer);
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
    }
    else
    {
        qDebug() << "no vlc instance";
        const char * string=libvlc_errmsg();
        if(string!=NULL)
            qDebug() << string;
    }
    connect(CatchChallenger::BaseWindow::baseWindow,&CatchChallenger::BaseWindow::gameIsLoaded,this,&MainWindow::gameIsLoaded);
}

MainWindow::~MainWindow()
{
    if(vlcPlayer!=NULL)
    {
        libvlc_media_player_stop(vlcPlayer);
        Audio::audio.removePlayer(vlcPlayer);
    }
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
    CatchChallenger::BaseWindow::baseWindow->deleteLater();
    CatchChallenger::BaseWindow::baseWindow=NULL;
}

void MainWindow::play(const QString &savegamesPath)
{
    sendSettings(internalServer,savegamesPath);
    internalServer->start();
    ui->stackedWidget->setCurrentWidget(CatchChallenger::BaseWindow::baseWindow);
    timeLaunched=QDateTime::currentDateTimeUtc().toTime_t();
    QSettings metaData(savegamesPath+QStringLiteral("metadata.conf"),QSettings::IniFormat);
    if(!metaData.contains(QStringLiteral("pass")))
    {
        QMessageBox::critical(NULL,tr("Error"),tr("Unable to load internal value"));
        return;
    }
    launchedGamePath=savegamesPath;
    haveLaunchedGame=true;
    pass=metaData.value(QStringLiteral("pass")).toString();

    CatchChallenger::BaseWindow::baseWindow->serverIsLoading();
}

void MainWindow::sendSettings(CatchChallenger::InternalServer * internalServer,const QString &savegamesPath)
{
    CatchChallenger::ServerSettings formatedServerSettings=internalServer->getSettings();

    CommonSettings::commonSettings.waitBeforeConnectAfterKick=0;
    CommonSettings::commonSettings.max_character=1;
    CommonSettings::commonSettings.min_character=1;

    formatedServerSettings.automatic_account_creation=true;
    formatedServerSettings.max_players=1;
    formatedServerSettings.tolerantMode=false;
    formatedServerSettings.sendPlayerNumber = false;
    formatedServerSettings.compressionType=CatchChallenger::CompressionType_None;

    formatedServerSettings.database_login.tryOpenType=CatchChallenger::DatabaseBase::Type::SQLite;
    formatedServerSettings.database_login.file=savegamesPath+QStringLiteral("catchchallenger.db.sqlite");
    formatedServerSettings.database_common.tryOpenType=CatchChallenger::DatabaseBase::Type::SQLite;
    formatedServerSettings.database_common.file=savegamesPath+QStringLiteral("catchchallenger.db.sqlite");
    formatedServerSettings.database_server.tryOpenType=CatchChallenger::DatabaseBase::Type::SQLite;
    formatedServerSettings.database_server.file=savegamesPath+QStringLiteral("catchchallenger.db.sqlite");
    formatedServerSettings.mapVisibility.mapVisibilityAlgorithm	= CatchChallenger::MapVisibilityAlgorithmSelection_None;
    formatedServerSettings.datapack_basePath=CatchChallenger::Api_client_real::client->datapackPath();

    {
        CatchChallenger::ServerSettings::ProgrammedEvent &event=formatedServerSettings.programmedEventList["day"]["day"];
        event.cycle=60;
        event.offset=0;
        event.value="day";
    }
    {
        CatchChallenger::ServerSettings::ProgrammedEvent &event=formatedServerSettings.programmedEventList["day"]["night"];
        event.cycle=60;
        event.offset=30;
        event.value="night";
    }

    internalServer->setSettings(formatedServerSettings);
}

void MainWindow::protocol_is_good()
{
    CatchChallenger::Api_client_real::client->setDatapackPath(QCoreApplication::applicationDirPath()+QStringLiteral("/datapack/"));
    CatchChallenger::Api_client_real::client->tryLogin(QStringLiteral("admin"),pass);
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
        if(CatchChallenger::Api_client_real::client!=NULL)
            if(CatchChallenger::Api_client_real::client->stage()==CatchChallenger::Api_client_real::StageConnexion::Stage2)
                return;
        const QByteArray &data=socket->readAll();
        //CatchChallenger::Api_client_real::client->rea
        if(!isVisible() && internalServer==NULL)
        {
            QCoreApplication::quit();
            return;
        }
        /*never be wrong, single player
        if(CatchChallenger::Api_client_real::client!=NULL && CatchChallenger::Api_client_real::client->protocolWrong())
            QMessageBox::about(this,tr("Quit"),tr("The server have closed the connexion"));*/1
        if(internalServer!=NULL)
            internalServer->stop();
        resetAll();
    }
    if(CatchChallenger::BaseWindow::baseWindow!=NULL)
    {
        CatchChallenger::BaseWindow::baseWindow->stateChanged(socketState);
        if(socketState==QAbstractSocket::ConnectedState)
            CatchChallenger::Api_client_real::client->sendProtocol();
    }
}

void MainWindow::serverError(const QString &error)
{
    QMessageBox::critical(NULL,tr("Error"),tr("The engine is closed due to: %1").arg(error));
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
        CatchChallenger::BaseWindow::baseWindow->serverIsReady();
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
                QString locaction=CatchChallenger::BaseWindow::baseWindow->lastLocation();
                QString mapPath=internalServer->getSettings().datapack_basePath+DATAPACK_BASE_PATH_MAP;
                if(locaction.startsWith(mapPath))
                    locaction.remove(0,mapPath.size());
                if(!locaction.isEmpty())
                    metaData.setValue(QStringLiteral("location"),locaction);
                uint64_t current_date_time=QDateTime::currentDateTimeUtc().toTime_t();
                if(current_date_time>timeLaunched)
                    metaData.setValue(QStringLiteral("time_played"),metaData.value(QStringLiteral("time_played")).toUInt()+(current_date_time-timeLaunched));
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
    if(CatchChallenger::Api_client_real::client!=NULL)
        CatchChallenger::Api_client_real::client->resetAll();
    ui->stackedWidget->setCurrentWidget(solowindow);
    if(internalServer!=NULL)
        internalServer->stop();
    saveTime();
}

void MainWindow::newError(QString error,QString detailedError)
{
    qDebug() << detailedError.toLocal8Bit();
    if(CatchChallenger::Api_client_real::client!=NULL)
        CatchChallenger::Api_client_real::client->tryDisconnect();
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
    if(vlcPlayer!=NULL)
        libvlc_media_player_stop(vlcPlayer);
}

void MainWindow::vlcevent(const libvlc_event_t *event, void *ptr)
{
    qDebug() << "vlc event";
    Q_UNUSED(ptr);
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
        default:
        break;
    }
}
