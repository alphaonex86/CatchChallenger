#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "NewGame.h"
#include "NewProfile.h"

#include <QSettings>
#include <QInputDialog>

#include "../base/render/MapVisualiserPlayer.h"
#include "../base/FacilityLib.h"
#include "../base/interface/DatapackClientLoader.h"
#include "../fight/interface/FightEngine.h"
#include "../base/DebugClass.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");
    qRegisterMetaType<CatchChallenger::Chat_type>("CatchChallenger::Chat_type");
    qRegisterMetaType<CatchChallenger::Player_type>("CatchChallenger::Player_type");
    qRegisterMetaType<CatchChallenger::Player_type>("QAbstractSocket::SocketState");
    qRegisterMetaType<CatchChallenger::Player_private_and_public_informations>("CatchChallenger::Player_private_and_public_informations");
    qRegisterMetaType<CatchChallenger::Player_public_informations>("CatchChallenger::Player_public_informations");
    qRegisterMetaType<CatchChallenger::Direction>("CatchChallenger::Direction");

    socket=new CatchChallenger::ConnectedSocket(new CatchChallenger::QFakeSocket());
    CatchChallenger::Api_client_real::client=new CatchChallenger::Api_client_virtual(socket);
    CatchChallenger::BaseWindow::baseWindow=new CatchChallenger::BaseWindow();
    spacer=new QSpacerItem(0,0,QSizePolicy::Expanding,QSizePolicy::Expanding);
    ui->setupUi(this);
    ui->stackedWidget->addWidget(CatchChallenger::BaseWindow::baseWindow);
    selectedSavegame=NULL;
    internalServer=NULL;
    datapackPath=QCoreApplication::applicationDirPath()+"/datapack/";
    savegamePath=QCoreApplication::applicationDirPath()+"/savegames/";
    datapackPathExists=QDir(datapackPath).exists();

    connect(CatchChallenger::Api_client_real::client,SIGNAL(protocol_is_good()),this,SLOT(protocol_is_good()),Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,SIGNAL(disconnected(QString)),this,SLOT(disconnected(QString)),Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,SIGNAL(message(QString)),this,SLOT(message(QString)),Qt::QueuedConnection);
    connect(socket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(error(QAbstractSocket::SocketError)),Qt::QueuedConnection);
    connect(socket,SIGNAL(stateChanged(QAbstractSocket::SocketState)),this,SLOT(stateChanged(QAbstractSocket::SocketState)),Qt::QueuedConnection);
    connect(socket,SIGNAL(stateChanged(QAbstractSocket::SocketState)),CatchChallenger::BaseWindow::baseWindow,SLOT(stateChanged(QAbstractSocket::SocketState)),Qt::QueuedConnection);

    ui->stackedWidget->addWidget(CatchChallenger::BaseWindow::baseWindow);
    CatchChallenger::BaseWindow::baseWindow->setMultiPlayer(false);

    stateChanged(QAbstractSocket::UnconnectedState);

    /*    ui->stackedWidget->setCurrentIndex(1);
    client->tryConnect(host,port);*/
    updateSavegameList();

    setWindowTitle("CatchChallenger - "+tr("Single player"));
    ui->SaveGame_New->setEnabled(datapackPathExists);
}

MainWindow::~MainWindow()
{
    saveTime();
    socket->disconnectFromHost();
    if(internalServer!=NULL)
        delete internalServer;
    internalServer=NULL;
    delete CatchChallenger::Api_client_real::client;
    delete CatchChallenger::BaseWindow::baseWindow;
    delete ui;
    socket->deleteLater();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    qDebug() << "close event";
    event->ignore();
    this->hide();
    socket->disconnectFromHost();
    if(internalServer==NULL)
        QCoreApplication::quit();
    else
        qDebug() << "server is running, need wait to close";
}

void MainWindow::resetAll()
{
    if(internalServer!=NULL)
        internalServer->stop();
    CatchChallenger::Api_client_real::client->resetAll();
    CatchChallenger::BaseWindow::baseWindow->resetAll();
    ui->stackedWidget->setCurrentIndex(0);
    lastMessageSend="";
/*    if(internalServer!=NULL)
        internalServer->deleteLater();
    internalServer=NULL;*/
    pass.clear();
    saveTime();

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
    CatchChallenger::BaseWindow::baseWindow->stateChanged(socketState);
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
    CatchChallenger::Api_client_real::client->tryLogin("admin",pass);
    timeLaunched=QDateTime::currentDateTimeUtc().toTime_t();
}

void MainWindow::try_stop_server()
{
    if(internalServer!=NULL)
        delete internalServer;
    internalServer=NULL;
    saveTime();
}

void MainWindow::on_SaveGame_New_clicked()
{
    resetAll();
    //load the information
    NewProfile newProfile(datapackPath,this);
    newProfile.exec();
    if(!newProfile.ok)
        return;
    NewProfile::Profile profile=newProfile.getProfile();
    NewGame nameGame(profile.forcedskin,this);
    if(!nameGame.haveSkin())
    {
        QMessageBox::critical(this,tr("Error"),QString("Sorry but no skin found into: %1").arg(QFileInfo(datapackPath+DATAPACK_BASE_PATH_SKIN).absoluteFilePath()));
        return;
    }
    nameGame.exec();
    if(!nameGame.haveTheInformation())
        return;
    int index=0;

    //locate the new folder and create it
    while(QDir().exists(savegamePath+QString::number(index)))
        index++;
    QString savegamesPath=savegamePath+QString::number(index)+"/";
    if(!QDir().mkpath(savegamesPath))
    {
        QMessageBox::critical(this,tr("Error"),QString("Unable to write savegame into: %1").arg(savegamesPath));
        return;
    }

    //initialize the db
    QFile dbSource(":/catchchallenger.db.sqlite");
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
    QFile dbDestination(savegamesPath+"catchchallenger.db.sqlite");
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
    QString pass=CatchChallenger::FacilityLib::randomPassword("abcdefghijklmnopqurstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890",16);
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
    db.setDatabaseName(savegamesPath+"catchchallenger.db.sqlite");
    if(!db.open())
    {
        QMessageBox::critical(this,tr("Error"),QString("Unable to initialize the savegame\nError: open database: %1").arg(db.lastError().text()));
        rmpath(savegamesPath);
        return;
    }


    //empty the player db and put the new player into it
    QSqlQuery sqlQuery;
    int player_id,size;
    do
    {
        player_id=rand();
        if(!sqlQuery.exec(QString("SELECT * FROM \"player\" WHERE id=%1").arg(player_id)))
        {
            db.close();
            QMessageBox::critical(this,tr("Error"),QString("Unable to initialize the savegame\nerror: initialize the entry: %1\n%2").arg(sqlQuery.lastError().text()).arg(sqlQuery.lastQuery()));
            rmpath(savegamesPath);
            return;
        }
        size=sqlQuery.size();
    } while(size>0);
    if(!sqlQuery.exec(
           QString("INSERT INTO \"player\"(\"id\",\"login\",\"password\",\"pseudo\",\"skin\",\"position_x\",\"position_y\",\"orientation\",\"map_name\",\"type\",\"clan\",\"cash\",\"rescue_map\",\"rescue_x\",\"rescue_y\",\"rescue_orientation\") VALUES(%1,'admin','%2','%3','%4',%5,%6,'bottom','%7','normal',NULL,%8,%9);")
           .arg(player_id)
           .arg(QString(passHash.toHex()))
           .arg(nameGame.pseudo())
           .arg(nameGame.skin())
           .arg(profile.x)
           .arg(profile.y)
           .arg(profile.map)
           .arg(profile.cash)
           .arg(QString("'%1',%2,%3,'bottom'").arg(profile.map).arg(profile.x).arg(profile.y))
                ))
    {
        db.close();
        QMessageBox::critical(this,tr("Error"),QString("Unable to initialize the savegame\nerror: initialize the entry: %1\n%2").arg(sqlQuery.lastError().text()).arg(sqlQuery.lastQuery()));
        rmpath(savegamesPath);
        return;
    }
    index=0;
    while(index<profile.monsters.size())
    {
        int monster_id;
        do
        {
            monster_id=rand();
            if(!sqlQuery.exec(QString("SELECT * FROM \"monster\" WHERE id=%1").arg(monster_id)))
            {
                db.close();
                QMessageBox::critical(this,tr("Error"),QString("Unable to initialize the savegame\nerror: initialize the entry: %1\n%2").arg(sqlQuery.lastError().text()).arg(sqlQuery.lastQuery()));
                rmpath(savegamesPath);
                return;
            }
            size=sqlQuery.size();
        } while(size>0);
        QString gender="unknown";
        if(CatchChallenger::FightEngine::fightEngine.monsters[profile.monsters.at(index).id].ratio_gender!=-1)
        {
            if(rand()%101<CatchChallenger::FightEngine::fightEngine.monsters[profile.monsters.at(index).id].ratio_gender)
                gender="female";
            else
                gender="male";
        }
        CatchChallenger::Monster::Stat stat=CatchChallenger::FightEngine::fightEngine.getStat(CatchChallenger::FightEngine::fightEngine.monsters[profile.monsters.at(index).id],profile.monsters.at(index).level);
        QList<CatchChallenger::PlayerMonster::PlayerSkill> skills;
        QList<CatchChallenger::Monster::AttackToLearn> attack=CatchChallenger::FightEngine::fightEngine.monsters[profile.monsters.at(index).id].learn;
        int sub_index=0;
        while(sub_index<attack.size())
        {
            if(attack[sub_index].learnAtLevel<=profile.monsters.at(index).level)
            {
                CatchChallenger::PlayerMonster::PlayerSkill temp;
                temp.level=attack[sub_index].learnSkillLevel;
                temp.skill=attack[sub_index].learnSkill;
                skills << temp;
            }
            sub_index++;
        }
        while(skills.size()>4)
            skills.removeFirst();
        if(!sqlQuery.exec(
               QString("INSERT INTO \"monster\"(\"id\",\"hp\",\"player\",\"monster\",\"level\",\"xp\",\"sp\",\"captured_with\",\"gender\",\"egg_step\",\"player_origin\") VALUES(%1,%2,%3,%4,%5,0,0,%6,\"%7\",0,%3);")
               .arg(monster_id)
               .arg(stat.hp)
               .arg(player_id)
               .arg(profile.monsters.at(index).id)
               .arg(profile.monsters.at(index).level)
               .arg(profile.monsters.at(index).captured_with)
               .arg(gender)
                    ))
        {
            db.close();
            QMessageBox::critical(this,tr("Error"),QString("Unable to initialize the savegame\nerror: initialize the entry: %1\n%2").arg(sqlQuery.lastError().text()).arg(sqlQuery.lastQuery()));
            rmpath(savegamesPath);
            return;
        }
        sub_index=0;
        while(sub_index<skills.size())
        {
            if(!sqlQuery.exec(
                   QString("INSERT INTO \"monster_skill\"(\"monster\",\"skill\",\"level\") VALUES(%1,%2,%3);")
                   .arg(monster_id)
                   .arg(skills[sub_index].skill)
                   .arg(skills[sub_index].level)
                        ))
            {
                db.close();
                QMessageBox::critical(this,tr("Error"),QString("Unable to initialize the savegame\nerror: initialize the entry: %1\n%2").arg(sqlQuery.lastError().text()).arg(sqlQuery.lastQuery()));
                rmpath(savegamesPath);
                return;
            }
            sub_index++;
        }
        index++;
    }
    index=0;
    while(index<profile.reputation.size())
    {
        if(!sqlQuery.exec(
               QString("INSERT INTO \"reputation\"(\"player\",\"type\",\"point\",\"level\") VALUES(%1,\"%2\",%3,%4);")
               .arg(player_id)
               .arg(profile.reputation.at(index).type)
               .arg(profile.reputation.at(index).point)
               .arg(profile.reputation.at(index).level)
                    ))
        {
            db.close();
            QMessageBox::critical(this,tr("Error"),QString("Unable to initialize the savegame\nerror: initialize the entry: %1\n%2").arg(sqlQuery.lastError().text()).arg(sqlQuery.lastQuery()));
            rmpath(savegamesPath);
            return;
        }
        index++;
    }
    index=0;
    while(index<profile.items.size())
    {
        if(!sqlQuery.exec(
               QString("INSERT INTO \"item\"(\"item_id\",\"player_id\",\"quantity\") VALUES(%1,%2,%3);")
               .arg(profile.items.at(index).id)
               .arg(player_id)
               .arg(profile.items.at(index).quantity)
                    ))
        {
            db.close();
            QMessageBox::critical(this,tr("Error"),QString("Unable to initialize the savegame\nerror: initialize the entry: %1\n%2").arg(sqlQuery.lastError().text()).arg(sqlQuery.lastQuery()));
            rmpath(savegamesPath);
            return;
        }
        index++;
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

void MainWindow::savegameLabelDoubleClicked()
{
    savegameLabelClicked();
    on_SaveGame_Play_clicked();
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
    if(!datapackPathExists)
    {
        ui->savegameEmpty->setText(QString("<html><head/><body><p align=\"center\"><span style=\"font-size:12pt;color:#a0a0a0;\">%1</span></p></body></html>").arg(tr("No datapack!")));
        return;
    }
    QString lastSelectedPath;
    if(selectedSavegame!=NULL)
        lastSelectedPath=savegamePathList[selectedSavegame];
    selectedSavegame=NULL;
    int index=0;
    while(savegame.size()>0)
    {
        delete savegame.at(0);
        savegame.removeAt(0);
        index++;
    }
    savegamePathList.clear();
    savegameWithMetaData.clear();
    QFileInfoList entryList=QDir(savegamePath).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    index=0;
    while(index<entryList.size())
    {
        bool ok=false;
        QFileInfo fileInfo=entryList.at(index);
        if(!fileInfo.isDir())
        {
            index++;
            continue;
        }
        QString savegamesPath=fileInfo.absoluteFilePath()+"/";
        QSettings metaData(savegamesPath+"metadata.conf",QSettings::IniFormat);
        SaveGameLabel *newEntry=new SaveGameLabel();
        connect(newEntry,SIGNAL(clicked()),this,SLOT(savegameLabelClicked()),Qt::QueuedConnection);
        connect(newEntry,SIGNAL(doubleClicked()),this,SLOT(savegameLabelDoubleClicked()),Qt::QueuedConnection);
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
                            time_played=QObject::tr("%n day(s) played","",time_played_number/(3600*24));
                        else if(time_played_number>=3600*24)
                            time_played=QObject::tr("%n day(s) and %1 played","",time_played_number/3600*24).arg(QObject::tr("%n hour(s)","",(time_played_number%(3600*24))/3600));
                        else if(time_played_number>=3600)
                            time_played=QObject::tr("%n hour(s) and %1 played","",time_played_number/3600).arg(QObject::tr("%n minute(s)","",(time_played_number%3600)/60));
                        else
                            time_played=QObject::tr("%n minute(s) and %1 played","",time_played_number/60).arg(QObject::tr("%n second(s)","",time_played_number%60));

                        //load the map name
                        QString map=metaData.value("location").toString();
                        map.replace(".tmx",".xml");
                        QString mapName=getMapName(datapackPath+DATAPACK_BASE_PATH_MAP+map);
                        if(mapName.isEmpty())
                        {
                            QString zone=getMapZone(datapackPath+DATAPACK_BASE_PATH_MAP+metaData.value("location").toString());
                            //try load the zone
                            if(!zone.isEmpty())
                                mapName=getZoneName(zone);
                        }
                        QString lastLine;
                        if(mapName.isEmpty())
                            lastLine=time_played;
                        else
                            lastLine=QString("%1 (%2)").arg(mapName).arg(time_played);

                        newEntry->setText(QString("<span style=\"font-size:12pt;font-weight:600;\">%1</span><br/><span style=\"color:#909090;\">%2<br/>%3</span>")
                                          .arg(metaData.value("title").toString())
                                          .arg(dateString)
                                          .arg(lastLine)
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
        savegamePathList[newEntry]=savegamesPath;
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

QString MainWindow::getMapName(const QString &file)
{
    //open and quick check the file
    QFile xmlFile(file);
    QByteArray xmlContent;
    if(!xmlFile.open(QIODevice::ReadOnly))
    {
        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, error: %2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
        return QString();
    }
    xmlContent=xmlFile.readAll();
    xmlFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(xmlFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr));
        return QString();
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="map")
    {
        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, \"plants\" root balise not found for the xml file").arg(xmlFile.fileName()));
        return QString();
    }
    QDomElement item = root.firstChildElement("name");
    while(!item.isNull())
    {
        if(item.isElement())
            if(item.hasAttribute("lang"))
                if(item.attribute("lang")=="en")
                    return item.text();
        item = item.nextSiblingElement("name");
    }
    return QString();
}

QString MainWindow::getMapZone(const QString &file)
{
    //open and quick check the file
    QFile xmlFile(file);
    QByteArray xmlContent;
    if(!xmlFile.open(QIODevice::ReadOnly))
    {
        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, error: %2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
        return QString();
    }
    xmlContent=xmlFile.readAll();
    xmlFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(xmlFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr));
        return QString();
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="map")
    {
        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, \"plants\" root balise not found for the xml file").arg(xmlFile.fileName()));
        return QString();
    }
    QDomElement properties = root.firstChildElement("properties");
    while(!properties.isNull())
    {
        if(properties.isElement())
        {
            QDomElement property = properties.firstChildElement("property");
            while(!property.isNull())
            {
                if(property.isElement())
                    if(property.hasAttribute("name") && property.hasAttribute("value"))
                        if(property.attribute("name")=="zone")
                            return property.attribute("value");
                property = property.nextSiblingElement("property");
            }
        }
        properties = properties.nextSiblingElement("properties");
    }
    return QString();
}

QString MainWindow::getZoneName(const QString &zone)
{
    //open and quick check the file
    QFile xmlFile(datapackPath+DATAPACK_BASE_PATH_ZONE+zone+".xml");
    QByteArray xmlContent;
    if(!xmlFile.open(QIODevice::ReadOnly))
    {
        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, error: %2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
        return QString();
    }
    xmlContent=xmlFile.readAll();
    xmlFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(xmlFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr));
        return QString();
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="zone")
    {
        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, \"plants\" root balise not found for the xml file").arg(xmlFile.fileName()));
        return QString();
    }

    //load the content
    QDomElement item = root.firstChildElement("name");
    while(!item.isNull())
    {
        if(item.isElement())
            if(item.hasAttribute("lang"))
                if(item.attribute("lang")=="en")
                    return item.text();
        item = item.nextSiblingElement("name");
    }
    return QString();
}

void MainWindow::on_SaveGame_Delete_clicked()
{
    if(selectedSavegame==NULL)
        return;

    if(!rmpath(savegamePathList[selectedSavegame]))
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

    QString savegamesPath=savegamePathList[selectedSavegame];
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

    QString savegamesPath=savegamePathList[selectedSavegame];
    if(!savegameWithMetaData[selectedSavegame])
        return;
    int index=0;
    while(QDir(savegamePath+QString::number(index)+"/").exists())
        index++;
    QString destinationPath=savegamePath+QString::number(index)+"/";
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
    if(!QFile::copy(savegamesPath+"catchchallenger.db.sqlite",destinationPath+"catchchallenger.db.sqlite"))
    {
        rmpath(destinationPath);
        QMessageBox::critical(this,tr("Error"),QString("Unable to write another savegame (Error: catchchallenger.db.sqlite)"));
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

    QString savegamesPath=savegamePathList[selectedSavegame];
    if(!savegameWithMetaData[selectedSavegame])
        return;

    play(savegamesPath);
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
    }
    else
    {
        CatchChallenger::BaseWindow::baseWindow->serverIsReady();
        socket->connectToHost("localhost",9999);
    }
}

void MainWindow::saveTime()
{
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
                QString mapPath=datapackPath+DATAPACK_BASE_PATH_MAP;
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
        haveLaunchedGame=false;
    }
}

void MainWindow::play(const QString &savegamesPath)
{
    resetAll();
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
        delete internalServer;
        saveTime();
    }
    internalServer=new CatchChallenger::InternalServer();
    sendSettings(internalServer,savegamesPath);
    connect(internalServer,SIGNAL(is_started(bool)),this,SLOT(is_started(bool)),Qt::QueuedConnection);

    ui->stackedWidget->setCurrentIndex(1);
    CatchChallenger::BaseWindow::baseWindow->serverIsLoading();
}

void MainWindow::sendSettings(CatchChallenger::InternalServer * internalServer,const QString &savegamesPath)
{
    CatchChallenger::ServerSettings formatedServerSettings;

    formatedServerSettings.max_players=1;
    formatedServerSettings.tolerantMode=false;
    formatedServerSettings.commmonServerSettings.sendPlayerNumber = false;

    formatedServerSettings.database.type=CatchChallenger::ServerSettings::Database::DatabaseType_SQLite;
    formatedServerSettings.database.sqlite.file=savegamesPath+"catchchallenger.db.sqlite";
    formatedServerSettings.mapVisibility.mapVisibilityAlgorithm	= CatchChallenger::MapVisibilityAlgorithm_none;

    internalServer->setSettings(formatedServerSettings);
}
