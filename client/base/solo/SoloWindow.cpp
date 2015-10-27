#include "SoloWindow.h"
#include "ui_solowindow.h"

#include <QSettings>
#include <QInputDialog>

#include "../base/render/MapVisualiserPlayer.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../base/interface/DatapackClientLoader.h"
#include "../base/LanguagesSelect.h"
#include "../fight/interface/ClientFightEngine.h"
#include "../../general/base/CommonDatapack.h"
#include "../InternetUpdater.h"
#include "../FacilityLibClient.h"

const QString SoloWindow::text_savegame_version=QStringLiteral("savegame_version");
const QString SoloWindow::text_QSQLITE=QStringLiteral("QSQLITE");
const QString SoloWindow::text_savegameupdate=QStringLiteral("savegameupdate");
const QString SoloWindow::text_catchchallenger_db_sqlite=QStringLiteral("catchchallenger.db.sqlite");
const QString SoloWindow::text_time_played=QStringLiteral("time_played");
const QString SoloWindow::text_location=QStringLiteral("location");
const QString SoloWindow::text_dotxml=QStringLiteral(".xml");
const QString SoloWindow::text_dottmx=QStringLiteral(".tmx");
const QString SoloWindow::text_metadatadotconf=QStringLiteral("metadata.conf");
const QString SoloWindow::text_slash=QStringLiteral("/");
const QString SoloWindow::text_title=QStringLiteral("title");
const QString SoloWindow::text_pass=QStringLiteral("pass");
const QString SoloWindow::text_hover_entry=QStringLiteral("QLabel::hover{border:1px solid #bbb;background-color:rgb(180,180,180,100);border-radius:10px;}");
const QString SoloWindow::text_map=QStringLiteral("map");
const QString SoloWindow::text_zone=QStringLiteral("zone");
const QString SoloWindow::text_name=QStringLiteral("name");
const QString SoloWindow::text_value=QStringLiteral("value");
const QString SoloWindow::text_properties=QStringLiteral("properties");
const QString SoloWindow::text_property=QStringLiteral("property");
const QString SoloWindow::text_lang=QStringLiteral("lang");
const QString SoloWindow::text_en=QStringLiteral("en");
const QString SoloWindow::text_full_entry=QStringLiteral("<span style=\"font-size:12pt;font-weight:600;\">%1</span><br/><span style=\"color:#909090;\">%2<br/>%3</span>");
const QString SoloWindow::text_CATCHCHALLENGER_SAVEGAME_VERSION=QStringLiteral(CATCHCHALLENGER_SAVEGAME_VERSION);

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

    ui->update->setVisible(false);
    if(standAlone)
    {
        ui->horizontalLayout_main->removeItem(ui->horizontalSpacer_Back);
        InternetUpdater::internetUpdater=new InternetUpdater();
        connect(InternetUpdater::internetUpdater,&InternetUpdater::newUpdate,this,&SoloWindow::newUpdate);
    }
    ui->SaveGame_Back->setVisible(!standAlone);
    ui->languages->setVisible(standAlone);
    ui->SaveGame_New->setEnabled(datapackPathExists);
}

SoloWindow::~SoloWindow()
{
    //delete ui;
}

void SoloWindow::on_SaveGame_New_clicked()
{
    bool ok;
    QString gameName=QInputDialog::getText(this,tr("Game name"),tr("Give the game name"),QLineEdit::Normal,QString(),&ok);
    if(!ok)
        return;
    if(gameName.isEmpty())
    {
        QMessageBox::critical(this,tr("Error"),tr("The game name can't be empty"));
        return;
    }
    //locate the new folder and create it
    int index=0;
    while(QDir().exists(savegamePath+QString::number(index)))
        index++;
    QString savegamesPath=savegamePath+QString::number(index)+SoloWindow::text_slash;
    if(!QDir().mkpath(savegamesPath))
    {
        QMessageBox::critical(this,tr("Error"),QStringLiteral("Unable to write savegame into: %1").arg(savegamesPath));
        return;
    }

    //initialize the db
    QByteArray dbData;
    {
        QFile dbSource(QStringLiteral(":/catchchallenger.db.sqlite"));
        if(!dbSource.open(QIODevice::ReadOnly))
        {
            QMessageBox::critical(this,tr("Error"),QStringLiteral("Unable to open the db model: %1").arg(savegamesPath));
            CatchChallenger::FacilityLibGeneral::rmpath(savegamesPath.toStdString());
            return;
        }
        dbData=dbSource.readAll();
        if(dbData.isEmpty())
        {
            QMessageBox::critical(this,tr("Error"),QStringLiteral("Unable to read the db model: %1").arg(savegamesPath));
            CatchChallenger::FacilityLibGeneral::rmpath(savegamesPath.toStdString());
            return;
        }
        dbSource.close();
    }
    {
        QFile dbDestination(savegamesPath+SoloWindow::text_catchchallenger_db_sqlite);
        if(!dbDestination.open(QIODevice::WriteOnly))
        {
            QMessageBox::critical(this,tr("Error"),QStringLiteral("Unable to write savegame into: %1").arg(savegamesPath));
            CatchChallenger::FacilityLibGeneral::rmpath(savegamesPath.toStdString());
            return;
        }
        if(dbDestination.write(dbData)<0)
        {
            dbDestination.close();
            QMessageBox::critical(this,tr("Error"),QStringLiteral("Unable to write savegame into: %1").arg(savegamesPath));
            CatchChallenger::FacilityLibGeneral::rmpath(savegamesPath.toStdString());
            return;
        }
        dbDestination.close();
    }

    //initialise the pass
    QString pass=QString::fromStdString(CatchChallenger::FacilityLibGeneral::randomPassword("abcdefghijklmnopqurstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890",32));

    //initialise the meta data
    bool settingOk=false;
    {
        QSettings metaData(savegamesPath+SoloWindow::text_metadatadotconf,QSettings::IniFormat);
        if(metaData.isWritable())
        {
            if(metaData.status()==QSettings::NoError)
            {
                metaData.setValue(SoloWindow::text_title,gameName);
                metaData.setValue(SoloWindow::text_location,QString());
                metaData.setValue(SoloWindow::text_time_played,0);
                metaData.setValue(SoloWindow::text_pass,pass);
                metaData.setValue(SoloWindow::text_savegame_version,SoloWindow::text_CATCHCHALLENGER_SAVEGAME_VERSION);
                settingOk=true;
            }
            else
                qDebug() << "Settings error: " << metaData.status();
        }
    }

    //check if meta data is ok, and db can be open
    if(!settingOk)
    {
        QMessageBox::critical(this,tr("Error"),QStringLiteral("Unable to write savegame into: %1").arg(savegamesPath));
        CatchChallenger::FacilityLibGeneral::rmpath(savegamesPath.toStdString());
        return;
    }

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

void SoloWindow::setOnlySolo()
{
    #ifdef CATCHCHALLENGER_GITCOMMIT
    ui->version->setText(QStringLiteral(CATCHCHALLENGER_VERSION)+QStringLiteral(" - ")+QStringLiteral(CATCHCHALLENGER_GITCOMMIT));
    #else
    ui->version->setText(QStringLiteral(CATCHCHALLENGER_VERSION));
    #endif
}

void SoloWindow::SoloWindowListEntryEnvoluedUpdate()
{
    int index=0;
    while(index<savegame.size())
    {
        if(savegame.at(index)==selectedSavegame)
            savegame.at(index)->setStyleSheet(QStringLiteral("QLabel{border:1px solid #6b6;background-color:rgb(100,180,100,120);border-radius:10px;}QLabel::hover{border:1px solid #494;background-color:rgb(70,150,70,120);border-radius:10px;}"));
        else
            savegame.at(index)->setStyleSheet(QStringLiteral("QLabel::hover{border:1px solid #bbb;background-color:rgb(180,180,180,100);border-radius:10px;}"));
        index++;
    }
    ui->SaveGame_Play->setEnabled(selectedSavegame!=NULL && savegameWithMetaData.value(selectedSavegame));
    ui->SaveGame_Rename->setEnabled(selectedSavegame!=NULL && savegameWithMetaData.value(selectedSavegame));
    ui->SaveGame_Copy->setEnabled(selectedSavegame!=NULL && savegameWithMetaData.value(selectedSavegame));
    ui->SaveGame_Delete->setEnabled(selectedSavegame!=NULL);
}

/*bool SoloWindow::CatchChallenger::FacilityLib::rmpath(const QDir &dir)
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
            if(!CatchChallenger::FacilityLib::rmpath(dir.absolutePath()+'/'+fileInfo.fileName()+'/'))
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
}*/

void SoloWindow::updateSavegameList()
{
    if(!datapackPathExists)
    {
        ui->savegameEmpty->setText(QStringLiteral("<html><head/><body><p align=\"center\"><span style=\"font-size:12pt;color:#a0a0a0;\">%1</span></p></body></html>").arg(tr("No datapack!")));
        return;
    }
    QString lastSelectedPath;
    if(selectedSavegame!=NULL)
        lastSelectedPath=savegamePathList.value(selectedSavegame);
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
        QString savegamesPath=fileInfo.absoluteFilePath()+text_slash;
        QSettings metaData(savegamesPath+text_metadatadotconf,QSettings::IniFormat);
        ListEntryEnvolued *newEntry=new ListEntryEnvolued();
        connect(newEntry,&ListEntryEnvolued::clicked,this,&SoloWindow::SoloWindowListEntryEnvoluedClicked,Qt::QueuedConnection);
        connect(newEntry,&ListEntryEnvolued::doubleClicked,this,&SoloWindow::SoloWindowListEntryEnvoluedDoubleClicked,Qt::QueuedConnection);
        newEntry->setStyleSheet(text_hover_entry);
        QString dateString;
        if(!QFileInfo(savegamesPath+text_metadatadotconf).exists())
            newEntry->setText(QStringLiteral("<span style=\"font-size:12pt;font-weight:600;\">Missing metadata</span><br/><span style=\"color:#909090;\">Missing metadata<br/>Bug</span>"));
        else
        {
            dateString=QFileInfo(savegamesPath+text_metadatadotconf).lastModified().toString(QStringLiteral("dd/MM/yyyy hh:mm:ssAP"));
            if(!metaData.isWritable())
                newEntry->setText(QStringLiteral("<span style=\"font-size:12pt;font-weight:600;\">%1</span><br/><span style=\"color:#909090;\">%2<br/>Bug</span>").arg("Bug").arg(dateString));
            else
            {
                if(metaData.status()==QSettings::NoError)
                {
                    if(metaData.contains(text_title) && metaData.contains(text_location) && metaData.contains(SoloWindow::text_time_played) && metaData.contains(SoloWindow::text_pass))
                    {
                        //update process
                        if(!metaData.contains(text_savegame_version))
                            metaData.setValue(text_savegame_version,QStringLiteral("0.4"));
                        QString version=metaData.value(text_savegame_version).toString();

                        if(version!=SoloWindow::text_CATCHCHALLENGER_SAVEGAME_VERSION)
                        {
                            if(version==QStringLiteral("0.4"))
                            {
                                QStringList values;
                                values << "ALTER TABLE plant ADD id INT;";
                                values << "CREATE UNIQUE INDEX \"plant_primarykey\" on plant (id ASC);";
                                QSqlDatabase conn = QSqlDatabase::addDatabase(SoloWindow::text_QSQLITE,SoloWindow::text_savegameupdate);
                                conn.setDatabaseName(savegamesPath+SoloWindow::text_catchchallenger_db_sqlite);
                                if(conn.open())
                                {
                                    int index=0;
                                    while(index<values.size())
                                    {
                                        QSqlQuery query(conn);
                                        if(!query.exec(values.at(index)))
                                            qDebug() << "query to update the savegame" << query.lastError().driverText() << query.lastError().driverText();
                                        index++;
                                    }
                                    QSqlQuery query(conn);
                                    if(!query.exec(QStringLiteral("SELECT map,x,y FROM plant")))
                                        qDebug() << "query to update the savegame" << query.lastError().driverText() << query.lastError().driverText();
                                    else
                                    {
                                        int index=0;
                                        while(query.next())
                                        {
                                            QSqlQuery queryUpdate(conn);
                                            if(!queryUpdate.exec(QStringLiteral("UPDATE character SET id=%1 WHERE map='%2',x=%3,y=%4 FROM plant").arg(index).arg(query.value(0).toString()).arg(query.value(1).toUInt()).arg(query.value(2).toUInt())))
                                                qDebug() << "query to update the savegame" << query.lastError().driverText() << query.lastError().driverText();
                                            index++;
                                        }
                                    }
                                    conn.close();
                                }
                                else
                                    qDebug() << "database con't be open to update the savegame" << conn.lastError().driverText() << conn.lastError().databaseText() << values.at(index) << "for" << (savegamesPath+QStringLiteral("catchchallenger.db.sqlite"));
                                QSqlDatabase::removeDatabase(SoloWindow::text_savegameupdate);
                                version=QStringLiteral("0.5");
                                metaData.setValue(SoloWindow::text_savegame_version,version);
                            }
                            if(version==QStringLiteral("0.5"))
                            {
                                QStringList values;
                                values << "CREATE TABLE \"character_itemOnMap\" (\"character\" INTEGER,\"itemOnMap\" INTEGER);";
                                values << "CREATE INDEX \"character_itemOnMap_index\" on character_itemonmap (character ASC);";
                                values << "CREATE TABLE dictionary_itemonmap (\"id\" INTEGER,\"map\" TEXT,\"x\" INTEGER,\"y\" INTEGER);";
                                QSqlDatabase conn = QSqlDatabase::addDatabase("QSQLITE","savegameupdate");
                                conn.setDatabaseName(savegamesPath+QStringLiteral("catchchallenger.db.sqlite"));
                                if(conn.open())
                                {
                                    int index=0;
                                    while(index<values.size())
                                    {
                                        QSqlQuery query(conn);
                                        if(!query.exec(values.at(index)))
                                            qDebug() << "query to update the savegame" << query.lastError().driverText() << query.lastError().driverText() << values.at(index) << "for" << (savegamesPath+QStringLiteral("catchchallenger.db.sqlite"));
                                        index++;
                                    }
                                    conn.close();
                                }
                                else
                                    qDebug() << "database con't be open to update the savegame" << conn.lastError().driverText() << conn.lastError().databaseText();
                                QSqlDatabase::removeDatabase(SoloWindow::text_savegameupdate);
                                version=QStringLiteral("0.6");
                                metaData.setValue(SoloWindow::text_savegame_version,version);
                            }
                        }
                        int time_played_number=metaData.value(SoloWindow::text_time_played).toUInt(&ok);
                        QString time_played;
                        if(!ok || time_played_number>3600*24*365*50)
                            time_played=QStringLiteral("Time player: bug");
                        else
                            time_played=QStringLiteral("%1 played").arg(CatchChallenger::FacilityLibClient::timeToString(time_played_number));
                        //load the map name
                        QString mapName;
                        QString map=metaData.value(SoloWindow::text_location).toString();
                        if(!map.isEmpty())
                        {
                            map.replace(SoloWindow::text_dottmx,SoloWindow::text_dotxml);
                            if(QFileInfo(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MAPMAIN)+map).isFile())
                                mapName=getMapName(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MAPMAIN)+map);
                            if(mapName.isEmpty())
                            {
                                const QString &tmxFile=datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MAPMAIN)+metaData.value(SoloWindow::text_location).toString();
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
                            lastLine=QStringLiteral("%1 (%2)").arg(mapName).arg(time_played);

                        if(version!=SoloWindow::text_CATCHCHALLENGER_SAVEGAME_VERSION)
                        {
                            newEntry->setText(QStringLiteral("<span style=\"font-size:12pt;font-weight:600;\">%1</span><br />Version not compatible (%2)</span>")
                                              .arg(metaData.value("title").toString())
                                              .arg(version)
                                              );
                        }
                        else
                            newEntry->setText(SoloWindow::text_full_entry
                                          .arg(metaData.value(SoloWindow::text_title).toString())
                                          .arg(dateString)
                                          .arg(lastLine)
                                          );
                    }
                    else if(metaData.contains(SoloWindow::text_title))
                        newEntry->setText(QStringLiteral("<span style=\"font-size:12pt;font-weight:600;\">%1</span></span>")
                                          .arg(metaData.value(SoloWindow::text_title).toString())
                                          );
                    else
                        newEntry->setText(QStringLiteral("<span style=\"font-size:12pt;font-weight:600;\">%1</span></span>")
                                          .arg(tr("Unknown title"))
                                          );
                }
                else
                    newEntry->setText(QStringLiteral("<span style=\"font-size:12pt;font-weight:600;\">%1</span><br/><span style=\"color:#909090;\">%2<br/>Bug</span>").arg(metaData.value("title").toString()).arg(dateString));
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
        qDebug() << (QStringLiteral("Unable to open the xml file to get map name: %1, error: %2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
        return QString();
    }
    xmlContent=xmlFile.readAll();
    xmlFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << (QStringLiteral("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(xmlFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr));
        return QString();
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=SoloWindow::text_map)
    {
        qDebug() << (QStringLiteral("Unable to open the xml file: %1, \"plants\" root balise not found for the xml file").arg(xmlFile.fileName()));
        return QString();
    }
    const QString &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
    QDomElement item = root.firstChildElement(SoloWindow::text_name);
    if(!language.isEmpty() && language!=SoloWindow::text_en)
        while(!item.isNull())
        {
            if(item.isElement())
                if(item.hasAttribute(SoloWindow::text_lang) && item.attribute(SoloWindow::text_lang)==language)
                    return item.text();
            item = item.nextSiblingElement(SoloWindow::text_name);
        }
    item = root.firstChildElement(SoloWindow::text_name);
    while(!item.isNull())
    {
        if(item.isElement())
            if(!item.hasAttribute(SoloWindow::text_lang) || item.attribute(SoloWindow::text_lang)==SoloWindow::text_en)
                return item.text();
        item = item.nextSiblingElement(SoloWindow::text_name);
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
        qDebug() << (QStringLiteral("Unable to open the xml file to get map zone: %1, error: %2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
        return QString();
    }
    xmlContent=xmlFile.readAll();
    xmlFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << (QStringLiteral("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(xmlFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr));
        return QString();
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=SoloWindow::text_map)
    {
        qDebug() << (QStringLiteral("Unable to open the xml file: %1, \"plants\" root balise not found for the xml file").arg(xmlFile.fileName()));
        return QString();
    }
    QDomElement properties = root.firstChildElement(SoloWindow::text_properties);
    while(!properties.isNull())
    {
        if(properties.isElement())
        {
            QDomElement property = properties.firstChildElement(SoloWindow::text_property);
            while(!property.isNull())
            {
                if(property.isElement())
                    if(property.hasAttribute(SoloWindow::text_name) && property.hasAttribute(SoloWindow::text_value))
                        if(property.attribute(SoloWindow::text_name)==SoloWindow::text_zone)
                            return property.attribute(SoloWindow::text_value);
                property = property.nextSiblingElement(SoloWindow::text_property);
            }
        }
        properties = properties.nextSiblingElement(SoloWindow::text_properties);
    }
    return QString();
}

QString SoloWindow::getZoneName(const QString &zone)
{
    //open and quick check the file
    QFile xmlFile(datapackPath+DATAPACK_BASE_PATH_ZONE1+CatchChallenger::Api_client_real::client->mainDatapackCode()+DATAPACK_BASE_PATH_ZONE2+zone+".xml");
    QByteArray xmlContent;
    if(!xmlFile.open(QIODevice::ReadOnly))
    {
        qDebug() << (QStringLiteral("Unable to open the xml file to get zone name: %1, error: %2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
        return QString();
    }
    xmlContent=xmlFile.readAll();
    xmlFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << (QStringLiteral("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(xmlFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr));
        return QString();
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=SoloWindow::text_zone)
    {
        qDebug() << (QStringLiteral("Unable to open the xml file: %1, \"plants\" root balise not found for the xml file").arg(xmlFile.fileName()));
        return QString();
    }

    //load the content
    QDomElement item = root.firstChildElement(SoloWindow::text_name);
    const QString &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
    while(!item.isNull())
    {
        if(item.isElement())
            if(item.hasAttribute(SoloWindow::text_lang) && item.attribute(SoloWindow::text_lang)==language)
                return item.text();
        item = item.nextSiblingElement(SoloWindow::text_name);
    }
    item = root.firstChildElement(SoloWindow::text_name);
    while(!item.isNull())
    {
        if(item.isElement())
            if(!item.hasAttribute(SoloWindow::text_lang) || item.attribute(SoloWindow::text_lang)==SoloWindow::text_en)
                return item.text();
        item = item.nextSiblingElement(SoloWindow::text_name);
    }
    return QString();
}

void SoloWindow::on_SaveGame_Delete_clicked()
{
    if(QMessageBox::question(this,tr("Remove"),tr("Are you sure remove this savegame?"),QMessageBox::Yes|QMessageBox::No,QMessageBox::No)!=QMessageBox::Yes)
        return;

    if(selectedSavegame==NULL)
        return;

    if(!CatchChallenger::FacilityLibGeneral::rmpath(savegamePathList.value(selectedSavegame).toStdString()))
    {
        QMessageBox::critical(this,tr("Error"),QStringLiteral("Unable to remove the savegame"));
        return;
    }

    updateSavegameList();
}

void SoloWindow::on_SaveGame_Rename_clicked()
{
    if(selectedSavegame==NULL)
        return;

    QString savegamesPath=savegamePathList.value(selectedSavegame);
    if(!savegameWithMetaData.value(selectedSavegame))
        return;
    QSettings metaData(savegamesPath+SoloWindow::text_metadatadotconf,QSettings::IniFormat);
    if(!QFileInfo(savegamesPath+SoloWindow::text_metadatadotconf).exists())
    {
        QMessageBox::critical(this,tr("Error"),QStringLiteral("No meta data file"));
        return;
    }
    QString newName=QInputDialog::getText(NULL,tr("New name"),tr("Write the new name"),QLineEdit::Normal,metaData.value(SoloWindow::text_title).toString());
    if(newName.isEmpty())
        return;
    metaData.setValue(SoloWindow::text_title,newName);

    updateSavegameList();
}

void SoloWindow::on_SaveGame_Copy_clicked()
{
    if(selectedSavegame==NULL)
        return;

    QString savegamesPath=savegamePathList.value(selectedSavegame);
    if(!savegameWithMetaData.value(selectedSavegame))
        return;
    int index=0;
    while(QDir(savegamePath+QString::number(index)+SoloWindow::text_slash).exists())
        index++;
    QString destinationPath=savegamePath+QString::number(index)+SoloWindow::text_slash;
    if(!QDir().mkpath(destinationPath))
    {
        QMessageBox::critical(this,tr("Error"),QStringLiteral("Unable to write another savegame"));
        return;
    }
    if(!QFile::copy(savegamesPath+SoloWindow::text_metadatadotconf,destinationPath+SoloWindow::text_metadatadotconf))
    {
        CatchChallenger::FacilityLibGeneral::rmpath(destinationPath.toStdString());
        QMessageBox::critical(this,tr("Error"),QStringLiteral("Unable to write another savegame (Error: metadata.conf)"));
        updateSavegameList();
        return;
    }
    if(!QFile::copy(savegamesPath+SoloWindow::text_catchchallenger_db_sqlite,destinationPath+SoloWindow::text_catchchallenger_db_sqlite))
    {
        CatchChallenger::FacilityLibGeneral::rmpath(destinationPath.toStdString());
        QMessageBox::critical(this,tr("Error"),QStringLiteral("Unable to write another savegame (Error: catchchallenger.db.sqlite)"));
        updateSavegameList();
        return;
    }
    QSettings metaData(destinationPath+SoloWindow::text_metadatadotconf,QSettings::IniFormat);
    metaData.setValue(SoloWindow::text_title,tr("Copy of %1").arg(metaData.value(SoloWindow::text_title).toString()));
    updateSavegameList();
}

void SoloWindow::on_SaveGame_Play_clicked()
{
    if(selectedSavegame==NULL)
        return;

    QString savegamesPath=savegamePathList.value(selectedSavegame);
    if(!savegameWithMetaData.value(selectedSavegame))
        return;

    emit play(savegamesPath);
}

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

void SoloWindow::on_SaveGame_Back_clicked()
{
    emit back();
}

void SoloWindow::on_languages_clicked()
{
    LanguagesSelect::languagesSelect->exec();
}

void SoloWindow::newUpdate(const QString &version)
{
    ui->update->setText(InternetUpdater::getText(version));
    ui->update->setVisible(true);
}
