#include "SubServer.hpp"
#include "../Language.hpp"

SubServer::SubServer() :
    /*addServer(nullptr),
    login(nullptr),*/
    reply(nullptr)
{
    srand(time(0));
    login=nullptr;

    temp_xmlConnexionInfoList=loadXmlConnexionInfoList();
    temp_customConnexionInfoList=loadConfigConnexionInfoList();
    mergedConnexionInfoList=temp_customConnexionInfoList;
    mergedConnexionInfoList.insert(mergedConnexionInfoList.end(),temp_xmlConnexionInfoList.begin(),temp_xmlConnexionInfoList.end());
    std::sort(mergedConnexionInfoList.begin(),mergedConnexionInfoList.end());

    server_add=new CustomButton(":/CC/images/interface/greenbutton.png",this);
    server_add->setOutlineColor(QColor(44,117,0));
    server_remove=new CustomButton(":/CC/images/interface/redbutton.png",this);
    server_remove->setOutlineColor(QColor(125,0,0));
    server_edit=new CustomButton(":/CC/images/interface/greenbutton.png",this);
    server_edit->setOutlineColor(QColor(44,117,0));
    server_refresh=new CustomButton(":/CC/images/interface/refresh.png",this);

    server_select=new CustomButton(":/CC/images/interface/next.png",this);
    back=new CustomButton(":/CC/images/interface/back.png",this);

    wdialog=new CCWidget(this);
    warning=new QGraphicsTextItem(this);
    serverEmpty=new QGraphicsTextItem(this);
    scrollZone=new CCScrollZone(this);

    if(!connect(server_add,&CustomButton::clicked,this,&SubServer::server_add_clicked))
        abort();
    if(!connect(server_remove,&CustomButton::clicked,this,&SubServer::server_remove_clicked))
        abort();
    if(!connect(server_edit,&CustomButton::clicked,this,&SubServer::server_edit_clicked))
        abort();
    if(!connect(server_select,&CustomButton::clicked,this,&SubServer::server_select_clicked))
        abort();
    if(!connect(server_refresh,&CustomButton::clicked,this,&SubServer::on_server_refresh_clicked))
        abort();
    newLanguage();

    //need be the last
    downloadFile();
    displayServerList();
    addServer=nullptr;
}

SubServer::~SubServer()
{
}

void SubServer::displayServerList()
{
    serverEmpty->setVisible(mergedConnexionInfoList.empty());
    #if defined(NOTCPSOCKET) && defined(NOWEBSOCKET)
    #error Web socket and tcp socket are both not supported
    return;
    #endif
    std::cout << "display mergedConnexionInfoList.size(): " << mergedConnexionInfoList.size() << std::endl;
}

void SubServer::server_add_clicked()
{
    if(addServer!=nullptr)
        delete addServer;
    addServer=new AddOrEditServer();
    if(!connect(addServer,&AddOrEditServer::quitOption,this,&SubServer::server_add_finished))
        abort();
    emit setAbove(addServer);
}

void SubServer::server_add_finished()
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

void SubServer::server_edit_clicked()
{
    if(addServer!=nullptr)
        delete addServer;

}

void SubServer::server_edit_finished()
{
    emit setAbove(nullptr);
}

void SubServer::server_remove_clicked()
{
}

void SubServer::httpFinished()
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

void SubServer::saveConnexionInfoList()
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
            Settings::settings->setValue(QStringLiteral("lastConnexion"),connexionInfo.lastConnexion);
        else
            Settings::settings->remove(QStringLiteral("lastConnexion"));
        Settings::settings->setValue(QStringLiteral("proxyHost"),connexionInfo.proxyHost);
        Settings::settings->setValue(QStringLiteral("proxyPort"),connexionInfo.proxyPort);
        Settings::settings->endGroup();
        index++;
    }
    QStringList groups=Settings::settings->childGroups();
    index=0;
    while(index<groups.size())
    {
        const QString &groupName=groups.at(index);
        if(groupName.startsWith("Custom-") && !valid.contains(groupName))
            Settings::settings->remove(groupName);
        index++;
    }
    Settings::settings->sync();
}

std::vector<ConnexionInfo> SubServer::loadXmlConnexionInfoList()
{
    QString wPath=QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    if(QFileInfo(wPath+"/server_list.xml").isFile())
        return loadXmlConnexionInfoList(wPath+"/server_list.xml");
    return loadXmlConnexionInfoList(QStringLiteral(":/other/default_server_list.xml"));
}

std::vector<ConnexionInfo> SubServer::loadXmlConnexionInfoList(const QByteArray &xmlContent)
{
    std::vector<ConnexionInfo> returnedVar;
    tinyxml2::XMLDocument domDocument;
    const auto loadOkay = domDocument.Parse(xmlContent.data(),xmlContent.size());
    if(loadOkay!=0)
    {
        std::cerr << "SubServer::loadXmlConnexionInfoList, " << domDocument.ErrorName() << std::endl;
        return returnedVar;
    }

    const tinyxml2::XMLElement *root = domDocument.RootElement();
    if(root==NULL)
    {
        std::cerr << "Unable to open the file: SubServer::loadXmlConnexionInfoList, no root balise found for the xml file" << std::endl;
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

std::vector<ConnexionInfo> SubServer::loadXmlConnexionInfoList(const QString &file)
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

std::vector<ConnexionInfo> SubServer::loadConfigConnexionInfoList()
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

void SubServer::downloadFile()
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
    if(!connect(reply, &QNetworkReply::finished, this, &SubServer::httpFinished))
        abort();
    //if(!connect(reply, &QNetworkReply::metaDataChanged, this, &MainWindow::metaDataChanged))
        //abort(); seam buggy
    warning->setVisible(true);
    warning->setHtml(tr("Loading the server list..."));
    //ui->server_refresh->setEnabled(false);
}

void SubServer::on_server_refresh_clicked()
{
    if(reply!=NULL)
    {
        reply->abort();
        reply->deleteLater();
        reply=NULL;
    }
    downloadFile();
}


void SubServer::server_select_clicked()
{
    if(login!=nullptr)
        delete login;
    login=new Login();
    if(!connect(login,&Login::quitLogin,this,&SubServer::server_select_finished))
        abort();
    emit setAbove(login);
}

void SubServer::server_select_finished()
{
    emit setAbove(nullptr);
    if(login==nullptr)
        return;
    if(!login->isOk())
        return;
}

void SubServer::newLanguage()
{
    server_add->setText(tr("Add"));
    server_remove->setText(tr("Remove"));
    server_edit->setText(tr("Edit"));
    warning->setHtml("<span style=\"background-color: rgb(255, 255, 163);\nborder: 1px solid rgb(255, 221, 50);\nborder-radius:5px;\ncolor: rgb(0, 0, 0);\">"+tr("Loading the servers list...")+"</span>");
    serverEmpty->setHtml(QStringLiteral("<html><body><p align=\"center\"><span style=\"font-size:12pt;color:#a0a0a0;\">%1</span></p></body></html>").arg(tr("Empty")));
}

QRectF SubServer::boundingRect() const
{
    return QRectF();
}

void SubServer::paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *w)
{
    unsigned int space=10;
    unsigned int fontSize=20;
    unsigned int multiItemH=100;
    if(w->height()>300)
        fontSize=w->height()/6;
    if(w->width()<600 || w->height()<600)
    {
        server_add->setSize(148,61);
        server_add->setPixelSize(23);
        server_remove->setSize(148,61);
        server_remove->setPixelSize(23);
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
        server_remove->setSize(148,61);
        server_remove->setPixelSize(23);
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
        server_remove->setSize(224,92);
        server_remove->setPixelSize(35);
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
    server_add->setPos(tXTButton+tWidthTButtonOffset,y);
    tWidthTButtonOffset+=server_add->width()+space;
    server_edit->setPos(tXTButton+tWidthTButtonOffset,y);
    tWidthTButtonOffset+=server_edit->width()+space;
    server_remove->setPos(tXTButton+tWidthTButtonOffset,y);
    tWidthTButtonOffset+=server_remove->width()+space;
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
}

void SubServer::mousePressEventXY(const QPointF &p,bool &pressValidated)
{
    back->mousePressEventXY(p,pressValidated);
    server_select->mousePressEventXY(p,pressValidated);
}

void SubServer::mouseReleaseEventXY(const QPointF &p, bool &pressValidated)
{
    back->mouseReleaseEventXY(p,pressValidated);
    server_select->mouseReleaseEventXY(p,pressValidated);
}

void SubServer::logged(std::vector<CatchChallenger::ServerFromPoolForDisplay> serverOrdenedList)
{
    //do the grouping for characterGroup count
    {
        serverByCharacterGroup.clear();
        unsigned int index=0;
        uint8_t serverByCharacterGroupTempIndexToDisplay=1;
        while(index<serverOrdenedList.size())
        {
            const ServerFromPoolForDisplay &server=serverOrdenedList.at(index);
            if(server.charactersGroupIndex>serverOrdenedList.size())
                abort();
            if(serverByCharacterGroup.find(server.charactersGroupIndex)!=serverByCharacterGroup.cend())
                serverByCharacterGroup[server.charactersGroupIndex].first++;
            else
            {
                serverByCharacterGroup[server.charactersGroupIndex].first=1;
                serverByCharacterGroup[server.charactersGroupIndex].second=serverByCharacterGroupTempIndexToDisplay;
                serverByCharacterGroupTempIndexToDisplay++;
            }
            index++;
        }
    }

    //clear and determine what kind of view
    ui->serverList->clear();
    LogicialGroup logicialGroup=client->getLogicialGroup();
    bool fullView=true;
    if(serverOrdenedList.size()>10)
        fullView=false;
    const uint64_t &current__date=QDateTime::currentDateTime().toTime_t();

    //reload, bug if before init
    if(icon_server_list_star1.isNull())
    {
        SubServer::icon_server_list_star1=QIcon(":/CC/images/interface/server_list/star1.png");
        if(SubServer::icon_server_list_star1.isNull())
            abort();
        SubServer::icon_server_list_star2=QIcon(":/CC/images/interface/server_list/star2.png");
        SubServer::icon_server_list_star3=QIcon(":/CC/images/interface/server_list/star3.png");
        SubServer::icon_server_list_star4=QIcon(":/CC/images/interface/server_list/star4.png");
        SubServer::icon_server_list_star5=QIcon(":/CC/images/interface/server_list/star5.png");
        SubServer::icon_server_list_star6=QIcon(":/CC/images/interface/server_list/star6.png");
        SubServer::icon_server_list_stat1=QIcon(":/CC/images/interface/server_list/stat1.png");
        SubServer::icon_server_list_stat2=QIcon(":/CC/images/interface/server_list/stat2.png");
        SubServer::icon_server_list_stat3=QIcon(":/CC/images/interface/server_list/stat3.png");
        SubServer::icon_server_list_stat4=QIcon(":/CC/images/interface/server_list/stat4.png");
        SubServer::icon_server_list_bug=QIcon(":/CC/images/interface/server_list/bug.png");
        if(SubServer::icon_server_list_bug.isNull())
            abort();
        icon_server_list_color.push_back(QIcon(":/CC/images/colorflags/0.png"));
        icon_server_list_color.push_back(QIcon(":/CC/images/colorflags/1.png"));
        icon_server_list_color.push_back(QIcon(":/CC/images/colorflags/2.png"));
        icon_server_list_color.push_back(QIcon(":/CC/images/colorflags/3.png"));
    }
    //do the average value
    {
        averagePlayedTime=0;
        averageLastConnect=0;
        int entryCount=0;
        unsigned int index=0;
        while(index<serverOrdenedList.size())
        {
            const ServerFromPoolForDisplay &server=serverOrdenedList.at(index);
            if(server.playedTime>0 && server.lastConnect<=current__date)
            {
                averagePlayedTime+=server.playedTime;
                averageLastConnect+=server.lastConnect;
                entryCount++;
            }
            index++;
        }
        if(entryCount>0)
        {
            averagePlayedTime/=entryCount;
            averageLastConnect/=entryCount;
        }
    }
    addToServerList(logicialGroup,ui->serverList->invisibleRootItem(),current__date,fullView);
    ui->serverList->expandAll();
}

bool CatchChallenger::ServerFromPoolForDisplay::operator<(const ServerFromPoolForDisplay &serverFromPoolForDisplay) const
{
    if(serverFromPoolForDisplay.uniqueKey<this->uniqueKey)
        return true;
    else
        return false;
}

void SubServer::addToServerList(LogicialGroup &logicialGroup, QTreeWidgetItem *item, const uint64_t &currentDate, const bool &fullView)
{
    if(client->getServerOrdenedList().empty())
        std::cerr << "SubServer::addToServerList(): client->serverOrdenedList.empty()" << std::endl;
    item->setText(0,QString::fromStdString(logicialGroup.name));
    {
        //to order the group
        std::vector<std::string> keys;
        for(const auto &n : logicialGroup.logicialGroupList)
            keys.push_back(n.first);
        qSort(keys);
        //list the group
        unsigned int index=0;
        while(index<keys.size())
        {
            QTreeWidgetItem * const itemGroup=new QTreeWidgetItem(item);
            addToServerList(*logicialGroup.logicialGroupList[keys.at(index)],itemGroup,currentDate,fullView);
            index++;
        }
    }
    {
        qSort(logicialGroup.servers);
        //list the server
        unsigned int index=0;
        while(index<logicialGroup.servers.size())
        {
            const ServerFromPoolForDisplay &server=logicialGroup.servers.at(index);
            QTreeWidgetItem *itemServer=new QTreeWidgetItem(item);
            std::string text;
            std::string groupText;
            if(characterListForSelection.size()>1 && serverByCharacterGroup.size()>1)
            {
                const uint8_t groupInt=serverByCharacterGroup.at(server.charactersGroupIndex).second;
                //comment the if to always show it
                if(groupInt>=icon_server_list_color.size())
                    groupText=QStringLiteral(" (%1)").arg(groupInt).toStdString();
                itemServer->setToolTip(0,tr("Server group: %1, UID: %2").arg(groupInt).arg(server.uniqueKey));
                if(!icon_server_list_color.empty())
                    itemServer->setIcon(0,icon_server_list_color.at(groupInt%icon_server_list_color.size()));
            }
            std::string name=server.name;
            if(name.empty())
                name=tr("Default server").toStdString();
            if(fullView)
            {
                text=name+groupText;
                if(server.playedTime>0)
                {
                    if(!server.description.empty())
                        text+=" "+tr("%1 played").arg(QString::fromStdString(FacilityLibClient::timeToString(server.playedTime))).toStdString();
                    else
                        text+="\n"+tr("%1 played").arg(QString::fromStdString(FacilityLibClient::timeToString(server.playedTime))).toStdString();
                }
                if(!server.description.empty())
                    text+="\n"+server.description;
            }
            else
            {
                if(server.description.empty())
                    text=name+groupText;
                else
                    text=name+groupText+" - "+server.description;
            }
            itemServer->setText(0,QString::fromStdString(text));

            //do the icon here
            if(server.playedTime>5*365*24*3600)
            {
                itemServer->setIcon(0,SubServer::icon_server_list_bug);
                itemServer->setToolTip(0,tr("Played time greater than 5y, bug?"));
            }
            else if(server.lastConnect>0 && server.lastConnect<1420070400)
            {
                itemServer->setIcon(0,SubServer::icon_server_list_bug);
                itemServer->setToolTip(0,tr("Played before 2015, bug?"));
            }
            else if(server.maxPlayer<=65533 && (server.maxPlayer<server.currentPlayer || server.maxPlayer==0))
            {
                itemServer->setIcon(0,SubServer::icon_server_list_bug);
                if(server.maxPlayer<server.currentPlayer)
                    itemServer->setToolTip(0,tr("maxPlayer<currentPlayer"));
                else
                    itemServer->setToolTip(0,tr("maxPlayer==0"));
            }
            else if(server.playedTime>0 || server.lastConnect>0)
            {
                uint64_t dateDiff=0;
                if(currentDate>server.lastConnect)
                    dateDiff=currentDate-server.lastConnect;
                if(server.playedTime>24*3600*31)
                {
                    if(dateDiff<24*3600)
                    {
                        itemServer->setIcon(0,SubServer::icon_server_list_star1);
                        itemServer->setToolTip(0,tr("Played time greater than 24h, last connect in this last 24h"));
                    }
                    else
                    {
                        itemServer->setIcon(0,SubServer::icon_server_list_star2);
                        itemServer->setToolTip(0,tr("Played time greater than 24h, last connect not in this last 24h"));
                    }
                }
                else if(server.lastConnect<averageLastConnect)
                {
                    if(server.playedTime<averagePlayedTime)
                    {
                        itemServer->setIcon(0,SubServer::icon_server_list_star3);
                        itemServer->setToolTip(0,tr("Into the more recent server used, out of the most used server"));
                    }
                    else
                    {
                        itemServer->setIcon(0,SubServer::icon_server_list_star4);
                        itemServer->setToolTip(0,tr("Into the more recent server used, into the most used server"));
                    }
                }
                else
                {
                    if(server.playedTime<averagePlayedTime)
                    {
                        itemServer->setIcon(0,SubServer::icon_server_list_star5);
                        itemServer->setToolTip(0,tr("Out of the more recent server used, out of the most used server"));
                    }
                    else
                    {
                        itemServer->setIcon(0,SubServer::icon_server_list_star6);
                        itemServer->setToolTip(0,tr("Out of the more recent server used, into the most used server"));
                    }
                }

            }
            if(server.maxPlayer<=65533)
            {
                //do server.currentPlayer/server.maxPlayer icon
                if(server.maxPlayer<=0 || server.currentPlayer>server.maxPlayer)
                    itemServer->setIcon(1,SubServer::icon_server_list_bug);
                else
                {
                    //to be very sure
                    if(server.maxPlayer>0)
                    {
                        int percent=(server.currentPlayer*100)/server.maxPlayer;
                        if(server.currentPlayer==server.maxPlayer || (server.maxPlayer>50 && percent>98))
                            itemServer->setIcon(1,SubServer::icon_server_list_stat4);
                        else if(server.currentPlayer>30 && percent>50)
                            itemServer->setIcon(1,SubServer::icon_server_list_stat3);
                        else if(server.currentPlayer>5 && percent>20)
                            itemServer->setIcon(1,SubServer::icon_server_list_stat2);
                        else
                            itemServer->setIcon(1,SubServer::icon_server_list_stat1);
                    }
                }
                itemServer->setText(1,QStringLiteral("%1/%2").arg(server.currentPlayer).arg(server.maxPlayer));
            }
            const std::vector<ServerFromPoolForDisplay> &serverOrdenedList=client->getServerOrdenedList();
            if(server.serverOrdenedListIndex<serverOrdenedList.size())
                itemServer->setData(99,99,server.serverOrdenedListIndex);
            else
            {
                error("SubServer::addToServerList(): server.serverOrdenedListIndex>=serverOrdenedList.size(), "+
                      std::to_string(server.serverOrdenedListIndex)+
                      ">="+
                      std::to_string(serverOrdenedList.size())+
                      ", error");
                return;
            }
            /*if(ui->serverList->iconSize()>100)
            {
                itemServer->setIcon(0,SubServer::icon_server_list_stat3);
            }
            else
                itemServer->setIcon(0,QIcon());*/
            index++;
        }
    }
}

