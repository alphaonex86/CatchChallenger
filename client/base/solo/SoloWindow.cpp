#include "SoloWindow.h"
#include "ui_solowindow.h"

#include <QSettings>
#include <QInputDialog>

#include "../base/render/MapVisualiserPlayer.h"
#include "../../general/base/FacilityLib.h"
#include "../base/interface/DatapackClientLoader.h"
#include "../base/LanguagesSelect.h"
#include "../fight/interface/ClientFightEngine.h"
#include "../../general/base/DebugClass.h"
#include "../../general/base/CommonDatapack.h"
#include "../InternetUpdater.h"

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
    QString savegamesPath=savegamePath+QString::number(index)+"/";
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
            CatchChallenger::FacilityLib::rmpath(savegamesPath);
            return;
        }
        dbData=dbSource.readAll();
        if(dbData.isEmpty())
        {
            QMessageBox::critical(this,tr("Error"),QStringLiteral("Unable to read the db model: %1").arg(savegamesPath));
            CatchChallenger::FacilityLib::rmpath(savegamesPath);
            return;
        }
        dbSource.close();
    }
    {
        QFile dbDestination(savegamesPath+"catchchallenger.db.sqlite");
        if(!dbDestination.open(QIODevice::WriteOnly))
        {
            QMessageBox::critical(this,tr("Error"),QStringLiteral("Unable to write savegame into: %1").arg(savegamesPath));
            CatchChallenger::FacilityLib::rmpath(savegamesPath);
            return;
        }
        if(dbDestination.write(dbData)<0)
        {
            dbDestination.close();
            QMessageBox::critical(this,tr("Error"),QStringLiteral("Unable to write savegame into: %1").arg(savegamesPath));
            CatchChallenger::FacilityLib::rmpath(savegamesPath);
            return;
        }
        dbDestination.close();
    }

    //initialise the pass
    QString pass=CatchChallenger::FacilityLib::randomPassword("abcdefghijklmnopqurstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890",32);

    //initialise the meta data
    bool settingOk=false;
    {
        QSettings metaData(savegamesPath+QStringLiteral("metadata.conf"),QSettings::IniFormat);
        if(metaData.isWritable())
        {
            if(metaData.status()==QSettings::NoError)
            {
                metaData.setValue(QStringLiteral("title"),gameName);
                metaData.setValue(QStringLiteral("location"),QStringLiteral(""));
                metaData.setValue(QStringLiteral("time_played"),0);
                metaData.setValue(QStringLiteral("pass"),pass);
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
        CatchChallenger::FacilityLib::rmpath(savegamesPath);
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
        QString savegamesPath=fileInfo.absoluteFilePath()+QStringLiteral("/");
        QSettings metaData(savegamesPath+QStringLiteral("metadata.conf"),QSettings::IniFormat);
        ListEntryEnvolued *newEntry=new ListEntryEnvolued();
        connect(newEntry,&ListEntryEnvolued::clicked,this,&SoloWindow::SoloWindowListEntryEnvoluedClicked,Qt::QueuedConnection);
        connect(newEntry,&ListEntryEnvolued::doubleClicked,this,&SoloWindow::SoloWindowListEntryEnvoluedDoubleClicked,Qt::QueuedConnection);
        newEntry->setStyleSheet(QStringLiteral("QLabel::hover{border:1px solid #bbb;background-color:rgb(180,180,180,100);border-radius:10px;}"));
        QString dateString;
        if(!QFileInfo(savegamesPath+QStringLiteral("metadata.conf")).exists())
            newEntry->setText(QStringLiteral("<span style=\"font-size:12pt;font-weight:600;\">Missing metadata</span><br/><span style=\"color:#909090;\">Missing metadata<br/>Bug</span>"));
        else
        {
            dateString=QFileInfo(savegamesPath+QStringLiteral("metadata.conf")).lastModified().toString(QStringLiteral("dd/MM/yyyy hh:mm:ssAP"));
            if(!metaData.isWritable())
                newEntry->setText(QStringLiteral("<span style=\"font-size:12pt;font-weight:600;\">%1</span><br/><span style=\"color:#909090;\">%2<br/>Bug</span>").arg("Bug").arg(dateString));
            else
            {
                if(metaData.status()==QSettings::NoError)
                {
                    if(metaData.contains(QStringLiteral("title")) && metaData.contains(QStringLiteral("location")) && metaData.contains(QStringLiteral("time_played")) && metaData.contains(QStringLiteral("pass")))
                    {
                        int time_played_number=metaData.value("time_played").toUInt(&ok);
                        QString time_played;
                        if(!ok || time_played_number>3600*24*365*50)
                            time_played="Time player: bug";
                        else
                            time_played=QStringLiteral("%1 played").arg(CatchChallenger::FacilityLib::timeToString(time_played_number));
                        //load the map name
                        QString mapName;
                        QString map=metaData.value(QStringLiteral("location")).toString();
                        if(!map.isEmpty())
                        {
                            map.replace(QStringLiteral(".tmx"),QStringLiteral(".xml"));
                            if(QFileInfo(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MAP)+map).isFile())
                                mapName=getMapName(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MAP)+map);
                            if(mapName.isEmpty())
                            {
                                QString tmxFile=datapackPath+QStringLiteral(DATAPACK_BASE_PATH_MAP)+metaData.value(QStringLiteral("location")).toString();
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

                        newEntry->setText(QStringLiteral("<span style=\"font-size:12pt;font-weight:600;\">%1</span><br/><span style=\"color:#909090;\">%2<br/>%3</span>")
                                          .arg(metaData.value("title").toString())
                                          .arg(dateString)
                                          .arg(lastLine)
                                          );
                    }
                    else if(metaData.contains("title"))
                        newEntry->setText(QStringLiteral("<span style=\"font-size:12pt;font-weight:600;\">%1</span></span>")
                                          .arg(metaData.value("title").toString())
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
        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file to get map name: %1, error: %2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
        return QString();
    }
    xmlContent=xmlFile.readAll();
    xmlFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(xmlFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr));
        return QString();
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=QStringLiteral("map"))
    {
        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, \"plants\" root balise not found for the xml file").arg(xmlFile.fileName()));
        return QString();
    }
    const QString &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
    QDomElement item = root.firstChildElement(QStringLiteral("name"));
    if(!language.isEmpty() && language!=QStringLiteral("en"))
        while(!item.isNull())
        {
            if(item.isElement())
                if(item.hasAttribute(QStringLiteral("lang")) && item.attribute(QStringLiteral("lang"))==language)
                    return item.text();
            item = item.nextSiblingElement(QStringLiteral("name"));
        }
    item = root.firstChildElement(QStringLiteral("name"));
    while(!item.isNull())
    {
        if(item.isElement())
            if(!item.hasAttribute(QStringLiteral("lang")) || item.attribute(QStringLiteral("lang"))==QStringLiteral("en"))
                return item.text();
        item = item.nextSiblingElement(QStringLiteral("name"));
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
        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file to get map zone: %1, error: %2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
        return QString();
    }
    xmlContent=xmlFile.readAll();
    xmlFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(xmlFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr));
        return QString();
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="map")
    {
        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, \"plants\" root balise not found for the xml file").arg(xmlFile.fileName()));
        return QString();
    }
    QDomElement properties = root.firstChildElement(QStringLiteral("properties"));
    while(!properties.isNull())
    {
        if(properties.isElement())
        {
            QDomElement property = properties.firstChildElement(QStringLiteral("property"));
            while(!property.isNull())
            {
                if(property.isElement())
                    if(property.hasAttribute(QStringLiteral("name")) && property.hasAttribute(QStringLiteral("value")))
                        if(property.attribute(QStringLiteral("name"))==QStringLiteral("zone"))
                            return property.attribute(QStringLiteral("value"));
                property = property.nextSiblingElement(QStringLiteral("property"));
            }
        }
        properties = properties.nextSiblingElement(QStringLiteral("properties"));
    }
    return QString();
}

QString SoloWindow::getZoneName(const QString &zone)
{
    //open and quick check the file
    QFile xmlFile(datapackPath+QStringLiteral(DATAPACK_BASE_PATH_ZONE)+zone+QStringLiteral(".xml"));
    QByteArray xmlContent;
    if(!xmlFile.open(QIODevice::ReadOnly))
    {
        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file to get zone name: %1, error: %2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
        return QString();
    }
    xmlContent=xmlFile.readAll();
    xmlFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(xmlFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr));
        return QString();
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=QStringLiteral("zone"))
    {
        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Unable to open the xml file: %1, \"plants\" root balise not found for the xml file").arg(xmlFile.fileName()));
        return QString();
    }

    //load the content
    QDomElement item = root.firstChildElement(QStringLiteral("name"));
    const QString &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
    while(!item.isNull())
    {
        if(item.isElement())
            if(item.hasAttribute(QStringLiteral("lang")) && item.attribute(QStringLiteral("lang"))==language)
                return item.text();
        item = item.nextSiblingElement(QStringLiteral("name"));
    }
    item = root.firstChildElement(QStringLiteral("name"));
    while(!item.isNull())
    {
        if(item.isElement())
            if(!item.hasAttribute(QStringLiteral("lang")) || item.attribute(QStringLiteral("lang"))==QStringLiteral("en"))
                return item.text();
        item = item.nextSiblingElement(QStringLiteral("name"));
    }
    return QString();
}

void SoloWindow::on_SaveGame_Delete_clicked()
{
    if(selectedSavegame==NULL)
        return;

    if(!CatchChallenger::FacilityLib::rmpath(savegamePathList.value(selectedSavegame)))
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
    QSettings metaData(savegamesPath+QStringLiteral("metadata.conf"),QSettings::IniFormat);
    if(!QFileInfo(savegamesPath+QStringLiteral("metadata.conf")).exists())
    {
        QMessageBox::critical(this,tr("Error"),QStringLiteral("No meta data file"));
        return;
    }
    QString newName=QInputDialog::getText(NULL,tr("New name"),tr("Write the new name"),QLineEdit::Normal,metaData.value(QStringLiteral("title")).toString());
    if(newName.isEmpty())
        return;
    metaData.setValue(QStringLiteral("title"),newName);

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
    while(QDir(savegamePath+QString::number(index)+QStringLiteral("/")).exists())
        index++;
    QString destinationPath=savegamePath+QString::number(index)+QStringLiteral("/");
    if(!QDir().mkpath(destinationPath))
    {
        QMessageBox::critical(this,tr("Error"),QStringLiteral("Unable to write another savegame"));
        return;
    }
    if(!QFile::copy(savegamesPath+QStringLiteral("metadata.conf"),destinationPath+QStringLiteral("metadata.conf")))
    {
        CatchChallenger::FacilityLib::rmpath(destinationPath);
        QMessageBox::critical(this,tr("Error"),QStringLiteral("Unable to write another savegame (Error: metadata.conf)"));
        return;
    }
    if(!QFile::copy(savegamesPath+QStringLiteral("catchchallenger.db.sqlite"),destinationPath+QStringLiteral("catchchallenger.db.sqlite")))
    {
        CatchChallenger::FacilityLib::rmpath(destinationPath);
        QMessageBox::critical(this,tr("Error"),QStringLiteral("Unable to write another savegame (Error: catchchallenger.db.sqlite)"));
        return;
    }
    QSettings metaData(destinationPath+QStringLiteral("metadata.conf"),QSettings::IniFormat);
    metaData.setValue("title",tr("Copy of %1").arg(metaData.value(QStringLiteral("title")).toString()));
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
    ui->stackedWidget->setCurrentWidget();
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
    ui->stackedWidget->setCurrentWidget();
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

void SoloWindow::on_languages_clicked()
{
    LanguagesSelect::languagesSelect->exec();
}

void SoloWindow::newUpdate(const QString &version)
{
    ui->update->setText(InternetUpdater::getText(version));
    ui->update->setVisible(true);
}
