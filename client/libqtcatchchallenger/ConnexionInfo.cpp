#include "ConnexionInfo.hpp"
#include "../../general/base/cpp11addition.hpp"
#include "../../general/base/FacilityLibGeneral.hpp"
#include "../../general/tinyXML2/tinyxml2.hpp"

#include <QDateTime>
#include <QDir>
#include <QSet>
#include <QStringList>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <cstring>
#include <iostream>

ConnexionInfo::ConnexionInfo() :
    isCustom(false),
    isArgument(false),
    port(0),
    connexionCounter(0),
    lastConnexion(0),
    proxyPort(0),
    lan(false)
{
}

bool ConnexionInfo::operator<(const ConnexionInfo &connexionInfo) const
{
    if(connexionCounter!=connexionInfo.connexionCounter)
        return connexionCounter>connexionInfo.connexionCounter;
    //equal entries must give false, std::sort needs a strict order
    return lastConnexion>connexionInfo.lastConnexion;
}

static void readLangText(const tinyxml2::XMLElement *lang,const char *tag,QString &value)
{
    const tinyxml2::XMLElement *element=lang->FirstChildElement(tag);
    if(element!=NULL && element->GetText()!=NULL)
        value=element->GetText();
}

static void readLang(const tinyxml2::XMLElement *lang,ConnexionInfo &connexionInfo)
{
    readLangText(lang,"name",connexionInfo.name);
    readLangText(lang,"register_page",connexionInfo.register_page);
    readLangText(lang,"lost_passwd_page",connexionInfo.lost_passwd_page);
    readLangText(lang,"site_page",connexionInfo.site_page);
}

static bool isLang(const tinyxml2::XMLElement *lang,const std::string &language)
{
    const char * const attribute=lang->Attribute("lang");
    if(language=="en")
        return attribute==NULL || strcmp(attribute,"en")==0;
    return attribute!=NULL && language==attribute;
}

static const tinyxml2::XMLElement *findLang(const tinyxml2::XMLElement *server,const std::string &language)
{
    const tinyxml2::XMLElement *lang=server->FirstChildElement("lang");
    while(lang!=NULL && !isLang(lang,language))
        lang=lang->NextSiblingElement("lang");
    return lang;
}

std::vector<ConnexionInfo> ConnexionInfo::loadXmlList(const QByteArray &xmlContent,const std::string &language,QSettings &settings)
{
    std::vector<ConnexionInfo> returnedVar;
    tinyxml2::XMLDocument domDocument;
    const tinyxml2::XMLError loadOkay=domDocument.Parse(xmlContent.data(),xmlContent.size());
    if(loadOkay!=0)
    {
        std::cerr << "ConnexionInfo::loadXmlList(), " << domDocument.ErrorName() << std::endl;
        return returnedVar;
    }
    const tinyxml2::XMLElement *root=domDocument.RootElement();
    if(root==NULL || root->Name()==NULL || strcmp(root->Name(),"servers")!=0)
    {
        std::cerr << "ConnexionInfo::loadXmlList(), \"servers\" root balise not found" << std::endl;
        return returnedVar;
    }

    const QRegularExpression regexHost("^[a-zA-Z0-9\\.\\-_]+$");
    const uint64_t now=static_cast<uint64_t>(QDateTime::currentMSecsSinceEpoch()/1000);
    bool ok;
    const tinyxml2::XMLElement *server=root->FirstChildElement("server");
    while(server!=NULL)
    {
        if(server->Attribute("unique_code")!=NULL)
        {
            ConnexionInfo connexionInfo;
            connexionInfo.unique_code=server->Attribute("unique_code");
            if(server->Attribute("host")!=NULL)
                connexionInfo.host=server->Attribute("host");
            if(server->Attribute("port")!=NULL)
            {
                const uint32_t temp_port=stringtouint32(server->Attribute("port"),&ok);
                if(!connexionInfo.host.contains(regexHost))
                    std::cerr << "server_list.xml: host is wrong: " << connexionInfo.host.toStdString() << std::endl;
                else if(!ok)
                    std::cerr << "server_list.xml: port is not a number: " << server->Attribute("port") << std::endl;
                else if(temp_port<1 || temp_port>65535)
                    std::cerr << "server_list.xml: port is not in range: " << server->Attribute("port") << std::endl;
                else if(connexionInfo.unique_code.isEmpty())
                    std::cerr << "server_list.xml: unique_code can't be empty" << std::endl;
                else
                    connexionInfo.port=static_cast<uint16_t>(temp_port);
            }
            if(server->Attribute("ws")!=NULL)
                connexionInfo.ws=server->Attribute("ws");
            const tinyxml2::XMLElement *lang=NULL;
            if(!language.empty())
                lang=findLang(server,language);
            if(lang==NULL)
                lang=findLang(server,"en");
            if(lang!=NULL)
                readLang(lang,connexionInfo);

            settings.beginGroup(QStringLiteral("Xml-%1").arg(connexionInfo.unique_code));
            connexionInfo.connexionCounter=settings.value(QStringLiteral("connexionCounter"),0).toUInt(&ok);
            if(!ok)
                connexionInfo.connexionCounter=0;
            connexionInfo.proxyPort=static_cast<uint16_t>(settings.value(QStringLiteral("proxyPort"),9050).toUInt(&ok));
            if(!ok)
                connexionInfo.proxyPort=9050;
            connexionInfo.proxyHost=settings.value(QStringLiteral("proxyHost")).toString();
            connexionInfo.lastConnexion=settings.value(QStringLiteral("lastConnexion"),static_cast<quint64>(now)).toULongLong(&ok);
            if(!ok || connexionInfo.lastConnexion>now)
                connexionInfo.lastConnexion=now;
            settings.endGroup();
            returnedVar.push_back(connexionInfo);
        }
        else
            std::cerr << "server_list.xml: server without unique_code" << std::endl;
        server=server->NextSiblingElement("server");
    }
    return returnedVar;
}

std::vector<ConnexionInfo> ConnexionInfo::loadXmlListFromFile(const QString &file,const std::string &language,QSettings &settings)
{
    QFile itemsFile(file);
    if(!itemsFile.open(QIODevice::ReadOnly))
    {
        std::cerr << "Unable to open the file: " << file.toStdString() << ", error: " << itemsFile.errorString().toStdString() << std::endl;
        return std::vector<ConnexionInfo>();
    }
    const QByteArray xmlContent=itemsFile.readAll();
    itemsFile.close();
    return loadXmlList(xmlContent,language,settings);
}

std::vector<ConnexionInfo> ConnexionInfo::loadXmlList(const QString &defaultFile,const std::string &language,QSettings &settings)
{
    const QString downloaded=QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)+QStringLiteral("/server_list.xml");
    if(QFileInfo(downloaded).isFile())
        return loadXmlListFromFile(downloaded,language,settings);
    return loadXmlListFromFile(defaultFile,language,settings);
}

QString ConnexionInfo::newCustomCode()
{
    return QString::fromStdString(CatchChallenger::FacilityLibGeneral::randomPassword("abcdefghijklmnopqurstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890",16));
}

QString ConnexionInfo::datapackCachePath() const
{
    QString prefix=QStringLiteral("Xml-");
    if(isArgument)
        prefix=QStringLiteral("argument-");
    else if(isCustom)
        prefix=QStringLiteral("Custom-");
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)+QStringLiteral("/datapack/")+prefix+unique_code).absolutePath();
}

std::vector<ConnexionInfo> ConnexionInfo::loadCustomList(QSettings &settings)
{
    std::vector<ConnexionInfo> list;
    const uint64_t now=static_cast<uint64_t>(QDateTime::currentMSecsSinceEpoch()/1000);
    const QStringList groups=settings.childGroups();
    int index=0;
    while(index<groups.size())
    {
        const QString &groupName=groups.at(index);
        if(groupName.startsWith(QStringLiteral("Custom-")) && groupName.size()>7)
        {
            settings.beginGroup(groupName);
            ConnexionInfo connexionInfo;
            connexionInfo.isCustom=true;
            connexionInfo.unique_code=groupName.mid(7);
            connexionInfo.ws=settings.value(QStringLiteral("ws")).toString();
            connexionInfo.host=settings.value(QStringLiteral("host")).toString();
            bool ok=true;
            if(connexionInfo.ws.isEmpty())
            {
                const unsigned int port=settings.value(QStringLiteral("port")).toUInt(&ok);
                if(!ok || port<1 || port>65535 || connexionInfo.host.isEmpty())
                {
                    std::cerr << "custom server " << groupName.toStdString() << " dropped: no ws and wrong host/port" << std::endl;
                    ok=false;
                }
                else
                    connexionInfo.port=static_cast<uint16_t>(port);
            }
            if(ok)
            {
                connexionInfo.name=settings.value(QStringLiteral("name")).toString();
                connexionInfo.connexionCounter=settings.value(QStringLiteral("connexionCounter"),0).toUInt(&ok);
                if(!ok)
                    connexionInfo.connexionCounter=0;
                connexionInfo.lastConnexion=settings.value(QStringLiteral("lastConnexion"),static_cast<quint64>(now)).toULongLong(&ok);
                if(!ok || connexionInfo.lastConnexion>now)
                    connexionInfo.lastConnexion=now;
                connexionInfo.proxyHost=settings.value(QStringLiteral("proxyHost")).toString();
                if(!connexionInfo.proxyHost.isEmpty())
                {
                    connexionInfo.proxyPort=static_cast<uint16_t>(settings.value(QStringLiteral("proxyPort"),0).toUInt(&ok));
                    if(!ok || connexionInfo.proxyPort==0)
                    {
                        std::cerr << "custom server " << groupName.toStdString() << ": wrong proxy port, proxy ignored" << std::endl;
                        connexionInfo.proxyHost.clear();
                        connexionInfo.proxyPort=0;
                    }
                }
                list.push_back(connexionInfo);
            }
            settings.endGroup();
        }
        index++;
    }
    return list;
}

void ConnexionInfo::saveList(const std::vector<ConnexionInfo> &list,QSettings &settings)
{
    QSet<QString> customGroups;
    unsigned int index=0;
    while(index<list.size())
    {
        const ConnexionInfo &connexionInfo=list.at(index);
        if(connexionInfo.lan || connexionInfo.isArgument)
        {}
        else if(connexionInfo.unique_code.isEmpty())
            std::cerr << "ConnexionInfo::saveList(): server " << connexionInfo.name.toStdString() << " without unique_code not saved" << std::endl;
        else
        {
            if(connexionInfo.isCustom)
            {
                const QString group=QStringLiteral("Custom-%1").arg(connexionInfo.unique_code);
                customGroups.insert(group);
                settings.beginGroup(group);
                if(!connexionInfo.ws.isEmpty())
                {
                    settings.setValue(QStringLiteral("ws"),connexionInfo.ws);
                    settings.remove(QStringLiteral("host"));
                    settings.remove(QStringLiteral("port"));
                }
                else
                {
                    settings.setValue(QStringLiteral("host"),connexionInfo.host);
                    settings.setValue(QStringLiteral("port"),connexionInfo.port);
                    settings.remove(QStringLiteral("ws"));
                }
                settings.setValue(QStringLiteral("name"),connexionInfo.name);
            }
            else
                settings.beginGroup(QStringLiteral("Xml-%1").arg(connexionInfo.unique_code));
            if(connexionInfo.connexionCounter>0)
                settings.setValue(QStringLiteral("connexionCounter"),connexionInfo.connexionCounter);
            else
                settings.remove(QStringLiteral("connexionCounter"));
            if(connexionInfo.lastConnexion>0 && connexionInfo.connexionCounter>0)
                settings.setValue(QStringLiteral("lastConnexion"),static_cast<quint64>(connexionInfo.lastConnexion));
            else
                settings.remove(QStringLiteral("lastConnexion"));
            settings.setValue(QStringLiteral("proxyHost"),connexionInfo.proxyHost);
            settings.setValue(QStringLiteral("proxyPort"),connexionInfo.proxyPort);
            settings.endGroup();
        }
        index++;
    }
    const QStringList groups=settings.childGroups();
    int groupIndex=0;
    while(groupIndex<groups.size())
    {
        const QString &groupName=groups.at(groupIndex);
        if(groupName.startsWith(QStringLiteral("Custom-")) && !customGroups.contains(groupName))
            settings.remove(groupName);
        groupIndex++;
    }
    settings.sync();
    if(settings.status()!=QSettings::NoError)
        std::cerr << "ConnexionInfo::saveList(): settings not written, status " << settings.status() << std::endl;
}

static bool splitHostPort(const QString &text,QString &host,uint16_t &port)
{
    const int separator=text.lastIndexOf(':');
    if(separator<1)
        return false;
    bool ok=false;
    const unsigned int value=text.mid(separator+1).toUInt(&ok);
    if(!ok || value<1 || value>65535)
        return false;
    host=text.left(separator);
    port=static_cast<uint16_t>(value);
    return true;
}

unsigned int ConnexionInfo::migrateOldCustomList(QSettings &settings)
{
    if(!settings.contains(QStringLiteral("connexionList")))
        return 0;
    const QStringList connexionList=settings.value(QStringLiteral("connexionList")).toStringList();
    QStringList nameList=settings.value(QStringLiteral("nameList")).toStringList();
    QStringList connexionCounterList=settings.value(QStringLiteral("connexionCounterList")).toStringList();
    QStringList lastConnexionList=settings.value(QStringLiteral("lastConnexionList")).toStringList();
    QStringList proxyList=settings.value(QStringLiteral("proxyList")).toStringList();
    //a list of an other size is not aligned with connexionList: ignored
    if(nameList.size()!=connexionList.size())
        nameList.clear();
    if(connexionCounterList.size()!=connexionList.size())
        connexionCounterList.clear();
    if(lastConnexionList.size()!=connexionList.size())
        lastConnexionList.clear();
    if(proxyList.size()!=connexionList.size())
        proxyList.clear();
    const QRegularExpression regexHost(QStringLiteral("^[a-zA-Z0-9\\.\\-_:]+$"));
    std::vector<ConnexionInfo> list;
    int index=0;
    while(index<connexionList.size())
    {
        const QString &connexion=connexionList.at(index);
        ConnexionInfo connexionInfo;
        connexionInfo.isCustom=true;
        connexionInfo.unique_code=newCustomCode();
        bool ok=true;
        if(connexion.startsWith(QStringLiteral("ws://")) || connexion.startsWith(QStringLiteral("wss://")))
            connexionInfo.ws=connexion;
        else if(!splitHostPort(connexion,connexionInfo.host,connexionInfo.port) || !connexionInfo.host.contains(regexHost))
        {
            std::cerr << "old custom server dropped, wrong host:port: " << connexion.toStdString() << std::endl;
            ok=false;
        }
        if(ok)
        {
            if(!nameList.isEmpty())
                connexionInfo.name=nameList.at(index);
            if(!connexionCounterList.isEmpty())
                connexionInfo.connexionCounter=connexionCounterList.at(index).toUInt();
            if(!lastConnexionList.isEmpty())
                connexionInfo.lastConnexion=lastConnexionList.at(index).toULongLong();
            if(!proxyList.isEmpty() && !proxyList.at(index).isEmpty())
                if(!splitHostPort(proxyList.at(index),connexionInfo.proxyHost,connexionInfo.proxyPort))
                    std::cerr << "old custom server " << connexion.toStdString() << ": wrong proxy, ignored: " << proxyList.at(index).toStdString() << std::endl;
            list.push_back(connexionInfo);
        }
        index++;
    }
    //keep the already migrated or added servers
    std::vector<ConnexionInfo> all=loadCustomList(settings);
    all.insert(all.end(),list.begin(),list.end());
    saveList(all,settings);
    settings.remove(QStringLiteral("connexionList"));
    settings.remove(QStringLiteral("nameList"));
    settings.remove(QStringLiteral("connexionCounterList"));
    settings.remove(QStringLiteral("lastConnexionList"));
    settings.remove(QStringLiteral("proxyList"));
    settings.sync();
    std::cout << "migrated " << list.size() << " custom server(s) to the Custom- settings groups" << std::endl;
    return static_cast<unsigned int>(list.size());
}
