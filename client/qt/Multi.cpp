#include "Multi.h"
#include "ui_Multi.h"
#include "interface/ListEntryEnvolued.h"
#include "../../general/base/FacilityLibGeneral.h"
#include "../../general/base/tinyXML2/tinyxml2.h"
#include "../../general/base/cpp11addition.h"
#include "ClientVariable.h"
#include "LanguagesSelect.h"
#include "AddServer.h"
#include <iostream>
#include <QRegularExpression>
#include <QMessageBox>
#include <QFile>
#include <QStandardPaths>
#include <utime.h>
#include <QFileInfo>
#include "Ultimate.h"
#include "../../general/base/Version.h"
#include "PlatformMacro.h"
#include <QNetworkRequest>
#include <QDir>
#include "Login.h"
#include "Settings.h"

Multi::Multi(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Multi),
    addServer(nullptr),
    login(nullptr),
    reply(nullptr)
{
    ui->setupUi(this);
    srand(time(0));

    temp_xmlConnexionInfoList=loadXmlConnexionInfoList();
    temp_customConnexionInfoList=loadConfigConnexionInfoList();
    mergedConnexionInfoList=temp_customConnexionInfoList;
    mergedConnexionInfoList.insert(mergedConnexionInfoList.end(),temp_xmlConnexionInfoList.begin(),temp_xmlConnexionInfoList.end());
    std::sort(mergedConnexionInfoList.begin(),mergedConnexionInfoList.end());
    downloadFile();
    selectedServer.unique_code.clear();
    selectedServer.isCustom=false;
    displayServerList();
    ui->warning->setVisible(false);

    if(!connect(ui->server_add,&QPushButton::clicked,this,&Multi::server_add_clicked))
        abort();
    if(!connect(ui->server_remove,&QPushButton::clicked,this,&Multi::server_remove_clicked))
        abort();
    if(!connect(ui->server_edit,&QPushButton::clicked,this,&Multi::server_edit_clicked))
        abort();
    if(!connect(ui->server_select,&QPushButton::clicked,this,&Multi::server_select_clicked))
        abort();
    if(!connect(ui->back,&QPushButton::clicked,this,&Multi::backMain))
        abort();
}

Multi::~Multi()
{
    delete ui;
}

void Multi::displayServerList()
{
    //clean the previous content
    QHash<ListEntryEnvolued *,ConnexionInfo *>::const_iterator i = serverConnexion.constBegin();
    while (i != serverConnexion.constEnd()) {
        delete i.key();
        ++i;
    }

    unsigned int index=0;
    serverConnexion.clear();
    if(mergedConnexionInfoList.empty())
        ui->serverEmpty->setText(QStringLiteral("<html><body><p align=\"center\"><span style=\"font-size:12pt;color:#a0a0a0;\">%1</span></p></body></html>").arg(tr("Empty")));
    #if defined(NOTCPSOCKET) && defined(NOWEBSOCKET)
    #error Web socket and tcp socket are both not supported
    return;
    #endif
    index=0;
    std::cout << "display mergedConnexionInfoList.size(): " << mergedConnexionInfoList.size() << std::endl;
    if(selectedServer.unique_code.isEmpty())
    {
        ui->server_remove->setEnabled(false);
        ui->server_edit->setEnabled(false);
    }
    ui->server_select->setEnabled(!selectedServer.unique_code.isEmpty());
    while(index<mergedConnexionInfoList.size())
    {
        ListEntryEnvolued *newEntry=new ListEntryEnvolued();
        if(!connect(newEntry,&ListEntryEnvolued::clicked,this,&Multi::serverListEntryEnvoluedClicked,Qt::QueuedConnection))
            abort();
        if(!connect(newEntry,&ListEntryEnvolued::doubleClicked,this,&Multi::serverListEntryEnvoluedDoubleClicked,Qt::QueuedConnection))
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

        if(connexionInfo.isCustom==selectedServer.isCustom && connexionInfo.unique_code==selectedServer.unique_code)
        {
            newEntry->setStyleSheet(QStringLiteral("QLabel{border:1px solid #6b6;background-color:rgb(100,180,100,120);border-radius:10px;}QLabel::hover{border:1px solid #494;background-color:rgb(70,150,70,120);border-radius:10px;}"));
            ui->server_remove->setEnabled(connexionInfo.isCustom);
            ui->server_edit->setEnabled(connexionInfo.isCustom);
        }
        else
            newEntry->setStyleSheet(QStringLiteral("QLabel::hover{border:1px solid #bbb;background-color:rgb(180,180,180,100);border-radius:10px;}"));

        ui->scrollAreaWidgetContentsServer->layout()->addWidget(newEntry);

        serverConnexion[newEntry]=&mergedConnexionInfoList[index];
        index++;
    }
    ui->serverWidget->setVisible(index==0);
}

void Multi::serverListEntryEnvoluedClicked()
{
    ListEntryEnvolued * selectedSavegame=qobject_cast<ListEntryEnvolued *>(QObject::sender());
    if(selectedSavegame==NULL)
        return;
    if(serverConnexion.find(selectedSavegame)==serverConnexion.cend())
        return;
    const ConnexionInfo * const connexionInfo=serverConnexion.value(selectedSavegame);
    this->selectedServer.isCustom=connexionInfo->isCustom;
    this->selectedServer.unique_code=connexionInfo->unique_code;
    displayServerList();
}

void Multi::server_add_clicked()
{
    if(addServer!=nullptr)
        delete addServer;
    addServer=new AddOrEditServer(this);
    if(!connect(addServer,&QDialog::accepted,this,&Multi::server_add_finished))
        abort();
    if(!connect(addServer,&QDialog::rejected,this,&Multi::server_add_finished))
        abort();
    emit setAbove(addServer);
}

void Multi::server_add_finished()
{
    emit setAbove(nullptr);
    if(addServer==nullptr)
        return;
    if(!addServer->isOk())
        return;
    if(!Settings::settings.isWritable())
    {
        ui->warning->setText(tr("Option is not writable"));
        ui->warning->setVisible(true);
    }
    #ifdef __EMSCRIPTEN__
    std::cerr << "AddOrEditServer returned" <<  std::endl;
    #endif
    ConnexionInfo connexionInfo;
    connexionInfo.connexionCounter=0;
    connexionInfo.lastConnexion=static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch()/1000);

    connexionInfo.name=addServer->name();
    connexionInfo.unique_code=QString::fromStdString(CatchChallenger::FacilityLibGeneral::randomPassword("abcdefghijklmnopqurstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890",16));
    connexionInfo.isCustom=true;

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

void Multi::server_edit_clicked()
{
    if(addServer!=nullptr)
        delete addServer;
    addServer=new AddOrEditServer(this);
    if(!connect(addServer,&QDialog::accepted,this,&Multi::server_add_finished))
        abort();
    if(!connect(addServer,&QDialog::rejected,this,&Multi::server_add_finished))
        abort();
    emit setAbove(addServer);
}

void Multi::server_edit_finished()
{
    emit setAbove(nullptr);
    if(addServer==nullptr)
        return;
    if(!addServer->isOk())
        return;
    #ifdef __EMSCRIPTEN__
    std::cerr << "AddOrEditServer returned" <<  std::endl;
    #endif
    ConnexionInfo connexionInfo;
    connexionInfo.connexionCounter=0;
    connexionInfo.lastConnexion=static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch()/1000);

    connexionInfo.name=addServer->name();
    connexionInfo.unique_code=QString::fromStdString(CatchChallenger::FacilityLibGeneral::randomPassword("abcdefghijklmnopqurstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890",16));
    connexionInfo.isCustom=true;

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

void Multi::server_remove_clicked()
{
    if(selectedServer.unique_code.isEmpty())
        return;
    unsigned int index=0;
    while(index<mergedConnexionInfoList.size())
    {
        ConnexionInfo &connexionInfo=mergedConnexionInfoList[index];
        if(connexionInfo.isCustom==selectedServer.isCustom && connexionInfo.unique_code==selectedServer.unique_code)
        {
            if(!connexionInfo.isCustom)
                return;
            mergedConnexionInfoList.erase(mergedConnexionInfoList.begin()+index);
            saveConnexionInfoList();
            selectedServer.unique_code.clear();
            selectedServer.isCustom=false;
            displayServerList();
            return;
        }
        index++;
    }
    std::cerr << "remove server not found" << std::endl;
}

void Multi::httpFinished()
{
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
    ui->warning->setVisible(true);
    ui->warning->setText(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first());

    QByteArray content=reply->readAll();
    QString wPath=QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QDir().mkpath(wPath);
    QFile cache(wPath+QStringLiteral("/server_list.xml"));
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
    else
        std::cerr << "Can't write server list cache" << std::endl;
    temp_xmlConnexionInfoList=loadXmlConnexionInfoList(content);
    mergedConnexionInfoList=temp_customConnexionInfoList;
    mergedConnexionInfoList.insert(mergedConnexionInfoList.end(),temp_xmlConnexionInfoList.begin(),temp_xmlConnexionInfoList.end());
    std::cout << "mergedConnexionInfoList.size(): " << mergedConnexionInfoList.size() << std::endl;
    std::sort(mergedConnexionInfoList.begin(),mergedConnexionInfoList.end());//qSort(mergedConnexionInfoList);
    displayServerList();
    reply->deleteLater();
    reply=NULL;
}

void Multi::saveConnexionInfoList()
{
    unsigned int index=0;
    while(index<mergedConnexionInfoList.size())
    {
        const ConnexionInfo &connexionInfo=mergedConnexionInfoList.at(index);
        if(connexionInfo.unique_code.isEmpty())
            abort();

        if(connexionInfo.isCustom)
        {
            Settings::settings.beginGroup(QStringLiteral("Custom-%1").arg(connexionInfo.unique_code));
            if(!connexionInfo.ws.isEmpty())
            {
                Settings::settings.setValue(QStringLiteral("ws"),connexionInfo.ws);
                Settings::settings.remove("host");
                Settings::settings.remove("port");
            }
            else {
                Settings::settings.setValue(QStringLiteral("host"),connexionInfo.host);
                Settings::settings.setValue(QStringLiteral("port"),connexionInfo.port);
                Settings::settings.remove("ws");
            }
            Settings::settings.setValue(QStringLiteral("name"),connexionInfo.name);
        }
        else
            Settings::settings.beginGroup(QStringLiteral("Xml-%1").arg(connexionInfo.unique_code));
        if(connexionInfo.connexionCounter>0)
            Settings::settings.setValue(QStringLiteral("connexionCounter"),connexionInfo.connexionCounter);
        else
            Settings::settings.remove(QStringLiteral("connexionCounter"));
        if(connexionInfo.lastConnexion>0 && connexionInfo.connexionCounter>0)
            Settings::settings.setValue(QStringLiteral("lastConnexion"),connexionInfo.lastConnexion);
        else
            Settings::settings.remove(QStringLiteral("lastConnexion"));
        Settings::settings.setValue(QStringLiteral("proxyHost"),connexionInfo.proxyHost);
        Settings::settings.setValue(QStringLiteral("proxyPort"),connexionInfo.proxyPort);
        Settings::settings.endGroup();
        index++;
    }
    Settings::settings.sync();
}

void Multi::serverListEntryEnvoluedDoubleClicked()
{
    server_select_clicked();
}

std::vector<Multi::ConnexionInfo> Multi::loadXmlConnexionInfoList()
{
    QString wPath=QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    if(QFileInfo(wPath+"/server_list.xml").isFile())
        return loadXmlConnexionInfoList(wPath+"/server_list.xml");
    return loadXmlConnexionInfoList(QStringLiteral(":/other/default_server_list.xml"));
}

std::vector<Multi::ConnexionInfo> Multi::loadXmlConnexionInfoList(const QByteArray &xmlContent)
{
    std::vector<ConnexionInfo> returnedVar;
    tinyxml2::XMLDocument domDocument;
    const auto loadOkay = domDocument.Parse(xmlContent.data(),xmlContent.size());
    if(loadOkay!=0)
    {
        std::cerr << "Multi::loadXmlConnexionInfoList, " << domDocument.ErrorName() << std::endl;
        return returnedVar;
    }

    const tinyxml2::XMLElement *root = domDocument.RootElement();
    if(root==NULL)
    {
        std::cerr << "Unable to open the file: Multi::loadXmlConnexionInfoList, no root balise found for the xml file" << std::endl;
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
    const std::string &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
    while(server!=NULL)
    {
        if(server->Attribute("unique_code")!=NULL)
        {
            ConnexionInfo connexionInfo;
            connexionInfo.isCustom=false;
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
            Settings::settings.beginGroup(QStringLiteral("Xml-%1").arg(server->Attribute("unique_code")));
            if(Settings::settings.contains(QStringLiteral("connexionCounter")))
            {
                connexionInfo.connexionCounter=Settings::settings.value("connexionCounter").toUInt(&ok);
                if(!ok)
                    connexionInfo.connexionCounter=0;
            }
            else
                connexionInfo.connexionCounter=0;

            //proxy
            if(Settings::settings.contains(QStringLiteral("proxyPort")))
            {
                connexionInfo.proxyPort=static_cast<uint16_t>(Settings::settings.value("proxyPort").toUInt(&ok));
                if(!ok)
                    connexionInfo.proxyPort=9050;
            }
            else
                connexionInfo.proxyPort=9050;
            if(Settings::settings.contains(QStringLiteral("proxyHost")))
                connexionInfo.proxyHost=Settings::settings.value(QStringLiteral("proxyHost")).toString();
            else
                connexionInfo.proxyHost=QString();

            if(Settings::settings.contains(QStringLiteral("lastConnexion")))
            {
                connexionInfo.lastConnexion=Settings::settings.value(QStringLiteral("lastConnexion")).toUInt(&ok);
                if(!ok)
                    connexionInfo.lastConnexion=static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch()/1000);
            }
            else
                connexionInfo.lastConnexion=static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch()/1000);

            //name
            Settings::settings.endGroup();
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

std::vector<Multi::ConnexionInfo> Multi::loadXmlConnexionInfoList(const QString &file)
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

bool Multi::ConnexionInfo::operator<(const ConnexionInfo &connexionInfo) const
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

std::vector<Multi::ConnexionInfo> Multi::loadConfigConnexionInfoList()
{
    std::vector<ConnexionInfo> returnedVar;
    QStringList groups=Settings::settings.childGroups();
    int index=0;
    while(index<groups.size())
    {
        const QString &groupName=groups.at(index);
        Settings::settings.beginGroup(groupName);
        if((Settings::settings.contains("host") && Settings::settings.contains("port")) || Settings::settings.contains("ws"))
        {
            QString ws="";
            if(Settings::settings.contains("ws"))
                ws=Settings::settings.value("ws").toString();
            QString Shost="";
            if(Settings::settings.contains("host"))
                Shost=Settings::settings.value("host").toString();
            QString port_string="";
            if(Settings::settings.contains("port"))
                port_string=Settings::settings.value("port").toString();

            QString name="";
            if(Settings::settings.contains("name"))
                name=Settings::settings.value("name").toString();
            QString connexionCounter="0";
            if(Settings::settings.contains("connexionCounter"))
                connexionCounter=Settings::settings.value("connexionCounter").toString();
            QString lastConnexion="0";
            if(Settings::settings.contains("lastConnexion"))
                lastConnexion=Settings::settings.value("lastConnexion").toString();

            QString proxyHost="";
            if(Settings::settings.contains("proxyHost"))
                proxyHost=Settings::settings.value("proxyHost").toString();
            QString proxyPort="0";
            if(Settings::settings.contains("proxyPort"))
                proxyPort=Settings::settings.value("proxyPort").toString();

            bool ok=true;
            ConnexionInfo connexionInfo;
            if(!ws.isEmpty())
                connexionInfo.ws=ws;
            else
            {
                #ifndef NOTCPSOCKET
                uint16_t port=static_cast<uint16_t>(port_string.toInt(&ok));
                if(!ok)
                    qDebug() << "dropped connexion, port wrong: " << port_string;
                else
                {
                    connexionInfo.host=Shost;
                    connexionInfo.port=port;
                }
                #else
                ok=false;
                #endif
            }
            if(groupName.startsWith("Custom-"))
                connexionInfo.isCustom=true;
            else if(groupName.startsWith("Xml-"))
                connexionInfo.isCustom=false;
            else
                ok=false;
            if(ok)
            {
                if(groupName.startsWith("Custom-"))
                    connexionInfo.unique_code=groupName.mid(7);
                else //Xml-
                    connexionInfo.unique_code=groupName.mid(7);
                connexionInfo.name=name;
                connexionInfo.connexionCounter=connexionCounter.toUInt(&ok);
                if(!ok)
                {
                    qDebug() << "ignored bug for connexion : connexionCounter";
                    connexionInfo.connexionCounter=0;
                }
                connexionInfo.lastConnexion=lastConnexion.toUInt(&ok);
                if(!ok)
                {
                    qDebug() << "ignored bug for connexion : lastConnexion";
                    connexionInfo.lastConnexion=static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch()/1000);
                }
                if(connexionInfo.lastConnexion>(QDateTime::currentMSecsSinceEpoch()/1000))
                {
                    qDebug() << "ignored bug for connexion : lastConnexion<time()";
                    connexionInfo.lastConnexion=static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch()/1000);
                }
                if(!proxyHost.isEmpty())
                {
                    bool ok;
                    uint16_t proxy_port=static_cast<uint16_t>(proxyPort.toInt(&ok));
                    if(ok)
                    {
                        connexionInfo.proxyHost=proxyHost;
                        connexionInfo.proxyPort=proxy_port;
                    }
                }
                returnedVar.push_back(connexionInfo);
            }
        }
        else
            qDebug() << "dropped connexion, info seam wrong";
        index++;
        Settings::settings.endGroup();
    }
    return returnedVar;
}

void Multi::downloadFile()
{
    #ifndef __EMSCRIPTEN__
    QString catchChallengerVersion;
    if(Ultimate::ultimate.isUltimate())
        catchChallengerVersion=QStringLiteral("CatchChallenger Ultimate/%1").arg(QString::fromStdString(CatchChallenger::Version::str));
    else
        catchChallengerVersion=QStringLiteral("CatchChallenger/%1").arg(QString::fromStdString(CatchChallenger::Version::str));

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
    if(!connect(reply, &QNetworkReply::finished, this, &Multi::httpFinished))
        abort();
    /*if(!connect(reply, &QNetworkReply::metaDataChanged, this, &MainWindow::metaDataChanged))
        abort(); seam buggy*/
    ui->warning->setVisible(true);
    ui->warning->setText(tr("Loading the server list..."));
    //ui->server_refresh->setEnabled(false);
}

/*void Multi::on_server_refresh_clicked()
{
    if(reply!=NULL)
    {
        reply->abort();
        reply->deleteLater();
        reply=NULL;
    }
    downloadFile();
}*/


void Multi::server_select_clicked()
{
    if(login!=nullptr)
        delete login;
    login=new Login(this);
    if(!connect(login,&QDialog::accepted,this,&Multi::server_select_finished))
        abort();
    if(!connect(login,&QDialog::rejected,this,&Multi::server_select_finished))
        abort();
    if(selectedServer.isCustom)
        Settings::settings.beginGroup(QStringLiteral("Custom-%1").arg(selectedServer.unique_code));
    else
        Settings::settings.beginGroup(QStringLiteral("Xml-%1").arg(selectedServer.unique_code));
    if(Settings::settings.contains("last"))
    {
        const QString loginString=Settings::settings.value("last").toString();
        login->setLogin(loginString);
        Settings::settings.beginGroup("auth");
        if(Settings::settings.contains(loginString))
            login->setPass(Settings::settings.value(loginString).toString());
        Settings::settings.endGroup();
    }
    Settings::settings.endGroup();
    emit setAbove(login);
}

void Multi::server_select_finished()
{
    emit setAbove(nullptr);
    if(login==nullptr)
        return;
    if(!login->isOk())
        return;
    #ifdef __EMSCRIPTEN__
    std::cerr << "server_select_finished returned" <<  std::endl;
    #endif

    if(selectedServer.isCustom)
        Settings::settings.beginGroup(QStringLiteral("Custom-%1").arg(selectedServer.unique_code));
    else
        Settings::settings.beginGroup(QStringLiteral("Xml-%1").arg(selectedServer.unique_code));
    const QString loginString=login->getLogin();
    Settings::settings.setValue("last",loginString);
    if(login->getRememberPassword())
    {
        Settings::settings.beginGroup("auth");
        Settings::settings.setValue(loginString,login->getPass());
        Settings::settings.endGroup();
    }
    Settings::settings.endGroup();
    Settings::settings.sync();
    unsigned int index=0;
    while(index<mergedConnexionInfoList.size())
    {
        ConnexionInfo &connexionInfo=mergedConnexionInfoList[index];
        if(connexionInfo.isCustom==selectedServer.isCustom && connexionInfo.unique_code==selectedServer.unique_code)
        {
            connexionInfo.connexionCounter++;
            saveConnexionInfoList();
            displayServerList();//need be after connectTheExternalSocket() because it reset selectedServer
            emit connectToServer(connexionInfo,loginString,login->getPass());
            break;
        }
        index++;
    }
}
