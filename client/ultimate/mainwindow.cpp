#include "mainwindow.h"
#include "AddServer.h"
#include "ui_mainwindow.h"
#include "../base/InternetUpdater.h"
#include <QStandardPaths>
#include <QNetworkProxy>
#include <QCoreApplication>
#include <QSslKey>
#include <QInputDialog>
#include <QSettings>

#ifdef Q_CC_GNU
//this next header is needed to change file time/date under gcc
#include <utime.h>
#include <sys/stat.h>
#include <iostream>
#endif

#ifdef Q_OS_LINUX
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#endif

#include "../base/render/MapVisualiserPlayer.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/CommonSettingsCommon.h"
#include "../base/LanguagesSelect.h"
#include "../base/Api_client_real.h"
#include "../base/Api_client_virtual.h"
#include "../base/SslCert.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    toQuit(false),
    ui(new Ui::MainWindow)
{
    static bool crackedVersion=false;
    if(!crackedVersion)
    {
        while(1)
        {
            QSettings keySettings;
            QString key;
            if(keySettings.contains(QStringLiteral("key")))
            {
                QCryptographicHash hash(QCryptographicHash::Sha224);
                hash.addData(keySettings.value(QStringLiteral("key")).toString().toUtf8());
                const QByteArray &result=hash.result();
                if(!result.isEmpty() && result.at(0)==0x00 && result.at(1)==0x00)
                    break;
            }
            key=QInputDialog::getText(this,tr("Key"),tr("Give the key of this software, more information on <a href=\"http://catchchallenger.first-world.info/\">catchchallenger.first-world.info</a>"));
            if(key.isEmpty())
            {
                QCoreApplication::quit();
                toQuit=true;
                return;
            }
            {
                QCryptographicHash hash(QCryptographicHash::Sha224);
                hash.addData(key.toUtf8());
                const QByteArray &result=hash.result();
                if(!result.isEmpty() && result.at(0)==0x00 && result.at(1)==0x00)
                {
                    keySettings.setValue(QStringLiteral("key"),key);
                    break;
                }
            }
        }
    }

    qDebug() << "QStandardPaths::writableLocation(QStandardPaths::DataLocation)" << QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    serverMode=ServerMode_None;
    qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");
    qRegisterMetaType<CatchChallenger::Chat_type>("CatchChallenger::Chat_type");
    qRegisterMetaType<CatchChallenger::Player_type>("CatchChallenger::Player_type");
    qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");
    qRegisterMetaType<CatchChallenger::Chat_type>("CatchChallenger::Chat_type");
    //qRegisterMetaType<QAbstractSocket::SocketState>("QAbstractSocket::SocketState");-> bug [Fatal] QMetaType::registerType: Binary compatibility break -- Size mismatch for type 'QAbstractSocket::SocketState' [1062]. Previously registered size 1, now registering size 4.
    qRegisterMetaType<CatchChallenger::Player_private_and_public_informations>("CatchChallenger::Player_private_and_public_informations");
    qRegisterMetaType<CatchChallenger::Player_public_informations>("CatchChallenger::Player_public_informations");
    qRegisterMetaType<CatchChallenger::Direction>("CatchChallenger::Direction");
    qRegisterMetaType<QList<RssNews::RssEntry> >("QList<RssNews::RssEntry>");

    socket=NULL;
    realSslSocket=NULL;
    internalServer=NULL;
    CatchChallenger::Api_client_real::client=NULL;
    reply=NULL;
    spacer=new QSpacerItem(0,0,QSizePolicy::Expanding,QSizePolicy::Expanding);
    spacerServer=new QSpacerItem(0,0,QSizePolicy::Expanding,QSizePolicy::Expanding);
    ui->setupUi(this);
    ui->update->setVisible(false);
    ui->news->setVisible(false);
    InternetUpdater::internetUpdater=new InternetUpdater();
    connect(InternetUpdater::internetUpdater,&InternetUpdater::newUpdate,this,&MainWindow::newUpdate);
    RssNews::rssNews=new RssNews();
    connect(RssNews::rssNews,&RssNews::rssEntryList,this,&MainWindow::rssEntryList);
    solowindow=new SoloWindow(this,QCoreApplication::applicationDirPath()+QStringLiteral("/datapack/internal/"),QStandardPaths::writableLocation(QStandardPaths::DataLocation)+QStringLiteral("/savegames/"),false);
    connect(solowindow,&SoloWindow::back,this,&MainWindow::gameSolo_back);
    connect(solowindow,&SoloWindow::play,this,&MainWindow::gameSolo_play);
    ui->stackedWidget->addWidget(solowindow);
    ui->stackedWidget->setCurrentWidget(ui->mode);
    ui->warning->setVisible(false);
    ui->server_refresh->setEnabled(true);
    temp_xmlConnexionInfoList=loadXmlConnexionInfoList();
    temp_customConnexionInfoList=loadConfigConnexionInfoList();
    mergedConnexionInfoList=temp_customConnexionInfoList+temp_xmlConnexionInfoList;
    qSort(mergedConnexionInfoList);
    displayServerList();
    CatchChallenger::BaseWindow::baseWindow=new CatchChallenger::BaseWindow();
    ui->stackedWidget->addWidget(CatchChallenger::BaseWindow::baseWindow);
    connect(socket,static_cast<void(CatchChallenger::ConnectedSocket::*)(QAbstractSocket::SocketError)>(&CatchChallenger::ConnectedSocket::error),this,&MainWindow::error,Qt::QueuedConnection);
    connect(CatchChallenger::BaseWindow::baseWindow,&CatchChallenger::BaseWindow::newError,this,&MainWindow::newError,Qt::QueuedConnection);
    //connect(CatchChallenger::BaseWindow::baseWindow,                &CatchChallenger::BaseWindow::needQuit,             this,&MainWindow::needQuit);
    connect(&updateTheOkButtonTimer,&QTimer::timeout,this,&MainWindow::updateTheOkButton);

    stopFlood.setSingleShot(false);
    stopFlood.start(1500);
    updateTheOkButtonTimer.setSingleShot(false);
    updateTheOkButtonTimer.start(1000);
    numberForFlood=0;
    haveShowDisconnectionReason=false;

    stateChanged(QAbstractSocket::UnconnectedState);

    setWindowTitle(QStringLiteral("CatchChallenger Ultimate"));
    downloadFile();

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
    #ifdef CATCHCHALLENGER_GITCOMMIT
    ui->version->setText(QStringLiteral(CATCHCHALLENGER_VERSION)+QStringLiteral(" - ")+QStringLiteral(CATCHCHALLENGER_GITCOMMIT));
    #else
    ui->version->setText(QStringLiteral(CATCHCHALLENGER_VERSION));
    #endif
}

MainWindow::~MainWindow()
{
    if(vlcPlayer!=NULL)
    {
        libvlc_media_player_stop(vlcPlayer);
        Audio::audio.removePlayer(vlcPlayer);
    }
    if(CatchChallenger::BaseWindow::baseWindow!=NULL)
    {
        CatchChallenger::BaseWindow::baseWindow->deleteLater();
        CatchChallenger::BaseWindow::baseWindow=NULL;
    }
    if(CatchChallenger::BaseWindow::baseWindow!=NULL)
        delete CatchChallenger::BaseWindow::baseWindow;
    /*if(socket!=NULL)
    {
        socket->disconnectFromHost();
        socket->abort();
        delete socket;
        socket=NULL;
    }*/
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(vlcPlayer!=NULL)
        libvlc_media_player_stop(vlcPlayer);
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
        {
            socket->disconnectFromHost();
            if(socket!=NULL)
                socket->abort();
        }
    }
    else
        QCoreApplication::quit();
}

QList<ConnexionInfo> MainWindow::loadConfigConnexionInfoList()
{
    QList<ConnexionInfo> returnedVar;
    QStringList connexionList;
    QStringList nameList;
    QStringList connexionCounterList;
    QStringList lastConnexionList;
    QStringList proxyList;
    if(settings.contains(QStringLiteral("connexionList")))
        connexionList=settings.value(QStringLiteral("connexionList")).toStringList();
    if(settings.contains(QStringLiteral("proxyList")))
        proxyList=settings.value(QStringLiteral("proxyList")).toStringList();
    if(settings.contains(QStringLiteral("nameList")))
        nameList=settings.value(QStringLiteral("nameList")).toStringList();
    if(settings.contains(QStringLiteral("connexionCounterList")))
        connexionCounterList=settings.value(QStringLiteral("connexionCounterList")).toStringList();
    if(settings.contains(QStringLiteral("lastConnexionList")))
        lastConnexionList=settings.value(QStringLiteral("lastConnexionList")).toStringList();
    if(nameList.size()!=connexionList.size())
        nameList.clear();
    if(connexionCounterList.size()!=connexionList.size())
        connexionCounterList.clear();
    if(proxyList.size()!=connexionList.size())
        proxyList.clear();
    if(lastConnexionList.size()!=connexionList.size())
        lastConnexionList.clear();
    while(nameList.size()<connexionList.size())
        nameList << QString();
    while(proxyList.size()<connexionList.size())
        proxyList << QString();
    while(connexionCounterList.size()<connexionList.size())
        connexionCounterList << QString();
    while(lastConnexionList.size()<connexionList.size())
        lastConnexionList << QString();
    int index=0;
    const QRegularExpression regexConnexion(QStringLiteral("^[a-zA-Z0-9\\.\\-_]+:[0-9]{1,5}$"));
    const QRegularExpression hostRemove(QStringLiteral(":[0-9]{1,5}$"));
    const QRegularExpression postRemove("^.*:");
    while(index<connexionList.size())
    {
        QString connexion=connexionList.at(index);
        QString name=nameList.at(index);
        QString connexionCounter=connexionCounterList.at(index);
        QString lastConnexion=lastConnexionList.at(index);
        QString proxy=proxyList.at(index);
        if(connexion.contains(regexConnexion))
        {
            QString host=connexion;
            host.remove(hostRemove);
            QString port_string=connexion;
            port_string.remove(postRemove);
            bool ok;
            uint16_t port=port_string.toInt(&ok);
            if(ok)
            {
                ConnexionInfo connexionInfo;
                connexionInfo.host=host;
                connexionInfo.port=port;
                connexionInfo.name=name;
                while(customServerName.contains(connexionInfo.name))
                    connexionInfo.name=tr("Copy of %1").arg(connexionInfo.name);
                customServerName << name;
                connexionInfo.connexionCounter=connexionCounter.toUInt(&ok);
                if(!ok)
                    connexionInfo.connexionCounter=0;
                connexionInfo.lastConnexion=lastConnexion.toUInt(&ok);
                if(!ok)
                    connexionInfo.lastConnexion=QDateTime::currentMSecsSinceEpoch()/1000;
                if(connexionInfo.lastConnexion>(QDateTime::currentMSecsSinceEpoch()/1000))
                    connexionInfo.lastConnexion=QDateTime::currentMSecsSinceEpoch()/1000;
                if(proxy.contains(regexConnexion))
                {
                    QString host=proxy;
                    host.remove(hostRemove);
                    QString proxy_port_string=proxy;
                    proxy_port_string.remove(postRemove);
                    bool ok;
                    uint16_t proxy_port=proxy_port_string.toInt(&ok);
                    if(ok)
                    {
                        connexionInfo.proxyHost=host;
                        connexionInfo.proxyPort=proxy_port;
                    }
                }
                returnedVar << connexionInfo;
            }
        }
        index++;
    }
    return returnedVar;
}

QList<ConnexionInfo> MainWindow::loadXmlConnexionInfoList()
{
    if(QFileInfo(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).isDir())
    {
        if(QFileInfo(QStandardPaths::writableLocation(QStandardPaths::DataLocation)+QStringLiteral("/server_list.xml")).isFile())
            return loadXmlConnexionInfoList(QStandardPaths::writableLocation(QStandardPaths::DataLocation)+QStringLiteral("/server_list.xml"));
        else
            return loadXmlConnexionInfoList(QStringLiteral(":/other/default_server_list.xml"));
    }
    else
        return loadXmlConnexionInfoList(QStringLiteral(":/other/default_server_list.xml"));
}

QList<ConnexionInfo> MainWindow::loadXmlConnexionInfoList(const QByteArray &xmlContent)
{
    QList<ConnexionInfo> returnedVar;
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg("server_list.xml").arg(errorLine).arg(errorColumn).arg(errorStr);
        return returnedVar;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=QStringLiteral("servers"))
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, \"servers\" root balise not found for the xml file").arg("server_list.xml");
        return returnedVar;
    }

    QRegularExpression regexHost("^[a-zA-Z0-9\\.\\-_]+$");
    bool ok;
    //load the content
    QDomElement server = root.firstChildElement(QStringLiteral("server"));
    while(!server.isNull())
    {
        if(server.isElement())
        {
            if(server.hasAttribute(QStringLiteral("host")) && server.hasAttribute(QStringLiteral("port")) && server.hasAttribute(QStringLiteral("unique_code")))
            {
                ConnexionInfo connexionInfo;
                connexionInfo.host=server.attribute(QStringLiteral("host"));
                connexionInfo.unique_code=server.attribute(QStringLiteral("unique_code"));
                uint32_t temp_port=server.attribute(QStringLiteral("port")).toUInt(&ok);
                if(!connexionInfo.host.contains(regexHost))
                    qDebug() << QStringLiteral("Unable to open the file: %1, host is wrong: %4 child.tagName(): %2 (at line: %3)").arg("server_list.xml").arg(server.tagName()).arg(server.lineNumber()).arg(connexionInfo.host);
                else if(!ok)
                    qDebug() << QStringLiteral("Unable to open the file: %1, port is not a number: %4 child.tagName(): %2 (at line: %3)").arg("server_list.xml").arg(server.tagName()).arg(server.lineNumber()).arg(server.attribute("port"));
                else if(temp_port<1 || temp_port>65535)
                    qDebug() << QStringLiteral("Unable to open the file: %1, port is not in range: %4 child.tagName(): %2 (at line: %3)").arg("server_list.xml").arg(server.tagName()).arg(server.lineNumber()).arg(server.attribute("port"));
                else if(connexionInfo.unique_code.isEmpty())
                    qDebug() << QStringLiteral("Unable to open the file: %1, unique_code can't be empty: %4 child.tagName(): %2 (at line: %3)").arg("server_list.xml").arg(server.tagName()).arg(server.lineNumber()).arg(server.attribute("port"));
                else
                {
                    connexionInfo.port=temp_port;
                    QDomElement lang;
                    const QString &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
                    bool found=false;
                    if(!language.isEmpty() && language!=QStringLiteral("en"))
                    {
                        lang = server.firstChildElement(QStringLiteral("lang"));
                        while(!lang.isNull())
                        {
                            if(lang.isElement())
                            {
                                if(lang.hasAttribute(QStringLiteral("lang")) && lang.attribute(QStringLiteral("lang"))==language)
                                {
                                    QDomElement name = lang.firstChildElement(QStringLiteral("name"));
                                    if(!name.isNull())
                                        connexionInfo.name=name.text();
                                    QDomElement register_page = lang.firstChildElement(QStringLiteral("register_page"));
                                    if(!register_page.isNull())
                                        connexionInfo.register_page=register_page.text();
                                    QDomElement lost_passwd_page = lang.firstChildElement(QStringLiteral("lost_passwd_page"));
                                    if(!lost_passwd_page.isNull())
                                        connexionInfo.lost_passwd_page=lost_passwd_page.text();
                                    QDomElement site_page = lang.firstChildElement(QStringLiteral("site_page"));
                                    if(!site_page.isNull())
                                        connexionInfo.site_page=site_page.text();
                                    found=true;
                                    break;
                                }
                            }
                            else
                                qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg("server_list.xml").arg(lang.tagName()).arg(lang.lineNumber());
                            lang = lang.nextSiblingElement(QStringLiteral("lang"));
                        }
                    }
                    if(!found)
                    {
                        lang = server.firstChildElement(QStringLiteral("lang"));
                        while(!lang.isNull())
                        {
                            if(lang.isElement())
                            {
                                if(!lang.hasAttribute(QStringLiteral("lang")) || lang.attribute(QStringLiteral("lang"))==QStringLiteral("en"))
                                {
                                    QDomElement name = lang.firstChildElement(QStringLiteral("name"));
                                    if(!name.isNull())
                                        connexionInfo.name=name.text();
                                    QDomElement register_page = lang.firstChildElement(QStringLiteral("register_page"));
                                    if(!register_page.isNull())
                                        connexionInfo.register_page=register_page.text();
                                    QDomElement lost_passwd_page = lang.firstChildElement(QStringLiteral("lost_passwd_page"));
                                    if(!lost_passwd_page.isNull())
                                        connexionInfo.lost_passwd_page=lost_passwd_page.text();
                                    QDomElement site_page = lang.firstChildElement(QStringLiteral("site_page"));
                                    if(!site_page.isNull())
                                        connexionInfo.site_page=site_page.text();
                                    break;
                                }
                            }
                            else
                                qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg("server_list.xml").arg(lang.tagName()).arg(lang.lineNumber());
                            lang = lang.nextSiblingElement(QStringLiteral("lang"));
                        }
                    }
                    settings.beginGroup(QStringLiteral("Xml-%1").arg(server.attribute("unique_code")));
                    if(settings.contains(QStringLiteral("connexionCounter")))
                    {
                        connexionInfo.connexionCounter=settings.value(QStringLiteral("connexionCounter")).toUInt(&ok);
                        if(!ok)
                            connexionInfo.connexionCounter=0;
                    }
                    else
                        connexionInfo.connexionCounter=0;

                    //proxy
                    if(settings.contains(QStringLiteral("proxyPort")))
                    {
                        connexionInfo.proxyPort=settings.value(QStringLiteral("proxyPort")).toUInt(&ok);
                        if(!ok)
                            connexionInfo.proxyPort=9050;
                    }
                    else
                        connexionInfo.proxyPort=9050;
                    if(settings.contains(QStringLiteral("proxyHost")))
                        connexionInfo.proxyHost=settings.value(QStringLiteral("proxyHost")).toString();
                    else
                        connexionInfo.proxyHost=QString();

                    if(settings.contains(QStringLiteral("lastConnexion")))
                    {
                        connexionInfo.lastConnexion=settings.value(QStringLiteral("lastConnexion")).toUInt(&ok);
                        if(!ok)
                            connexionInfo.lastConnexion=QDateTime::currentMSecsSinceEpoch()/1000;
                    }
                    else
                        connexionInfo.lastConnexion=QDateTime::currentMSecsSinceEpoch()/1000;
                    settings.endGroup();
                    if(connexionInfo.lastConnexion>(QDateTime::currentMSecsSinceEpoch()/1000))
                        connexionInfo.lastConnexion=QDateTime::currentMSecsSinceEpoch()/1000;
                    returnedVar << connexionInfo;
                }
            }
            else
                qDebug() << QStringLiteral("Unable to open the file: %1, missing host or port: child.tagName(): %2 (at line: %3)").arg("server_list.xml").arg(server.tagName()).arg(server.lineNumber());
        }
        else
            qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg("server_list.xml").arg(server.tagName()).arg(server.lineNumber());
        server = server.nextSiblingElement(QStringLiteral("server"));
    }
    return returnedVar;
}

QList<ConnexionInfo> MainWindow::loadXmlConnexionInfoList(const QString &file)
{
    QList<ConnexionInfo> returnedVar;
    //open and quick check the file
    QFile itemsFile(file);
    QByteArray xmlContent;
    if(!itemsFile.open(QIODevice::ReadOnly))
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, error: %2").arg(itemsFile.fileName()).arg(itemsFile.errorString());
        return returnedVar;
    }
    xmlContent=itemsFile.readAll();
    itemsFile.close();
    return loadXmlConnexionInfoList(xmlContent);
}

bool ConnexionInfo::operator<(const ConnexionInfo &connexionInfo) const
{
    if(connexionCounter<connexionInfo.connexionCounter)
        return false;
    if(connexionCounter>connexionInfo.connexionCounter)
        return true;
    if(lastConnexion<connexionInfo.lastConnexion)
        return false;
    if(lastConnexion>connexionInfo.lastConnexion)
        return true;
    return true;
}

void MainWindow::displayServerList()
{
    QString unique_code;
    if(serverConnexion.contains(selectedServer))
    {
        if(serverConnexion.value(selectedServer)->unique_code.isEmpty())
            unique_code=QString("%1:%2").arg(serverConnexion.value(selectedServer)->host).arg(serverConnexion.value(selectedServer)->port);
        else
            unique_code=serverConnexion.value(selectedServer)->unique_code;
    }
    selectedServer=NULL;
    int index=0;
    while(server.size()>0)
    {
        delete server.at(0);
        server.removeAt(0);
        index++;
    }
    serverConnexion.clear();
    if(mergedConnexionInfoList.isEmpty())
        ui->serverEmpty->setText(QStringLiteral("<html><body><p align=\"center\"><span style=\"font-size:12pt;color:#a0a0a0;\">%1</span></p></body></html>").arg(tr("Empty")));
    index=0;
    while(index<mergedConnexionInfoList.size())
    {
        ListEntryEnvolued *newEntry=new ListEntryEnvolued();
        connect(newEntry,&ListEntryEnvolued::clicked,this,&MainWindow::serverListEntryEnvoluedClicked,Qt::QueuedConnection);
        connect(newEntry,&ListEntryEnvolued::doubleClicked,this,&MainWindow::serverListEntryEnvoluedDoubleClicked,Qt::QueuedConnection);
        const ConnexionInfo &connexionInfo=mergedConnexionInfoList.at(index);
        QString connexionInfoHost=connexionInfo.host;
        if(connexionInfoHost.size()>32)
            connexionInfoHost=connexionInfoHost.left(15)+"..."+connexionInfoHost.right(15);
        QString name;
        QString star;
        if(connexionInfo.connexionCounter>0)
            star+=QStringLiteral("<img src=\":/images/interface/top.png\" alt=\"\" />");
        QString lastConnexion;
        if(connexionInfo.connexionCounter>0)
            lastConnexion=tr("Last connexion: %1").arg(QDateTime::fromMSecsSinceEpoch((uint64_t)connexionInfo.lastConnexion*1000).toString());
        QString custom;
        if(connexionInfo.unique_code.isEmpty())
        {
            custom=QStringLiteral(" (%1)").arg(tr("Custom"));
            if(unique_code==QString("%1:%2").arg(connexionInfoHost).arg(connexionInfo.port))
                selectedServer=newEntry;
        }
        else
        {
            if(unique_code==connexionInfo.unique_code)
                selectedServer=newEntry;
        }
        if(connexionInfo.name.isEmpty())
        {
            name=QStringLiteral("%1:%2").arg(connexionInfoHost).arg(connexionInfo.port);
            newEntry->setText(QStringLiteral("%3<span style=\"font-size:12pt;font-weight:600;\">%1:%2</span><br/><span style=\"color:#909090;\">%4%5</span>")
                              .arg(connexionInfoHost)
                              .arg(connexionInfo.port)
                              .arg(star)
                              .arg(lastConnexion)
                              .arg(custom)
                              );
        }
        else
        {
            name=connexionInfo.name;
            newEntry->setText(QStringLiteral("%4<span style=\"font-size:12pt;font-weight:600;\">%1</span><br/><span style=\"color:#909090;\">%2:%3 %5%6</span>")
                              .arg(connexionInfo.name)
                              .arg(connexionInfoHost)
                              .arg(connexionInfo.port)
                              .arg(star)
                              .arg(lastConnexion)
                              .arg(custom)
                              );
        }
        newEntry->setStyleSheet(QStringLiteral("QLabel::hover{border:1px solid #bbb;background-color:rgb(180,180,180,100);border-radius:10px;}"));

        ui->scrollAreaWidgetContentsServer->layout()->addWidget(newEntry);

        server << newEntry;
        serverConnexion[newEntry]=&mergedConnexionInfoList[index];
        if(connexionInfo.unique_code.isEmpty())
            customServerConnexion << newEntry;
        index++;
    }
    ui->serverWidget->setVisible(index==0);
    if(index>0)
    {
        ui->scrollAreaWidgetContentsServer->layout()->removeItem(spacerServer);
        delete spacerServer;
        spacerServer=new QSpacerItem(0,0,QSizePolicy::Expanding,QSizePolicy::Expanding);
        ui->scrollAreaWidgetContentsServer->layout()->addItem(spacerServer);
    }
    serverListEntryEnvoluedUpdate();
}

void MainWindow::serverListEntryEnvoluedClicked()
{
    ListEntryEnvolued * selectedSavegame=qobject_cast<ListEntryEnvolued *>(QObject::sender());
    if(selectedSavegame==NULL)
        return;
    this->selectedServer=selectedSavegame;
    serverListEntryEnvoluedUpdate();
}

void MainWindow::serverListEntryEnvoluedUpdate()
{
    int index=0;
    while(index<server.size())
    {
        if(server.at(index)==selectedServer)
            server.at(index)->setStyleSheet(QStringLiteral("QLabel{border:1px solid #6b6;background-color:rgb(100,180,100,120);border-radius:10px;}QLabel::hover{border:1px solid #494;background-color:rgb(70,150,70,120);border-radius:10px;}"));
        else
            server.at(index)->setStyleSheet(QStringLiteral("QLabel::hover{border:1px solid #bbb;background-color:rgb(180,180,180,100);border-radius:10px;}"));
        index++;
    }
    ui->server_select->setEnabled(selectedServer!=NULL);
    ui->server_remove->setEnabled(selectedServer!=NULL && customServerConnexion.contains(selectedServer));
    ui->server_edit->setEnabled(selectedServer!=NULL && customServerConnexion.contains(selectedServer));
}

void MainWindow::on_server_add_clicked()
{
    AddOrEditServer addServer(this);
    addServer.exec();
    if(!addServer.isOk())
        return;
    if(!addServer.server().contains(QRegularExpression("^[a-zA-Z0-9\\.\\-_]+$")))
    {
        QMessageBox::warning(this,tr("Error"),tr("The host seam don't be a valid hostname or ip"));
        return;
    }
    if(customServerName.contains(addServer.name()))
    {
        QMessageBox::warning(this,tr("Error"),tr("The name is already taken"));
        return;
    }
    ConnexionInfo connexionInfo;
    connexionInfo.connexionCounter=0;
    connexionInfo.host=addServer.server();
    connexionInfo.lastConnexion=QDateTime::currentMSecsSinceEpoch()/1000;
    connexionInfo.name=addServer.name();
    connexionInfo.port=addServer.port();
    connexionInfo.proxyHost=addServer.proxyServer();
    connexionInfo.proxyPort=addServer.proxyPort();
    mergedConnexionInfoList << connexionInfo;
    saveConnexionInfoList();
    displayServerList();
}

void MainWindow::on_server_select_clicked()
{
    if(!serverConnexion.contains(selectedServer))
    {
        QMessageBox::warning(this,tr("Internal error"),tr("Internal error: Can't select this server"));
        return;
    }
    ui->stackedWidget->setCurrentWidget(ui->login);
    updateTheOkButton();
    if(customServerConnexion.contains(selectedServer))
        settings.beginGroup(QStringLiteral("%1-%2").arg(serverConnexion[selectedServer]->host).arg(serverConnexion[selectedServer]->port));
    else
        settings.beginGroup(QStringLiteral("Xml-%1").arg(serverConnexion[selectedServer]->unique_code));
    if(!serverConnexion.value(selectedServer)->register_page.isEmpty())
    {
        ui->label_login_register->setVisible(true);
        ui->label_login_register->setText(tr("<a href=\"%1\"><span style=\"text-decoration:underline;color:#0057ae;\">Register</span></a>").arg(serverConnexion.value(selectedServer)->register_page));
    }
    else
        ui->label_login_register->setVisible(false);
    if(!serverConnexion.value(selectedServer)->site_page.isEmpty())
    {
        ui->label_login_website->setVisible(true);
        ui->label_login_website->setText(tr("<a href=\"%1\"><span style=\"text-decoration:underline;color:#0057ae;\">Web site</span></a>").arg(serverConnexion.value(selectedServer)->site_page));
    }
    else
        ui->label_login_website->setVisible(false);
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
    settings.endGroup();
    ui->checkBoxRememberPassword->setChecked(!ui->lineEditPass->text().isEmpty());
}

void MainWindow::on_login_cancel_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->poolServersList);
}

void MainWindow::on_server_remove_clicked()
{
    if(selectedServer==NULL)
        return;
    if(!customServerConnexion.contains(selectedServer))
        return;
    int index=0;
    while(index<mergedConnexionInfoList.size())
    {
        if(serverConnexion[selectedServer]==&mergedConnexionInfoList.at(index))
        {
            customServerName.remove(serverConnexion[selectedServer]->name);
            mergedConnexionInfoList.removeAt(index);
            if(customServerConnexion.contains(selectedServer))
                saveConnexionInfoList();
            else
            {
                settings.beginGroup(QStringLiteral("Xml-%1").arg(serverConnexion[selectedServer]->unique_code));
                if(settings.contains(QStringLiteral("connexionCounter")))
                    settings.setValue(QStringLiteral("connexionCounter"),settings.value(QStringLiteral("connexionCounter")).toUInt()+1);
                else
                    settings.setValue(QStringLiteral("connexionCounter"),1);
                settings.setValue(QStringLiteral("lastConnexion"),QDateTime::currentMSecsSinceEpoch()/1000);
                settings.endGroup();
            }
            serverConnexion.remove(selectedServer);
            selectedServer=NULL;
            displayServerList();
            break;
        }
        index++;
    }
}

void MainWindow::saveConnexionInfoList()
{
    QStringList connexionList;
    QStringList nameList;
    QStringList connexionCounterList;
    QStringList lastConnexionList;
    QStringList proxyList;
    int index;
    index=0;
    while(index<mergedConnexionInfoList.size())
    {
        const ConnexionInfo &connexionInfo=mergedConnexionInfoList.at(index);
        if(connexionInfo.unique_code.isEmpty())
        {
            QString proxy;
            if(!connexionInfo.proxyHost.isEmpty())
                proxy=QStringLiteral("%1:%2").arg(connexionInfo.proxyHost).arg(connexionInfo.proxyPort);
            else
                proxy=QStringLiteral("");
            connexionList << QStringLiteral("%1:%2").arg(connexionInfo.host).arg(connexionInfo.port);
            nameList << connexionInfo.name;
            connexionCounterList << QString::number(connexionInfo.connexionCounter);
            lastConnexionList << QString::number(connexionInfo.lastConnexion);
            proxyList << proxy;
        }
        else
        {
            settings.beginGroup(QStringLiteral("Xml-%1").arg(connexionInfo.unique_code));
            if(connexionInfo.connexionCounter>0)
                settings.setValue(QStringLiteral("connexionCounter"),connexionInfo.connexionCounter);
            else
                settings.remove(QStringLiteral("connexionCounter"));
            if(connexionInfo.lastConnexion>0 && connexionInfo.connexionCounter>0)
                settings.setValue(QStringLiteral("lastConnexion"),connexionInfo.lastConnexion);
            else
                settings.remove(QStringLiteral("lastConnexion"));
            settings.endGroup();
        }
        index++;
    }
    settings.setValue(QStringLiteral("connexionList"),connexionList);
    settings.setValue(QStringLiteral("nameList"),nameList);
    settings.setValue(QStringLiteral("connexionCounterList"),connexionCounterList);
    settings.setValue(QStringLiteral("lastConnexionList"),lastConnexionList);
    settings.setValue(QStringLiteral("proxyList"),proxyList);
}

void MainWindow::serverListEntryEnvoluedDoubleClicked()
{
    on_server_select_clicked();
}

void MainWindow::resetAll()
{
    if(vlcPlayer!=NULL)
        libvlc_media_player_play(vlcPlayer);
    if(CatchChallenger::Api_client_real::client!=NULL)
    {
        CatchChallenger::Api_client_real::client->resetAll();
        CatchChallenger::Api_client_real::client->deleteLater();
        CatchChallenger::Api_client_real::client=NULL;
    }
    if(CatchChallenger::BaseWindow::baseWindow!=NULL)
        CatchChallenger::BaseWindow::baseWindow->resetAll();
    setWindowTitle(QStringLiteral("CatchChallenger Ultimate"));
    switch(serverMode)
    {
        case ServerMode_Internal:
            ui->stackedWidget->setCurrentWidget(solowindow);
            /* do just at game starting
             *if(internalServer!=NULL)
            {
                internalServer->stop();
                internalServer->deleteLater();
                internalServer=NULL;
            }*/
            saveTime();
        break;
        case ServerMode_Remote:
            ui->stackedWidget->setCurrentWidget(ui->poolServersList);
        break;
        default:
            ui->stackedWidget->setCurrentWidget(ui->mode);
        break;
    }
    if(socket!=NULL)
    {
        socket->disconnectFromHost();
        //can be above !=NULL but here ==NULL
        if(socket!=NULL)
            socket->abort();
    }
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
}

void MainWindow::sslErrors(const QList<QSslError> &errors)
{
    haveShowDisconnectionReason=true;
    QStringList sslErrors;
    int index=0;
    while(index<errors.size())
    {
        qDebug() << "Ssl error:" << errors.at(index).errorString();
        sslErrors << errors.at(index).errorString();
        index++;
    }
    /*QMessageBox::warning(this,tr("Ssl error"),sslErrors.join("\n"));
    realSocket->disconnectFromHost();*/
}

void MainWindow::disconnected(QString reason)
{
    QMessageBox::information(this,tr("Disconnected"),tr("Disconnected by the reason: %1").arg(reason));
    /*if(serverConnexion.contains(selectedServer))
        lastServerIsKick[serverConnexion.value(selectedServer)->host]=true;*/
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
    if(ui->lineEditLogin->text().size()<3)
    {
        QMessageBox::warning(this,tr("Error"),tr("Your login need to be at minimum of 3 characters"));
        return;
    }
    serverMode=ServerMode_Remote;
    lastServerConnect[serverConnexion.value(selectedServer)->host]=QDateTime::currentDateTime();
    lastServerIsKick[serverConnexion.value(selectedServer)->host]=false;
    if(customServerConnexion.contains(selectedServer))
        settings.beginGroup(QStringLiteral("%1-%2").arg(serverConnexion.value(selectedServer)->host).arg(serverConnexion.value(selectedServer)->port));
    else
        settings.beginGroup(QStringLiteral("Xml-%1").arg(serverConnexion.value(selectedServer)->unique_code));

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

    settings.endGroup();
    if(socket!=NULL)
    {
        socket->disconnectFromHost();
        socket->abort();
        delete socket;
        socket=NULL;
        realSslSocket=NULL;
    }
    realSslSocket=new QSslSocket();
    socket=new CatchChallenger::ConnectedSocket(realSslSocket);
    CatchChallenger::Api_client_real::client=new CatchChallenger::Api_client_real(socket);
    if(!serverConnexion.value(selectedServer)->proxyHost.isEmpty())
    {
        QNetworkProxy proxy=realSslSocket->proxy();
        proxy.setType(QNetworkProxy::Socks5Proxy);
        proxy.setHostName(serverConnexion.value(selectedServer)->proxyHost);
        proxy.setPort(serverConnexion.value(selectedServer)->proxyPort);
        realSslSocket->setProxy(proxy);
    }
    ui->stackedWidget->setCurrentWidget(CatchChallenger::BaseWindow::baseWindow);

    CatchChallenger::BaseWindow::baseWindow->stateChanged(QAbstractSocket::ConnectingState);
    connect(realSslSocket,static_cast<void(QSslSocket::*)(const QList<QSslError> &errors)>(&QSslSocket::sslErrors),  this,&MainWindow::sslErrors,Qt::QueuedConnection);
    connect(realSslSocket,&QSslSocket::stateChanged,    this,&MainWindow::stateChanged,Qt::DirectConnection);
    connect(realSslSocket,static_cast<void(QSslSocket::*)(QAbstractSocket::SocketError)>(&QSslSocket::error),           this,&MainWindow::error,Qt::QueuedConnection);
    realSslSocket->connectToHost(serverConnexion.value(selectedServer)->host,serverConnexion.value(selectedServer)->port);
    serverConnexion.value(selectedServer)->connexionCounter++;
    serverConnexion.value(selectedServer)->lastConnexion=QDateTime::currentMSecsSinceEpoch()/1000;
    saveConnexionInfoList();
    displayServerList();
    connectTheExternalSocket();
}

void MainWindow::connectTheExternalSocket()
{
    //continue the normal procedure
    if(!serverConnexion.value(selectedServer)->proxyHost.isEmpty())
    {
        QNetworkProxy proxy=realSslSocket->proxy();
        proxy.setType(QNetworkProxy::Socks5Proxy);
        proxy.setHostName(serverConnexion.value(selectedServer)->proxyHost);
        proxy.setPort(serverConnexion.value(selectedServer)->proxyPort);
        static_cast<CatchChallenger::Api_client_real *>(CatchChallenger::Api_client_real::client)->setProxy(proxy);
    }
    connect(CatchChallenger::Api_client_real::client,               &CatchChallenger::Api_protocol::protocol_is_good,   this,&MainWindow::protocol_is_good,Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,               &CatchChallenger::Api_protocol::disconnected,       this,&MainWindow::disconnected);
    //connect(CatchChallenger::Api_client_real::client,               &CatchChallenger::Api_protocol::message,            this,&MainWindow::message,Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,               &CatchChallenger::Api_protocol::logged,             this,&MainWindow::logged,Qt::QueuedConnection);
    CatchChallenger::BaseWindow::baseWindow->connectAllSignals();
    CatchChallenger::BaseWindow::baseWindow->setMultiPlayer(true);
    QDir datapack(serverToDatapachPath(selectedServer));
    if(!datapack.exists())
        if(!datapack.mkpath(datapack.absolutePath()))
        {
            disconnected(tr("Not able to create the folder %1").arg(datapack.absolutePath()));
            return;
        }
    CatchChallenger::Api_client_real::client->setDatapackPath(datapack.absolutePath());
    CatchChallenger::BaseWindow::baseWindow->stateChanged(QAbstractSocket::ConnectedState);
}

QString MainWindow::serverToDatapachPath(ListEntryEnvolued * selectedServer) const
{
    QDir datapack;
    if(customServerConnexion.contains(selectedServer))
    {
        if(!serverConnexion.value(selectedServer)->name.isEmpty())
             datapack=QDir(QStringLiteral("%1/datapack/%2/").arg(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).arg(serverConnexion.value(selectedServer)->name));
        else
             datapack=QDir(QStringLiteral("%1/datapack/%2-%3/").arg(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).arg(serverConnexion.value(selectedServer)->host).arg(serverConnexion.value(selectedServer)->port));
    }
    else
        datapack=QDir(QStringLiteral("%1/datapack/Xml-%2").arg(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).arg(serverConnexion.value(selectedServer)->unique_code));
    return datapack.absolutePath();
}

void MainWindow::stateChanged(QAbstractSocket::SocketState socketState)
{
    if(socketState==QAbstractSocket::ConnectedState)
    {
        if(realSslSocket==NULL)
            CatchChallenger::Api_client_real::client->sendProtocol();
        else
            qDebug() << "Tcp socket found, skip sendProtocol()";
    }
    if(socketState==QAbstractSocket::UnconnectedState)
    {
        if(CatchChallenger::Api_client_real::client!=NULL)
            if(CatchChallenger::Api_client_real::client->stage()==CatchChallenger::Api_client_real::StageConnexion::Stage2)
                return;
        if(!isVisible() && internalServer==NULL)
        {
            QCoreApplication::quit();
            return;
        }
        if(CatchChallenger::Api_client_real::client!=NULL && CatchChallenger::Api_client_real::client->protocolWrong())
            QMessageBox::about(this,tr("Quit"),tr("The server have closed the connexion"));
        if(internalServer!=NULL)
            internalServer->stop();
        /* to fix bug: firstly try connect but connexion refused on localhost, secondly try local game */
        if(realSslSocket!=NULL)
        {
            realSslSocket->deleteLater();
            realSslSocket=NULL;
        }
        if(socket!=NULL)
        {
            socket->deleteLater();
            socket=NULL;
        }
        /*socket will do that's if(realSocket!=NULL)
        {
            delete realSocket;
            realSocket=NULL;
        }*/
        resetAll();
        /*if(serverMode==ServerMode_Remote)
            QMessageBox::about(this,tr("Quit"),tr("The server have closed the connexion"));*/
    }
    if(CatchChallenger::BaseWindow::baseWindow!=NULL)
        CatchChallenger::BaseWindow::baseWindow->stateChanged(socketState);
}

void MainWindow::error(QAbstractSocket::SocketError socketError)
{
    if(CatchChallenger::Api_client_real::client!=NULL)
        if(CatchChallenger::Api_client_real::client->stage()==CatchChallenger::Api_client_real::StageConnexion::Stage2)
            return;
    resetAll();
    switch(socketError)
    {
    case QAbstractSocket::RemoteHostClosedError:
        if(realSslSocket!=NULL)
            return;
        if(haveShowDisconnectionReason)
        {
            haveShowDisconnectionReason=false;
            return;
        }
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

void MainWindow::newError(QString error,QString detailedError)
{
    std::cout << "MainWindow::newError(): " << error.toStdString() << ": " << detailedError.toStdString() << std::endl;
    if(CatchChallenger::Api_client_real::client!=NULL)
    {
        CatchChallenger::Api_client_real::client->tryDisconnect();
        QMessageBox::critical(this,tr("Error"),error);
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
    if(serverMode==ServerMode_Internal)
        CatchChallenger::Api_client_real::client->tryLogin(QStringLiteral("admin"),pass);
    else
        CatchChallenger::Api_client_real::client->tryLogin(ui->lineEditLogin->text(),ui->lineEditPass->text());
}

void MainWindow::needQuit()
{
    CatchChallenger::Api_client_real::client->tryDisconnect();
}

void MainWindow::ListEntryEnvoluedClicked()
{
    ListEntryEnvolued * selectedSavegame=qobject_cast<ListEntryEnvolued *>(QObject::sender());
    if(selectedSavegame==NULL)
        return;
    this->selectedDatapack=selectedSavegame;
    ListEntryEnvoluedUpdate();
}

void MainWindow::ListEntryEnvoluedUpdate()
{
    int index=0;
    while(index<datapack.size())
    {
        if(datapack.at(index)==selectedDatapack)
            datapack.at(index)->setStyleSheet(QStringLiteral("QLabel{border:1px solid #6b6;background-color:rgb(100,180,100,120);border-radius:10px;}QLabel::hover{border:1px solid #494;background-color:rgb(70,150,70,120);border-radius:10px;}"));
        else
            datapack.at(index)->setStyleSheet(QStringLiteral("QLabel::hover{border:1px solid #bbb;background-color:rgb(180,180,180,100);border-radius:10px;}"));
        index++;
    }
    ui->deleteDatapack->setEnabled(selectedDatapack!=NULL &&
            (datapackPathList[selectedDatapack]!=QFileInfo(QCoreApplication::applicationDirPath()+QStringLiteral("/datapack/internal")).absoluteFilePath()
            ||
            datapackPathList[selectedDatapack]!=QFileInfo(QCoreApplication::applicationDirPath()+QStringLiteral("/datapack/Internal")).absoluteFilePath())
            );
}

void MainWindow::ListEntryEnvoluedDoubleClicked()
{
    on_deleteDatapack_clicked();
}

QPair<QString,QString> MainWindow::getDatapackInformations(const QString &filePath)
{
    QPair<QString,QString> returnVar;
    returnVar.first=tr("Unknown");
    returnVar.second=tr("Unknown");
    //open and quick check the file
    QFile itemsFile(filePath);
    QByteArray xmlContent;
    if(!itemsFile.open(QIODevice::ReadOnly))
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, error: %2").arg(itemsFile.fileName()).arg(itemsFile.errorString());
        return returnVar;
    }
    xmlContent=itemsFile.readAll();
    itemsFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(itemsFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
        return returnVar;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=QStringLiteral("informations"))
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, \"items\" root balise not found for the xml file").arg(itemsFile.fileName());
        return returnVar;
    }

    QStringList authors;
    //load the content
    QDomElement item = root.firstChildElement(QStringLiteral("author"));
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute(QStringLiteral("pseudo")) && item.hasAttribute(QStringLiteral("name")))
                authors << QStringLiteral("%1 (%2)").arg(item.attribute(QStringLiteral("pseudo"))).arg(item.attribute(QStringLiteral("name")));
            else if(item.hasAttribute(QStringLiteral("name")))
                authors << item.attribute(QStringLiteral("name"));
            else if(item.hasAttribute(QStringLiteral("pseudo")))
                authors << item.attribute(QStringLiteral("pseudo"));
        }
        item = item.nextSiblingElement(QStringLiteral("author"));
    }
    if(authors.isEmpty())
        returnVar.first=tr("Unknown");
    else
        returnVar.first=authors.join(QStringLiteral(", "));

    returnVar.second=tr("Unknown");
    item = root.firstChildElement(QStringLiteral("name"));
    bool found=false;
    const QString &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
    if(!language.isEmpty() && language!=QStringLiteral("en"))
        while(!item.isNull())
        {
            if(item.isElement())
            {
                if(item.hasAttribute(QStringLiteral("lang")) && item.attribute(QStringLiteral("lang"))==language)
                {
                    returnVar.second=item.text();
                    found=true;
                    break;
                }
            }
            item = item.nextSiblingElement(QStringLiteral("name"));
        }
    if(!found)
    {
        item = root.firstChildElement(QStringLiteral("name"));
        while(!item.isNull())
        {
            if(item.isElement())
            {
                if(!item.hasAttribute(QStringLiteral("lang")) || item.attribute(QStringLiteral("lang"))==QStringLiteral("en"))
                {
                    returnVar.second=item.text();
                    break;
                }
            }
            item = item.nextSiblingElement(QStringLiteral("name"));
        }
    }
    return returnVar;
}

void MainWindow::on_manageDatapack_clicked()
{
    QString lastSelectedPath;
    if(selectedDatapack!=NULL)
        lastSelectedPath=datapackPathList[selectedDatapack];
    selectedDatapack=NULL;
    int index=0;
    while(datapack.size()>0)
    {
        delete datapack.at(0);
        datapack.removeAt(0);
        index++;
    }
    datapackPathList.clear();
    QFileInfoList entryList=QDir(QCoreApplication::applicationDirPath()+QStringLiteral("/datapack/")).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    index=0;
    if(entryList.isEmpty())
        ui->datapackEmpty->setText(QStringLiteral("<html><body><p align=\"center\"><span style=\"font-size:12pt;color:#a0a0a0;\">%1</span></p></body></html>").arg(tr("Empty")));
    while(index<entryList.size())
    {
        QFileInfo fileInfo=entryList.at(index);
        if(!fileInfo.isDir())
        {
            index++;
            continue;
        }
        QPair<QString,QString> tempVar=getDatapackInformations(fileInfo.absoluteFilePath()+QStringLiteral("/informations.xml"));
        QString author=tempVar.first;
        QString name=tempVar.second;
        ListEntryEnvolued *newEntry=new ListEntryEnvolued();
        connect(newEntry,&ListEntryEnvolued::clicked,this,&MainWindow::ListEntryEnvoluedClicked,Qt::QueuedConnection);
        connect(newEntry,&ListEntryEnvolued::doubleClicked,this,&MainWindow::ListEntryEnvoluedDoubleClicked,Qt::QueuedConnection);
        QString from;
        if(fileInfo.fileName()==QStringLiteral("Internal") || fileInfo.fileName()==QStringLiteral("internal"))
            from=tr("Internal datapack");
        else
        {
            from=tr("Get from %1").arg(fileInfo.fileName());
            from=from.replace(QRegularExpression(QStringLiteral("-([0-9]+)$")),QStringLiteral(", ")+tr("port %1").arg(QStringLiteral("\\1")));
        }
        newEntry->setStyleSheet(QStringLiteral("QLabel::hover{border:1px solid #bbb;background-color:rgb(180,180,180,100);border-radius:10px;}"));
        newEntry->setText(QStringLiteral("<span style=\"font-size:12pt;font-weight:600;\">%1</span><br/><span style=\"color:#909090;\">%2<br/>%3</span>")
                          .arg(name)
                          .arg(tr("Author: %1").arg(author))
                          .arg(from)
                          );
        ui->scrollAreaWidgetContents->layout()->addWidget(newEntry);

        datapack << newEntry;
        datapackPathList[newEntry]=fileInfo.absoluteFilePath();
        index++;
    }
    ui->datapackWidget->setVisible(index==0);
    if(index>0)
    {
        ui->scrollAreaWidgetContents->layout()->removeItem(spacer);
        delete spacer;
        spacer=new QSpacerItem(0,0,QSizePolicy::Expanding,QSizePolicy::Expanding);
        ui->scrollAreaWidgetContents->layout()->addItem(spacer);
    }
    ListEntryEnvoluedUpdate();
    ui->stackedWidget->setCurrentWidget(ui->datapack);
}

void MainWindow::on_backDatapack_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->poolServersList);
}

void MainWindow::on_deleteDatapack_clicked()
{
    QMessageBox::StandardButton button=QMessageBox::question(this,tr("Are you sure?"),tr("Are you sure delete the datapack? This operation is not reversible."),QMessageBox::Yes|QMessageBox::No,QMessageBox::No);
    if(button!=QMessageBox::Yes)
        return;
    if(!CatchChallenger::FacilityLibGeneral::rmpath(datapackPathList[selectedDatapack].toStdString()))
        QMessageBox::warning(this,tr("Error"),tr("Remove the datapack path is not completed. Try after restarting the application"));
    on_manageDatapack_clicked();
}

void MainWindow::downloadFile()
{
    QNetworkRequest networkRequest(QString(CATCHCHALLENGER_SERVER_LIST_URL));
    networkRequest.setHeader(QNetworkRequest::UserAgentHeader,QStringLiteral("CatchChallenger Multi server %1").arg(CATCHCHALLENGER_VERSION));
    reply = qnam.get(networkRequest);
    connect(reply, &QNetworkReply::finished, this, &MainWindow::httpFinished);
    connect(reply, &QNetworkReply::metaDataChanged, this, &MainWindow::metaDataChanged);
    ui->warning->setVisible(true);
    ui->warning->setText(tr("Loading the server list..."));
    ui->server_refresh->setEnabled(false);
}

void MainWindow::on_server_refresh_clicked()
{
    if(reply!=NULL)
    {
        reply->abort();
        reply->deleteLater();
        reply=NULL;
    }
    downloadFile();
}

void MainWindow::metaDataChanged()
{
    if(!QFileInfo(QStandardPaths::writableLocation(QStandardPaths::DataLocation)+QStringLiteral("/server_list.xml")).isFile())
        return;
    QVariant val=reply->header(QNetworkRequest::LastModifiedHeader);
    if(!val.isValid())
        return;
    if(val.toDateTime()==QFileInfo(QStandardPaths::writableLocation(QStandardPaths::DataLocation)+QStringLiteral("/server_list.xml")).lastModified())
    {
        reply->abort();
        reply->deleteLater();
        ui->warning->setVisible(false);
        ui->server_refresh->setEnabled(true);
        reply=NULL;
        return;
    }
}

void MainWindow::httpFinished()
{
    ui->server_refresh->setEnabled(true);
    QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (reply->error())
    {
        ui->warning->setText(tr("Get server list failed: %1").arg(reply->errorString()));
        reply->deleteLater();
        reply=NULL;
        return;
    } else if (!redirectionTarget.isNull()) {
        ui->warning->setText(tr("Get server list redirection denied to: %1").arg(reply->errorString()));
        reply->deleteLater();
        reply=NULL;
        return;
    }
    ui->warning->setVisible(false);
    QByteArray content=reply->readAll();
    QFile cache(QStandardPaths::writableLocation(QStandardPaths::DataLocation)+QStringLiteral("/server_list.xml"));
    if(cache.open(QIODevice::ReadWrite))
    {
        cache.write(content);
        cache.resize(content.size());
        QVariant val=reply->header(QNetworkRequest::LastModifiedHeader);
        if(val.isValid())
        {
            #ifdef Q_CC_GNU
                //this function avalaible on unix and mingw
                utimbuf butime;
                butime.actime=val.toDateTime().toTime_t();
                butime.modtime=val.toDateTime().toTime_t();
                int returnVal=utime(cache.fileName().toLocal8Bit().data(),&butime);
                if(returnVal==0)
                    return;
                else
                {
                    qDebug() << QStringLiteral("Can't set time: %1").arg(cache.fileName());
                    return;
                }
            #else
                #error "Not supported on this platform"
            #endif
        }
        cache.close();
    }
    temp_xmlConnexionInfoList=loadXmlConnexionInfoList(content);
    mergedConnexionInfoList=temp_customConnexionInfoList+temp_xmlConnexionInfoList;
    qSort(mergedConnexionInfoList);
    displayServerList();
    reply->deleteLater();
    reply=NULL;
}

void MainWindow::on_multiplayer_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->poolServersList);
}

void MainWindow::on_server_back_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->mode);
}

void MainWindow::gameSolo_play(const QString &savegamesPath)
{
    resetAll();
    if(socket!=NULL)
    {
        socket->disconnectFromHost();
        socket->abort();
        delete socket;
        socket=NULL;
        realSslSocket=NULL;
    }
    socket=new CatchChallenger::ConnectedSocket(new CatchChallenger::QFakeSocket());
    CatchChallenger::Api_client_real::client=new CatchChallenger::Api_client_virtual(socket);//QCoreApplication::applicationDirPath()+QStringLiteral("/datapack/internal/")
    connect(CatchChallenger::Api_client_real::client,               &CatchChallenger::Api_protocol::protocol_is_good,   this,&MainWindow::protocol_is_good);
    connect(CatchChallenger::Api_client_real::client,               &CatchChallenger::Api_protocol::disconnected,       this,&MainWindow::disconnected);
    connect(CatchChallenger::Api_client_real::client,               &CatchChallenger::Api_protocol::message,            this,&MainWindow::message);
    connect(socket,                                                 &CatchChallenger::ConnectedSocket::stateChanged,    this,&MainWindow::stateChanged);
    CatchChallenger::BaseWindow::baseWindow->connectAllSignals();
    CatchChallenger::BaseWindow::baseWindow->setMultiPlayer(false);
    CatchChallenger::Api_client_real::client->setDatapackPath(QCoreApplication::applicationDirPath()+"/datapack/internal/");
    MapController::mapController->setDatapackPath(CatchChallenger::Api_client_real::client->datapackPathBase(),CatchChallenger::Api_client_real::client->mainDatapackCode());
    serverMode=ServerMode_Internal;
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
    if(internalServer!=NULL)
        delete internalServer;
    internalServer=new CatchChallenger::InternalServer();
    if(!sendSettings(internalServer,savegamesPath))
        return;
    connect(internalServer,&CatchChallenger::InternalServer::is_started,this,&MainWindow::is_started,Qt::QueuedConnection);
    connect(internalServer,&CatchChallenger::InternalServer::error,this,&MainWindow::serverError,Qt::QueuedConnection);
    internalServer->start();

    CatchChallenger::BaseWindow::baseWindow->serverIsLoading();
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
        if(internalServer!=NULL)
        {
            delete internalServer;
            internalServer=NULL;
        }
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
    if(serverMode!=ServerMode_Internal)
        return;
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

    formatedServerSettings.database_login.tryOpenType=CatchChallenger::DatabaseBase::DatabaseType::SQLite;
    formatedServerSettings.database_login.file=(savegamesPath+QStringLiteral("catchchallenger.db.sqlite")).toStdString();
    formatedServerSettings.database_base.tryOpenType=CatchChallenger::DatabaseBase::DatabaseType::SQLite;
    formatedServerSettings.database_base.file=(savegamesPath+QStringLiteral("catchchallenger.db.sqlite")).toStdString();
    formatedServerSettings.database_common.tryOpenType=CatchChallenger::DatabaseBase::DatabaseType::SQLite;
    formatedServerSettings.database_common.file=(savegamesPath+QStringLiteral("catchchallenger.db.sqlite")).toStdString();
    formatedServerSettings.database_server.tryOpenType=CatchChallenger::DatabaseBase::DatabaseType::SQLite;
    formatedServerSettings.database_server.file=(savegamesPath+QStringLiteral("catchchallenger.db.sqlite")).toStdString();
    formatedServerSettings.mapVisibility.mapVisibilityAlgorithm	= CatchChallenger::MapVisibilityAlgorithmSelection_None;
    formatedServerSettings.datapack_basePath=CatchChallenger::Api_client_real::client->datapackPathBase().toStdString();

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
    if(settings.contains("mainDatapackCode"))
        CommonSettingsServer::commonSettingsServer.mainDatapackCode=settings.value("mainDatapackCode","[main]").toString().toStdString();
    else
    {
        const QFileInfoList &list=QDir(CatchChallenger::Api_client_real::client->datapackPathBase()+"/map/main/").entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name);
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
        const QFileInfoList &list=QDir(CatchChallenger::Api_client_real::client->datapackPathBase()+"/map/main/"+QString::fromStdString(CommonSettingsServer::commonSettingsServer.mainDatapackCode)+"/sub/").entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name);
        if(!list.isEmpty())
        {
            settings.setValue("subDatapackCode",list.at(0).fileName());
            CommonSettingsServer::commonSettingsServer.subDatapackCode=list.at(0).fileName().toStdString();
        }
        else
            CommonSettingsServer::commonSettingsServer.subDatapackCode.clear();
    }

    internalServer->setSettings(formatedServerSettings);
    return true;
}

void MainWindow::gameSolo_back()
{
    ui->stackedWidget->setCurrentWidget(ui->mode);
}

void MainWindow::on_solo_clicked()
{
    int index=ui->stackedWidget->indexOf(solowindow);
    if(index>=0)
        ui->stackedWidget->setCurrentWidget(solowindow);
}

void MainWindow::on_languages_clicked()
{
    LanguagesSelect::languagesSelect->exec();
}

void MainWindow::newUpdate(const QString &version)
{
    ui->update->setText(InternetUpdater::getText(version));
    ui->update->setVisible(true);
}

void MainWindow::rssEntryList(const QList<RssNews::RssEntry> &entryList)
{
    if(entryList.isEmpty())
        return;
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
    if(serverConnexion.contains(selectedServer))
        lastServerWaitBeforeConnectAfterKick[serverConnexion.value(selectedServer)->host]=CommonSettingsServer::commonSettingsServer.waitBeforeConnectAfterKick;
}

void MainWindow::gameIsLoaded()
{
    if(vlcPlayer!=NULL)
        libvlc_media_player_stop(vlcPlayer);
}

void MainWindow::updateTheOkButton()
{
    if(selectedServer==NULL)
    {
        ui->pushButtonTryLogin->setEnabled(true);
        ui->pushButtonTryLogin->setText(tr("Ok"));
        return;
    }
    if(!serverConnexion.contains(selectedServer))
    {
        ui->pushButtonTryLogin->setEnabled(true);
        ui->pushButtonTryLogin->setText(tr("Ok"));
        return;
    }
    if(!lastServerWaitBeforeConnectAfterKick.contains(serverConnexion.value(selectedServer)->host))
    {
        ui->pushButtonTryLogin->setEnabled(true);
        ui->pushButtonTryLogin->setText(tr("Ok"));
        return;
    }
    if(!lastServerConnect.contains(serverConnexion.value(selectedServer)->host))
    {
        ui->pushButtonTryLogin->setEnabled(true);
        ui->pushButtonTryLogin->setText(tr("Ok"));
        return;
    }
    uint32_t timeToWait=5;
    if(lastServerIsKick.value(serverConnexion.value(selectedServer)->host))
        if(lastServerWaitBeforeConnectAfterKick.value(serverConnexion.value(selectedServer)->host)>timeToWait)
            timeToWait=lastServerWaitBeforeConnectAfterKick.value(serverConnexion.value(selectedServer)->host);
    uint32_t secondLastSinceConnexion=QDateTime::currentDateTime().toTime_t()-lastServerConnect.value(serverConnexion.value(selectedServer)->host).toTime_t();
    if(secondLastSinceConnexion>=timeToWait)
    {
        ui->pushButtonTryLogin->setEnabled(true);
        ui->pushButtonTryLogin->setText(tr("Ok"));
        return;
    }
    else
    {
        ui->pushButtonTryLogin->setEnabled(false);
        ui->pushButtonTryLogin->setText(tr("Ok (%1)").arg(timeToWait-secondLastSinceConnexion));
        return;
    }
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

void MainWindow::on_server_edit_clicked()
{
    if(selectedServer==NULL)
        return;
    if(!customServerConnexion.contains(selectedServer))
        return;
    int index=0;
    while(index<mergedConnexionInfoList.size())
    {
        ConnexionInfo * connexionInfo=serverConnexion.value(selectedServer);
        if(connexionInfo==&mergedConnexionInfoList.at(index))
        {
            AddOrEditServer editServer(this);
            editServer.setServer(connexionInfo->host);
            editServer.setPort(connexionInfo->port);
            editServer.setName(connexionInfo->name);
            editServer.setProxyServer(connexionInfo->proxyHost);
            editServer.setProxyPort(connexionInfo->proxyPort);
            editServer.exec();
            if(!editServer.isOk())
                return;
            if(!editServer.server().contains(QRegularExpression("^[a-zA-Z0-9\\.\\-_]+$")))
            {
                QMessageBox::warning(this,tr("Error"),tr("The host seam don't be a valid hostname or ip"));
                return;
            }
            if(customServerName.contains(editServer.name()) && editServer.name()!=connexionInfo->name)
            {
                QMessageBox::warning(this,tr("Error"),tr("The name is already taken"));
                return;
            }
            if(editServer.name()!=connexionInfo->name)
            {
                customServerName.remove(connexionInfo->name);
                customServerName << editServer.name();
            }

            connexionInfo->host=editServer.server();
            connexionInfo->name=editServer.name();
            connexionInfo->port=editServer.port();
            connexionInfo->proxyHost=editServer.proxyServer();
            connexionInfo->proxyPort=editServer.proxyPort();

            saveConnexionInfoList();
            displayServerList();
            break;
        }
        index++;
    }
}
