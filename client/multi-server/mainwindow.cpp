#include "mainwindow.h"
#include "AddServer.h"
#include "ui_mainwindow.h"

#include "../base/render/MapVisualiserPlayer.h"
#include "../../general/base/FacilityLib.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");
    qRegisterMetaType<CatchChallenger::Chat_type>("CatchChallenger::Chat_type");
    qRegisterMetaType<CatchChallenger::Player_type>("CatchChallenger::Player_type");

    realSocket=new QSslSocket();
    realSocket->ignoreSslErrors();
    realSocket->setPeerVerifyMode(QSslSocket::VerifyNone);
    connect(realSocket,static_cast<void(QSslSocket::*)(const QList<QSslError> &errors)>(&QSslSocket::sslErrors),      this,&MainWindow::sslErrors,Qt::QueuedConnection);
    socket=new CatchChallenger::ConnectedSocket(realSocket);
    CatchChallenger::Api_client_real::client=new CatchChallenger::Api_client_real(socket);
    CatchChallenger::BaseWindow::baseWindow=new CatchChallenger::BaseWindow();
    spacer=new QSpacerItem(0,0,QSizePolicy::Expanding,QSizePolicy::Expanding);
    spacerServer=new QSpacerItem(0,0,QSizePolicy::Expanding,QSizePolicy::Expanding);
    ui->setupUi(this);
    ui->stackedWidget->addWidget(CatchChallenger::BaseWindow::baseWindow);
    {
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
                    connexionInfoList << connexionInfo;
                }
            }
            index++;
        }
        displayServerList();
    }
    ui->warning->setVisible(false);
    connect(socket,static_cast<void(CatchChallenger::ConnectedSocket::*)(QAbstractSocket::SocketError)>(&CatchChallenger::ConnectedSocket::error),this,&MainWindow::error,Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,               &CatchChallenger::Api_protocol::protocol_is_good,   this,&MainWindow::protocol_is_good);
    connect(CatchChallenger::Api_client_real::client,               &CatchChallenger::Api_protocol::disconnected,       this,&MainWindow::disconnected);
    connect(CatchChallenger::Api_client_real::client,               &CatchChallenger::Api_protocol::message,            this,&MainWindow::message);
    connect(socket,                                                 &CatchChallenger::ConnectedSocket::stateChanged,    this,&MainWindow::stateChanged);
    //connect(CatchChallenger::BaseWindow::baseWindow,                &CatchChallenger::BaseWindow::needQuit,             this,&MainWindow::needQuit);

    stopFlood.setSingleShot(false);
    stopFlood.start(1500);
    numberForFlood=0;
    haveShowDisconnectionReason=false;
    ui->stackedWidget->addWidget(CatchChallenger::BaseWindow::baseWindow);
    CatchChallenger::BaseWindow::baseWindow->setMultiPlayer(true);

    stateChanged(QAbstractSocket::UnconnectedState);

    setWindowTitle("CatchChallenger");
}

MainWindow::~MainWindow()
{
    CatchChallenger::Api_client_real::client->tryDisconnect();
    delete CatchChallenger::Api_client_real::client;
    delete CatchChallenger::BaseWindow::baseWindow;
    delete ui;
    delete socket;
}

bool ConnexionInfo::operator<(const ConnexionInfo &connexionInfo) const
{
    if(connexionCounter<connexionInfo.connexionCounter)
        return true;
    if(connexionCounter>connexionInfo.connexionCounter)
        return false;
    if(lastConnexion<connexionInfo.lastConnexion)
        return true;
    if(lastConnexion>connexionInfo.lastConnexion)
        return false;
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
    qSort(connexionInfoList);
    index=0;
    if(connexionInfoList.isEmpty())
        ui->serverEmpty->setText(QString("<html><body><p align=\"center\"><span style=\"font-size:12pt;color:#a0a0a0;\">%1</span></p></body></html>").arg(tr("Empty")));
    while(index<connexionInfoList.size())
    {
        ListEntryEnvolued *newEntry=new ListEntryEnvolued();
        connect(newEntry,&ListEntryEnvolued::clicked,this,&MainWindow::serverListEntryEnvoluedClicked,Qt::QueuedConnection);
        connect(newEntry,&ListEntryEnvolued::doubleClicked,this,&MainWindow::serverListEntryEnvoluedDoubleClicked,Qt::QueuedConnection);
        const ConnexionInfo &connexionInfo=connexionInfoList.at(index);
        QString name;
        QString star;
        if(connexionInfo.connexionCounter>0)
            star+="<img src=\":/images/interface/top.png\" alt=\"\" />";
        QString lastConnexion;
        if(connexionInfo.connexionCounter>0)
            lastConnexion=tr("Last connexion: %1").arg(QDateTime::fromMSecsSinceEpoch((quint64)connexionInfo.lastConnexion*1000).toString());
        if(connexionInfo.name.isEmpty())
        {
            name=QString("%1:%2").arg(connexionInfo.host).arg(connexionInfo.port);
            newEntry->setText(QString("%3<span style=\"font-size:12pt;font-weight:600;\">%1:%2</span><br/><span style=\"color:#909090;\">%2:%3 (%6)</span>")
                              .arg(connexionInfo.host)
                              .arg(connexionInfo.port)
                              .arg(star)
                              .arg(lastConnexion)
                              .arg(tr("Custom"))
                              );
        }
        else
        {
            name=connexionInfo.name;
            newEntry->setText(QString("%4<span style=\"font-size:12pt;font-weight:600;\">%1</span><br/><span style=\"color:#909090;\">%2:%3 %5 (%6)</span>")
                              .arg(connexionInfo.name)
                              .arg(connexionInfo.host)
                              .arg(connexionInfo.port)
                              .arg(star)
                              .arg(lastConnexion)
                              .arg(tr("Custom"))
                              );
        }
        newEntry->setStyleSheet("QLabel::hover{border:1px solid #bbb;background-color:rgb(180,180,180,100);border-radius:10px;}");

        ui->scrollAreaWidgetContentsServer->layout()->addWidget(newEntry);

        server << newEntry;
        serverConnexion[newEntry]=&connexionInfoList[index];
        index++;
    }
    ui->serverEmpty->setVisible(index==0);
    if(index>0)
    {
        ui->scrollAreaWidgetContentsServer->layout()->removeItem(spacerServer);
        delete spacer;
        spacer=new QSpacerItem(0,0,QSizePolicy::Expanding,QSizePolicy::Expanding);
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
    ui->server_remove->setEnabled(selectedServer!=NULL);
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
    connexionInfoList << connexionInfo;
    saveConnexionInfoList();
    displayServerList();
}

void MainWindow::on_server_select_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->login);
    settings.beginGroup(QString("%1-%2").arg(serverConnexion[selectedServer]->host).arg(serverConnexion[selectedServer]->port));
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
    ui->stackedWidget->setCurrentWidget(ui->page);
}

void MainWindow::on_server_remove_clicked()
{
}

void MainWindow::on_server_refresh_clicked()
{
}

void MainWindow::saveConnexionInfoList()
{
    QStringList connexionList;
    QStringList nameList;
    QStringList connexionCounterList;
    QStringList lastConnexionList;
    int index=0;
    while(index<connexionInfoList.size())
    {
        const ConnexionInfo &connexionInfo=connexionInfoList.at(index);
        connexionList << QString("%1:%2").arg(connexionInfo.host).arg(connexionInfo.port);
        nameList << connexionInfo.name;
        connexionCounterList << QString::number(connexionInfo.connexionCounter);
        lastConnexionList << QString::number(connexionInfo.lastConnexion);
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
    CatchChallenger::Api_client_real::client->resetAll();
    CatchChallenger::BaseWindow::baseWindow->resetAll();
    ui->stackedWidget->setCurrentWidget(ui->page);
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
    settings.beginGroup(QString("%1-%2").arg(serverConnexion[selectedServer]->host).arg(serverConnexion[selectedServer]->port));
    settings.setValue("login",ui->lineEditLogin->text());
    if(ui->checkBoxRememberPassword->isChecked())
        settings.setValue("pass",ui->lineEditPass->text());
    else
        settings.remove("pass");
    settings.endGroup();
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
    ui->deleteDatapack->setEnabled(selectedDatapack!=NULL);
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
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute("lang"))
            {
                if(item.attribute("lang")=="en")
                {
                    returnVar.second=item.text();
                    break;
                }
            }
            else
            {
                returnVar.second=item.text();
                break;
            }
        }
        item = item.nextSiblingElement("name");
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
    ui->datapackEmpty->setVisible(index==0);
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
    ui->stackedWidget->setCurrentWidget(ui->page);
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
