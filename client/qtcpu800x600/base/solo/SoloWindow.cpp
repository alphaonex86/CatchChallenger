#include "SoloWindow.h"
#include "ui_solowindow.h"

#include <QSettings>
#include <QInputDialog>
#include <QSqlQuery>
#include <QSqlError>

#include "../../qtmaprender/MapVisualiserPlayer.hpp"
#include "../../../general/base/FacilityLib.hpp"
#include "../../../general/base/FacilityLibGeneral.hpp"
#include "../../../general/base/SavegameVersion.hpp"
#include "../../../general/base/Version.hpp"
#include "../../libcatchchallenger/DatapackClientLoader.hpp"
#include "../LanguagesSelect.h"
#include "../../libqtcatchchallenger/ClientFightEngine.hpp"
#include "../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../general/base/Version.hpp"
#include "../../../general/base/CommonDatapack.hpp"
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

SoloWindow::SoloWindow(QWidget *parent, const std::string &datapackPath, const std::string &savegamePath, const bool &standAlone) :
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
    datapackPathExists=QDir(QString::fromStdString(datapackPath)).exists();//QCoreApplication::applicationDirPath()+"/datapack/internal/";

    updateSavegameList();

    ui->update->setVisible(false);
    if(standAlone)
    {
        ui->horizontalLayout_main->removeItem(ui->horizontalSpacer_Back);
        InternetUpdater::internetUpdater=new InternetUpdater();
        if(!connect(InternetUpdater::internetUpdater,&InternetUpdater::newUpdate,this,&SoloWindow::newUpdate))
            abort();
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
    while(QDir().exists(QString::fromStdString(savegamePath)+QString::number(index)))
        index++;
    QString savegamesPath=QString::fromStdString(savegamePath)+QString::number(index)+"/";
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

    emit play(savegamesPath.toStdString());
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
    ui->version->setText(QString::fromStdString(CatchChallenger::Version::str)+QStringLiteral(" - ")+QStringLiteral(CATCHCHALLENGER_GITCOMMIT));
    #else
    ui->version->setText(QString::fromStdString(CatchChallenger::Version::str));
    #endif
}

void SoloWindow::SoloWindowListEntryEnvoluedUpdate()
{
    unsigned int index=0;
    while(index<savegame.size())
    {
        if(savegame.at(index)==selectedSavegame)
            savegame.at(index)->setStyleSheet(QStringLiteral("QLabel{border:1px solid #6b6;background-color:rgb(100,180,100,120);border-radius:10px;}QLabel::hover{border:1px solid #494;background-color:rgb(70,150,70,120);border-radius:10px;}"));
        else
            savegame.at(index)->setStyleSheet(QStringLiteral("QLabel::hover{border:1px solid #bbb;background-color:rgb(180,180,180,100);border-radius:10px;}"));
        index++;
    }
    ui->SaveGame_Play->setEnabled(selectedSavegame!=NULL && savegameWithMetaData.at(selectedSavegame));
    ui->SaveGame_Rename->setEnabled(selectedSavegame!=NULL && savegameWithMetaData.at(selectedSavegame));
    ui->SaveGame_Copy->setEnabled(selectedSavegame!=NULL && savegameWithMetaData.at(selectedSavegame));
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
    std::string lastSelectedPath;
    if(selectedSavegame!=NULL)
        lastSelectedPath=savegamePathList.at(selectedSavegame);
    selectedSavegame=NULL;
    int index=0;
    while(savegame.size()>0)
    {
        delete savegame.at(0);
        savegame.erase(savegame.cbegin()+0);
        index++;
    }
    savegamePathList.clear();
    savegameWithMetaData.clear();
    QFileInfoList entryList=QDir(QString::fromStdString(savegamePath)).entryInfoList(
                QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
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
        if(!connect(newEntry,&ListEntryEnvolued::clicked,this,&SoloWindow::SoloWindowListEntryEnvoluedClicked,Qt::QueuedConnection))
            abort();
        if(!connect(newEntry,&ListEntryEnvolued::doubleClicked,this,&SoloWindow::SoloWindowListEntryEnvoluedDoubleClicked,Qt::QueuedConnection))
            abort();
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
                            metaData.setValue(text_savegame_version,CATCHCHALLENGER_SAVEGAME_VERSION);
                        QString version=metaData.value(text_savegame_version).toString();

                        if(version!=SoloWindow::text_CATCHCHALLENGER_SAVEGAME_VERSION)
                        {
                            if(version==QStringLiteral("2.0"))
                            {
                                QStringList values;
                                values << "ALTER TABLE \"character\" ADD COLUMN lastdaillygift INTEGER;";
                                {
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
                                        conn.commit();
                                        conn.close();
                                        conn = QSqlDatabase();
                                        version=QStringLiteral("2.0.3.0");
                                        metaData.setValue(SoloWindow::text_savegame_version,version);
                                    }
                                    else
                                        qDebug() << "database con't be open to update the savegame" << conn.lastError().driverText() << conn.lastError().databaseText() << values.at(index) << "for" << (savegamesPath+QStringLiteral("catchchallenger.db.sqlite"));
                                }
                                QSqlDatabase::removeDatabase(SoloWindow::text_savegameupdate);//need out of scope of QSqlDatabase conn
                            }
                        }
                        int time_played_number=metaData.value(SoloWindow::text_time_played).toUInt(&ok);
                        QString time_played;
                        if(!ok || time_played_number>3600*24*365*50)
                            time_played=tr("Time player: bug");
                        else
                            time_played=tr("%1 played").arg(QString::fromStdString(CatchChallenger::FacilityLibClient::timeToString(time_played_number)));
                        //load the map name
                        std::string mapName;
                        std::string map=metaData.value(SoloWindow::text_location).toString().toStdString();
                        if(!map.empty())
                        {
                            stringreplaceAll(map,".tmx",".xml");
                            if(QFileInfo(QString::fromStdString(datapackPath)+DATAPACK_BASE_PATH_MAPMAIN+QString::fromStdString(map)).isFile())
                                mapName=getMapName(std::string(datapackPath)+DATAPACK_BASE_PATH_MAPMAIN+map);
                            if(mapName.empty())
                            {
                                const QString &tmxFile=QString::fromStdString(datapackPath)+DATAPACK_BASE_PATH_MAPMAIN+metaData.value(SoloWindow::text_location).toString();
                                if(QFileInfo(tmxFile).isFile())
                                {
                                    std::string zone=getMapZone(tmxFile.toStdString());
                                    //try load the zone
                                    if(!zone.empty())
                                        mapName=getZoneName(zone);
                                }
                            }
                        }
                        QString lastLine;
                        if(mapName.empty())
                            lastLine=time_played;
                        else
                            lastLine=QStringLiteral("%1 (%2)").arg(QString::fromStdString(mapName)).arg(time_played);

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

        if(lastSelectedPath==savegamesPath.toStdString())
            selectedSavegame=newEntry;
        savegame.push_back(newEntry);
        savegamePathList[newEntry]=savegamesPath.toStdString();
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

std::string SoloWindow::getMapName(const std::string &file)
{
    //open and quick check the file
    tinyxml2::XMLDocument domDocument;
    if(domDocument.LoadFile(file.c_str())!=0)
        return std::string();
    const tinyxml2::XMLElement *root = domDocument.RootElement();
    if(root==NULL)
        return std::string();
    if(root->Name()==NULL)
    {
        qDebug() << (QStringLiteral("Unable to open the xml file: %1, \"map\" root balise not found 2 for the xml file")
                     .arg(QString::fromStdString(file)));
        return std::string();
    }
    if(root->Name()==NULL || strcmp(root->Name(),"map")!=0)
    {
        qDebug() << (QStringLiteral("Unable to open the xml file: %1, \"map\" root balise not found for the xml file")
                     .arg(QString::fromStdString(file)));
        return std::string();
    }
    const std::string &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
    const tinyxml2::XMLElement *item = root->FirstChildElement("name");
    if(!language.empty() && language!="en")
        while(item!=NULL)
        {
            if(item->Attribute("lang") && item->Attribute("lang")==language && item->GetText()!=NULL)
                return item->GetText();
            item = item->NextSiblingElement("name");
        }
    item = root->FirstChildElement("name");
    while(item!=NULL)
    {
        if(item->Attribute("lang")==NULL || strcmp(item->Attribute("lang"),"en")==0)
            if(item->GetText()!=NULL)
                return item->GetText();
        item = item->NextSiblingElement("name");
    }
    return std::string();
}

std::string SoloWindow::getMapZone(const std::string &file)
{
    //open and quick check the file
    tinyxml2::XMLDocument domDocument;
    if(domDocument.LoadFile(file.c_str())!=0)
        return std::string();
    const tinyxml2::XMLElement *root = domDocument.RootElement();
    if(root==NULL)
        return std::string();
    if(root->Name()==NULL)
    {
        qDebug() << (QStringLiteral("Unable to open the xml file: %1, \"map\" root balise not found 2 for the xml file").arg(QString::fromStdString(file)));
        return std::string();
    }
    if(strcmp(root->Name(),"map")!=0)
    {
        qDebug() << (QStringLiteral("Unable to open the xml file: %1, \"map\" root balise not found for the xml file").arg(QString::fromStdString(file)));
        return std::string();
    }
    const tinyxml2::XMLElement *properties = root->FirstChildElement("properties");
    while(properties!=NULL)
    {
        const tinyxml2::XMLElement *property = properties->FirstChildElement("property");
        while(property!=NULL)
        {
            if(property->Attribute("name")!=NULL && property->Attribute("value")!=NULL)
                if(strcmp(property->Attribute("name"),"zone")==0)
                    return property->Attribute("value");
            property = property->NextSiblingElement("property");
        }
        properties = properties->NextSiblingElement("properties");
    }
    return std::string();
}

std::string SoloWindow::getZoneName(const std::string &zone)
{
    //open and quick check the file
    std::string file(QtDatapackClientLoader::datapackLoader->getDatapackPath()+
                     QtDatapackClientLoader::datapackLoader->getMainDatapackPath()+
                     DATAPACK_BASE_PATH_ZONE2+zone+".xml");
    //open and quick check the file
    tinyxml2::XMLDocument domDocument;
    if(domDocument.LoadFile(file.c_str())!=0)
        return std::string();
    const tinyxml2::XMLElement *root = domDocument.RootElement();
    if(root==NULL)
        return std::string();
    if(root->Name()==NULL || strcmp(root->Name(),"zone")!=0)
    {
        qDebug() << (QStringLiteral("Unable to open the xml file: %1, \"zone\" root balise not found for the xml file").arg(QString::fromStdString(file)));
        return std::string();
    }

    //load the content
    const tinyxml2::XMLElement *item = root->FirstChildElement("name");
    const std::string &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
    while(item!=NULL)
    {
        if(item->Attribute("lang")!=NULL && item->Attribute("lang")==language && item->GetText()!=NULL)
            return item->GetText();
        item = item->NextSiblingElement("name");
    }
    item = root->FirstChildElement("name");
    while(item!=NULL)
    {
        if(item->Attribute("lang")==NULL || strcmp(item->Attribute("lang"),"en")==0)
            if(item->GetText()!=NULL)
                return item->GetText();
        item = item->NextSiblingElement("name");
    }
    return std::string();
}

//work around QSS crash
void SoloWindow::setBuggyStyle()
{
    ui->page->setStyleSheet("#page{background-image: url(:/images/background.jpg);}");
    ui->frame_login_2->setStyleSheet("#frame_login_2{background-image: url(:/images/savegame-select.png);}");
}

void SoloWindow::on_SaveGame_Delete_clicked()
{
    if(QMessageBox::question(this,tr("Remove"),tr("Are you sure remove this savegame?"),QMessageBox::Yes|QMessageBox::No,QMessageBox::No)!=QMessageBox::Yes)
        return;

    if(selectedSavegame==NULL)
        return;

    if(!CatchChallenger::FacilityLibGeneral::rmpath(savegamePathList.at(selectedSavegame)))
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

    std::string savegamesPath=savegamePathList.at(selectedSavegame);
    if(!savegameWithMetaData.at(selectedSavegame))
        return;
    QSettings metaData(QString::fromStdString(savegamesPath)+SoloWindow::text_metadatadotconf,QSettings::IniFormat);
    if(!QFileInfo(QString::fromStdString(savegamesPath)+SoloWindow::text_metadatadotconf).exists())
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

    std::string savegamesPath=savegamePathList.at(selectedSavegame);
    if(!savegameWithMetaData.at(selectedSavegame))
        return;
    int index=0;
    while(QDir(QString::fromStdString(savegamePath)+QString::number(index)+SoloWindow::text_slash).exists())
        index++;
    QString destinationPath=QString::fromStdString(savegamePath)+QString::number(index)+SoloWindow::text_slash;
    if(!QDir().mkpath(destinationPath))
    {
        QMessageBox::critical(this,tr("Error"),QStringLiteral("Unable to write another savegame"));
        return;
    }

    {
        QFile source(QString::fromStdString(savegamesPath)+SoloWindow::text_metadatadotconf);
        if(source.open(QIODevice::ReadOnly))
        {
            QByteArray data=source.readAll();
            QFile destination(destinationPath+SoloWindow::text_metadatadotconf);
            if(destination.open(QIODevice::WriteOnly))
            {
                destination.write(data);
                destination.close();
            }
            else
                QMessageBox::critical(this,tr("Error"),tr("Unable to open destination file"));
            destination.setPermissions(destination.permissions() | QFileDevice::WriteOwner | QFileDevice::WriteUser);
            source.close();
        }
        else
            QMessageBox::critical(this,tr("Error"),tr("Unable to open source file"));
    }
    {
        QFile source(QString::fromStdString(savegamesPath)+SoloWindow::text_catchchallenger_db_sqlite);
        if(source.open(QIODevice::ReadOnly))
        {
            QByteArray data=source.readAll();
            QFile destination(destinationPath+SoloWindow::text_catchchallenger_db_sqlite);
            if(destination.open(QIODevice::WriteOnly))
            {
                destination.write(data);
                destination.close();
            }
            else
                QMessageBox::critical(this,tr("Error"),tr("Unable to open destination file"));
            destination.setPermissions(destination.permissions() | QFileDevice::WriteOwner | QFileDevice::WriteUser);
            source.close();
        }
        else
            QMessageBox::critical(this,tr("Error"),tr("Unable to open source file"));
    }

    QSettings metaData(destinationPath+SoloWindow::text_metadatadotconf,QSettings::IniFormat);
    metaData.setValue(SoloWindow::text_title,tr("Copy of %1").arg(metaData.value(SoloWindow::text_title).toString()));
    updateSavegameList();
}

void SoloWindow::on_SaveGame_Play_clicked()
{
    if(selectedSavegame==NULL)
        return;

    std::string savegamesPath=savegamePathList.at(selectedSavegame);
    if(!savegameWithMetaData.at(selectedSavegame))
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

void SoloWindow::newUpdate(const std::string &version)
{
    ui->update->setText(QString::fromStdString(InternetUpdater::getText(version)));
    ui->update->setVisible(true);
}
