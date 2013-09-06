#include "mainwindow.h"
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
    ui->setupUi(this);
    ui->stackedWidget->addWidget(CatchChallenger::BaseWindow::baseWindow);
    if(settings.contains("login"))
        ui->lineEditLogin->setText(settings.value("login").toString());
    if(settings.contains("pass"))
    {
        ui->lineEditPass->setText(settings.value("pass").toString());
        ui->checkBoxRememberPassword->setChecked(true);
    }
    if(settings.contains("server_list"))
        server_list=settings.value("server_list").toStringList();
    if(server_list.size()==0)
        server_list << static_cast<CatchChallenger::Api_client_real *>(CatchChallenger::Api_client_real::client)->getHost()+":"+QString::number(static_cast<CatchChallenger::Api_client_real *>(CatchChallenger::Api_client_real::client)->getPort());
    ui->comboBoxServerList->addItems(server_list);
    if(settings.contains("last_server"))
    {
        int index=ui->comboBoxServerList->findText(settings.value("last_server").toString());
        if(index!=-1)
            ui->comboBoxServerList->setCurrentIndex(index);
    }
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

    setWindowTitle("CatchChallenger - "+tr("Multi server"));
}

MainWindow::~MainWindow()
{
    CatchChallenger::Api_client_real::client->tryDisconnect();
    delete CatchChallenger::Api_client_real::client;
    delete CatchChallenger::BaseWindow::baseWindow;
    delete ui;
    delete socket;
}

void MainWindow::resetAll()
{
    CatchChallenger::Api_client_real::client->resetAll();
    CatchChallenger::BaseWindow::baseWindow->resetAll();
    ui->stackedWidget->setCurrentIndex(0);
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
    settings.setValue("login",ui->lineEditLogin->text());
    if(ui->checkBoxRememberPassword->isChecked())
        settings.setValue("pass",ui->lineEditPass->text());
    else
        settings.remove("pass");
    if(!ui->comboBoxServerList->currentText().contains(QRegularExpression("^[a-zA-Z0-9\\.\\-_]+:[0-9]{1,5}$")))
    {
        QMessageBox::warning(this,"Error","The server is not as form: [host]:[port]");
        return;
    }
    if(!server_list.contains(ui->comboBoxServerList->currentText()))
    {
        server_list.insert(0,ui->comboBoxServerList->currentText());
        settings.setValue("server_list",server_list);
    }
    settings.setValue("last_server",ui->comboBoxServerList->currentText());
    QString host=ui->comboBoxServerList->currentText();
    host.remove(QRegularExpression(":[0-9]{1,5}$"));
    QString port_string=ui->comboBoxServerList->currentText();
    port_string.remove(QRegularExpression("^.*:"));
    bool ok;
    quint16 port=port_string.toInt(&ok);
    if(!ok)
    {
        QMessageBox::warning(this,"Error","Wrong port number conversion");
        return;
    }

    ui->stackedWidget->setCurrentWidget(CatchChallenger::BaseWindow::baseWindow);
    static_cast<CatchChallenger::Api_client_real *>(CatchChallenger::Api_client_real::client)->tryConnect(host,port);
    MapController::mapController->setDatapackPath(CatchChallenger::Api_client_real::client->get_datapack_base_name());
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

void MainWindow::ListEntryEnvoluedDoubleClicked()
{
    on_deleteDatapack_clicked();
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
        ui->datapackEmpty->setText(QString("<html><head/><body><p align=\"center\"><span style=\"font-size:12pt;color:#a0a0a0;\">%1</span></p></body></html>").arg(tr("Empty")));
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
