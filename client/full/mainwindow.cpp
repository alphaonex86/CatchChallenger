#include "mainwindow.h"
#include "AddServer.h"
#include "ui_mainwindow.h"
#include <QStandardPaths>

#ifdef Q_CC_GNU
//this next header is needed to change file time/date under gcc
#include <utime.h>
#include <sys/stat.h>
#endif

#include "../base/render/MapVisualiserPlayer.h"
#include "../../general/base/FacilityLib.h"
#include "../base/LanguagesSelect.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    serverMode=ServerMode_None;
    qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");
    qRegisterMetaType<CatchChallenger::Chat_type>("CatchChallenger::Chat_type");
    qRegisterMetaType<CatchChallenger::Player_type>("CatchChallenger::Player_type");
    qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");
    qRegisterMetaType<CatchChallenger::Chat_type>("CatchChallenger::Chat_type");
    qRegisterMetaType<CatchChallenger::Player_type>("QAbstractSocket::SocketState");
    qRegisterMetaType<CatchChallenger::Player_private_and_public_informations>("CatchChallenger::Player_private_and_public_informations");
    qRegisterMetaType<CatchChallenger::Player_public_informations>("CatchChallenger::Player_public_informations");
    qRegisterMetaType<CatchChallenger::Direction>("CatchChallenger::Direction");

    socket=NULL;
    internalServer=NULL;
    reply=NULL;
    realSocket=new QSslSocket();
    realSocket->ignoreSslErrors();
    realSocket->setPeerVerifyMode(QSslSocket::VerifyNone);
    connect(realSocket,static_cast<void(QSslSocket::*)(const QList<QSslError> &errors)>(&QSslSocket::sslErrors),      this,&MainWindow::sslErrors,Qt::QueuedConnection);
    spacer=new QSpacerItem(0,0,QSizePolicy::Expanding,QSizePolicy::Expanding);
    spacerServer=new QSpacerItem(0,0,QSizePolicy::Expanding,QSizePolicy::Expanding);
    ui->setupUi(this);
    solowindow=new SoloWindow(this,QCoreApplication::applicationDirPath()+"/datapack/internal/",QCoreApplication::applicationDirPath()+"/savegames/",false);
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
    //connect(CatchChallenger::BaseWindow::baseWindow,                &CatchChallenger::BaseWindow::needQuit,             this,&MainWindow::needQuit);

    stopFlood.setSingleShot(false);
    stopFlood.start(1500);
    numberForFlood=0;
    haveShowDisconnectionReason=false;

    stateChanged(QAbstractSocket::UnconnectedState);

    setWindowTitle("CatchChallenger");
    downloadFile();
}

MainWindow::~MainWindow()
{
    if(CatchChallenger::BaseWindow::baseWindow!=NULL)
    {
        CatchChallenger::BaseWindow::baseWindow->deleteLater();
        CatchChallenger::BaseWindow::baseWindow=NULL;
    }
    if(CatchChallenger::Api_client_real::client!=NULL)
    {
        CatchChallenger::Api_client_real::client->tryDisconnect();
        CatchChallenger::Api_client_real::client->deleteLater();
    }
    if(CatchChallenger::BaseWindow::baseWindow!=NULL)
        delete CatchChallenger::BaseWindow::baseWindow;
    if(socket!=NULL)
    {
        socket->abort();
        socket->deleteLater();
    }
    delete ui;
}

QList<ConnexionInfo> MainWindow::loadConfigConnexionInfoList()
{
    QList<ConnexionInfo> returnedVar;
    QStringList connexionList;
    QStringList nameList;
    QStringList connexionCounterList;
    QStringList lastConnexionList;
    if(settings.contains("connexionList"))
        connexionList=settings.value("connexionList").toStringList();
    if(settings.contains("nameList"))
        nameList=settings.value("nameList").toStringList();
    if(settings.contains("connexionCounterList"))
        connexionCounterList=settings.value("connexionCounterList").toStringList();
    if(settings.contains("lastConnexionList"))
        lastConnexionList=settings.value("lastConnexionList").toStringList();
    if(nameList.size()!=connexionList.size())
        nameList.clear();
    if(connexionCounterList.size()!=connexionList.size())
        connexionCounterList.clear();
    if(lastConnexionList.size()!=connexionList.size())
        lastConnexionList.clear();
    while(nameList.size()<connexionList.size())
        nameList << QString();
    while(connexionCounterList.size()<connexionList.size())
        connexionCounterList << QString();
    while(lastConnexionList.size()<connexionList.size())
        lastConnexionList << QString();
    int index=0;
    while(index<connexionList.size())
    {
        QString connexion=connexionList.at(index);
        QString name=nameList.at(index);
        QString connexionCounter=connexionCounterList.at(index);
        QString lastConnexion=lastConnexionList.at(index);
        if(connexion.contains(QRegularExpression("^[a-zA-Z0-9\\.\\-_]+:[0-9]{1,5}$")))
        {
            QString host=connexion;
            host.remove(QRegularExpression(":[0-9]{1,5}$"));
            QString port_string=connexion;
            port_string.remove(QRegularExpression("^.*:"));
            bool ok;
            quint16 port=port_string.toInt(&ok);
            if(ok)
            {
                ConnexionInfo connexionInfo;
                connexionInfo.host=host;
                connexionInfo.port=port;
                connexionInfo.name=name;
                connexionInfo.connexionCounter=connexionCounter.toUInt(&ok);
                if(!ok)
                    connexionInfo.connexionCounter=0;
                connexionInfo.lastConnexion=lastConnexion.toUInt(&ok);
                if(!ok)
                    connexionInfo.lastConnexion=QDateTime::currentMSecsSinceEpoch()/1000;
                if(connexionInfo.lastConnexion>(QDateTime::currentMSecsSinceEpoch()/1000))
                    connexionInfo.lastConnexion=QDateTime::currentMSecsSinceEpoch()/1000;
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
        if(QFileInfo(QStandardPaths::writableLocation(QStandardPaths::DataLocation)+"/server_list.xml").isFile())
            return loadXmlConnexionInfoList(QStandardPaths::writableLocation(QStandardPaths::DataLocation)+"/server_list.xml");
        else
            return loadXmlConnexionInfoList(QString(":/other/default_server_list.xml"));
    }
    else
        return loadXmlConnexionInfoList(QString(":/other/default_server_list.xml"));
}

QList<ConnexionInfo> MainWindow::loadXmlConnexionInfoList(const QByteArray &xmlContent)
{
    QList<ConnexionInfo> returnedVar;
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << QString("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg("server_list.xml").arg(errorLine).arg(errorColumn).arg(errorStr);
        return returnedVar;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="servers")
    {
        qDebug() << QString("Unable to open the file: %1, \"servers\" root balise not found for the xml file").arg("server_list.xml");
        return returnedVar;
    }

    bool ok;
    //load the content
    QDomElement server = root.firstChildElement("server");
    while(!server.isNull())
    {
        if(server.isElement())
        {
            if(server.hasAttribute("host") && server.hasAttribute("port") && server.hasAttribute("unique_code"))
            {
                ConnexionInfo connexionInfo;
                connexionInfo.host=server.attribute("host");
                connexionInfo.unique_code=server.attribute("unique_code");
                quint32 temp_port=server.attribute("port").toUInt(&ok);
                if(!connexionInfo.host.contains(QRegularExpression("^[a-zA-Z0-9\\.\\-_]+$")))
                    qDebug() << QString("Unable to open the file: %1, host is wrong: %4 child.tagName(): %2 (at line: %3)").arg("server_list.xml").arg(server.tagName()).arg(server.lineNumber()).arg(connexionInfo.host);
                else if(!ok)
                    qDebug() << QString("Unable to open the file: %1, port is not a number: %4 child.tagName(): %2 (at line: %3)").arg("server_list.xml").arg(server.tagName()).arg(server.lineNumber()).arg(server.attribute("port"));
                else if(temp_port<1 || temp_port>65535)
                    qDebug() << QString("Unable to open the file: %1, port is not in range: %4 child.tagName(): %2 (at line: %3)").arg("server_list.xml").arg(server.tagName()).arg(server.lineNumber()).arg(server.attribute("port"));
                else if(connexionInfo.unique_code.isEmpty())
                    qDebug() << QString("Unable to open the file: %1, unique_code can't be empty: %4 child.tagName(): %2 (at line: %3)").arg("server_list.xml").arg(server.tagName()).arg(server.lineNumber()).arg(server.attribute("port"));
                else
                {
                    connexionInfo.port=temp_port;
                    QDomElement lang;
                    const QString &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
                    bool found=false;
                    if(!language.isEmpty() && language!="en")
                    {
                        lang = server.firstChildElement("lang");
                        while(!lang.isNull())
                        {
                            if(lang.isElement())
                            {
                                if(lang.hasAttribute("lang") && lang.attribute("lang")==language)
                                {
                                    QDomElement name = lang.firstChildElement("name");
                                    if(!name.isNull())
                                        connexionInfo.name=name.text();
                                    QDomElement register_page = lang.firstChildElement("register_page");
                                    if(!register_page.isNull())
                                        connexionInfo.register_page=register_page.text();
                                    QDomElement lost_passwd_page = lang.firstChildElement("lost_passwd_page");
                                    if(!lost_passwd_page.isNull())
                                        connexionInfo.lost_passwd_page=lost_passwd_page.text();
                                    QDomElement site_page = lang.firstChildElement("site_page");
                                    if(!site_page.isNull())
                                        connexionInfo.site_page=site_page.text();
                                    found=true;
                                    break;
                                }
                            }
                            else
                                qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg("server_list.xml").arg(lang.tagName()).arg(lang.lineNumber());
                            lang = lang.nextSiblingElement("lang");
                        }
                    }
                    if(!found)
                    {
                        lang = server.firstChildElement("lang");
                        while(!lang.isNull())
                        {
                            if(lang.isElement())
                            {
                                if(!lang.hasAttribute("lang") || lang.attribute("lang")=="en")
                                {
                                    QDomElement name = lang.firstChildElement("name");
                                    if(!name.isNull())
                                        connexionInfo.name=name.text();
                                    QDomElement register_page = lang.firstChildElement("register_page");
                                    if(!register_page.isNull())
                                        connexionInfo.register_page=register_page.text();
                                    QDomElement lost_passwd_page = lang.firstChildElement("lost_passwd_page");
                                    if(!lost_passwd_page.isNull())
                                        connexionInfo.lost_passwd_page=lost_passwd_page.text();
                                    QDomElement site_page = lang.firstChildElement("site_page");
                                    if(!site_page.isNull())
                                        connexionInfo.site_page=site_page.text();
                                    break;
                                }
                            }
                            else
                                qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg("server_list.xml").arg(lang.tagName()).arg(lang.lineNumber());
                            lang = lang.nextSiblingElement("lang");
                        }
                    }
                    settings.beginGroup(QString("Xml-%1").arg(server.attribute("unique_code")));
                    if(settings.contains("connexionCounter"))
                    {
                        connexionInfo.connexionCounter=settings.value("connexionCounter").toUInt(&ok);
                        if(!ok)
                            connexionInfo.connexionCounter=0;
                    }
                    else
                        connexionInfo.connexionCounter=0;
                    if(settings.contains("lastConnexion"))
                    {
                        connexionInfo.lastConnexion=settings.value("lastConnexion").toUInt(&ok);
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
                qDebug() << QString("Unable to open the file: %1, missing host or port: child.tagName(): %2 (at line: %3)").arg("server_list.xml").arg(server.tagName()).arg(server.lineNumber());
        }
        else
            qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg("server_list.xml").arg(server.tagName()).arg(server.lineNumber());
        server = server.nextSiblingElement("server");
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
        qDebug() << QString("Unable to open the file: %1, error: %2").arg(itemsFile.fileName()).arg(itemsFile.errorString());
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
        ui->serverEmpty->setText(QString("<html><body><p align=\"center\"><span style=\"font-size:12pt;color:#a0a0a0;\">%1</span></p></body></html>").arg(tr("Empty")));
    index=0;
    while(index<mergedConnexionInfoList.size())
    {
        ListEntryEnvolued *newEntry=new ListEntryEnvolued();
        connect(newEntry,&ListEntryEnvolued::clicked,this,&MainWindow::serverListEntryEnvoluedClicked,Qt::QueuedConnection);
        connect(newEntry,&ListEntryEnvolued::doubleClicked,this,&MainWindow::serverListEntryEnvoluedDoubleClicked,Qt::QueuedConnection);
        const ConnexionInfo &connexionInfo=mergedConnexionInfoList.at(index);
        QString name;
        QString star;
        if(connexionInfo.connexionCounter>0)
            star+="<img src=\":/images/interface/top.png\" alt=\"\" />";
        QString lastConnexion;
        if(connexionInfo.connexionCounter>0)
            lastConnexion=tr("Last connexion: %1").arg(QDateTime::fromMSecsSinceEpoch((quint64)connexionInfo.lastConnexion*1000).toString());
        QString custom;
        if(connexionInfo.unique_code.isEmpty())
            custom=QString(" (%1)").arg(tr("Custom"));
        if(connexionInfo.name.isEmpty())
        {
            name=QString("%1:%2").arg(connexionInfo.host).arg(connexionInfo.port);
            newEntry->setText(QString("%3<span style=\"font-size:12pt;font-weight:600;\">%1:%2</span><br/><span style=\"color:#909090;\">%2:%3%6</span>")
                              .arg(connexionInfo.host)
                              .arg(connexionInfo.port)
                              .arg(star)
                              .arg(lastConnexion)
                              .arg(custom)
                              );
        }
        else
        {
            name=connexionInfo.name;
            newEntry->setText(QString("%4<span style=\"font-size:12pt;font-weight:600;\">%1</span><br/><span style=\"color:#909090;\">%2:%3 %5%6</span>")
                              .arg(connexionInfo.name)
                              .arg(connexionInfo.host)
                              .arg(connexionInfo.port)
                              .arg(star)
                              .arg(lastConnexion)
                              .arg(custom)
                              );
        }
        newEntry->setStyleSheet("QLabel::hover{border:1px solid #bbb;background-color:rgb(180,180,180,100);border-radius:10px;}");

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
            server.at(index)->setStyleSheet("QLabel{border:1px solid #6b6;background-color:rgb(100,180,100,120);border-radius:10px;}QLabel::hover{border:1px solid #494;background-color:rgb(70,150,70,120);border-radius:10px;}");
        else
            server.at(index)->setStyleSheet("QLabel::hover{border:1px solid #bbb;background-color:rgb(180,180,180,100);border-radius:10px;}");
        index++;
    }
    ui->server_select->setEnabled(selectedServer!=NULL);
    ui->server_remove->setEnabled(selectedServer!=NULL && customServerConnexion.contains(selectedServer));
}

void MainWindow::on_server_add_clicked()
{
    AddServer addServer(this);
    addServer.exec();
    if(!addServer.isOk())
        return;
    if(!addServer.server().contains(QRegularExpression("^[a-zA-Z0-9\\.\\-_]+$")))
    {
        QMessageBox::warning(this,tr("Error"),tr("The host seam don't be a valid hostname or ip"));
        return;
    }
    ConnexionInfo connexionInfo;
    connexionInfo.connexionCounter=0;
    connexionInfo.host=addServer.server();
    connexionInfo.lastConnexion=QDateTime::currentMSecsSinceEpoch()/1000;
    connexionInfo.name=addServer.name();
    connexionInfo.port=addServer.port();
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
    if(customServerConnexion.contains(selectedServer))
        settings.beginGroup(QString("%1-%2").arg(serverConnexion[selectedServer]->host).arg(serverConnexion[selectedServer]->port));
    else
        settings.beginGroup(QString("Xml-%1").arg(serverConnexion[selectedServer]->unique_code));
    if(settings.contains("login"))
        ui->lineEditLogin->setText(settings.value("login").toString());
    else
        ui->lineEditLogin->setText(QString());
    if(settings.contains("pass"))
        ui->lineEditPass->setText(settings.value("pass").toString());
    else
        ui->lineEditPass->setText(QString());
    settings.endGroup();
    ui->checkBoxRememberPassword->setChecked(!ui->lineEditPass->text().isEmpty());
}

void MainWindow::on_login_cancel_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->multi);
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
            mergedConnexionInfoList.removeAt(index);
            saveConnexionInfoList();
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
    int index;
    index=0;
    while(index<mergedConnexionInfoList.size())
    {
        const ConnexionInfo &connexionInfo=mergedConnexionInfoList.at(index);
        if(connexionInfo.unique_code.isEmpty())
        {
            connexionList << QString("%1:%2").arg(connexionInfo.host).arg(connexionInfo.port);
            nameList << connexionInfo.name;
            connexionCounterList << QString::number(connexionInfo.connexionCounter);
            lastConnexionList << QString::number(connexionInfo.lastConnexion);
        }
        else
        {
            settings.beginGroup(QString("Xml-%1").arg(connexionInfo.unique_code));
            if(connexionInfo.connexionCounter>0)
                settings.setValue("connexionCounter",connexionInfo.connexionCounter);
            else
                settings.remove("connexionCounter");
            if(connexionInfo.lastConnexion>0 && connexionInfo.connexionCounter>0)
                settings.setValue("lastConnexion",connexionInfo.lastConnexion);
            else
                settings.remove("lastConnexion");
            settings.endGroup();
        }
        index++;
    }
    settings.setValue("connexionList",connexionList);
    settings.setValue("nameList",nameList);
    settings.setValue("connexionCounterList",connexionCounterList);
    settings.setValue("lastConnexionList",lastConnexionList);
}

void MainWindow::serverListEntryEnvoluedDoubleClicked()
{
    on_server_select_clicked();
}

void MainWindow::resetAll()
{
    if(CatchChallenger::Api_client_real::client!=NULL)
        CatchChallenger::Api_client_real::client->resetAll();
    switch(serverMode)
    {
        case ServerMode_Internal:
            ui->stackedWidget->setCurrentWidget(solowindow);
        break;
        case ServerMode_Remote:
            ui->stackedWidget->setCurrentWidget(ui->multi);
        break;
        default:
            ui->stackedWidget->setCurrentWidget(ui->mode);
        break;
    }
    chat_list_player_pseudo.clear();
    chat_list_player_type.clear();
    chat_list_type.clear();
    chat_list_text.clear();
    lastMessageSend="";
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
    ui->lineEditPass->setFocus();
}

void MainWindow::on_lineEditPass_returnPressed()
{
    on_pushButtonTryLogin_clicked();
}

void MainWindow::on_pushButtonTryLogin_clicked()
{
    serverMode=ServerMode_Remote;
    if(customServerConnexion.contains(selectedServer))
        settings.beginGroup(QString("%1-%2").arg(serverConnexion[selectedServer]->host).arg(serverConnexion[selectedServer]->port));
    else
        settings.beginGroup(QString("Xml-%1").arg(serverConnexion[selectedServer]->unique_code));
    settings.setValue("login",ui->lineEditLogin->text());
    if(ui->checkBoxRememberPassword->isChecked())
        settings.setValue("pass",ui->lineEditPass->text());
    else
        settings.remove("pass");
    settings.endGroup();
    if(socket!=NULL)
    {
        socket->abort();
        socket->deleteLater();
    }
    socket=new CatchChallenger::ConnectedSocket(realSocket);
    CatchChallenger::Api_client_real::client=new CatchChallenger::Api_client_real(socket);
    connect(CatchChallenger::Api_client_real::client,               &CatchChallenger::Api_protocol::protocol_is_good,   this,&MainWindow::protocol_is_good);
    connect(CatchChallenger::Api_client_real::client,               &CatchChallenger::Api_protocol::disconnected,       this,&MainWindow::disconnected);
    connect(CatchChallenger::Api_client_real::client,               &CatchChallenger::Api_protocol::message,            this,&MainWindow::message);
    connect(socket,                                                 &CatchChallenger::ConnectedSocket::stateChanged,    this,&MainWindow::stateChanged);
    CatchChallenger::BaseWindow::baseWindow->connectAllSignals();
    CatchChallenger::BaseWindow::baseWindow->setMultiPlayer(true);
    ui->stackedWidget->setCurrentWidget(CatchChallenger::BaseWindow::baseWindow);
    static_cast<CatchChallenger::Api_client_real *>(CatchChallenger::Api_client_real::client)->tryConnect(serverConnexion[selectedServer]->host,serverConnexion[selectedServer]->port);
    MapController::mapController->setDatapackPath(CatchChallenger::Api_client_real::client->get_datapack_base_name());
    serverConnexion[selectedServer]->connexionCounter++;
    serverConnexion[selectedServer]->lastConnexion=QDateTime::currentMSecsSinceEpoch()/1000;
    saveConnexionInfoList();
    displayServerList();
}

void MainWindow::stateChanged(QAbstractSocket::SocketState socketState)
{
    if(socketState==QAbstractSocket::UnconnectedState)
        resetAll();
    if(CatchChallenger::BaseWindow::baseWindow!=NULL)
        CatchChallenger::BaseWindow::baseWindow->stateChanged(socketState);
}

void MainWindow::error(QAbstractSocket::SocketError socketError)
{
    resetAll();
    switch(socketError)
    {
    case QAbstractSocket::RemoteHostClosedError:
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
        CatchChallenger::Api_client_real::client->tryLogin("admin",pass);
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
            datapack.at(index)->setStyleSheet("QLabel{border:1px solid #6b6;background-color:rgb(100,180,100,120);border-radius:10px;}QLabel::hover{border:1px solid #494;background-color:rgb(70,150,70,120);border-radius:10px;}");
        else
            datapack.at(index)->setStyleSheet("QLabel::hover{border:1px solid #bbb;background-color:rgb(180,180,180,100);border-radius:10px;}");
        index++;
    }
    ui->deleteDatapack->setEnabled(selectedDatapack!=NULL && datapackPathList[selectedDatapack]!=QFileInfo(QCoreApplication::applicationDirPath()+"/datapack/internal").absoluteFilePath());
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
        qDebug() << QString("Unable to open the file: %1, error: %2").arg(itemsFile.fileName()).arg(itemsFile.errorString());
        return returnVar;
    }
    xmlContent=itemsFile.readAll();
    itemsFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << QString("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(itemsFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
        return returnVar;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="informations")
    {
        qDebug() << QString("Unable to open the file: %1, \"items\" root balise not found for the xml file").arg(itemsFile.fileName());
        return returnVar;
    }

    QStringList authors;
    //load the content
    QDomElement item = root.firstChildElement("author");
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute("pseudo") && item.hasAttribute("name"))
                authors << QString("%1 (%2)").arg(item.attribute("pseudo")).arg(item.attribute("name"));
            else if(item.hasAttribute("name"))
                authors << item.attribute("name");
            else if(item.hasAttribute("pseudo"))
                authors << item.attribute("pseudo");
        }
        item = item.nextSiblingElement("author");
    }
    if(authors.isEmpty())
        returnVar.first=tr("Unknown");
    else
        returnVar.first=authors.join(", ");

    returnVar.second=tr("Unknown");
    item = root.firstChildElement("name");
    bool found=false;
    const QString &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
    if(!language.isEmpty() && language!="en")
        while(!item.isNull())
        {
            if(item.isElement())
            {
                if(item.hasAttribute("lang") && item.attribute("lang")==language)
                {
                    returnVar.second=item.text();
                    found=true;
                    break;
                }
            }
            item = item.nextSiblingElement("name");
        }
    if(!found)
    {
        root.firstChildElement("name");
        while(!item.isNull())
        {
            if(item.isElement())
            {
                if(!item.hasAttribute("lang") || item.attribute("lang")=="en")
                {
                    returnVar.second=item.text();
                    break;
                }
            }
            item = item.nextSiblingElement("name");
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
    QFileInfoList entryList=QDir(QCoreApplication::applicationDirPath()+"/datapack/").entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    index=0;
    if(entryList.isEmpty())
        ui->datapackEmpty->setText(QString("<html><body><p align=\"center\"><span style=\"font-size:12pt;color:#a0a0a0;\">%1</span></p></body></html>").arg(tr("Empty")));
    while(index<entryList.size())
    {
        QFileInfo fileInfo=entryList.at(index);
        if(!fileInfo.isDir())
        {
            index++;
            continue;
        }
        QPair<QString,QString> tempVar=getDatapackInformations(fileInfo.absoluteFilePath()+"/informations.xml");
        QString author=tempVar.first;
        QString name=tempVar.second;
        ListEntryEnvolued *newEntry=new ListEntryEnvolued();
        connect(newEntry,&ListEntryEnvolued::clicked,this,&MainWindow::ListEntryEnvoluedClicked,Qt::QueuedConnection);
        connect(newEntry,&ListEntryEnvolued::doubleClicked,this,&MainWindow::ListEntryEnvoluedDoubleClicked,Qt::QueuedConnection);
        QString from;
        if(fileInfo.fileName()=="Internal")
            from=tr("Internal datapack");
        else
        {
            from=tr("Get from %1").arg(fileInfo.fileName());
            from=from.replace(QRegularExpression("-([0-9]+)$"),QString(", ")+tr("port %1").arg("\\1"));
        }
        newEntry->setStyleSheet("QLabel::hover{border:1px solid #bbb;background-color:rgb(180,180,180,100);border-radius:10px;}");
        newEntry->setText(QString("<span style=\"font-size:12pt;font-weight:600;\">%1</span><br/><span style=\"color:#909090;\">%2<br/>%3</span>")
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
    ui->stackedWidget->setCurrentWidget(ui->multi);
}

void MainWindow::on_deleteDatapack_clicked()
{
    QMessageBox::StandardButton button=QMessageBox::question(this,tr("Are you sure?"),tr("Are you sure delete the datapack? This operation is not reversible."),QMessageBox::Yes|QMessageBox::No,QMessageBox::No);
    if(button!=QMessageBox::Yes)
        return;
    if(!CatchChallenger::FacilityLib::rmpath(datapackPathList[selectedDatapack]))
        QMessageBox::warning(this,tr("Error"),tr("Remove the datapack path is not completed. Try after restarting the application"));
    on_manageDatapack_clicked();
}

void MainWindow::downloadFile()
{
    QNetworkRequest networkRequest(QString(CATCHCHALLENGER_SERVER_LIST_URL));
    networkRequest.setHeader(QNetworkRequest::UserAgentHeader,QString("CatchChallenger Multi server %1").arg(CATCHCHALLENGER_VERSION));
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
    if(!QFileInfo(QStandardPaths::writableLocation(QStandardPaths::DataLocation)+"/server_list.xml").isFile())
        return;
    QVariant val=reply->header(QNetworkRequest::LastModifiedHeader);
    if(!val.isValid())
        return;
    if(val.toDateTime()==QFileInfo(QStandardPaths::writableLocation(QStandardPaths::DataLocation)+"/server_list.xml").lastModified())
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
    QFile cache(QStandardPaths::writableLocation(QStandardPaths::DataLocation)+"/server_list.xml");
    if(cache.open(QIODevice::ReadWrite))
    {
        cache.write(content);
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
                    qDebug() << QString("Can't set time: %1").arg(cache.fileName());
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
    ui->stackedWidget->setCurrentWidget(ui->multi);
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
        socket->abort();
        socket->deleteLater();
    }
    socket=new CatchChallenger::ConnectedSocket(new CatchChallenger::QFakeSocket());
    CatchChallenger::Api_client_real::client=new CatchChallenger::Api_client_virtual(socket,QCoreApplication::applicationDirPath()+"/datapack/internal/");
    connect(CatchChallenger::Api_client_real::client,               &CatchChallenger::Api_protocol::protocol_is_good,   this,&MainWindow::protocol_is_good);
    connect(CatchChallenger::Api_client_real::client,               &CatchChallenger::Api_protocol::disconnected,       this,&MainWindow::disconnected);
    connect(CatchChallenger::Api_client_real::client,               &CatchChallenger::Api_protocol::message,            this,&MainWindow::message);
    connect(socket,                                                 &CatchChallenger::ConnectedSocket::stateChanged,    this,&MainWindow::stateChanged);
    CatchChallenger::BaseWindow::baseWindow->connectAllSignals();
    CatchChallenger::BaseWindow::baseWindow->setMultiPlayer(false);
    MapController::mapController->setDatapackPath(CatchChallenger::Api_client_real::client->get_datapack_base_name());
    serverMode=ServerMode_Internal;
    ui->stackedWidget->setCurrentWidget(CatchChallenger::BaseWindow::baseWindow);
    timeLaunched=QDateTime::currentDateTimeUtc().toTime_t();
    QSettings metaData(savegamesPath+"metadata.conf",QSettings::IniFormat);
    if(!metaData.contains("pass"))
    {
        QMessageBox::critical(NULL,tr("Error"),tr("Unable to load internal value"));
        return;
    }
    launchedGamePath=savegamesPath;
    haveLaunchedGame=true;
    pass=metaData.value("pass").toString();
    if(internalServer!=NULL)
        delete internalServer;
    internalServer=new CatchChallenger::InternalServer();
    sendSettings(internalServer,savegamesPath);
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
        saveTime();
        if(!isVisible())
            QCoreApplication::quit();
        else
            resetAll();
    }
    else
    {
        CatchChallenger::BaseWindow::baseWindow->serverIsReady();
        socket->connectToHost("localhost",9999);
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
        QSettings metaData(launchedGamePath+"metadata.conf",QSettings::IniFormat);
        if(metaData.isWritable())
        {
            if(metaData.status()==QSettings::NoError)
            {
                QString locaction=CatchChallenger::BaseWindow::baseWindow->lastLocation();
                QString mapPath=internalServer->getSettings().datapack_basePath+DATAPACK_BASE_PATH_MAP;
                if(locaction.startsWith(mapPath))
                    locaction.remove(0,mapPath.size());
                if(!locaction.isEmpty())
                    metaData.setValue("location",locaction);
                quint64 current_date_time=QDateTime::currentDateTimeUtc().toTime_t();
                if(current_date_time>timeLaunched)
                    metaData.setValue("time_played",metaData.value("time_played").toUInt()+(current_date_time-timeLaunched));
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

void MainWindow::sendSettings(CatchChallenger::InternalServer * internalServer,const QString &savegamesPath)
{
    CatchChallenger::ServerSettings formatedServerSettings=internalServer->getSettings();

    formatedServerSettings.max_players=1;
    formatedServerSettings.tolerantMode=false;
    formatedServerSettings.commmonServerSettings.sendPlayerNumber = false;
    formatedServerSettings.commmonServerSettings.compressionType=CatchChallenger::CompressionType_None;

    formatedServerSettings.database.type=CatchChallenger::ServerSettings::Database::DatabaseType_SQLite;
    formatedServerSettings.database.sqlite.file=savegamesPath+"catchchallenger.db.sqlite";
    formatedServerSettings.mapVisibility.mapVisibilityAlgorithm	= CatchChallenger::MapVisibilityAlgorithm_none;
    formatedServerSettings.bitcoin.enabled=false;
    formatedServerSettings.datapack_basePath=CatchChallenger::Api_client_real::client->get_datapack_base_name();

    internalServer->setSettings(formatedServerSettings);
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
