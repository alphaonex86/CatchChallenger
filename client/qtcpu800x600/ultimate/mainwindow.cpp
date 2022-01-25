#include "mainwindow.h"
#include "AddServer.h"
#include "ui_mainwindow.h"
#include "../base/InternetUpdater.h"
#include "../base/BlacklistPassword.h"
#include "../../../general/base/CommonSettingsCommon.hpp"
#include "../../../general/base/CommonSettingsServer.hpp"
#include "../../../general/base/CompressionProtocol.hpp"
#include "../base/Ultimate.h"
#include <QStandardPaths>
#include <QNetworkProxy>
#include <QCoreApplication>
#include <QSslKey>
#include <QInputDialog>
#include <QSettings>
#include <algorithm>
#include <string>
#include <iostream>
#include "AskKey.h"

#include "../../../general/base/GeneralVariable.hpp"
#include "../../../general/base/Version.hpp"
#include "../base/PlatformMacro.h"
#include "../../libcatchchallenger/ClientVariable.hpp"

#ifdef Q_CC_GNU
//this next header is needed to change file time/date under gcc
#include <utime.h>
#include <sys/stat.h>
#include <iostream>
#endif

#ifdef __linux__
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#endif

#include "../../qtmaprender/MapVisualiserPlayer.hpp"
#include "../../../general/base/FacilityLib.hpp"
#include "../../../general/base/FacilityLibGeneral.hpp"
#include "../../../general/base/CommonSettingsCommon.hpp"
#include "../base/LanguagesSelect.h"
#include "../../libqtcatchchallenger/Api_client_real.hpp"
#include "../../libqtcatchchallenger/Api_client_virtual.hpp"
#include "../base/SslCert.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    toQuit(false),
    ui(new Ui::MainWindow)
    #ifndef CATCHCHALLENGER_NOAUDIO
    ,buffer(&data)
    #endif
{
    addServer=nullptr;
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
    qRegisterMetaType<QList<FeedNews::FeedEntry> >("QList<FeedNews::FeedEntry>");

    socket=NULL;
    #ifndef NOTCPSOCKET
    realSslSocket=NULL;
    #endif
    #ifndef NOWEBSOCKET
    realWebSocket=NULL;
    #endif
    #ifndef NOSINGLEPLAYER
    internalServer=NULL;
    #endif
    client=NULL;
    reply=NULL;
    spacer=new QSpacerItem(0,0,QSizePolicy::Expanding,QSizePolicy::Expanding);
    spacerServer=new QSpacerItem(0,0,QSizePolicy::Expanding,QSizePolicy::Expanding);
    ui->setupUi(this);
    ui->update->setVisible(false);
    if(settings.contains("news"))
    {
        ui->news->setVisible(true);
        ui->news->setText(settings.value("news").toString());
    }
    else
        ui->news->setVisible(false);
    #ifndef __EMSCRIPTEN__
    InternetUpdater::internetUpdater=new InternetUpdater();
    if(!connect(InternetUpdater::internetUpdater,&InternetUpdater::newUpdate,this,&MainWindow::newUpdate))
        abort();
    #endif
    FeedNews::feedNews=new FeedNews();
    if(!connect(FeedNews::feedNews,&FeedNews::feedEntryList,this,&MainWindow::feedEntryList))
        qDebug() << "connect(RssNews::rssNews,&RssNews::rssEntryList,this,&MainWindow::rssEntryList) failed";
    #ifndef NOSINGLEPLAYER
    solowindow=new SoloWindow(this,QCoreApplication::applicationDirPath().toStdString()+
                              "/datapack/internal/",
                              QStandardPaths::writableLocation(QStandardPaths::DataLocation).toStdString()+
                              "/savegames/",false);
    if(!connect(solowindow,&SoloWindow::back,this,&MainWindow::gameSolo_back))
        abort();
    if(!connect(solowindow,&SoloWindow::play,this,&MainWindow::gameSolo_play))
        abort();
    ui->stackedWidget->addWidget(solowindow);
    if(ui->stackedWidget->indexOf(solowindow)<0)
        ui->solo->hide();
    #endif
    #ifdef NOSINGLEPLAYER
    ui->solo->hide();
    #else
    //work around QSS crash
    solowindow->setBuggyStyle();
    #endif
    ui->stackedWidget->setCurrentWidget(ui->mode);
    ui->warning->setVisible(false);
    ui->server_refresh->setEnabled(true);
    temp_xmlConnexionInfoList=loadXmlConnexionInfoList();
    temp_customConnexionInfoList=loadConfigConnexionInfoList();
    mergedConnexionInfoList=temp_customConnexionInfoList;
    mergedConnexionInfoList.insert(mergedConnexionInfoList.end(),temp_xmlConnexionInfoList.begin(),temp_xmlConnexionInfoList.end());
    std::sort(mergedConnexionInfoList.begin(),mergedConnexionInfoList.end());
    selectedServer=NULL;
    displayServerList();
    baseWindow=new CatchChallenger::BaseWindow();
    ui->stackedWidget->addWidget(baseWindow);
    /*socket==NULL here if(!connect(socket,static_cast<void(CatchChallenger::ConnectedSocket::*)(QAbstractSocket::SocketError)>(&CatchChallenger::ConnectedSocket::error),this,&MainWindow::error,Qt::QueuedConnection))
        abort();*/
    if(!connect(baseWindow,&CatchChallenger::BaseWindow::newError,this,&MainWindow::newError,Qt::QueuedConnection))
        abort();
    //connect(baseWindow,                &CatchChallenger::BaseWindow::needQuit,             this,&MainWindow::needQuit);
    if(!connect(&updateTheOkButtonTimer,&QTimer::timeout,this,&MainWindow::updateTheOkButton))
        abort();

    stopFlood.setSingleShot(false);
    stopFlood.start(1500);
    updateTheOkButtonTimer.setSingleShot(false);
    updateTheOkButtonTimer.start(1000);
    numberForFlood=0;
    haveShowDisconnectionReason=false;
    completer=NULL;

    stateChanged(QAbstractSocket::UnconnectedState);

    setWindowTitle(QStringLiteral("CatchChallenger Ultimate"));
    downloadFile();

    #ifndef CATCHCHALLENGER_NOAUDIO
    // Create a new Media
    {
        player=NULL;
        const std::string &soundFile=QCoreApplication::applicationDirPath().toStdString()+CATCHCHALLENGER_CLIENT_MUSIC_LOADING;
        struct stat sb;
        if(stat(soundFile.c_str(),&sb)==0)
        {
            player = new QAudioOutput(Audio::audio.format(), this);
            if(Audio::decodeOpus(soundFile,data))
            {
                buffer.open(QBuffer::ReadOnly);
                buffer.seek(0);
                player->start(&buffer);

                Audio::audio.addPlayer(player);
            }
            else
            {
                delete player;
                player=NULL;
            }
        }
    }
    #endif
    if(!connect(baseWindow,&CatchChallenger::BaseWindow::gameIsLoaded,this,&MainWindow::gameIsLoaded))
        abort();
    #ifdef CATCHCHALLENGER_GITCOMMIT
    ui->version->setText(QString::fromStdString(CatchChallenger::Version::str)+QStringLiteral(" - ")+QStringLiteral(CATCHCHALLENGER_GITCOMMIT));
    #else
    ui->version->setText(QString::fromStdString(CatchChallenger::Version::str));
    #endif

    QSettings keySettings;
    if(keySettings.contains(QStringLiteral("key")))
    {
        if(Ultimate::ultimate.setKey(keySettings.value("key").toString().toStdString()))
            ui->UltimateKey->hide();
    }
}

MainWindow::~MainWindow()
{
    #ifndef CATCHCHALLENGER_NOAUDIO
    if(player!=NULL)
    {
        player->stop();
        buffer.close();
        Audio::audio.removePlayer(player);
        delete player;
        player=NULL;
    }
    #endif
    if(baseWindow!=NULL)
    {
        baseWindow->deleteLater();
        baseWindow=NULL;
    }
    if(completer!=NULL)
    {
        delete completer;
        completer=NULL;
    }
    if(addServer!=nullptr)
    {
        delete addServer;
        addServer=nullptr;
    }
    if(client!=NULL)
    {
        delete client;
        client=NULL;
    }
    /*if(socket!=NULL)
    {
        socket->disconnectFromHost();
        socket->abort();
        delete socket;
        socket=NULL;
    }*/
    delete ui;
}

//MS-Windows don't support show modal input into the contructor, then move out it
bool MainWindow::askForUltimateCopy()
{
    //after all to prevent not initialised pointer
    AskKey askKey(this);
    askKey.exec();
    QString key=askKey.key();
    if(key.isEmpty())
    {
        //Windows crash: QCoreApplication::quit();->do crash under windows
        std::cout << "key.isEmpty() for ultimate version: " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
        return false;
    }
    {
        if(Ultimate::ultimate.setKey(key.toStdString()))
        {
            ui->UltimateKey->hide();
            QSettings keySettings;
            keySettings.setValue("key",key);
        }
        else
            QMessageBox::critical(this,tr("Error"),tr("The key is wrong"));
    }
    return false;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    #ifndef CATCHCHALLENGER_NOAUDIO
    if(player!=NULL)
    {
        buffer.close();
        player->stop();
        delete player;
        player=NULL;
    }
    #endif
    event->ignore();
    hide();
    if(socket!=NULL
    #ifndef NOSINGLEPLAYER
            || internalServer!=NULL
    #endif
            )
    {
        #ifndef NOSINGLEPLAYER
        if(internalServer!=NULL)
        {
            if(internalServer->isListen())
                internalServer->stop();
            else
                QCoreApplication::quit();
        }
        else
            QCoreApplication::quit();
        #endif
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

std::vector<ConnexionInfo> MainWindow::loadConfigConnexionInfoList()
{
    std::vector<ConnexionInfo> returnedVar;
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
    const QRegularExpression regexConnexion(QStringLiteral("^[a-zA-Z0-9\\.\\-_:]+:[0-9]{1,5}$"));
    const QRegularExpression hostRemove(QStringLiteral(":[0-9]{1,5}$"));
    const QRegularExpression postRemove("^.*:");
    while(index<connexionList.size())
    {
        QString connexion=connexionList.at(index);
        QString name=nameList.at(index);
        QString connexionCounter=connexionCounterList.at(index);
        QString lastConnexion=lastConnexionList.at(index);
        QString proxy=proxyList.at(index);
        if(connexion.contains(regexConnexion) || connexion.startsWith("ws://") || connexion.startsWith("wss://"))
        {
            bool ok=true;
            #ifndef NOTCPSOCKET
            QString host=connexion;
            uint16_t port=0;
            #endif
            if(connexion.startsWith("ws://") || connexion.startsWith("wss://"))
            {}
            else
            {
                #ifndef NOTCPSOCKET
                host=connexion;
                host.remove(hostRemove);
                QString port_string=connexion;
                port_string.remove(postRemove);
                port=static_cast<uint16_t>(port_string.toInt(&ok));
                if(!ok)
                    qDebug() << "dropped connexion, port wrong: " << port_string;
                #else
                ok=false;
                #endif
            }
            if(ok)
            {
                ConnexionInfo connexionInfo;
                if(connexion.startsWith("ws://") || connexion.startsWith("wss://"))
                {
                    #ifndef NOWEBSOCKET
                    connexionInfo.ws=connexion;
                    #endif
                }
                else
                {
                    #ifndef NOTCPSOCKET
                    connexionInfo.host=host;
                    connexionInfo.port=port;
                    #endif
                }
                connexionInfo.name=name;
                while(customServerName.contains(connexionInfo.name))
                    connexionInfo.name=tr("Copy of %1").arg(connexionInfo.name);
                customServerName << name;
                connexionInfo.connexionCounter=connexionCounter.toUInt(&ok);
                if(!ok)
                {
                    qDebug() << "ignored bug for connexion : " << connexion << " connexionCounter";
                    connexionInfo.connexionCounter=0;
                }
                connexionInfo.lastConnexion=lastConnexion.toUInt(&ok);
                if(!ok)
                {
                    qDebug() << "ignored bug for connexion : " << connexion << " lastConnexion";
                    connexionInfo.lastConnexion=static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch()/1000);
                }
                if(connexionInfo.lastConnexion>(QDateTime::currentMSecsSinceEpoch()/1000))
                {
                    qDebug() << "ignored bug for connexion : " << connexion << " lastConnexion<time()";
                    connexionInfo.lastConnexion=static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch()/1000);
                }
                if(proxy.contains(regexConnexion))
                {
                    QString host=proxy;
                    host.remove(hostRemove);
                    QString proxy_port_string=proxy;
                    proxy_port_string.remove(postRemove);
                    bool ok;
                    uint16_t proxy_port=static_cast<uint16_t>(proxy_port_string.toInt(&ok));
                    if(ok)
                    {
                        connexionInfo.proxyHost=host;
                        connexionInfo.proxyPort=proxy_port;
                    }
                }
                returnedVar.push_back(connexionInfo);
            }
        }
        else
            qDebug() << "dropped connexion, info seam wrong: " << connexion;
        index++;
    }
    return returnedVar;
}

std::vector<ConnexionInfo> MainWindow::loadXmlConnexionInfoList()
{
    if(QFileInfo(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).isDir())
        if(QFileInfo(QStandardPaths::writableLocation(QStandardPaths::DataLocation)+QStringLiteral("/server_list.xml")).isFile())
            return loadXmlConnexionInfoList(QStandardPaths::writableLocation(QStandardPaths::DataLocation)+QStringLiteral("/server_list.xml"));
    return loadXmlConnexionInfoList(QStringLiteral(":/other/default_server_list.xml"));
}

std::vector<ConnexionInfo> MainWindow::loadXmlConnexionInfoList(const QByteArray &xmlContent)
{
    std::vector<ConnexionInfo> returnedVar;
    tinyxml2::XMLDocument domDocument;
    const auto loadOkay = domDocument.Parse(xmlContent.data(),xmlContent.size());
    if(loadOkay!=0)
    {
        std::cerr << "MainWindow::loadXmlConnexionInfoList, " << domDocument.ErrorName() << std::endl;
        return returnedVar;
    }

    const tinyxml2::XMLElement *root = domDocument.RootElement();
    if(root==NULL)
    {
        std::cerr << "Unable to open the file: MainWindow::loadXmlConnexionInfoList, no root balise found for the xml file" << std::endl;
        return returnedVar;
    }
    if(root->Name()==NULL)
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, \"servers\" root balise not found 2 for the xml file").arg("server_list.xml");
        return returnedVar;
    }
    if(strcmp(root->Name(),"servers")!=0)
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, \"servers\" root balise not found for the xml file").arg("server_list.xml");
        return returnedVar;
    }

    QRegularExpression regexHost("^[a-zA-Z0-9\\.\\-_]+$");
    bool ok;
    //load the content
    const tinyxml2::XMLElement *server = root->FirstChildElement("server");
    while(server!=NULL)
    {
        if(server->Attribute("unique_code")!=NULL)
        {
            ConnexionInfo connexionInfo;
            connexionInfo.unique_code=server->Attribute("unique_code");

            if(server->Attribute("host")!=NULL)
                connexionInfo.host=server->Attribute("host");
            if(server->Attribute("port")!=NULL)
            {
                uint32_t temp_port=stringtouint32(server->Attribute("port"),&ok);
                if(!connexionInfo.host.contains(regexHost))
                    qDebug() << QStringLiteral("Unable to open the file: %1, host is wrong: %3 child->Name(): %2")
                                .arg("server_list.xml").arg(server->Name()).arg(connexionInfo.host);
                else if(!ok)
                    qDebug() << QStringLiteral("Unable to open the file: %1, port is not a number: %3 child->Name(): %2")
                                .arg("server_list.xml").arg(server->Name()).arg(server->Attribute("port"));
                else if(temp_port<1 || temp_port>65535)
                    qDebug() << QStringLiteral("Unable to open the file: %1, port is not in range: %3 child->Name(): %2")
                                .arg("server_list.xml").arg(server->Name()).arg(server->Attribute("port"));
                else if(connexionInfo.unique_code.isEmpty())
                    qDebug() << QStringLiteral("Unable to open the file: %1, unique_code can't be empty: %3 child->Name(): %2")
                                .arg("server_list.xml").arg(server->Name()).arg(server->Attribute("port"));
                else
                    connexionInfo.port=static_cast<uint16_t>(temp_port);
            }
            if(server->Attribute("ws")!=NULL)
                connexionInfo.ws=server->Attribute("ws");
            const tinyxml2::XMLElement *lang;
            const std::string &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
            bool found=false;
            if(!language.empty() && language!="en")
            {
                lang = server->FirstChildElement("lang");
                while(lang!=NULL)
                {
                    if(lang->Attribute("lang") && lang->Attribute("lang")==language)
                    {
                        const tinyxml2::XMLElement *name = lang->FirstChildElement("name");
                        if(name!=NULL && name->GetText()!=NULL)
                            connexionInfo.name=name->GetText();
                        const tinyxml2::XMLElement *register_page = lang->FirstChildElement("register_page");
                        if(register_page!=NULL && register_page->GetText()!=NULL)
                            connexionInfo.register_page=register_page->GetText();
                        const tinyxml2::XMLElement *lost_passwd_page = lang->FirstChildElement("lost_passwd_page");
                        if(lost_passwd_page!=NULL && lost_passwd_page->GetText()!=NULL)
                            connexionInfo.lost_passwd_page=lost_passwd_page->GetText();
                        const tinyxml2::XMLElement *site_page = lang->FirstChildElement("site_page");
                        if(site_page!=NULL && site_page->GetText()!=NULL)
                            connexionInfo.site_page=site_page->GetText();
                        found=true;
                        break;
                    }
                    lang = lang->NextSiblingElement("lang");
                }
            }
            if(!found)
            {
                lang = server->FirstChildElement("lang");
                while(lang!=NULL)
                {
                    if(lang->Attribute("lang")==NULL || strcmp(lang->Attribute("lang"),"en")==0)
                    {
                        const tinyxml2::XMLElement *name = lang->FirstChildElement("name");
                        if(name!=NULL && name->GetText()!=NULL)
                            connexionInfo.name=name->GetText();
                        const tinyxml2::XMLElement *register_page = lang->FirstChildElement("register_page");
                        if(register_page!=NULL && register_page->GetText()!=NULL)
                            connexionInfo.register_page=register_page->GetText();
                        const tinyxml2::XMLElement *lost_passwd_page = lang->FirstChildElement("lost_passwd_page");
                        if(lost_passwd_page!=NULL && lost_passwd_page->GetText()!=NULL)
                            connexionInfo.lost_passwd_page=lost_passwd_page->GetText();
                        const tinyxml2::XMLElement *site_page = lang->FirstChildElement("site_page");
                        if(site_page!=NULL && site_page->GetText()!=NULL)
                            connexionInfo.site_page=site_page->GetText();
                        break;
                    }
                    lang = lang->NextSiblingElement("lang");
                }
            }
            settings.beginGroup(QStringLiteral("Xml-%1").arg(server->Attribute("unique_code")));
            if(settings.contains(QStringLiteral("connexionCounter")))
            {
                connexionInfo.connexionCounter=settings.value("connexionCounter").toUInt(&ok);
                if(!ok)
                    connexionInfo.connexionCounter=0;
            }
            else
                connexionInfo.connexionCounter=0;

            //proxy
            if(settings.contains(QStringLiteral("proxyPort")))
            {
                connexionInfo.proxyPort=static_cast<uint16_t>(settings.value("proxyPort").toUInt(&ok));
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
                    connexionInfo.lastConnexion=static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch()/1000);
            }
            else
                connexionInfo.lastConnexion=static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch()/1000);
            settings.endGroup();
            if(connexionInfo.lastConnexion>(QDateTime::currentMSecsSinceEpoch()/1000))
                connexionInfo.lastConnexion=static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch()/1000);
            returnedVar.push_back(connexionInfo);
        }
        else
            qDebug() << QStringLiteral("Unable to open the file: %1, missing host or port: child->Name(): %2")
                        .arg("server_list.xml").arg(server->Name());
        server = server->NextSiblingElement("server");
    }
    return returnedVar;
}

std::vector<ConnexionInfo> MainWindow::loadXmlConnexionInfoList(const QString &file)
{
    std::vector<ConnexionInfo> returnedVar;
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
        ConnexionInfo * connexionInfo=serverConnexion.value(selectedServer);
        if(connexionInfo->unique_code.isEmpty())
        {
            if(connexionInfo->proxyHost.isEmpty())
            {
                if(!connexionInfo->host.isEmpty())
                    unique_code=QString("%1:%2")
                        .arg(connexionInfo->host)
                        .arg(connexionInfo->port)
                        ;
                else
                    unique_code=QString(connexionInfo->ws.toUtf8().toHex());
            }
            else
            {
                if(!connexionInfo->host.isEmpty())
                    unique_code=QString("%1:%2:%3:%4")
                        .arg(connexionInfo->host)
                        .arg(connexionInfo->port)
                        .arg(connexionInfo->proxyHost)
                        .arg(connexionInfo->proxyPort)
                        ;
                else
                    unique_code=QString("%1:%2:%3")
                        .arg(QString(connexionInfo->ws.toUtf8().toHex()))
                        .arg(connexionInfo->proxyHost)
                        .arg(connexionInfo->proxyPort)
                        ;
            }
        }
        else
            unique_code=connexionInfo->unique_code;
    }
    const ListEntryEnvolued * tempSelectedServer=selectedServer;
    selectedServer=NULL;
    unsigned int index=0;
    while(!server.empty())
    {
        delete server.at(0);
        server.erase(server.begin());
        index++;
    }
    serverConnexion.clear();
    if(mergedConnexionInfoList.empty())
        ui->serverEmpty->setText(QStringLiteral("<html><body><p align=\"center\"><span style=\"font-size:12pt;color:#a0a0a0;\">%1</span></p></body></html>").arg(tr("Empty")));
    #if defined(NOTCPSOCKET) && defined(NOWEBSOCKET)
    #error Web socket and tcp socket are both not supported
    return;
    #endif
    index=0;
    std::cout << "display mergedConnexionInfoList.size(): " << mergedConnexionInfoList.size() << std::endl;
    while(index<mergedConnexionInfoList.size())
    {
        ListEntryEnvolued *newEntry=new ListEntryEnvolued();
        if(!connect(newEntry,&ListEntryEnvolued::clicked,this,&MainWindow::serverListEntryEnvoluedClicked,Qt::QueuedConnection))
            abort();
        if(!connect(newEntry,&ListEntryEnvolued::doubleClicked,this,&MainWindow::serverListEntryEnvoluedDoubleClicked,Qt::QueuedConnection))
            abort();
        const ConnexionInfo &connexionInfo=mergedConnexionInfoList.at(index);
        QString connexionInfoHost;
        #ifdef NOTCPSOCKET
        connexionInfoHost=connexionInfo.ws;
        #else
            #ifdef NOWEBSOCKET
            connexionInfoHost=connexionInfo.host;
            #else
            if(!connexionInfo.host.isEmpty())
                connexionInfoHost=connexionInfo.host;
            else
                connexionInfoHost=connexionInfo.ws;
            #endif
        #endif
        if(connexionInfoHost.isEmpty())
        {
            index++;
            continue;
        }
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
            QString tempUniqueCode;
            if(connexionInfo.proxyHost.isEmpty())
            {
                #ifndef NOTCPSOCKET
                if(!connexionInfo.host.isEmpty())
                    tempUniqueCode=QString("%1:%2")
                        .arg(connexionInfo.host)
                        .arg(connexionInfo.port)
                        ;
                else
                #endif
                    tempUniqueCode=QString(connexionInfo.ws.toUtf8().toHex());
            }
            else
            {
                #ifndef NOTCPSOCKET
                if(!connexionInfo.host.isEmpty())
                    tempUniqueCode=QString("%1:%2:%3:%4")
                        .arg(connexionInfo.host)
                        .arg(connexionInfo.port)
                        .arg(connexionInfo.proxyHost)
                        .arg(connexionInfo.proxyPort)
                        ;
                else
                #endif
                    tempUniqueCode=QString("%1:%2:%3")
                        .arg(QString(connexionInfo.ws.toUtf8().toHex()))
                        .arg(connexionInfo.proxyHost)
                        .arg(connexionInfo.proxyPort)
                        ;
            }
            if(unique_code==tempUniqueCode)
                selectedServer=newEntry;
        }
        else
        {
            if(unique_code==connexionInfo.unique_code)
                selectedServer=newEntry;
        }
        QString stringPort;
        #ifndef NOTCPSOCKET
        if(!connexionInfo.host.isEmpty())
            stringPort=":"+QString::number(connexionInfo.port);
        #endif
        if(connexionInfo.name.isEmpty())
        {
            #ifndef NOTCPSOCKET
            if(!connexionInfo.host.isEmpty())
                name=QStringLiteral("%1:%2").arg(connexionInfoHost).arg(connexionInfo.port);
            else
            #endif
                name=connexionInfoHost;
            newEntry->setText(QStringLiteral("%3<span style=\"font-size:12pt;font-weight:600;\">%1%2</span><br/><span style=\"color:#909090;\">%4%5</span>")
                              .arg(connexionInfoHost)
                              .arg(stringPort)
                              .arg(star)
                              .arg(lastConnexion)
                              .arg(custom)
                              );
        }
        else
        {
            name=connexionInfo.name;
            newEntry->setText(QStringLiteral("%4<span style=\"font-size:12pt;font-weight:600;\">%1</span><br/><span style=\"color:#909090;\">%2%3 %5%6</span>")
                              .arg(connexionInfo.name)
                              .arg(connexionInfoHost)
                              .arg(stringPort)
                              .arg(star)
                              .arg(lastConnexion)
                              .arg(custom)
                              );
        }
        newEntry->setStyleSheet(QStringLiteral("QLabel::hover{border:1px solid #bbb;background-color:rgb(180,180,180,100);border-radius:10px;}"));

        ui->scrollAreaWidgetContentsServer->layout()->addWidget(newEntry);

        server.push_back(newEntry);
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
    if(tempSelectedServer!=NULL && selectedServer==NULL)
    {
        qDebug() << "tempSelectedServer!=NULL && selectedServer==NULL, fix it!";
        abort();
    }
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
    unsigned int index=0;
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
    if(addServer!=nullptr)
        delete addServer;
    addServer=new AddOrEditServer(this);
    if(!connect(addServer,&QDialog::accepted,this,&MainWindow::server_add_finished))
        abort();
    if(!connect(addServer,&QDialog::rejected,this,&MainWindow::server_add_finished))
        abort();
    addServer->show();
}

void MainWindow::server_add_finished()
{
    if(addServer==nullptr)
        return;
    if(!addServer->isOk())
        return;
    if(addServer->type()==0)
    {
        if(!addServer->server().contains(QRegularExpression("^[a-zA-Z0-9\\.:\\-_]+$")))
        {
            QMessageBox::warning(this,tr("Error"),tr("The host seam don't be a valid hostname or ip"));
            return;
        }
    }
    else
    {
        if(!addServer->server().startsWith("ws://") && !addServer->server().startsWith("wss://"))
        {
            QMessageBox::warning(this,tr("Error"),tr("The web socket url seam wrong, not start with ws:// or wss://"));
            return;
        }
    }
    if(customServerName.contains(addServer->name()))
    {
        QMessageBox::warning(this,tr("Error"),tr("The name is already taken"));
        return;
    }
    #ifdef __EMSCRIPTEN__
    std::cerr << "AddOrEditServer returned" <<  std::endl;
    #endif
    ConnexionInfo connexionInfo;
    connexionInfo.connexionCounter=0;
    connexionInfo.lastConnexion=static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch()/1000);

    connexionInfo.name=addServer->name();

    if(addServer->type()==0)
    {
        connexionInfo.port=addServer->port();
        #ifndef NOTCPSOCKET
        connexionInfo.host=addServer->server();
        #endif
        connexionInfo.ws.clear();
    }
    else
    {
        connexionInfo.port=0;
        connexionInfo.host.clear();
        #ifndef NOWEBSOCKET
        connexionInfo.ws=addServer->server();
        #endif
    }

    connexionInfo.proxyHost=addServer->proxyServer();
    connexionInfo.proxyPort=addServer->proxyPort();
    mergedConnexionInfoList.push_back(connexionInfo);
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
    ConnexionInfo * connexionInfo=serverConnexion[selectedServer];
    if(connexionInfo->host.isEmpty())
        settings.beginGroup(QString(connexionInfo->ws.toUtf8().toHex()));
    else if(customServerConnexion.contains(selectedServer))
        settings.beginGroup(QStringLiteral("%1-%2").arg(connexionInfo->host).arg(connexionInfo->port));
    else
        settings.beginGroup(QStringLiteral("Xml-%1").arg(connexionInfo->unique_code));
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
        {
            if(completer!=NULL)
            {
                delete completer;
                completer=NULL;
            }
            completer = new QCompleter(loginList);
            completer->setCaseSensitivity(Qt::CaseInsensitive);
            ui->lineEditLogin->setCompleter(completer);

            ui->lineEditLogin->setText(loginList.first());
        }
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
    unsigned int index=0;
    while(index<mergedConnexionInfoList.size())
    {
        ConnexionInfo * connexionInfo=serverConnexion[selectedServer];
        if(connexionInfo==&mergedConnexionInfoList.at(index))
        {
            customServerName.remove(connexionInfo->name);
            mergedConnexionInfoList.erase(mergedConnexionInfoList.begin()+index);
            if(customServerConnexion.contains(selectedServer))
                saveConnexionInfoList();
            else
            {
                settings.beginGroup(QStringLiteral("Xml-%1").arg(connexionInfo->unique_code));
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
    unsigned int index=0;
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
            if(!connexionInfo.host.isEmpty())
                connexionList << QStringLiteral("%1:%2").arg(connexionInfo.host).arg(connexionInfo.port);
            else
                connexionList << connexionInfo.ws;
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
    settings.sync();
}

void MainWindow::serverListEntryEnvoluedDoubleClicked()
{
    on_server_select_clicked();
}

void MainWindow::resetAll()
{
    #ifndef CATCHCHALLENGER_NOAUDIO
    if(player!=NULL)
        player->start();
    #endif
    if(client!=NULL)
    {
        client->resetAll();
        static_cast<CatchChallenger::Api_protocol_Qt *>(client)->deleteLater();
        client=NULL;
    }
    if(completer!=NULL)
    {
        delete completer;
        completer=NULL;
    }
    if(baseWindow!=NULL)
        baseWindow->resetAll();
    setWindowTitle(QStringLiteral("CatchChallenger Ultimate"));
    switch(serverMode)
    {
        case ServerMode_Internal:
        #ifndef NOSINGLEPLAYER
            ui->stackedWidget->setCurrentWidget(solowindow);
            /* do just at game starting
             *if(internalServer!=NULL)
            {
                internalServer->stop();
                internalServer->deleteLater();
                internalServer=NULL;
            }*/
            saveTime();
        #endif
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
    lastServer.clear();
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

#ifndef __EMSCRIPTEN__
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
#endif

void MainWindow::disconnected(std::string reason)
{
    QMessageBox::information(this,tr("Disconnected"),tr("Disconnected by the reason: %1").arg(QString::fromStdString(reason)));
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
    if(selectedServer==NULL)
    {
        qDebug() << "selectedServer==NULL, fix it!";
        abort();
    }
    if(!serverConnexion.contains(selectedServer))
    {
        qDebug() << "!serverConnexion.contains(selectedServer)";
        abort();
    }
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
    serverMode=ServerMode_Remote;
    ConnexionInfo * const selectedServerConnexion=serverConnexion.value(selectedServer);
    if(selectedServerConnexion->host.isEmpty())
    {
        lastServerConnect[selectedServerConnexion->host]=QDateTime::currentDateTime();
        lastServerIsKick[selectedServerConnexion->host]=false;
    }
    if(customServerConnexion.contains(selectedServer))
    {
        if(selectedServerConnexion->host.isEmpty())
            settings.beginGroup(QString(selectedServerConnexion->ws.toUtf8().toHex()));
        else
            settings.beginGroup(QStringLiteral("%1-%2").arg(selectedServerConnexion->host).arg(selectedServerConnexion->port));
    }
    else
        settings.beginGroup(QStringLiteral("Xml-%1").arg(selectedServerConnexion->unique_code));

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
    if(completer!=NULL)
    {
        delete completer;
        completer=NULL;
    }

    settings.endGroup();
    settings.sync();
    if(socket!=NULL)
    {
        socket->disconnectFromHost();
        socket->abort();
        delete socket;
        socket=NULL;
        #ifndef NOTCPSOCKET
        realSslSocket=nullptr;
        #endif
        #ifndef NOWEBSOCKET
        realWebSocket=nullptr;
        #endif
    }
    #ifndef NOTCPSOCKET
    if(!selectedServerConnexion->host.isEmpty())
    {
        realSslSocket=new QSslSocket();
        socket=new CatchChallenger::ConnectedSocket(realSslSocket);
    }
    #endif
    #ifndef NOWEBSOCKET
    else if(!selectedServerConnexion->ws.isEmpty())
    {
        realWebSocket=new QWebSocket();
        socket=new CatchChallenger::ConnectedSocket(realWebSocket);
    }
    #endif
    CatchChallenger::Api_client_real *client=new CatchChallenger::Api_client_real(socket);
    this->client=client;

    if(!connect(client,               &CatchChallenger::Api_client_real::Qtprotocol_is_good,   this,&MainWindow::protocol_is_good,Qt::QueuedConnection))
        abort();
    if(!connect(client,               &CatchChallenger::Api_client_real::Qtdisconnected,       this,&MainWindow::disconnected))
        abort();
    //connect(client,               &CatchChallenger::Api_protocol::Qtmessage,            this,&MainWindow::message,Qt::QueuedConnection);
    if(!connect(client,               &CatchChallenger::Api_client_real::Qtlogged,             this,&MainWindow::logged,Qt::QueuedConnection))
        abort();

    if(!selectedServerConnexion->proxyHost.isEmpty())
    {
        QNetworkProxy proxy;
        #ifndef NOTCPSOCKET
        if(realSslSocket!=nullptr)
            proxy=realSslSocket->proxy();
        #endif
        #ifndef NOWEBSOCKET
        if(realWebSocket!=nullptr)
            proxy=realWebSocket->proxy();
        #endif
        proxy.setType(QNetworkProxy::Socks5Proxy);
        proxy.setHostName(selectedServerConnexion->proxyHost);
        proxy.setPort(selectedServerConnexion->proxyPort);
        #ifndef NOTCPSOCKET
        if(realSslSocket!=nullptr)
            realSslSocket->setProxy(proxy);
        #endif
        #ifndef NOWEBSOCKET
        if(realWebSocket!=nullptr)
            realWebSocket->setProxy(proxy);
        #endif
    }
    ui->stackedWidget->setCurrentWidget(baseWindow);
    baseWindow->setMultiPlayer(true,static_cast<CatchChallenger::Api_client_real *>(client));
    baseWindow->stateChanged(QAbstractSocket::ConnectingState);
    #ifndef NOTCPSOCKET
    if(realSslSocket!=nullptr)
    {
        if(!connect(realSslSocket,static_cast<void(QSslSocket::*)(const QList<QSslError> &errors)>(&QSslSocket::sslErrors),  this,&MainWindow::sslErrors,Qt::QueuedConnection))
            abort();
        if(!connect(realSslSocket,&QSslSocket::stateChanged,    this,&MainWindow::stateChanged,Qt::DirectConnection))
            abort();
        if(!connect(realSslSocket,static_cast<void(QSslSocket::*)(QAbstractSocket::SocketError)>(&QSslSocket::error),           this,&MainWindow::error,Qt::QueuedConnection))
            abort();

        lastServer=selectedServerConnexion->host+":"+QString::number(selectedServerConnexion->port);
        socket->connectToHost(selectedServerConnexion->host,selectedServerConnexion->port);
    }
    #endif
    #ifndef NOWEBSOCKET
    if(realWebSocket!=nullptr)
    {
        if(!connect(realWebSocket,&QWebSocket::stateChanged,    this,&MainWindow::stateChanged,Qt::DirectConnection))
            abort();
        if(!connect(realWebSocket,static_cast<void(QWebSocket::*)(QAbstractSocket::SocketError)>(&QWebSocket::error),           this,&MainWindow::error,Qt::QueuedConnection))
            abort();

        lastServer=selectedServerConnexion->ws;
        QUrl url{QString(selectedServerConnexion->ws)};
        QNetworkRequest request{url};
        request.setRawHeader("Sec-WebSocket-Protocol", "binary");
        realWebSocket->open(request);
    }
    #endif
    selectedServerConnexion->connexionCounter++;
    selectedServerConnexion->lastConnexion=static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch()/1000);
    saveConnexionInfoList();
    connectTheExternalSocket();
    displayServerList();//need be after connectTheExternalSocket() because it reset selectedServer
}

void MainWindow::connectTheExternalSocket()
{
    if(selectedServer==NULL)
    {
        qDebug() << "selectedServer==NULL, fix it!";
        abort();
    }
    if(!serverConnexion.contains(selectedServer))
    {
        qDebug() << "!serverConnexion.contains(selectedServer)";
        abort();
    }
    //continue the normal procedure
    if(!serverConnexion.value(selectedServer)->proxyHost.isEmpty())
    {
        QNetworkProxy proxy;
        #ifndef NOTCPSOCKET
        if(realSslSocket!=nullptr)
            proxy=realSslSocket->proxy();
        #endif
        #ifndef NOWEBSOCKET
        if(realWebSocket!=nullptr)
            proxy=realWebSocket->proxy();
        #endif
        proxy.setType(QNetworkProxy::Socks5Proxy);
        proxy.setHostName(serverConnexion.value(selectedServer)->proxyHost);
        proxy.setPort(serverConnexion.value(selectedServer)->proxyPort);
        static_cast<CatchChallenger::Api_client_real *>(client)->setProxy(proxy);
    }

    baseWindow->connectAllSignals();
    baseWindow->setMultiPlayer(true,static_cast<CatchChallenger::Api_client_real *>(client));
    QDir datapack(serverToDatapachPath(selectedServer));
    if(!datapack.exists())
        if(!datapack.mkpath(datapack.absolutePath()))
        {
            disconnected(tr("Not able to create the folder %1").arg(datapack.absolutePath()).toStdString());
            return;
        }
    client->setDatapackPath(datapack.absolutePath().toStdString());
    baseWindow->stateChanged(QAbstractSocket::ConnectedState);
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
    std::cout << "MainWindow::stateChanged(" << std::to_string((int)socketState) << ")" << std::endl;
    if(socketState==QAbstractSocket::ConnectedState)
    {
        //If comment: Internal problem: Api_protocol::sendProtocol() !haveFirstHeader
        #if !defined(NOTCPSOCKET) && !defined(NOWEBSOCKET)
        if(realSslSocket==NULL && realWebSocket==NULL)
            client->sendProtocol();
        else
            qDebug() << "Tcp/Web socket found, skip sendProtocol()";
        #elif !defined(NOTCPSOCKET)
        if(realSslSocket==NULL)
            client->sendProtocol();
        else
            qDebug() << "Tcp socket found, skip sendProtocol()";
        #elif !defined(NOWEBSOCKET)
        if(realWebSocket==NULL)
            client->sendProtocol();
        else
            qDebug() << "Web socket found, skip sendProtocol()";
        #endif
    }
    if(socketState==QAbstractSocket::UnconnectedState)
    {
        if(client!=NULL)
        {
            std::cout << "MainWindow::stateChanged(" << std::to_string((int)socketState) << ") client!=NULL" << std::endl;
            if(client->stage()==CatchChallenger::Api_client_real::StageConnexion::Stage2 || client->stage()==CatchChallenger::Api_client_real::StageConnexion::Stage3)
            {
                std::cout << "MainWindow::stateChanged(" << std::to_string((int)socketState) << ") call socketDisconnectedForReconnect" << std::endl;
                const std::string &lastServer=client->socketDisconnectedForReconnect();
                if(!lastServer.empty())
                    this->lastServer=QString::fromStdString(lastServer);
                return;
            }
        }
        std::cout << "MainWindow::stateChanged(" << std::to_string((int)socketState) << ") mostly quit" << std::endl;
        if(!isVisible()
                #ifndef NOSINGLEPLAYER
                && internalServer==NULL
                #endif
                )
        {
            QCoreApplication::quit();
            return;
        }
        if(client!=NULL)
            static_cast<CatchChallenger::Api_client_real *>(this->client)->closeDownload();
        if(client!=NULL && client->protocolWrong())
            QMessageBox::about(this,tr("Quit"),tr("The server have closed the connexion"));
        #ifndef NOSINGLEPLAYER
        if(internalServer!=NULL)
            internalServer->stop();
        #endif
        /* to fix bug: firstly try connect but connexion refused on localhost, secondly try local game */
        #ifndef NOTCPSOCKET
        if(realSslSocket!=NULL)
        {
            realSslSocket->deleteLater();
            realSslSocket=NULL;
        }
        #endif
        #ifndef NOWEBSOCKET
        if(realWebSocket!=NULL)
        {
            realWebSocket->deleteLater();
            realWebSocket=NULL;
        }
        #endif
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
    if(baseWindow!=NULL)
        baseWindow->stateChanged(socketState);
}

void MainWindow::error(QAbstractSocket::SocketError socketError)
{
    if(client!=NULL)
        if(client->stage()==CatchChallenger::Api_client_real::StageConnexion::Stage2)
            return;
    QString additionalText;
    if(!lastServer.isEmpty())
        additionalText=tr(" on %1").arg(lastServer);
    resetAll();
    switch(socketError)
    {
    case QAbstractSocket::RemoteHostClosedError:
        #ifndef NOTCPSOCKET
        if(realSslSocket!=NULL)
            return;
        #endif
        #ifndef NOWEBSOCKET
        if(realWebSocket!=NULL)
            return;
        #endif
        if(haveShowDisconnectionReason)
        {
            haveShowDisconnectionReason=false;
            return;
        }
        QMessageBox::information(this,tr("Connection closed"),tr("Connection closed by the server")+additionalText);
    break;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::information(this,tr("Connection closed"),tr("Connection refused by the server")+additionalText);
    break;
    case QAbstractSocket::SocketTimeoutError:
        QMessageBox::information(this,tr("Connection closed"),tr("Socket time out, server too long")+additionalText);
    break;
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::information(this,tr("Connection closed"),tr("The host address was not found")+additionalText);
    break;
    case QAbstractSocket::SocketAccessError:
        QMessageBox::information(this,tr("Connection closed"),tr("The socket operation failed because the application lacked the required privileges")+additionalText);
    break;
    case QAbstractSocket::SocketResourceError:
        QMessageBox::information(this,tr("Connection closed"),tr("The local system ran out of resources")+additionalText);
    break;
    case QAbstractSocket::NetworkError:
        QMessageBox::information(this,tr("Connection closed"),tr("An error occurred with the network (Connection refused on game server?)")+additionalText);
    break;
    case QAbstractSocket::UnsupportedSocketOperationError:
        QMessageBox::information(this,tr("Connection closed"),tr("The requested socket operation is not supported by the local operating system (e.g., lack of IPv6 support)")+additionalText);
    break;
    case QAbstractSocket::SslHandshakeFailedError:
        QMessageBox::information(this,tr("Connection closed"),tr("The SSL/TLS handshake failed, so the connection was closed")+additionalText);
    break;
    default:
        QMessageBox::information(this,tr("Connection error"),tr("Connection error: %1").arg(socketError)+additionalText);
    }
}

void MainWindow::newError(std::string error,std::string detailedError)
{
    std::cout << "MainWindow::newError(): " << error << ": " << detailedError << std::endl;
    if(client!=NULL)
        client->tryDisconnect();
    QMessageBox::critical(this,tr("Error"),QString::fromStdString(error));
}

void MainWindow::haveNewError()
{
    //QMessageBox::critical(this,tr("Error"),client->errorString());
}

void MainWindow::message(std::string message)
{
    std::cout << message << std::endl;
}

void MainWindow::protocol_is_good()
{
    if(serverMode==ServerMode_Internal)
        client->tryLogin("admin",pass.toStdString());
    else
        client->tryLogin(ui->lineEditLogin->text().toStdString(),ui->lineEditPass->text().toStdString());
}

void MainWindow::needQuit()
{
    client->tryDisconnect();
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
    unsigned int index=0;
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

std::pair<std::string,std::string> MainWindow::getDatapackInformations(const std::string &filePath)
{
    std::pair<std::string,std::string> returnVar;
    returnVar.first=tr("Unknown").toStdString();
    returnVar.second=tr("Unknown").toStdString();
    //open and quick check the file
    tinyxml2::XMLDocument domDocument;
    const auto loadOkay = domDocument.LoadFile(filePath.c_str());
    if(loadOkay!=0)
    {
        std::cerr << filePath+", "+domDocument.ErrorName() << std::endl;
        return returnVar;
    }
    const tinyxml2::XMLElement *root = domDocument.RootElement();
    if(root==NULL)
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, \"informations\" root balise not found for the xml file")
                    .arg(QString::fromStdString(filePath));
        return returnVar;
    }
    if(root->Name()==NULL)
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, \"informations\" root balise not found 2 for the xml file")
                    .arg(QString::fromStdString(filePath));
        return returnVar;
    }
    if(strcmp(root->Name(),"informations")!=0)
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, \"informations\" root balise not found for the xml file")
                    .arg(QString::fromStdString(filePath));
        return returnVar;
    }

    QStringList authors;
    //load the content
    const tinyxml2::XMLElement *item = root->FirstChildElement("author");
    while(item!=NULL)
    {
        if(item->Attribute("pseudo")!=NULL && item->Attribute("name")!=NULL)
            authors << QStringLiteral("%1 (%2)").arg(item->Attribute("pseudo")).arg(item->Attribute("name"));
        else if(item->Attribute("name")!=NULL)
            authors << item->Attribute("name");
        else if(item->Attribute("pseudo")!=NULL)
            authors << item->Attribute("pseudo");
        item = item->NextSiblingElement("author");
    }
    if(authors.isEmpty())
        returnVar.first=tr("Unknown").toStdString();
    else
        returnVar.first=authors.join(QStringLiteral(", ")).toStdString();

    returnVar.second=tr("Unknown").toStdString();
    item = root->FirstChildElement("name");
    bool found=false;
    const std::string &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
    if(!language.empty() && language!="en")
        while(item!=NULL)
        {
            if(item->Attribute("lang")!=NULL && item->Attribute("lang")==language && item->GetText()!=NULL)
            {
                returnVar.second=item->GetText();
                found=true;
                break;
            }
            item = item->NextSiblingElement("name");
        }
    if(!found)
    {
        item = root->FirstChildElement("name");
        while(item!=NULL)
        {
            if(item->Attribute("lang")==NULL || strcmp(item->Attribute("lang"),"en")==0)
                if(item->GetText()!=NULL)
                {
                    returnVar.second=item->GetText();
                    break;
                }
            item = item->NextSiblingElement("name");
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
        datapack.erase(datapack.begin());
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
        std::pair<std::string,std::string> tempVar=getDatapackInformations(fileInfo.absoluteFilePath().toStdString()+
                                                                           "/informations.xml");
        QString author=QString::fromStdString(tempVar.first);
        QString name=QString::fromStdString(tempVar.second);
        ListEntryEnvolued *newEntry=new ListEntryEnvolued();
        if(!connect(newEntry,&ListEntryEnvolued::clicked,this,&MainWindow::ListEntryEnvoluedClicked,Qt::QueuedConnection))
            abort();
        if(!connect(newEntry,&ListEntryEnvolued::doubleClicked,this,&MainWindow::ListEntryEnvoluedDoubleClicked,Qt::QueuedConnection))
            abort();
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

        datapack.push_back(newEntry);
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
    #ifndef __EMSCRIPTEN__
    QString catchChallengerVersion;
    #ifdef CATCHCHALLENGER_VERSION_ULTIMATE
    catchChallengerVersion=QStringLiteral("CatchChallenger Ultimate/%1").arg(QString::fromStdString(CatchChallenger::Version::str));
    #else
        #ifdef CATCHCHALLENGER_VERSION_SINGLESERVER
        catchChallengerVersion=QStringLiteral("CatchChallenger SingleServer/%1").arg(QString::fromStdString(CatchChallenger::Version::str));
        #else
            #ifdef CATCHCHALLENGER_VERSION_SOLO
            catchChallengerVersion=QStringLiteral("CatchChallenger Solo/%1").arg(QString::fromStdString(CatchChallenger::Version::str));
            #else
            catchChallengerVersion=QStringLiteral("CatchChallenger/%1").arg(QString::fromStdString(CatchChallenger::Version::str));
            #endif
        #endif
    #endif
    #if defined(_WIN32) || defined(Q_OS_MAC)
    catchChallengerVersion+=QStringLiteral(" (OS: %1)").arg(QString::fromStdString(InternetUpdater::GetOSDisplayString()));
    #endif
    catchChallengerVersion+=QStringLiteral(" ")+CATCHCHALLENGER_PLATFORM_CODE;
    #endif

    QNetworkRequest networkRequest(QString(CATCHCHALLENGER_SERVER_LIST_URL));
    #ifndef __EMSCRIPTEN__
    networkRequest.setHeader(QNetworkRequest::UserAgentHeader,catchChallengerVersion);
    #endif
    reply = qnam.get(networkRequest);
    if(!connect(reply, &QNetworkReply::finished, this, &MainWindow::httpFinished))
        abort();
    /*if(!connect(reply, &QNetworkReply::metaDataChanged, this, &MainWindow::metaDataChanged))
        abort(); seam buggy*/
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
        std::cerr << "Get server list failed: " << reply->errorString().toStdString() << std::endl;
        reply->deleteLater();
        reply=NULL;
        return;
    } else if (!redirectionTarget.isNull()) {
        ui->warning->setText(tr("Get server list redirection denied to: %1").arg(reply->errorString()));
        std::cerr << "Get server list redirection denied to: " << reply->errorString().toStdString() << std::endl;
        reply->deleteLater();
        reply=NULL;
        return;
    }
    std::cout << "Got new server list" << std::endl;
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
    mergedConnexionInfoList=temp_customConnexionInfoList;
    mergedConnexionInfoList.insert(mergedConnexionInfoList.end(),temp_xmlConnexionInfoList.begin(),temp_xmlConnexionInfoList.end());
    std::cout << "mergedConnexionInfoList.size(): " << mergedConnexionInfoList.size() << std::endl;
    std::sort(mergedConnexionInfoList.begin(),mergedConnexionInfoList.end());//qSort(mergedConnexionInfoList);
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

#ifndef NOSINGLEPLAYER
void MainWindow::gameSolo_play(const std::string &savegamesPath)
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
    socket=new CatchChallenger::ConnectedSocket(new QFakeSocket());
    CatchChallenger::Api_client_virtual *client=new CatchChallenger::Api_client_virtual(socket);//QCoreApplication::applicationDirPath()+QStringLiteral("/datapack/internal/")
    this->client=client;

    if(!connect(client,               &CatchChallenger::Api_protocol_Qt::Qtprotocol_is_good,   this,&MainWindow::protocol_is_good))
        abort();
    if(!connect(client,               &CatchChallenger::Api_protocol_Qt::Qtdisconnected,       this,&MainWindow::disconnected))
        abort();
    if(!connect(client,               &CatchChallenger::Api_protocol_Qt::Qtmessage,            this,&MainWindow::message))
        abort();
    if(!connect(socket,                                                 &CatchChallenger::ConnectedSocket::stateChanged,    this,&MainWindow::stateChanged))
        abort();
    baseWindow->setMultiPlayer(false,static_cast<CatchChallenger::Api_client_real *>(this->client));
    baseWindow->connectAllSignals();//need always be after setMultiPlayer()
    client->setDatapackPath(QCoreApplication::applicationDirPath().toStdString()+"/datapack/internal/");
    baseWindow->mapController->setDatapackPath(client->datapackPathBase(),client->mainDatapackCode());
    serverMode=ServerMode_Internal;
    ui->stackedWidget->setCurrentWidget(baseWindow);
    timeLaunched=QDateTime::currentDateTimeUtc().toTime_t();
    QSettings metaData(QString::fromStdString(savegamesPath)+QStringLiteral("metadata.conf"),QSettings::IniFormat);
    if(!metaData.contains(QStringLiteral("pass")))
    {
        QMessageBox::critical(NULL,tr("Error"),tr("Unable to load internal value"));
        return;
    }
    launchedGamePath=QString::fromStdString(savegamesPath);
    haveLaunchedGame=true;
    pass=metaData.value(QStringLiteral("pass")).toString();
    if(internalServer!=NULL)
        delete internalServer;
    //internalServer=new CatchChallenger::InternalServer(metaData);
    internalServer=new CatchChallenger::InternalServer();
    if(!sendSettings(internalServer,QString::fromStdString(savegamesPath)))
        return;
    if(!connect(internalServer,&CatchChallenger::InternalServer::is_started,this,&MainWindow::is_started,Qt::QueuedConnection))
        abort();
    if(!connect(internalServer,&CatchChallenger::InternalServer::error,this,&MainWindow::serverErrorStd,Qt::QueuedConnection))
        abort();
    internalServer->start();

    baseWindow->serverIsLoading();
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
    else
        QMessageBox::critical(this,"Bug prevent","Sorry but some Qt version is buggy, it's why this section is closed.");
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
    formatedServerSettings.compressionType=CompressionProtocol::CompressionType::None;
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
    formatedServerSettings.datapack_basePath=client->datapackPathBase();

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
            const QFileInfoList &list=QDir(QString::fromStdString(client->datapackPathBase())+"/map/main/")
                    .entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name);
            if(list.isEmpty())
            {
                QMessageBox::critical(this,tr("Error"),tr("No main code detected into the current datapack"));
                return false;
            }
            settings.setValue("mainDatapackCode",list.at(0).fileName());
            CommonSettingsServer::commonSettingsServer.mainDatapackCode=list.at(0).fileName().toStdString();
        }
        QDir mainDir(QString::fromStdString(client->datapackPathBase())+"map/main/"+
                     QString::fromStdString(CommonSettingsServer::commonSettingsServer.mainDatapackCode)+"/");
        if(!mainDir.exists())
        {
            const QFileInfoList &list=QDir(QString::fromStdString(client->datapackPathBase())+"/map/main/")
                    .entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name);
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
            const QFileInfoList &list=QDir(QString::fromStdString(client->datapackPathBase())+"/map/main/"+
                                           QString::fromStdString(CommonSettingsServer::commonSettingsServer.mainDatapackCode)+"/sub/")
                    .entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name);
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
            QDir subDir(QString::fromStdString(client->datapackPathBase())+"/map/main/"+
                        QString::fromStdString(CommonSettingsServer::commonSettingsServer.mainDatapackCode)+"/sub/"+
                        QString::fromStdString(CommonSettingsServer::commonSettingsServer.subDatapackCode)+"/");
            if(!subDir.exists())
            {
                const QFileInfoList &list=QDir(QString::fromStdString(client->datapackPathBase())+"/map/main/"+
                                               QString::fromStdString(CommonSettingsServer::commonSettingsServer.mainDatapackCode)+"/sub/")
                        .entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name);
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
                QString locaction=QString::fromStdString(baseWindow->lastLocation());
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
#endif

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
        #ifndef NOSINGLEPLAYER
        if(internalServer!=NULL)
        {
            delete internalServer;
            internalServer=NULL;
        }
        #endif
        if(!isVisible())
            QCoreApplication::quit();
        else
            resetAll();
    }
    else
    {
        baseWindow->serverIsReady();
        lastServer="localhost:9999";
        socket->connectToHost(QStringLiteral("localhost"),9999);
    }
}

void MainWindow::on_languages_clicked()
{
    LanguagesSelect::languagesSelect->exec();
}

#ifndef __EMSCRIPTEN__
void MainWindow::newUpdate(const std::string &version)
{
    ui->update->setText(QString::fromStdString(InternetUpdater::getText(version)));
    ui->update->setVisible(true);
}
#endif

void MainWindow::feedEntryList(const std::vector<FeedNews::FeedEntry> &entryList, std::string error=std::string())
{
    if(entryList.empty())
    {
        if(error.empty())
            ui->news->setVisible(false);
        else
        {
            ui->news->setToolTip(QString::fromStdString(error));
            ui->news->setStyleSheet("#news{background-color: rgb(220, 220, 240);\nborder: 1px solid rgb(100, 150, 240);\nborder-radius:5px;\ncolor: rgb(0, 0, 0);\nbackground-image: url(:/images/multi/warning.png);\nbackground-repeat: no-repeat;\nbackground-position: right;}");
        }
        return;
    }
    if(entryList.size()==1)
        ui->news->setText(tr("Latest news:")+" "+QStringLiteral("<a href=\"%1\">%2</a>")
                          .arg(QString::fromStdString(entryList.at(0).link))
                          .arg(QString::fromStdString(entryList.at(0).title)));
    else
    {
        QStringList entryHtmlList;
        unsigned int index=0;
        while(index<entryList.size() && index<3)
        {
            entryHtmlList << QStringLiteral(" - <a href=\"%1\">%2</a>")
                             .arg(QString::fromStdString(entryList.at(index).link))
                             .arg(QString::fromStdString(entryList.at(index).title));
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
    if(serverConnexion.contains(selectedServer))
        lastServerWaitBeforeConnectAfterKick[serverConnexion.value(selectedServer)->host]=CommonSettingsServer::commonSettingsServer.waitBeforeConnectAfterKick;
}

void MainWindow::gameIsLoaded()
{
    #ifndef CATCHCHALLENGER_NOAUDIO
    if(player!=NULL)
        player->stop();
    #endif
    this->setWindowTitle(QStringLiteral("CatchChallenger Ultimate - %1").arg(QString::fromStdString(client->getPseudo())));
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

void MainWindow::on_server_edit_clicked()
{
    if(selectedServer==NULL)
        return;
    if(!customServerConnexion.contains(selectedServer))
        return;
    unsigned int index=0;
    while(index<mergedConnexionInfoList.size())
    {
        ConnexionInfo * connexionInfo=serverConnexion.value(selectedServer);
        if(connexionInfo==&mergedConnexionInfoList.at(index))
        {
            AddOrEditServer editServer(this);
            editServer.setName(connexionInfo->name);

            if(connexionInfo->ws.isEmpty())
            {
                editServer.setType(0);
                editServer.setServer(connexionInfo->host);
                editServer.setPort(connexionInfo->port);
            }
            else
            {
                editServer.setType(1);
                editServer.setServer(connexionInfo->ws);
                editServer.setPort(connexionInfo->port);
            }

            editServer.setProxyServer(connexionInfo->proxyHost);
            editServer.setProxyPort(connexionInfo->proxyPort);
            editServer.exec();
            if(!editServer.isOk())
                return;
            if(editServer.type()==0)
            {
                if(!editServer.server().contains(QRegularExpression("^[a-zA-Z0-9\\.:\\-_]+$")))
                {
                    QMessageBox::warning(this,tr("Error"),tr("The host seam don't be a valid hostname or ip"));
                    return;
                }
            }
            else
            {
                if(!editServer.server().startsWith("ws://") && !editServer.server().startsWith("wss://"))
                {
                    QMessageBox::warning(this,tr("Error"),tr("The web socket url seam wrong, not start with ws:// or wss://"));
                    return;
                }
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

            connexionInfo->name=editServer.name();
            if(editServer.type()==0)
            {
                #ifndef NOTCPSOCKET
                connexionInfo->host=editServer.server();
                #endif
                connexionInfo->port=editServer.port();
                connexionInfo->ws.clear();
            }
            else
            {
                #ifndef NOWEBSOCKET
                connexionInfo->ws=editServer.server();
                #endif
                connexionInfo->port=editServer.port();
                connexionInfo->host.clear();
            }
            connexionInfo->proxyHost=editServer.proxyServer();
            connexionInfo->proxyPort=editServer.proxyPort();

            saveConnexionInfoList();
            displayServerList();
            break;
        }
        index++;
    }
}

void MainWindow::on_showPassword_toggled(bool)
{
    if(ui->showPassword->isChecked())
        ui->lineEditPass->setEchoMode(QLineEdit::Normal);
    else
        ui->lineEditPass->setEchoMode(QLineEdit::Password);
}

void MainWindow::on_UltimateKey_clicked()
{
    askForUltimateCopy();
}
