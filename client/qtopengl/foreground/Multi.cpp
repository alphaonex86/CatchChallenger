#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QStandardPaths>
#include <QDateTime>
#include "Multi.hpp"
#include "../../../general/base/FacilityLibGeneral.hpp"
#include "../../../general/base/tinyXML2/tinyxml2.hpp"
#include "../../../general/base/cpp11addition.hpp"
#include "../../../general/base/Version.hpp"
#include "../../qt/PlatformMacro.hpp"
#include "../../qt/Settings.hpp"
#include "../../qt/Ultimate.hpp"
#include "../../qt/ClientVariable.hpp"
#include "../Language.hpp"
#include "../MultiItem.hpp"
#include "../above/AddOrEditServer.hpp"
#include "../above/Login.hpp"
#include <iostream>
#include <utime.h>
#if defined(_WIN32) || defined(Q_OS_MAC)
#include "../../qt/InternetUpdater.hpp"
#endif

Multi::Multi() :
    reply(nullptr)
{
    srand(time(0));
    login=nullptr;

    temp_xmlConnexionInfoList=loadXmlConnexionInfoList();
    temp_customConnexionInfoList=loadConfigConnexionInfoList();
    mergedConnexionInfoList=temp_customConnexionInfoList;
    mergedConnexionInfoList.insert(mergedConnexionInfoList.end(),temp_xmlConnexionInfoList.begin(),temp_xmlConnexionInfoList.end());
    std::sort(mergedConnexionInfoList.begin(),mergedConnexionInfoList.end());
    selectedServer.unique_code.clear();
    selectedServer.isCustom=false;

    server_add=new CustomButton(":/CC/images/interface/greenbutton.png",this);
    server_add->setOutlineColor(QColor(44,117,0));
    server_remove=new CustomButton(":/CC/images/interface/delete.png",this);
    server_remove->setOutlineColor(QColor(125,0,0));
    server_edit=new CustomButton(":/CC/images/interface/greenbutton.png",this);
    server_edit->setOutlineColor(QColor(44,117,0));
    server_refresh=new CustomButton(":/CC/images/interface/refresh.png",this);

    server_select=new CustomButton(":/CC/images/interface/next.png",this);
    back=new CustomButton(":/CC/images/interface/back.png",this);

    wdialog=new ImagesStrechMiddle(46,":/CC/images/interface/message.png",this);
    warning=new QGraphicsTextItem(this);
    serverEmpty=new QGraphicsTextItem(this);
    scrollZone=new CCScrollZone(this);

    if(!connect(server_add,&CustomButton::clicked,this,&Multi::server_add_clicked))
        abort();
    if(!connect(server_remove,&CustomButton::clicked,this,&Multi::server_remove_clicked))
        abort();
    if(!connect(server_edit,&CustomButton::clicked,this,&Multi::server_edit_clicked))
        abort();
    if(!connect(server_select,&CustomButton::clicked,this,&Multi::server_select_clicked))
        abort();
    if(!connect(back,&CustomButton::clicked,this,&Multi::backMain))
        abort();
    if(!connect(server_refresh,&CustomButton::clicked,this,&Multi::on_server_refresh_clicked))
        abort();
    newLanguage();

    //need be the last
    downloadFile();
    displayServerList();
    addServer=nullptr;
}

Multi::~Multi()
{
}

void Multi::displayServerList()
{
    serverEmpty->setVisible(mergedConnexionInfoList.empty());
    #if defined(NOTCPSOCKET) && defined(NOWEBSOCKET)
    #error Web socket and tcp socket are both not supported
    return;
    #endif
    if(selectedServer.unique_code.isEmpty())
    {
        server_remove->setEnabled(false);
        server_edit->setEnabled(false);
    }
    //clean the previous content
    foreach (MultiItem *item, serverConnexion)
        delete item;
    serverConnexion.clear();
    server_select->setEnabled(!selectedServer.unique_code.isEmpty());
    //serverWidget->setVisible(index==0);
    unsigned int index=0;
    while(index<mergedConnexionInfoList.size())
    {
        const ConnexionInfo &connexionInfo=mergedConnexionInfoList.at(index);
        if(connexionInfo.host.isEmpty() && connexionInfo.ws.isEmpty())
        {
            index++;
            continue;
        }
        MultiItem *newEntry=new MultiItem(connexionInfo,scrollZone);
        if(selectedServer.unique_code==connexionInfo.unique_code)
        {
            server_edit->setEnabled(connexionInfo.isCustom);
            server_remove->setEnabled(connexionInfo.isCustom);
            newEntry->setSelected(true);
        }
        else
            newEntry->setSelected(false);
        /*if(!connect(newEntry,&ListEntryEnvolued::clicked,this,&Multi::serverListEntryEnvoluedClicked,Qt::QueuedConnection))
            abort();
        if(!connect(newEntry,&ListEntryEnvolued::doubleClicked,this,&Multi::serverListEntryEnvoluedDoubleClicked,Qt::QueuedConnection))
            abort();*/

        //scrollAreaWidgetContentsServer->layout()->addWidget(newEntry);

        serverConnexion.push_back(newEntry);
        index++;
    }
}

void Multi::server_add_clicked()
{
    if(addServer==nullptr)
    {
        addServer=new AddOrEditServer();
        if(!connect(addServer,&AddOrEditServer::removeAbove,this,&Multi::server_add_finished))
            abort();
    }
    emit setAbove(addServer);
}

void Multi::server_add_finished()
{
    emit setAbove(nullptr);
    if(addServer==nullptr)
        return;
    if(!addServer->isOk())
        return;
    if(!Settings::settings->isWritable())
    {
        warning->setHtml(tr("Option is not writable")+": "+QString::number(Settings::settings->status()));
        warning->setVisible(true);
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
    temp_customConnexionInfoList.push_back(connexionInfo);
    mergedConnexionInfoList=temp_customConnexionInfoList;
    mergedConnexionInfoList.insert(mergedConnexionInfoList.end(),temp_xmlConnexionInfoList.begin(),temp_xmlConnexionInfoList.end());
    std::sort(mergedConnexionInfoList.begin(),mergedConnexionInfoList.end());
    saveConnexionInfoList();
    displayServerList();
}

void Multi::server_edit_clicked()
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
            if(addServer==nullptr)
            {
                addServer=new AddOrEditServer();
                if(!connect(addServer,&AddOrEditServer::removeAbove,this,&Multi::server_edit_finished))
                    abort();
            }
            #if ! defined(NOTCPSOCKET) && ! defined(NOWEBSOCKET)
            if(connexionInfo.ws.isEmpty())
            {
                addServer->setType(0);
                addServer->setServer(connexionInfo.host);
                addServer->setPort(connexionInfo.port);
            }
            else
            {
                addServer->setType(1);
                addServer->setServer(connexionInfo.ws);
            }
            #else
                #if defined(NOTCPSOCKET)
                addServer->setType(1);
                addServer->setServer(connexionInfo.ws);
                #else
                    #if defined(NOWEBSOCKET)
                    addServer->setType(0);
                    addServer->setServer(connexionInfo.host);
                    addServer->setPort(connexionInfo.port);
                    #endif
                #endif
            #endif
            addServer->setName(connexionInfo.name);
            addServer->setProxyServer(connexionInfo.proxyHost);
            addServer->setProxyPort(connexionInfo.proxyPort);
            addServer->setEdit(true);
            emit setAbove(addServer);
            return;
        }
        index++;
    }
    std::cerr << "remove server not found" << std::endl;
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
    unsigned int index=0;
    while(index<temp_customConnexionInfoList.size())
    {
        ConnexionInfo &connexionInfo=temp_customConnexionInfoList[index];
        if(connexionInfo.isCustom==selectedServer.isCustom && connexionInfo.unique_code==selectedServer.unique_code)
        {
            if(!connexionInfo.isCustom)
                return;

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
        }
        index++;
    }
    mergedConnexionInfoList=temp_customConnexionInfoList;
    mergedConnexionInfoList.insert(mergedConnexionInfoList.end(),temp_xmlConnexionInfoList.begin(),temp_xmlConnexionInfoList.end());
    std::sort(mergedConnexionInfoList.begin(),mergedConnexionInfoList.end());
    saveConnexionInfoList();
    displayServerList();
}

void Multi::server_remove_clicked()
{
    if(selectedServer.unique_code.isEmpty())
        return;
    unsigned int index=0;
    while(index<temp_customConnexionInfoList.size())
    {
        ConnexionInfo &connexionInfo=temp_customConnexionInfoList[index];
        if(connexionInfo.isCustom==selectedServer.isCustom && connexionInfo.unique_code==selectedServer.unique_code)
        {
            if(!connexionInfo.isCustom)
                return;
            temp_customConnexionInfoList.erase(temp_customConnexionInfoList.begin()+index);
            selectedServer.unique_code.clear();
            selectedServer.isCustom=false;
            break;
        }
        index++;
    }
    mergedConnexionInfoList=temp_customConnexionInfoList;
    mergedConnexionInfoList.insert(mergedConnexionInfoList.end(),temp_xmlConnexionInfoList.begin(),temp_xmlConnexionInfoList.end());
    std::sort(mergedConnexionInfoList.begin(),mergedConnexionInfoList.end());
    saveConnexionInfoList();
    displayServerList();
}

void Multi::httpFinished()
{
    warning->setVisible(false);
    QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (reply->error())
    {
        warning->setHtml(tr("Get server list failed: %1").arg(reply->errorString()));
        std::cerr << "Get server list failed: " << reply->errorString().toStdString() << std::endl;
        reply->deleteLater();
        reply=NULL;
        return;
    } else if (!redirectionTarget.isNull()) {
        warning->setHtml(tr("Get server list redirection denied to: %1").arg(reply->errorString()));
        std::cerr << "Get server list redirection denied to: " << reply->errorString().toStdString() << std::endl;
        reply->deleteLater();
        reply=NULL;
        return;
    }
    std::cout << "Got new server list" << std::endl;

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
    QSet<QString> valid;
    unsigned int index=0;
    while(index<temp_customConnexionInfoList.size())
    {
        const ConnexionInfo &connexionInfo=temp_customConnexionInfoList.at(index);
        if(connexionInfo.unique_code.isEmpty())
            abort();

        if(connexionInfo.isCustom)
        {
            valid.insert(QStringLiteral("Custom-%1").arg(connexionInfo.unique_code));
            Settings::settings->beginGroup(QStringLiteral("Custom-%1").arg(connexionInfo.unique_code));
            if(!connexionInfo.ws.isEmpty())
            {
                Settings::settings->setValue(QStringLiteral("ws"),connexionInfo.ws);
                Settings::settings->remove("host");
                Settings::settings->remove("port");
            }
            else {
                Settings::settings->setValue(QStringLiteral("host"),connexionInfo.host);
                Settings::settings->setValue(QStringLiteral("port"),connexionInfo.port);
                Settings::settings->remove("ws");
            }
            Settings::settings->setValue(QStringLiteral("name"),connexionInfo.name);
        }
        else
            Settings::settings->beginGroup(QStringLiteral("Xml-%1").arg(connexionInfo.unique_code));
        if(connexionInfo.connexionCounter>0)
            Settings::settings->setValue(QStringLiteral("connexionCounter"),connexionInfo.connexionCounter);
        else
            Settings::settings->remove(QStringLiteral("connexionCounter"));
        if(connexionInfo.lastConnexion>0 && connexionInfo.connexionCounter>0)
            Settings::settings->setValue(QStringLiteral("lastConnexion"),(quint64)connexionInfo.lastConnexion);
        else
            Settings::settings->remove(QStringLiteral("lastConnexion"));
        Settings::settings->setValue(QStringLiteral("proxyHost"),connexionInfo.proxyHost);
        Settings::settings->setValue(QStringLiteral("proxyPort"),connexionInfo.proxyPort);
        Settings::settings->endGroup();
        index++;
    }
    QStringList groups=Settings::settings->childGroups();
    index=0;
    while(index<(unsigned int)groups.size())
    {
        const QString &groupName=groups.at(index);
        if(groupName.startsWith("Custom-") && !valid.contains(groupName))
            Settings::settings->remove(groupName);
        index++;
    }
    Settings::settings->sync();
}

std::vector<ConnexionInfo> Multi::loadXmlConnexionInfoList()
{
    QString wPath=QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    if(QFileInfo(wPath+"/server_list.xml").isFile())
        return loadXmlConnexionInfoList(wPath+"/server_list.xml");
    return loadXmlConnexionInfoList(QStringLiteral(":/CC/other/default_server_list.xml"));
}

std::vector<ConnexionInfo> Multi::loadXmlConnexionInfoList(const QByteArray &xmlContent)
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
    #ifndef CATCHCHALLENGER_BOT
    const std::string &language=Language::language.getLanguage().toStdString();
    #else
    const std::string language("en");
    #endif
    while(server!=NULL)
    {
        if(server->Attribute("unique_code")!=NULL)
        {
            ConnexionInfo connexionInfo;
            connexionInfo.isCustom=false;
            connexionInfo.unique_code=server->Attribute("unique_code");
            connexionInfo.port=0;
            connexionInfo.connexionCounter=0;
            connexionInfo.lastConnexion=0;
            connexionInfo.proxyPort=0;

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
            Settings::settings->beginGroup(QStringLiteral("Xml-%1").arg(server->Attribute("unique_code")));
            if(Settings::settings->contains(QStringLiteral("connexionCounter")))
            {
                connexionInfo.connexionCounter=Settings::settings->value("connexionCounter").toUInt(&ok);
                if(!ok)
                    connexionInfo.connexionCounter=0;
            }
            else
                connexionInfo.connexionCounter=0;

            //proxy
            if(Settings::settings->contains(QStringLiteral("proxyPort")))
            {
                connexionInfo.proxyPort=static_cast<uint16_t>(Settings::settings->value("proxyPort").toUInt(&ok));
                if(!ok)
                    connexionInfo.proxyPort=9050;
            }
            else
                connexionInfo.proxyPort=9050;
            if(Settings::settings->contains(QStringLiteral("proxyHost")))
                connexionInfo.proxyHost=Settings::settings->value(QStringLiteral("proxyHost")).toString();
            else
                connexionInfo.proxyHost=QString();

            if(Settings::settings->contains(QStringLiteral("lastConnexion")))
            {
                connexionInfo.lastConnexion=Settings::settings->value(QStringLiteral("lastConnexion")).toUInt(&ok);
                if(!ok)
                    connexionInfo.lastConnexion=static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch()/1000);
            }
            else
                connexionInfo.lastConnexion=static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch()/1000);

            //name
            Settings::settings->endGroup();
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

std::vector<ConnexionInfo> Multi::loadXmlConnexionInfoList(const QString &file)
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

std::vector<ConnexionInfo> Multi::loadConfigConnexionInfoList()
{
    std::vector<ConnexionInfo> returnedVar;
    QStringList groups=Settings::settings->childGroups();
    int index=0;
    while(index<groups.size())
    {
        const QString &groupName=groups.at(index);
        Settings::settings->beginGroup(groupName);
        if((Settings::settings->contains("host") && Settings::settings->contains("port")) || Settings::settings->contains("ws"))
        {
            QString ws="";
            if(Settings::settings->contains("ws"))
                ws=Settings::settings->value("ws").toString();
            QString Shost="";
            if(Settings::settings->contains("host"))
                Shost=Settings::settings->value("host").toString();
            QString port_string="";
            if(Settings::settings->contains("port"))
                port_string=Settings::settings->value("port").toString();

            QString name="";
            if(Settings::settings->contains("name"))
                name=Settings::settings->value("name").toString();
            QString connexionCounter="0";
            if(Settings::settings->contains("connexionCounter"))
                connexionCounter=Settings::settings->value("connexionCounter").toString();
            QString lastConnexion="0";
            if(Settings::settings->contains("lastConnexion"))
                lastConnexion=Settings::settings->value("lastConnexion").toString();

            QString proxyHost="";
            if(Settings::settings->contains("proxyHost"))
                proxyHost=Settings::settings->value("proxyHost").toString();
            QString proxyPort="0";
            if(Settings::settings->contains("proxyPort"))
                proxyPort=Settings::settings->value("proxyPort").toString();

            bool ok=true;
            ConnexionInfo connexionInfo;
            connexionInfo.isCustom=false;
            connexionInfo.port=0;
            connexionInfo.connexionCounter=0;
            connexionInfo.lastConnexion=0;
            connexionInfo.proxyPort=0;

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
        Settings::settings->endGroup();
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

    QNetworkRequest networkRequest;
    networkRequest.setUrl(QUrl(CATCHCHALLENGER_SERVER_LIST_URL));
    #ifndef __EMSCRIPTEN__
    networkRequest.setHeader(QNetworkRequest::UserAgentHeader,catchChallengerVersion);
    #endif

    reply = qnam.get(networkRequest);
    if(!connect(reply, &QNetworkReply::finished, this, &Multi::httpFinished))
        abort();
    //if(!connect(reply, &QNetworkReply::metaDataChanged, this, &MainWindow::metaDataChanged))
        //abort(); seam buggy
    warning->setVisible(true);
    warning->setHtml(tr("Loading the server list..."));
    //ui->server_refresh->setEnabled(false);
}

void Multi::on_server_refresh_clicked()
{
    if(reply!=NULL)
    {
        reply->abort();
        reply->deleteLater();
        reply=NULL;
    }
    downloadFile();
}


void Multi::server_select_clicked()
{
    if(login==nullptr)
    {
        login=new Login();
        if(!connect(login,&Login::removeAbove,this,&Multi::server_select_finished))
            abort();
    }
    if(selectedServer.isCustom)
        Settings::settings->beginGroup(QStringLiteral("Custom-%1").arg(selectedServer.unique_code));
    else
        Settings::settings->beginGroup(QStringLiteral("Xml-%1").arg(selectedServer.unique_code));
    if(Settings::settings->contains("last"))
        login->setAuth(Settings::settings->value("last").toStringList());
    int index=0;
    while((unsigned int)index<(unsigned int)mergedConnexionInfoList.size())
    {
        auto e=mergedConnexionInfoList.at(index);
        if(e.isCustom==selectedServer.isCustom && e.unique_code==selectedServer.unique_code)
        {
            login->setLinks(e.site_page,e.register_page);
            break;
        }
        index++;
    }
    Settings::settings->endGroup();
    emit setAbove(login);
}

void Multi::server_select_finished()
{
    emit setAbove(nullptr);
    if(login==nullptr)
        return;
    if(!login->isOk())
        return;
    if(selectedServer.isCustom)
        Settings::settings->beginGroup(QStringLiteral("Custom-%1").arg(selectedServer.unique_code));
    else
        Settings::settings->beginGroup(QStringLiteral("Xml-%1").arg(selectedServer.unique_code));
    QStringList v=login->getAuth();
    if(!v.isEmpty())
    {
        Settings::settings->setValue("last",v);
        Settings::settings->sync();
    }
    Settings::settings->endGroup();
    #ifdef __EMSCRIPTEN__
    std::cerr << "server_select_finished returned" <<  std::endl;
    #endif

    if(selectedServer.isCustom)
        Settings::settings->beginGroup(QStringLiteral("Custom-%1").arg(selectedServer.unique_code));
    else
        Settings::settings->beginGroup(QStringLiteral("Xml-%1").arg(selectedServer.unique_code));
    const QString loginString=login->getLogin();
    Settings::settings->endGroup();
    Settings::settings->sync();
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

void Multi::newLanguage()
{
    server_add->setText(tr("Add"));
    //server_remove->setText(tr("Remove"));
    server_edit->setText(tr("Edit"));
    warning->setHtml("<span style=\"background-color: rgb(255, 255, 163);\nborder: 1px solid rgb(255, 221, 50);\nborder-radius:5px;\ncolor: rgb(0, 0, 0);\">"+tr("Loading the servers list...")+"</span>");
    serverEmpty->setHtml(QStringLiteral("<html><body><p align=\"center\"><span style=\"font-size:12pt;color:#a0a0a0;\">%1</span></p></body></html>").arg(tr("Empty")));
}

QRectF Multi::boundingRect() const
{
    return QRectF();
}

void Multi::paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *w)
{
    unsigned int space=10;
//    unsigned int fontSize=20;
    unsigned int multiItemH=100;
    /*if(w->height()>300)
        fontSize=w->height()/6;*/
    if(w->width()<600 || w->height()<600)
    {
        server_add->setSize(148,61);
        server_add->setPixelSize(23);
        server_remove->setSize(56,62);
        server_edit->setSize(148,61);
        server_edit->setPixelSize(23);
        server_refresh->setSize(56,62);
        server_select->setSize(56,62);
        back->setSize(56,62);
        multiItemH=50;
    }
    else if(w->width()<900 || w->height()<600)
    {
        server_add->setSize(148,61);
        server_add->setPixelSize(23);
        server_remove->setSize(56,62);
        server_edit->setSize(148,61);
        server_edit->setPixelSize(23);
        server_refresh->setSize(56,62);
        server_select->setSize(84,93);
        back->setSize(84,93);
        multiItemH=75;
    }
    else {
        space=30;
        server_add->setSize(223,92);
        server_add->setPixelSize(35);
        server_remove->setSize(84,93);
        server_edit->setSize(223,92);
        server_edit->setPixelSize(35);
        server_refresh->setSize(84,93);
        server_select->setSize(84,93);
        back->setSize(84,93);
    }


    unsigned int tWidthTButton=server_add->width()+space+
            server_edit->width()+space+
            server_remove->width()+space+
            server_refresh->width();
    unsigned int tXTButton=w->width()/2-tWidthTButton/2;
    unsigned int tWidthTButtonOffset=0;
    unsigned int y=w->height()-space-server_select->height()-space-server_add->height();
    server_remove->setPos(tXTButton+tWidthTButtonOffset,y);
    tWidthTButtonOffset+=server_remove->width()+space;
    server_add->setPos(tXTButton+tWidthTButtonOffset,y);
    tWidthTButtonOffset+=server_add->width()+space;
    server_edit->setPos(tXTButton+tWidthTButtonOffset,y);
    tWidthTButtonOffset+=server_edit->width()+space;
    server_refresh->setPos(tXTButton+tWidthTButtonOffset,y);

    tWidthTButton=server_select->width()+space+
            back->width();
    y=w->height()-space-server_select->height();
    tXTButton=w->width()/2-tWidthTButton/2;
    back->setPos(tXTButton,y);
    server_select->setPos(tXTButton+back->width()+space,y);

    y=w->height()-space-server_select->height()-space-server_add->height();
    wdialog->setHeight(w->height()-space-server_select->height()-space-server_add->height()-space-space);
    if((unsigned int)w->width()<(800+space*2))
    {
        wdialog->setWidth(w->width()-space*2);
        wdialog->setPos(space,space);
    }
    else
    {
        wdialog->setWidth(800);
        wdialog->setPos(w->width()/2-wdialog->width()/2,space);
    }
    warning->setPos(w->width()/2-warning->boundingRect().width(),space+wdialog->height()-wdialog->currentBorderSize()-warning->boundingRect().height());

    unsigned int offsetMultiItem=0;
    foreach(MultiItem *item, serverConnexion)
    {
        item->setPos(wdialog->x()+wdialog->currentBorderSize(),space+wdialog->currentBorderSize()+offsetMultiItem);
        item->setSize(wdialog->width()-wdialog->currentBorderSize()*2,multiItemH);
        offsetMultiItem+=multiItemH+space/3;
    }
}

void Multi::mousePressEventXY(const QPointF &p, bool &pressValidated, bool &callParentClass)
{
    server_add->mousePressEventXY(p,pressValidated);
    server_remove->mousePressEventXY(p,pressValidated);
    server_edit->mousePressEventXY(p,pressValidated);
    server_refresh->mousePressEventXY(p,pressValidated);
    back->mousePressEventXY(p,pressValidated);
    server_select->mousePressEventXY(p,pressValidated);
    foreach(MultiItem *item, serverConnexion)
        item->mousePressEventXY(p,pressValidated);
}

void Multi::mouseReleaseEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass)
{
    server_add->mouseReleaseEventXY(p,pressValidated);
    server_remove->mouseReleaseEventXY(p,pressValidated);
    server_edit->mouseReleaseEventXY(p,pressValidated);
    server_refresh->mouseReleaseEventXY(p,pressValidated);
    back->mouseReleaseEventXY(p,pressValidated);
    server_select->mouseReleaseEventXY(p,pressValidated);
    MultiItem *newSelectedItem=nullptr;
    foreach(MultiItem *item, serverConnexion)
    {
        const bool wasSelected=item->isSelected();
        item->mouseReleaseEventXY(p,pressValidated);
        if(wasSelected==false && newSelectedItem==nullptr && item->isSelected())
        {
            newSelectedItem=item;
            const ConnexionInfo &info=item->connexionInfo();
            selectedServer.unique_code=info.unique_code;
            selectedServer.isCustom=info.isCustom;
            server_edit->setEnabled(info.isCustom);
            server_remove->setEnabled(info.isCustom);
        }
    }
    server_select->setEnabled(!selectedServer.unique_code.isEmpty());
    if(newSelectedItem!=nullptr)
        foreach(MultiItem *item, serverConnexion)
            if(newSelectedItem!=item)
                item->setSelected(false);
}
