#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "../base/render/MapVisualiserPlayer.h"
#include "../base/LanguagesSelect.h"
#include "../base/InternetUpdater.h"
#include "../base/BlacklistPassword.h"
#include "../base/ClientVariable.h"
#ifndef CATCHCHALLENGER_NOAUDIO
#include "../base/Audio.h"
#endif
#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/CommonSettingsServer.h"
#include "../../general/base/Version.h"
#include "../base/SslCert.h"
#include <QNetworkProxy>
#include <QStandardPaths>
#include <QSslKey>
#include <algorithm>
#include <string>

#ifdef __linux__
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#endif

#define SERVER_DNS_OR_IP "catchchallenger.first-world.info"
#define SERVER_PORT 42489

#define SERVER_NAME tr("Official server")

//#define SERVER_DNS_OR_IP "localhost"
//#define SERVER_PORT 39034

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");
    qRegisterMetaType<CatchChallenger::Chat_type>("CatchChallenger::Chat_type");
    qRegisterMetaType<CatchChallenger::Player_type>("CatchChallenger::Player_type");
    qRegisterMetaType<QList<FeedNews::FeedEntry> >("QList<FeedNews::FeedEntry>");

    realSslSocket=new QSslSocket();
    socket=NULL;
    realSslSocket=NULL;
    client=NULL;
    ui->setupUi(this);
    ui->update->setVisible(false);
    if(settings.contains("news"))
    {
        ui->news->setVisible(true);
        ui->news->setText(settings.value("news").toString());
    }
    else
        ui->news->setVisible(false);
    server_name=SERVER_NAME;
    server_dns_or_ip=SERVER_DNS_OR_IP;
    server_port=SERVER_PORT;
    QString settingsServerPath=QCoreApplication::applicationDirPath()+QStringLiteral("/server.conf");
    if(QFile(settingsServerPath).exists())
    {
        QSettings settingsServer(settingsServerPath,QSettings::IniFormat);
        if(settingsServer.contains(QStringLiteral("server_dns_or_ip")) && settingsServer.contains(QStringLiteral("server_port")) && settingsServer.contains(QStringLiteral("proxy_port")))
        {
            bool ok,ok2;
            uint16_t server_port_temp=settingsServer.value(QStringLiteral("server_port")).toString().toUShort(&ok);
            uint16_t proxy_port_temp=settingsServer.value(QStringLiteral("proxy_port")).toString().toUShort(&ok2);
            if(settingsServer.value(QStringLiteral("server_dns_or_ip")).toString().contains(QRegularExpression(QStringLiteral("^([a-zA-Z0-9]{8}\\.onion|.*\\.i2p)$"))) && ok && ok2 && server_port_temp>0 && proxy_port_temp>0)
            {
                server_name=tr("Hidden server");
                if(settingsServer.contains(QStringLiteral("server_name")))
                    server_name=settingsServer.value(QStringLiteral("server_name")).toString();
                server_dns_or_ip=settingsServer.value(QStringLiteral("server_dns_or_ip")).toString();
                proxy_dns_or_ip=QStringLiteral("localhost");
                server_port=server_port_temp;
                proxy_port=proxy_port_temp;
                if(settingsServer.contains(QStringLiteral("proxy_dns_or_ip")))
                    proxy_dns_or_ip=settingsServer.value(QStringLiteral("proxy_dns_or_ip")).toString();
                ui->label_login_register->setStyleSheet(ui->label_login_register->styleSheet()+QStringLiteral("text-decoration:line-through;"));
                ui->label_login_website->setStyleSheet(ui->label_login_website->styleSheet()+QStringLiteral("text-decoration:line-through;"));
                ui->label_login_register->setText(tr("Register"));
                ui->label_login_website->setText(tr("Web site"));
            }
        }
    }
    InternetUpdater::internetUpdater=new InternetUpdater();
    connect(InternetUpdater::internetUpdater,&InternetUpdater::newUpdate,this,&MainWindow::newUpdate);
    FeedNews::feedNews=new FeedNews();
    connect(FeedNews::feedNews,&FeedNews::feedEntryList,this,&MainWindow::feedEntryList);
    baseWindow=new CatchChallenger::BaseWindow();
    ui->stackedWidget->addWidget(baseWindow);
    {
        serverLoginList.clear();
        if(settings.contains(QStringLiteral("login")))
        {
            const QStringList &loginList=settings.value("login").toStringList();
            int index=0;
            while(index<loginList.size())
            {
                if(settings.contains(loginList.at(index)))
                    serverLoginList[loginList.at(index)]=settings.value(loginList.at(index)).toString();
                else
                    serverLoginList[loginList.at(index)]=QString();
                index++;
            }
            if(!loginList.isEmpty())
                ui->lineEditLogin->setText(loginList.first());
            else
                ui->lineEditLogin->setText(QString());
        }
        else
            ui->lineEditLogin->setText(QString());
        if(serverLoginList.contains(ui->lineEditLogin->text()))
            ui->lineEditPass->setText(serverLoginList.value(ui->lineEditLogin->text()));
        else
            ui->lineEditPass->setText(QString());
        ui->checkBoxRememberPassword->setChecked(!ui->lineEditPass->text().isEmpty());
    }
    connect(&updateTheOkButtonTimer,&QTimer::timeout,this,&MainWindow::updateTheOkButton);

    stopFlood.setSingleShot(false);
    stopFlood.start(1500);
    updateTheOkButtonTimer.setSingleShot(false);
    updateTheOkButtonTimer.start(1000);
    numberForFlood=0;
    haveShowDisconnectionReason=false;
    ui->stackedWidget->addWidget(baseWindow);
    connect(baseWindow,&CatchChallenger::BaseWindow::newError,this,&MainWindow::newError,Qt::QueuedConnection);

    stateChanged(QAbstractSocket::UnconnectedState);

    setWindowTitle(QStringLiteral("CatchChallenger - ")+server_name);

    #ifndef CATCHCHALLENGER_NOAUDIO
    vlcPlayer=NULL;
    if(Audio::audio.vlcInstance!=NULL)
    {
        const QString &soundFile=QCoreApplication::applicationDirPath()+QStringLiteral(CATCHCHALLENGER_CLIENT_MUSIC_LOADING);
        if(QFile(soundFile).exists())
        {
            // Create a new Media
            const QString &musicPath=QDir::toNativeSeparators(soundFile);
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
    connect(baseWindow,&CatchChallenger::BaseWindow::gameIsLoaded,this,&MainWindow::gameIsLoaded);
    #ifdef CATCHCHALLENGER_GITCOMMIT
    ui->version->setText(QStringLiteral(CATCHCHALLENGER_VERSION)+QStringLiteral(" - ")+QStringLiteral(CATCHCHALLENGER_GITCOMMIT));
    #else
    ui->version->setText(QStringLiteral(CATCHCHALLENGER_VERSION));
    #endif
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
    if(client!=NULL)
    {
        client->tryDisconnect();
        delete client;
    }
    delete baseWindow;
    delete ui;
    if(socket!=NULL)
        delete socket;
}

void MainWindow::resetAll()
{
    if(client!=NULL)
    {
        client->resetAll();
        client->deleteLater();
        client=NULL;
    }
    if(baseWindow!=NULL)
        baseWindow->resetAll();
    ui->stackedWidget->setCurrentWidget(ui->page_login);
    updateTheOkButton();
    chat_list_player_pseudo.clear();
    chat_list_player_type.clear();
    chat_list_type.clear();
    chat_list_text.clear();
    lastMessageSend.clear();
    if(ui->lineEditLogin->text().isEmpty())
        ui->lineEditLogin->setFocus();
    else if(ui->lineEditPass->text().isEmpty())
        ui->lineEditPass->setFocus();
    else
        ui->pushButtonTryLogin->setFocus();
    //stateChanged(QAbstractSocket::UnconnectedState);//don't call here, else infinity rescursive call
    setWindowTitle(QStringLiteral("CatchChallenger - ")+server_name);
}

void MainWindow::sslErrors(const QList<QSslError> &errors)
{
    haveShowDisconnectionReason=true;
    QStringList sslErrors;
    int index=0;
    while(index<errors.size())
    {
        qDebug() << QStringLiteral("Ssl error:") << errors.at(index).errorString();
        sslErrors << errors.at(index).errorString();
        index++;
    }
    /*QMessageBox::warning(this,tr("Ssl error"),sslErrors.join("\n"));
    realSocket->disconnectFromHost();*/
}

void MainWindow::disconnected(QString reason)
{
    QMessageBox::information(this,tr("Disconnected"),tr("Disconnected by the reason: %1").arg(reason));
    lastServerIsKick[server_dns_or_ip]=true;
    haveShowDisconnectionReason=true;
    resetAll();
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
    break;
    default:
    break;
    }
}

void MainWindow::on_lineEditLogin_returnPressed()
{
    if(ui->lineEditPass->text().isEmpty())
        ui->lineEditPass->setFocus();
    else
        on_pushButtonTryLogin_clicked();
}

void MainWindow::on_lineEditPass_returnPressed()
{
    on_pushButtonTryLogin_clicked();
}

void MainWindow::on_pushButtonTryLogin_clicked()
{
    if(!ui->pushButtonTryLogin->isEnabled())
        return;
    if(ui->lineEditPass->text().size()<6)
    {
        QMessageBox::warning(this,tr("Error"),tr("Your password need to be at minimum of 6 characters"));
        return;
    }
    {
        std::string pass=ui->lineEditPass->text().toStdString();
        std::transform(pass.begin(), pass.end(), pass.begin(), ::tolower);
        unsigned int index=0;
        while(index<BlacklistPassword::list.size())
        {
            if(BlacklistPassword::list.at(index)==pass)
            {
                QMessageBox::warning(this,tr("Error"),tr("Your password is into the most common password in the world, too easy to crack dude! Change it!"));
                return;
            }
            index++;
        }
    }
    if(ui->lineEditLogin->text().size()<3)
    {
        QMessageBox::warning(this,tr("Error"),tr("Your login need to be at minimum of 3 characters"));
        return;
    }
    if(ui->lineEditPass->text()==ui->lineEditLogin->text())
    {
        QMessageBox::warning(this,tr("Error"),tr("Your login can't be same as your login"));
        return;
    }
    lastServerConnect[server_dns_or_ip]=QDateTime::currentDateTime();
    lastServerIsKick[server_dns_or_ip]=false;
    QStringList loginList=settings.value("login").toStringList();
    if(serverLoginList.contains(ui->lineEditLogin->text()))
        loginList.removeOne(ui->lineEditLogin->text());
    if(ui->checkBoxRememberPassword->isChecked())
    {
        serverLoginList[ui->lineEditLogin->text()]=ui->lineEditPass->text();
        settings.setValue(ui->lineEditLogin->text(),ui->lineEditPass->text());
    }
    else
    {
        serverLoginList[ui->lineEditLogin->text()]=QString();
        settings.remove(ui->lineEditLogin->text());
    }
    loginList.insert(0,ui->lineEditLogin->text());
    settings.setValue("login",loginList);

    if(socket!=NULL)
    {
        socket->disconnectFromHost();
        socket->abort();
        delete socket;
        socket=NULL;
        realSslSocket=NULL;
    }

    QString host=server_dns_or_ip;
    uint16_t port=server_port;

    ui->stackedWidget->setCurrentWidget(baseWindow);
    realSslSocket=new QSslSocket();
    socket=new CatchChallenger::ConnectedSocket(realSslSocket);
    client=new CatchChallenger::Api_client_real(socket);
    if(!proxy_dns_or_ip.isEmpty())
    {
        QNetworkProxy proxy=realSslSocket->proxy();
        proxy.setHostName(proxy_dns_or_ip);
        proxy.setPort(proxy_port);
        proxy.setType(QNetworkProxy::Socks5Proxy);
        realSslSocket->setProxy(proxy);
    }
    ui->stackedWidget->setCurrentWidget(baseWindow);
    //connect(realSslSocket,&QSslSocket::readyRead,this,&MainWindow::readForFirstHeader,Qt::DirectConnection);

    baseWindow->stateChanged(QAbstractSocket::ConnectingState);
    connect(realSslSocket,static_cast<void(QSslSocket::*)(const QList<QSslError> &errors)>(&QSslSocket::sslErrors),  this,&MainWindow::sslErrors,Qt::QueuedConnection);
    connect(realSslSocket,&QSslSocket::stateChanged,    this,&MainWindow::stateChanged,Qt::DirectConnection);
    connect(realSslSocket,static_cast<void(QSslSocket::*)(QAbstractSocket::SocketError)>(&QSslSocket::error),           this,&MainWindow::error,Qt::QueuedConnection);
    realSslSocket->connectToHost(host,port);
    connectTheExternalSocket();
}

void MainWindow::connectTheExternalSocket()
{
    //continue the normal procedure
    if(!proxy_dns_or_ip.isEmpty())
    {
        QNetworkProxy proxy=realSslSocket->proxy();
        proxy.setType(QNetworkProxy::Socks5Proxy);
        proxy.setHostName(proxy_dns_or_ip);
        proxy.setPort(proxy_port);
        static_cast<CatchChallenger::Api_client_real *>(client)->setProxy(proxy);
    }
    connect(client,               &CatchChallenger::Api_protocol::protocol_is_good,   this,&MainWindow::protocol_is_good,Qt::QueuedConnection);
    connect(client,               &CatchChallenger::Api_protocol::disconnected,       this,&MainWindow::disconnected);
    connect(client,               &CatchChallenger::Api_protocol::message,            this,&MainWindow::message,Qt::QueuedConnection);
    connect(client,               &CatchChallenger::Api_protocol::logged,             this,&MainWindow::logged,Qt::QueuedConnection);
    baseWindow->connectAllSignals();
    baseWindow->setMultiPlayer(true,client);
    QDir datapack(QStandardPaths::writableLocation(QStandardPaths::DataLocation)+QStringLiteral("/datapack/"));
    if(!datapack.exists())
        if(!datapack.mkpath(datapack.absolutePath()))
        {
            disconnected(tr("Not able to create the folder %1").arg(datapack.absolutePath()));
            return;
        }
    client->setDatapackPath(datapack.absolutePath());
    baseWindow->mapController->setDatapackPath(client->datapackPathBase(),client->mainDatapackCode());
    baseWindow->stateChanged(QAbstractSocket::ConnectedState);
}

void MainWindow::stateChanged(QAbstractSocket::SocketState socketState)
{
    if(socketState==QAbstractSocket::ConnectedState)
    {
    }
    qDebug() << QStringLiteral("socketState:") << (int)socketState;
    if(socketState==QAbstractSocket::UnconnectedState)
    {
        if(client!=NULL)
            if(client->stage()==CatchChallenger::Api_client_real::StageConnexion::Stage2 || client->stage()==CatchChallenger::Api_client_real::StageConnexion::Stage3)
            {
                client->socketDisconnectedForReconnect();
                return;
            }
        if(!isVisible())
        {
            QCoreApplication::quit();
            return;
        }
        if(client!=NULL && client->protocolWrong())
            QMessageBox::about(this,tr("Quit"),tr("The server have closed the connexion"));
        /*socket will do that's if(realSocket!=NULL)
        {
            realSocket->deleteLater;
            realSocket=NULL;
        }*/
        resetAll();
        /*if(serverMode==ServerMode_Remote)
            QMessageBox::about(this,tr("Quit"),tr("The server have closed the connexion"));*/
    }
    if(socketState==QAbstractSocket::ConnectedState)
    {
        haveShowDisconnectionReason=false;
        baseWindow->stateChanged(socketState);
    }
}

void MainWindow::error(QAbstractSocket::SocketError socketError)
{
    if(client!=NULL)
        if(client->stage()==CatchChallenger::Api_client_real::StageConnexion::Stage2)
            return;
    qDebug() << QStringLiteral("socketError:") << (int)socketError;
    resetAll();
    switch(socketError)
    {
    case QAbstractSocket::RemoteHostClosedError:
        if(realSslSocket!=NULL)
            return;
        if(haveShowDisconnectionReason)
            return;
        QMessageBox::information(this,tr("Connection closed"),tr("Connection closed by the server"));
    break;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::information(this,tr("Connection closed"),tr("Connection refused by the server"));
    break;
    case QAbstractSocket::SocketTimeoutError:
        QMessageBox::information(this,tr("Connection closed"),tr("Socket time out, server too long"));
    break;
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::information(this,tr("Connection closed"),tr("The host address was not found."));
    break;
    case QAbstractSocket::SocketAccessError:
        QMessageBox::information(this,tr("Connection closed"),tr("The socket operation failed because the application lacked the required privileges."));
    break;
    case QAbstractSocket::SocketResourceError:
        QMessageBox::information(this,tr("Connection closed"),tr("The local system ran out of resources"));
    break;
    case QAbstractSocket::NetworkError:
        QMessageBox::information(this,tr("Connection closed"),tr("An error occurred with the network"));
    break;
    case QAbstractSocket::UnsupportedSocketOperationError:
        QMessageBox::information(this,tr("Connection closed"),tr("The requested socket operation is not supported by the local operating system (e.g., lack of IPv6 support)"));
    break;
    case QAbstractSocket::SslHandshakeFailedError:
        QMessageBox::information(this,tr("Connection closed"),tr("The SSL/TLS handshake failed, so the connection was closed"));
    break;
    default:
        QMessageBox::information(this,tr("Connection error"),tr("Connection error: %1").arg(socketError));
    }
}

void MainWindow::haveNewError()
{
//	QMessageBox::critical(this,tr("Error"),client->errorString());
}

void MainWindow::message(QString message)
{
    qDebug() << message;
}

void MainWindow::protocol_is_good()
{
    client->tryLogin(ui->lineEditLogin->text(),ui->lineEditPass->text());
}

void MainWindow::needQuit()
{
    client->tryDisconnect();
}

void MainWindow::have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations)
{
    setWindowTitle(QStringLiteral("CatchChallenger - %1 - %2").arg(server_name).arg(QString::fromStdString(informations.public_informations.pseudo)));
}

void MainWindow::on_languages_clicked()
{
    LanguagesSelect::languagesSelect->exec();
}

void MainWindow::newError(QString error,QString detailedError)
{
    qDebug() << detailedError.toLocal8Bit();
    if(client!=NULL)
        client->tryDisconnect();
    QMessageBox::critical(this,tr("Error"),error);
}

void MainWindow::newUpdate(const QString &version)
{
    ui->update->setText(InternetUpdater::getText(version));
    ui->update->setVisible(true);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    event->ignore();
    hide();
    if(socket!=NULL)
    {
        if(socket!=NULL)
            socket->disconnectFromHost();
        if(socket!=NULL)
            socket->abort();
    }
    QCoreApplication::quit();
}

void MainWindow::feedEntryList(const QList<FeedNews::FeedEntry> &entryList,QString error)
{
    if(entryList.isEmpty())
    {
        if(error.isEmpty())
            ui->news->setVisible(false);
        else
        {
            ui->news->setToolTip(error);
            ui->news->setStyleSheet("#news{background-color: rgb(220, 220, 240);\nborder: 1px solid rgb(100, 150, 240);\nborder-radius:5px;\ncolor: rgb(0, 0, 0);\nbackground-image: url(:/images/multi/warning.png);\nbackground-repeat: no-repeat;\nbackground-position: right;}");
        }
        return;
    }
    if(entryList.size()==1)
        ui->news->setText(tr("Latest news:")+QStringLiteral(" ")+QStringLiteral("<a href=\"%1\">%2</a>").arg(entryList.at(0).link).arg(entryList.at(0).title));
    else
    {
        QStringList entryHtmlList;
        int index=0;
        while(index<entryList.size() && index<3)
        {
            entryHtmlList << QStringLiteral(" - <a href=\"%1\">%2</a>").arg(entryList.at(index).link).arg(entryList.at(index).title);
            index++;
        }
        ui->news->setText(tr("Latest news:")+QStringLiteral("<br />")+entryHtmlList.join("<br />"));
    }
    settings.setValue("news",ui->news->text());
    ui->news->setStyleSheet("#news{background-color:rgb(220,220,240);border:1px solid rgb(100,150,240);border-radius:5px;color:rgb(0,0,0);}");
    ui->news->setVisible(true);
}

void MainWindow::on_lineEditLogin_textChanged(const QString &arg1)
{
    if(serverLoginList.contains(arg1))
        ui->lineEditPass->setText(serverLoginList.value(arg1));
    else
        ui->lineEditPass->setText(QString());
}

void MainWindow::logged()
{
    lastServerWaitBeforeConnectAfterKick[server_dns_or_ip]=CommonSettingsServer::commonSettingsServer.waitBeforeConnectAfterKick;
}

void MainWindow::updateTheOkButton()
{
    if(!lastServerWaitBeforeConnectAfterKick.contains(server_dns_or_ip))
    {
        ui->pushButtonTryLogin->setEnabled(true);
        ui->pushButtonTryLogin->setText(tr("Ok"));
        return;
    }
    if(!lastServerConnect.contains(server_dns_or_ip))
    {
        ui->pushButtonTryLogin->setEnabled(true);
        ui->pushButtonTryLogin->setText(tr("Ok"));
        return;
    }
    uint32_t timeToWait=5;
    if(lastServerIsKick.value(server_dns_or_ip))
        if(lastServerWaitBeforeConnectAfterKick.value(server_dns_or_ip)>timeToWait)
            timeToWait=lastServerWaitBeforeConnectAfterKick.value(server_dns_or_ip);
    uint32_t secondLstSinceConnexion=lastServerConnect.value(server_dns_or_ip).toTime_t()-QDateTime::currentDateTime().toTime_t();
    if(secondLstSinceConnexion>=timeToWait)
    {
        ui->pushButtonTryLogin->setEnabled(true);
        ui->pushButtonTryLogin->setText(tr("Ok"));
        return;
    }
    else
    {
        ui->pushButtonTryLogin->setEnabled(false);
        ui->pushButtonTryLogin->setText(tr("Ok (%1)").arg(timeToWait-secondLstSinceConnexion));
        return;
    }
}

void MainWindow::gameIsLoaded()
{
    #ifndef CATCHCHALLENGER_NOAUDIO
    if(vlcPlayer!=NULL)
        libvlc_media_player_stop(vlcPlayer);
    #endif
    this->setWindowTitle(QStringLiteral("CatchChallenger - %1 - %2").arg(server_name).arg(client->getPseudo()));
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
