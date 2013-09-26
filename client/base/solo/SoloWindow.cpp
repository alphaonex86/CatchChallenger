#include "SoloWindow.h"
#include "ui_solowindow.h"
#include "NewGame.h"

#include <QSettings>
#include <QInputDialog>

#include "../base/render/MapVisualiserPlayer.h"
#include "../../general/base/FacilityLib.h"
#include "../base/interface/DatapackClientLoader.h"
#include "../base/LanguagesSelect.h"
#include "../fight/interface/ClientFightEngine.h"
#include "../../general/base/DebugClass.h"
#include "../../general/base/CommonDatapack.h"

SoloWindow::SoloWindow(QWidget *parent,const QString &datapackPath,const QString &savegamePath,const bool &standAlone) :
    QMainWindow(parent),
    ui(new Ui::SoloWindow)
{
    ui->setupUi(this);
    /*socket=new CatchChallenger::ConnectedSocket(new CatchChallenger::QFakeSocket());
    CatchChallenger::Api_client_real::client=new CatchChallenger::Api_client_virtual(socket);
    CatchChallenger::BaseWindow::baseWindow=new CatchChallenger::BaseWindow();*/
    this->standAlone=standAlone;
    spacer=new QSpacerItem(0,0,QSizePolicy::Expanding,QSizePolicy::Expanding);
    selectedSavegame=NULL;
    this->datapackPath=datapackPath;//QCoreApplication::applicationDirPath()+"/datapack/";
    this->savegamePath=savegamePath;//QCoreApplication::applicationDirPath()+"/savegames/";
    datapackPathExists=QDir(datapackPath).exists();

    /*connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::protocol_is_good,this,&SoloWindow::protocol_is_good,Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_protocol::disconnected,this,&SoloWindow::disconnected,Qt::QueuedConnection);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::message,this,&SoloWindow::message,Qt::QueuedConnection);
    connect(socket,static_cast<void(CatchChallenger::ConnectedSocket::*)(QAbstractSocket::SocketError)>(&CatchChallenger::ConnectedSocket::error),this,&SoloWindow::error,Qt::QueuedConnection);
    connect(socket,&CatchChallenger::ConnectedSocket::stateChanged,this,&SoloWindow::stateChanged,Qt::QueuedConnection);
    connect(socket,&CatchChallenger::ConnectedSocket::stateChanged,CatchChallenger::BaseWindow::baseWindow,&CatchChallenger::BaseWindow::stateChanged,Qt::QueuedConnection);*/

    updateSavegameList();

    if(standAlone)
        ui->horizontalLayout_main->removeItem(ui->horizontalSpacer_Back);
    ui->SaveGame_Back->setVisible(!standAlone);
    ui->SaveGame_New->setEnabled(datapackPathExists);

    newProfile=NULL;
}

SoloWindow::~SoloWindow()
{
    //delete ui;
}

void SoloWindow::on_SaveGame_New_clicked()
{
    //load the information
    if(newProfile!=NULL)
        delete newProfile;
    newProfile=new NewProfile(datapackPath,this);
    connect(newProfile,&NewProfile::finished,this,&SoloWindow::NewProfile_finished);
}

void SoloWindow::NewProfile_finished()
{
    if(newProfile->profileListSize()>1)
        if(!newProfile->ok)
            return;
    newProfile->ok=false;
    NewProfile::Profile profile=newProfile->getProfile();
    newProfile->deleteLater();
    newProfile=NULL;
    NewGame nameGame(datapackPath+DATAPACK_BASE_PATH_SKIN,profile.forcedskin,this);
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
    QString pass=CatchChallenger::FacilityLib::randomPassword("abcdefghijklmnopqurstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890",32);
    QCryptographicHash hash(QCryptographicHash::Sha512);
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
                metaData.setValue("location","");
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

    /// \warning in pointer to remove the object and don't keep open the file
    QSqlDatabase *db = new QSqlDatabase();
    *db=QSqlDatabase::addDatabase("QSQLITE","init");
    db->setDatabaseName(savegamesPath+"catchchallenger.db.sqlite");
    if(!db->open())
    {
        QMessageBox::critical(this,tr("Error"),QString("Unable to initialize the savegame\nError: open database: %1").arg(db->lastError().text()));
        rmpath(savegamesPath);
        return;
    }

    //empty the player db and put the new player into it
    int player_id,size;
    do
    {
        QSqlQuery sqlQuery(*db);
        player_id=rand();
        if(!sqlQuery.exec(QString("SELECT * FROM \"player\" WHERE id=%1").arg(player_id)))
        {
            closeDb(db);
            db=NULL;
            QMessageBox::critical(this,tr("Error"),QString("Unable to initialize the savegame\nerror: initialize the entry: %1\n%2").arg(sqlQuery.lastError().text()).arg(sqlQuery.lastQuery()));
            rmpath(savegamesPath);
            return;
        }
        size=sqlQuery.size();
    } while(size>0);
    {
        QString textQuery=QString("INSERT INTO \"player\"(\"id\",\"login\",\"password\",\"pseudo\",\"skin\",\"position_x\",\"position_y\",\"orientation\",\"map_name\",\"type\",\"clan\",\"cash\",\"rescue_map\",\"rescue_x\",\"rescue_y\",\"rescue_orientation\",\"unvalidated_rescue_map\",\"unvalidated_rescue_x\",\"unvalidated_rescue_y\",\"unvalidated_rescue_orientation\",\"market_cash\",\"market_bitcoin\",\"date\",\"warehouse_cash\",\"allow\",\"clan_leader\",\"bitcoin_offset\") VALUES(%1,'admin','%2','%3','%4',%5,%6,'bottom','%7','normal',0,%8,%9,%9,0,0,"+QString::number(QDateTime::currentMSecsSinceEpoch()/1000)+",0,'',0,0);")
                .arg(player_id)
                .arg(QString(passHash.toHex()))
                .arg(nameGame.pseudo())
                .arg(nameGame.skin())
                .arg(profile.x)
                .arg(profile.y)
                .arg(profile.map)
                .arg(profile.cash)
                .arg(QString("'%1',%2,%3,'bottom'").arg(profile.map).arg(profile.x).arg(profile.y));
        QSqlQuery sqlQuery(*db);
        if(!sqlQuery.exec(textQuery))
        {
            closeDb(db);
            db=NULL;
            QMessageBox::critical(this,tr("Error"),QString("Unable to initialize the savegame\nerror: initialize the entry: %1\n%2").arg(sqlQuery.lastError().text()).arg(sqlQuery.lastQuery()));
            rmpath(savegamesPath);
            return;
        }
    }
    index=0;
    int monster_position=1;
    while(index<profile.monsters.size())
    {
        int monster_id=0;
        /* drop the random to do the correct increment
        do
        {
            monster_id=rand();
            QSqlQuery sqlQuery(*db);
            if(!sqlQuery.exec(QString("SELECT * FROM \"monster\" WHERE id=%1").arg(monster_id)))
            {
                closeDb(db);
                db=NULL;
                QMessageBox::critical(this,tr("Error"),QString("Unable to initialize the savegame\nerror: initialize the entry: %1\n%2").arg(sqlQuery.lastError().text()).arg(sqlQuery.lastQuery()));
                rmpath(savegamesPath);
                return;
            }
            size=sqlQuery.size();
        } while(size>0);*/
        do
        {
            monster_id++;
            QSqlQuery sqlQuery(*db);
            if(!sqlQuery.exec(QString("SELECT * FROM \"monster\" WHERE id=%1").arg(monster_id)))
            {
                closeDb(db);
                db=NULL;
                QMessageBox::critical(this,tr("Error"),QString("Unable to initialize the savegame\nerror: initialize the entry: %1\n%2").arg(sqlQuery.lastError().text()).arg(sqlQuery.lastQuery()));
                rmpath(savegamesPath);
                return;
            }
            size=sqlQuery.size();
        } while(size>0);
        QString gender="unknown";
        if(CatchChallenger::CommonDatapack::commonDatapack.monsters[profile.monsters.at(index).id].ratio_gender!=-1)
        {
            if(rand()%101<CatchChallenger::CommonDatapack::commonDatapack.monsters[profile.monsters.at(index).id].ratio_gender)
                gender="female";
            else
                gender="male";
        }
        CatchChallenger::Monster::Stat stat=CatchChallenger::ClientFightEngine::fightEngine.getStat(CatchChallenger::CommonDatapack::commonDatapack.monsters[profile.monsters.at(index).id],profile.monsters.at(index).level);
        QList<CatchChallenger::PlayerMonster::PlayerSkill> skills;
        QList<CatchChallenger::Monster::AttackToLearn> attack=CatchChallenger::CommonDatapack::commonDatapack.monsters[profile.monsters.at(index).id].learn;
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
        {
            QSqlQuery sqlQuery(*db);
            if(!sqlQuery.exec(
                   QString("INSERT INTO \"monster\"(\"id\",\"hp\",\"player\",\"monster\",\"level\",\"xp\",\"sp\",\"captured_with\",\"gender\",\"egg_step\",\"player_origin\",\"place\",\"position\",\"market_price\",\"market_bitcoin\") VALUES(%1,%2,%3,%4,%5,0,0,%6,\"%7\",0,%3,\"wear\",%8,0,0);")
                   .arg(monster_id)
                   .arg(stat.hp)
                   .arg(player_id)
                   .arg(profile.monsters.at(index).id)
                   .arg(profile.monsters.at(index).level)
                   .arg(profile.monsters.at(index).captured_with)
                   .arg(gender)
                   .arg(monster_position)
                        ))
            {
                closeDb(db);
                db=NULL;
                QMessageBox::critical(this,tr("Error"),QString("Unable to initialize the savegame\nerror: initialize the entry: %1\n%2").arg(sqlQuery.lastError().text()).arg(sqlQuery.lastQuery()));
                rmpath(savegamesPath);
                return;
            }
            monster_position++;
        }
        sub_index=0;
        while(sub_index<skills.size())
        {
            quint8 endurance=0;
            if(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.contains(skills[sub_index].skill))
                if(skills[sub_index].level<=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills[skills[sub_index].skill].level.size() && skills[sub_index].level>0)
                    endurance=CatchChallenger::CommonDatapack::commonDatapack.monsterSkills[skills[sub_index].skill].level.at(skills[sub_index].level-1).endurance;
            QSqlQuery sqlQuery(*db);
            if(!sqlQuery.exec(
                   QString("INSERT INTO \"monster_skill\"(\"monster\",\"skill\",\"level\",\"endurance\") VALUES(%1,%2,%3,%4);")
                   .arg(monster_id)
                   .arg(skills[sub_index].skill)
                   .arg(skills[sub_index].level)
                   .arg(endurance)
                        ))
            {
                closeDb(db);
                db=NULL;
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
        QSqlQuery sqlQuery(*db);
        if(!sqlQuery.exec(
               QString("INSERT INTO \"reputation\"(\"player\",\"type\",\"point\",\"level\") VALUES(%1,\"%2\",%3,%4);")
               .arg(player_id)
               .arg(profile.reputation.at(index).type)
               .arg(profile.reputation.at(index).point)
               .arg(profile.reputation.at(index).level)
                    ))
        {
            closeDb(db);
            db=NULL;
            QMessageBox::critical(this,tr("Error"),QString("Unable to initialize the savegame\nerror: initialize the entry: %1\n%2").arg(sqlQuery.lastError().text()).arg(sqlQuery.lastQuery()));
            rmpath(savegamesPath);
            return;
        }
        index++;
    }
    index=0;
    while(index<profile.items.size())
    {
        QSqlQuery sqlQuery(*db);
        if(!sqlQuery.exec(
               QString("INSERT INTO \"item\"(\"item_id\",\"player_id\",\"quantity\",\"place\") VALUES(%1,%2,%3,\"wear\");")
               .arg(profile.items.at(index).id)
               .arg(player_id)
               .arg(profile.items.at(index).quantity)
                    ))
        {
            closeDb(db);
            db=NULL;
            QMessageBox::critical(this,tr("Error"),QString("Unable to initialize the savegame\nerror: initialize the entry: %1\n%2").arg(sqlQuery.lastError().text()).arg(sqlQuery.lastQuery()));
            rmpath(savegamesPath);
            return;
        }
        index++;
    }

    closeDb(db);
    db=NULL;

    updateSavegameList();

    emit play(savegamesPath);
}

void SoloWindow::closeDb(QSqlDatabase *db)
{
    db->commit();
    db->close();
    QString connectionName=db->connectionName();
    delete db;
    QSqlDatabase::removeDatabase(connectionName);
}

void SoloWindow::SoloWindowListEntryEnvoluedClicked()
{
    ListEntryEnvolued * selectedSavegame=qobject_cast<ListEntryEnvolued *>(QObject::sender());
    if(selectedSavegame==NULL)
        return;
    this->selectedSavegame=selectedSavegame;
    SoloWindowListEntryEnvoluedUpdate();
}

void SoloWindow::SoloWindowListEntryEnvoluedDoubleClicked()
{
    SoloWindowListEntryEnvoluedClicked();
    on_SaveGame_Play_clicked();
}

void SoloWindow::SoloWindowListEntryEnvoluedUpdate()
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

bool SoloWindow::rmpath(const QDir &dir)
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

void SoloWindow::updateSavegameList()
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
        ListEntryEnvolued *newEntry=new ListEntryEnvolued();
        connect(newEntry,&ListEntryEnvolued::clicked,this,&SoloWindow::SoloWindowListEntryEnvoluedClicked,Qt::QueuedConnection);
        connect(newEntry,&ListEntryEnvolued::doubleClicked,this,&SoloWindow::SoloWindowListEntryEnvoluedDoubleClicked,Qt::QueuedConnection);
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
                        if(!ok || time_played_number>3600*24*365*50)
                            time_played="Time player: bug";
                        else if(time_played_number>=3600*24*10)
                            time_played=QObject::tr("%n day(s) played","",time_played_number/(3600*24));
                        else if(time_played_number>=3600*24)
                            time_played=QObject::tr("%n day(s) and %1 played","",time_played_number/(3600*24)).arg(QObject::tr("%n hour(s)","",(time_played_number%(3600*24))/3600));
                        else if(time_played_number>=3600)
                            time_played=QObject::tr("%n hour(s) and %1 played","",time_played_number/3600).arg(QObject::tr("%n minute(s)","",(time_played_number%3600)/60));
                        else
                            time_played=QObject::tr("%n minute(s) and %1 played","",time_played_number/60).arg(QObject::tr("%n second(s)","",time_played_number%60));

                        //load the map name
                        QString mapName;
                        QString map=metaData.value("location").toString();
                        if(!map.isEmpty())
                        {
                            map.replace(".tmx",".xml");
                            if(QFileInfo(datapackPath+DATAPACK_BASE_PATH_MAP+map).isFile())
                                mapName=getMapName(datapackPath+DATAPACK_BASE_PATH_MAP+map);
                            if(mapName.isEmpty())
                            {
                                QString tmxFile=datapackPath+DATAPACK_BASE_PATH_MAP+metaData.value("location").toString();
                                if(QFileInfo(tmxFile).isFile())
                                {
                                    QString zone=getMapZone(tmxFile);
                                    //try load the zone
                                    if(!zone.isEmpty())
                                        mapName=getZoneName(zone);
                                }
                            }
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
    SoloWindowListEntryEnvoluedUpdate();
}

QString SoloWindow::getMapName(const QString &file)
{
    //open and quick check the file
    QFile xmlFile(file);
    QByteArray xmlContent;
    if(!xmlFile.open(QIODevice::ReadOnly))
    {
        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file to get map name: %1, error: %2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
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
    const QString &language=LanguagesSelect::languagesSelect.getCurrentLanguages();
    QDomElement item = root.firstChildElement("name");
    if(!language.isEmpty() && language!="en")
        while(!item.isNull())
        {
            if(item.isElement())
                if(item.hasAttribute("lang") && item.attribute("lang")==language)
                    return item.text();
            item = item.nextSiblingElement("name");
        }
    item = root.firstChildElement("name");
    while(!item.isNull())
    {
        if(item.isElement())
            if(!item.hasAttribute("lang") || item.attribute("lang")=="en")
                return item.text();
        item = item.nextSiblingElement("name");
    }
    return QString();
}

QString SoloWindow::getMapZone(const QString &file)
{
    //open and quick check the file
    QFile xmlFile(file);
    QByteArray xmlContent;
    if(!xmlFile.open(QIODevice::ReadOnly))
    {
        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file to get map zone: %1, error: %2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
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

QString SoloWindow::getZoneName(const QString &zone)
{
    //open and quick check the file
    QFile xmlFile(datapackPath+DATAPACK_BASE_PATH_ZONE+zone+".xml");
    QByteArray xmlContent;
    if(!xmlFile.open(QIODevice::ReadOnly))
    {
        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file to get zone name: %1, error: %2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
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
    const QString &language=LanguagesSelect::languagesSelect.getCurrentLanguages();
    while(!item.isNull())
    {
        if(item.isElement())
            if(item.hasAttribute("lang") && item.attribute("lang")==language)
                return item.text();
        item = item.nextSiblingElement("name");
    }
    item = root.firstChildElement("name");
    while(!item.isNull())
    {
        if(item.isElement())
            if(!item.hasAttribute("lang") || item.attribute("lang")=="en")
                return item.text();
        item = item.nextSiblingElement("name");
    }
    return QString();
}

void SoloWindow::on_SaveGame_Delete_clicked()
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

void SoloWindow::on_SaveGame_Rename_clicked()
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

void SoloWindow::on_SaveGame_Copy_clicked()
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

void SoloWindow::on_SaveGame_Play_clicked()
{
    if(selectedSavegame==NULL)
        return;

    QString savegamesPath=savegamePathList[selectedSavegame];
    if(!savegameWithMetaData[selectedSavegame])
        return;

    emit play(savegamesPath);
}

/*void SoloWindow::closeEvent(QCloseEvent *event)
{
    qDebug() << "close event";
    event->ignore();
    this->hide();
    socket->disconnectFromHost();
    if(internalServer==NULL)
        QCoreApplication::quit();
    else
        qDebug() << "server is running, need wait to close";
}*/

/*void SoloWindow::resetAll()
{
    if(internalServer!=NULL)
        internalServer->stop();
    CatchChallenger::Api_client_real::client->resetAll();
    CatchChallenger::BaseWindow::baseWindow->resetAll();
    ui->stackedWidget->setCurrentIndex(0);
    lastMessageSend="";
    if(internalServer!=NULL)
        internalServer->deleteLater();
    internalServer=NULL;
    pass.clear();
    //saveTime();//not here because called at start!

    //stateChanged(QAbstractSocket::UnconnectedState);//don't call here, else infinity rescursive call
}*/

/*void SoloWindow::disconnected(QString)
{
    resetAll();
}*/

void SoloWindow::changeEvent(QEvent *e)
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

/*void SoloWindow::stateChanged(QAbstractSocket::SocketState socketState)
{
    if(socketState==QAbstractSocket::UnconnectedState)
        resetAll();
    CatchChallenger::BaseWindow::baseWindow->stateChanged(socketState);
}

void SoloWindow::error(QAbstractSocket::SocketError socketError)
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

void SoloWindow::haveNewError()
{
//	QMessageBox::critical(this,tr("Error"),client->errorString());
}

void SoloWindow::message(QString message)
{
    qDebug() << message;
}

void SoloWindow::protocol_is_good()
{
    CatchChallenger::Api_client_real::client->tryLogin("admin",pass);
}

void SoloWindow::try_stop_server()
{
    if(internalServer!=NULL)
        delete internalServer;
    internalServer=NULL;
    saveTime();
}

void SoloWindow::play(const QString &savegamesPath)
{
    resetAll();
    ui->stackedWidget->setCurrentIndex(1);
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
    {
        delete internalServer;
        saveTime();
    }
    internalServer=new CatchChallenger::InternalServer();
    sendSettings(internalServer,savegamesPath);
    connect(internalServer,&CatchChallenger::InternalServer::is_started,this,&SoloWindow::is_started,Qt::QueuedConnection);
    connect(internalServer,&CatchChallenger::InternalServer::error,this,&SoloWindow::serverError,Qt::QueuedConnection);

    CatchChallenger::BaseWindow::baseWindow->serverIsLoading();
}

void SoloWindow::sendSettings(CatchChallenger::InternalServer * internalServer,const QString &savegamesPath)
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

    internalServer->setSettings(formatedServerSettings);
}*/

void SoloWindow::on_SaveGame_Back_clicked()
{
    emit back();
}
