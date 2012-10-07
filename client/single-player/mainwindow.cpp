#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "NewGame.h"

#include <QSettings>
#include <QInputDialog>

#include "../base/render/MapVisualiserPlayer.h"
#include "../base/FacilityLib.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");
    qRegisterMetaType<Pokecraft::Chat_type>("Pokecraft::Chat_type");
    qRegisterMetaType<Pokecraft::Player_type>("Pokecraft::Player_type");
    qRegisterMetaType<Pokecraft::Player_type>("QAbstractSocket::SocketState");
    qRegisterMetaType<Pokecraft::Player_private_and_public_informations>("Pokecraft::Player_private_and_public_informations");
    qRegisterMetaType<Pokecraft::Player_public_informations>("Pokecraft::Player_public_informations");
    qRegisterMetaType<Pokecraft::Direction>("Pokecraft::Direction");

    socket=new Pokecraft::ConnectedSocket(new Pokecraft::QFakeSocket());
    client=new Pokecraft::Api_client_virtual(socket);
    baseWindow=new Pokecraft::BaseWindow(client);
    spacer=new QSpacerItem(0,0,QSizePolicy::Expanding,QSizePolicy::Expanding);
    ui->setupUi(this);
    ui->stackedWidget->addWidget(baseWindow);
    selectedSavegame=NULL;
    internalServer=NULL;

    connect(client,SIGNAL(protocol_is_good()),this,SLOT(protocol_is_good()),Qt::QueuedConnection);
    connect(client,SIGNAL(disconnected(QString)),this,SLOT(disconnected(QString)),Qt::QueuedConnection);
    connect(client,SIGNAL(message(QString)),this,SLOT(message(QString)),Qt::QueuedConnection);
    connect(socket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(error(QAbstractSocket::SocketError)),Qt::QueuedConnection);
    connect(socket,SIGNAL(stateChanged(QAbstractSocket::SocketState)),this,SLOT(stateChanged(QAbstractSocket::SocketState)),Qt::QueuedConnection);
    connect(socket,SIGNAL(stateChanged(QAbstractSocket::SocketState)),baseWindow,SLOT(stateChanged(QAbstractSocket::SocketState)),Qt::QueuedConnection);

    ui->stackedWidget->addWidget(baseWindow);
    baseWindow->setMultiPlayer(false);

    stateChanged(QAbstractSocket::UnconnectedState);

    /*    ui->stackedWidget->setCurrentIndex(1);
    client->tryConnect(host,port);*/
    updateSavegameList();
}

MainWindow::~MainWindow()
{
    needQuit();
    socket->disconnectFromHost();
    if(internalServer!=NULL)
        internalServer->deleteLater();
    delete client;
    delete baseWindow;
    delete ui;
    socket->deleteLater();
}

void MainWindow::resetAll()
{
    client->resetAll();
    baseWindow->resetAll();
    ui->stackedWidget->setCurrentIndex(0);
    lastMessageSend="";
    if(internalServer!=NULL)
        internalServer->deleteLater();
    internalServer=NULL;
    pass.clear();
    needQuit();

    //stateChanged(QAbstractSocket::UnconnectedState);//don't call here, else infinity rescursive call
}

void MainWindow::disconnected(QString)
{
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

void MainWindow::stateChanged(QAbstractSocket::SocketState socketState)
{
    if(socketState==QAbstractSocket::UnconnectedState)
        resetAll();
    baseWindow->stateChanged(socketState);
}

void MainWindow::error(QAbstractSocket::SocketError socketError)
{
    resetAll();
    switch(socketError)
    {
    case QAbstractSocket::RemoteHostClosedError:
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
    client->tryLogin("admin",pass);
    timeLaunched=QDateTime::currentDateTimeUtc().toTime_t();
}

void MainWindow::try_stop_server()
{
    socket->disconnectFromHost();
    if(internalServer!=NULL)
        internalServer->deleteLater();
    internalServer=NULL;
    needQuit();
}

void MainWindow::on_SaveGame_New_clicked()
{
    //load the information
    NewGame nameGame(this);
    if(!nameGame.haveSkin())
    {
        QMessageBox::critical(this,tr("Error"),QString("Sorry but no skin found into: %1").arg(QFileInfo(QCoreApplication::applicationDirPath()+"/datapack/"+DATAPACK_BASE_PATH_SKIN).absoluteFilePath()));
        return;
    }
    nameGame.exec();
    if(!nameGame.haveTheInformation())
        return;
    int index=0;

    //locate the new folder and create it
    while(QDir().exists(QCoreApplication::applicationDirPath()+"/savegames/"+QString::number(index)))
        index++;
    QString savegamesPath=QCoreApplication::applicationDirPath()+"/savegames/"+QString::number(index)+"/";
    if(!QDir().mkpath(savegamesPath))
    {
        QMessageBox::critical(this,tr("Error"),QString("Unable to write savegame into: %1").arg(savegamesPath));
        return;
    }

    //initialize the db
    QFile dbSource(":/pokecraft.db.sqlite");
    if(!dbSource.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(this,tr("Error"),QString("Unable to open the db model: %1").arg(savegamesPath));
        rmpath(savegamesPath);
        return;
    }
    QByteArray dbData=dbSource.readAll();
    if(dbData.isEmpty())
    {
        QMessageBox::critical(this,tr("Error"),QString("Unable to read the db model: %1").arg(savegamesPath));
        rmpath(savegamesPath);
        return;
    }
    dbSource.close();
    QFile dbDestination(savegamesPath+"pokecraft.db.sqlite");
    if(!dbDestination.open(QIODevice::WriteOnly))
    {
        QMessageBox::critical(this,tr("Error"),QString("Unable to write savegame into: %1").arg(savegamesPath));
        rmpath(savegamesPath);
        return;
    }
    if(!dbDestination.write(dbData))
    {
        dbDestination.close();
        QMessageBox::critical(this,tr("Error"),QString("Unable to write savegame into: %1").arg(savegamesPath));
        rmpath(savegamesPath);
        return;
    }
    dbDestination.close();

    //initialise the pass
    QString pass=Pokecraft::FacilityLib::randomPassword("abcdefghijklmnopqurstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890",16);
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(pass.toUtf8());
    QByteArray passHash=hash.result();

    //initialise the meta data
    bool settingOk=false;
    {
        QSettings metaData(savegamesPath+"metadata.conf",QSettings::IniFormat);
        if(metaData.isWritable())
        {
            if(metaData.status()==QSettings::NoError)
            {
                metaData.setValue("title",nameGame.gameName());
                metaData.setValue("location","Starting city");
                metaData.setValue("time_played",0);
                metaData.setValue("pass",pass);
                settingOk=true;
            }
            else
                qDebug() << "Settings error: " << metaData.status();
        }
    }

    //check if meta data is ok, and db can be open
    if(!settingOk)
    {
        QMessageBox::critical(this,tr("Error"),QString("Unable to write savegame into: %1").arg(savegamesPath));
        rmpath(savegamesPath);
        return;
    }
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(savegamesPath+"pokecraft.db.sqlite");
    if(!db.open())
    {
        QMessageBox::critical(this,tr("Error"),QString("Unable to initialize the savegame\nError: open database: %1").arg(db.lastError().text()));
        rmpath(savegamesPath);
        return;
    }


    //empty the player db and put the new player into it
    QSqlQuery sqlQuery;
    if(!sqlQuery.exec(QString("DELETE FROM \"player\"")))
    {
        db.close();
        QMessageBox::critical(this,tr("Error"),QString("Unable to initialize the savegame (error: clean the table: %1)").arg(sqlQuery.lastError().text()));
        rmpath(savegamesPath);
        return;
    }
    if(!sqlQuery.exec(QString("INSERT INTO \"player\" VALUES(NULL,'admin','%1','%2','%3',1,1,'bottom','world/0.0.tmx','normal',NULL);").arg(QString(passHash.toHex())).arg(nameGame.pseudo()).arg(nameGame.skin())))
    {
        db.close();
        QMessageBox::critical(this,tr("Error"),QString("Unable to initialize the savegame (error: initialize the entry: %1)").arg(sqlQuery.lastError().text()));
        rmpath(savegamesPath);
        return;
    }
    db.close();

    updateSavegameList();

    play(savegamesPath);
}

void MainWindow::savegameLabelClicked()
{
    SaveGameLabel * selectedSavegame=qobject_cast<SaveGameLabel *>(QObject::sender());
    if(selectedSavegame==NULL)
        return;
    this->selectedSavegame=selectedSavegame;
    savegameLabelUpdate();
}

void MainWindow::savegameLabelUpdate()
{
    int index=0;
    while(index<savegame.size())
    {
        if(savegame.at(index)==selectedSavegame)
            savegame.at(index)->setStyleSheet("QLabel{border:1px solid #6b6;background-color:rgb(100,180,100,120);border-radius:10px;}QLabel::hover{border:1px solid #494;background-color:rgb(70,150,70,120);border-radius:10px;}");
        else
            savegame.at(index)->setStyleSheet("QLabel::hover{border:1px solid #bbb;background-color:rgb(180,180,180,100);border-radius:10px;}");
        index++;
    }
    ui->SaveGame_Play->setEnabled(selectedSavegame!=NULL && savegameWithMetaData[selectedSavegame]);
    ui->SaveGame_Rename->setEnabled(selectedSavegame!=NULL && savegameWithMetaData[selectedSavegame]);
    ui->SaveGame_Copy->setEnabled(selectedSavegame!=NULL && savegameWithMetaData[selectedSavegame]);
    ui->SaveGame_Delete->setEnabled(selectedSavegame!=NULL);
}

bool MainWindow::rmpath(const QDir &dir)
{
    if(!dir.exists())
        return true;
    bool allHaveWork=true;
    QFileInfoList list = dir.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);
    for (int i = 0; i < list.size(); ++i)
    {
        QFileInfo fileInfo(list.at(i));
        if(!fileInfo.isDir())
        {
            if(!QFile(fileInfo.absoluteFilePath()).remove())
            {
                qDebug() << "Unable the remove the file: " << fileInfo.absoluteFilePath();
                allHaveWork=false;
            }
        }
        else
        {
            //return the fonction for scan the new folder
            if(!rmpath(dir.absolutePath()+'/'+fileInfo.fileName()+'/'))
                allHaveWork=false;
        }
    }
    if(!allHaveWork)
        return allHaveWork;
    if(!dir.rmdir(dir.absolutePath()))
    {
        qDebug() << "Unable the remove the file: " << dir.absolutePath();
        return false;
    }
    return true;
}

void MainWindow::updateSavegameList()
{
    QString lastSelectedPath;
    if(selectedSavegame!=NULL)
        lastSelectedPath=savegamePath[selectedSavegame];
    selectedSavegame=NULL;
    int index=0;
    while(savegame.size()>0)
    {
        delete savegame.at(0);
        savegame.removeAt(0);
        index++;
    }
    savegamePath.clear();
    savegameWithMetaData.clear();
    index=0;
    while(QDir(QCoreApplication::applicationDirPath()+"/savegames/"+QString::number(index)+"/").exists())
    {
        bool ok=false;
        QString savegamesPath=QCoreApplication::applicationDirPath()+"/savegames/"+QString::number(index)+"/";
        QSettings metaData(savegamesPath+"metadata.conf",QSettings::IniFormat);
        SaveGameLabel *newEntry=new SaveGameLabel();
        connect(newEntry,SIGNAL(clicked()),this,SLOT(savegameLabelClicked()),Qt::QueuedConnection);
        newEntry->setStyleSheet("QLabel::hover{border:1px solid #bbb;background-color:rgb(180,180,180,100);border-radius:10px;}");
        QString dateString;
        if(!QFileInfo(savegamesPath+"metadata.conf").exists())
            newEntry->setText(QString("<span style=\"font-size:12pt;font-weight:600;\">Missing metadata</span><br/><span style=\"color:#909090;\">Missing metadata<br/>Bug</span>"));
        else
        {
            dateString=QFileInfo(savegamesPath+"metadata.conf").lastModified().toString("dd/MM/yyyy hh:mm:ssAP");
            if(!metaData.isWritable())
                newEntry->setText(QString("<span style=\"font-size:12pt;font-weight:600;\">%1</span><br/><span style=\"color:#909090;\">%2<br/>Bug</span>").arg("Bug").arg(dateString));
            else
            {
                if(metaData.status()==QSettings::NoError)
                {
                    if(metaData.contains("title") && metaData.contains("location") && metaData.contains("time_played") && metaData.contains("pass"))
                    {
                        int time_played_number=metaData.value("time_played").toUInt(&ok);
                        QString time_played;
                        if(!ok)
                            time_played="Time player: bug";
                        else if(time_played_number>=3600*24*10)
                            time_played=QObject::tr("%n day(s) played","",time_played_number/3600*24);
                        else if(time_played_number>=3600*24)
                            time_played=QObject::tr("%n day(s) and %1 played","",time_played_number/3600*24).arg(QObject::tr("%n hour(s)","",time_played_number%3600*24));
                        else if(time_played_number>=3600)
                            time_played=QObject::tr("%n hour(s) and %1 played","",time_played_number/3600).arg(QObject::tr("%n minute(s)","",time_played_number%3600));
                        else
                            time_played=QObject::tr("%n minute(s) and %1 played","",time_played_number/60).arg(QObject::tr("%n second(s)","",time_played_number%60));

                        //load the map name
                        QString mapName=metaData.value("location").toString();
                        Tiled::MapReader reader;
                        Tiled::Map * tiledMap = reader.readMap(QCoreApplication::applicationDirPath()+"/datapack/"+DATAPACK_BASE_PATH_MAP+metaData.value("location").toString());
                        if(tiledMap)
                        {
                            if(tiledMap->properties().contains("name"))
                                mapName=tiledMap->properties().value("name");
                            delete tiledMap;
                        }

                        newEntry->setText(QString("<span style=\"font-size:12pt;font-weight:600;\">%1</span><br/><span style=\"color:#909090;\">%2<br/>%3 (%4)</span>")
                                          .arg(metaData.value("title").toString())
                                          .arg(dateString)
                                          .arg(mapName)
                                          .arg(time_played)
                                          );
                    }
                }
                else
                    newEntry->setText(QString("<span style=\"font-size:12pt;font-weight:600;\">%1</span><br/><span style=\"color:#909090;\">%2<br/>Bug</span>").arg(metaData.value("title").toString()).arg(dateString));
            }
        }
        ui->scrollAreaWidgetContents->layout()->addWidget(newEntry);

        if(lastSelectedPath==savegamesPath)
            selectedSavegame=newEntry;
        savegame << newEntry;
        savegamePath[newEntry]=savegamesPath;
        savegameWithMetaData[newEntry]=ok;
        index++;
    }
    ui->savegameEmpty->setVisible(index==0);
    if(index>0)
    {
        ui->scrollAreaWidgetContents->layout()->removeItem(spacer);
        delete spacer;
        spacer=new QSpacerItem(0,0,QSizePolicy::Expanding,QSizePolicy::Expanding);
        ui->scrollAreaWidgetContents->layout()->addItem(spacer);
    }
    savegameLabelUpdate();
}

void MainWindow::on_SaveGame_Delete_clicked()
{
    if(selectedSavegame==NULL)
        return;

    if(!rmpath(savegamePath[selectedSavegame]))
    {
        QMessageBox::critical(this,tr("Error"),QString("Unable to remove the savegame"));
        return;
    }

    updateSavegameList();
}

void MainWindow::on_SaveGame_Rename_clicked()
{
    if(selectedSavegame==NULL)
        return;

    QString savegamesPath=savegamePath[selectedSavegame];
    if(!savegameWithMetaData[selectedSavegame])
        return;
    QSettings metaData(savegamesPath+"metadata.conf",QSettings::IniFormat);
    if(!QFileInfo(savegamesPath+"metadata.conf").exists())
    {
        QMessageBox::critical(this,tr("Error"),QString("No meta data file"));
        return;
    }
    QString newName=QInputDialog::getText(NULL,"New name","Write the new name",QLineEdit::Normal,metaData.value("title").toString());
    if(newName.isEmpty())
        return;
    metaData.setValue("title",newName);

    updateSavegameList();
}

void MainWindow::on_SaveGame_Copy_clicked()
{
    if(selectedSavegame==NULL)
        return;

    QString savegamesPath=savegamePath[selectedSavegame];
    if(!savegameWithMetaData[selectedSavegame])
        return;
    int index=0;
    while(QDir(QCoreApplication::applicationDirPath()+"/savegames/"+QString::number(index)+"/").exists())
        index++;
    QString destinationPath=QCoreApplication::applicationDirPath()+"/savegames/"+QString::number(index)+"/";
    if(!QDir().mkpath(destinationPath))
    {
        QMessageBox::critical(this,tr("Error"),QString("Unable to write another savegame"));
        return;
    }
    if(!QFile::copy(savegamesPath+"metadata.conf",destinationPath+"metadata.conf"))
    {
        rmpath(destinationPath);
        QMessageBox::critical(this,tr("Error"),QString("Unable to write another savegame (Error: metadata.conf)"));
        return;
    }
    if(!QFile::copy(savegamesPath+"pokecraft.db.sqlite",destinationPath+"pokecraft.db.sqlite"))
    {
        rmpath(destinationPath);
        QMessageBox::critical(this,tr("Error"),QString("Unable to write another savegame (Error: pokecraft.db.sqlite)"));
        return;
    }
    QSettings metaData(destinationPath+"metadata.conf",QSettings::IniFormat);
    metaData.setValue("title",tr("Copy of %1").arg(metaData.value("title").toString()));
    updateSavegameList();
}

void MainWindow::on_SaveGame_Play_clicked()
{
    if(selectedSavegame==NULL)
        return;

    QString savegamesPath=savegamePath[selectedSavegame];
    if(!savegameWithMetaData[selectedSavegame])
        return;

    play(savegamesPath);
}

void MainWindow::is_started(bool started)
{
    if(!started)
    {
        needQuit();
        return;
    }
    baseWindow->serverIsReady();
    socket->connectToHost("localhost",9999);
}

void MainWindow::needQuit()
{
    if(!haveLaunchedGame)
        return;
    haveLaunchedGame=false;

    bool settingOk=false;
    QSettings metaData(launchedGamePath+"metadata.conf",QSettings::IniFormat);
    if(metaData.isWritable())
    {
        if(metaData.status()==QSettings::NoError)
        {
            QString locaction=baseWindow->lastLocation();
            QString mapPath=QCoreApplication::applicationDirPath()+"/datapack/"+DATAPACK_BASE_PATH_MAP;
            if(locaction.startsWith(mapPath))
                locaction.remove(0,mapPath.size());
            if(!locaction.isEmpty())
                metaData.setValue("location",locaction);
            metaData.setValue("time_played",metaData.value("time_played").toUInt()+(QDateTime::currentDateTimeUtc().toTime_t()-timeLaunched));
            settingOk=true;
        }
        else
            qDebug() << "Settings error: " << metaData.status();
    }
    updateSavegameList();
    if(!settingOk)
    {
        QMessageBox::critical(NULL,tr("Error"),tr("Unable to save internal value at game stopping"));
        return;
    }
}

void MainWindow::play(const QString &savegamesPath)
{
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
    {
        internalServer->deleteLater();
        needQuit();
    }
    internalServer=new Pokecraft::InternalServer(savegamesPath+"pokecraft.db.sqlite");
    connect(internalServer,SIGNAL(try_stop_server()),this,SLOT(try_stop_server()),Qt::QueuedConnection);
    connect(internalServer,SIGNAL(is_started(bool)),this,SLOT(is_started(bool)),Qt::QueuedConnection);

    ui->stackedWidget->setCurrentIndex(1);
    baseWindow->serverIsLoading();
}
