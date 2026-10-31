#include "ConnexionInfo.hpp"
#include "../../general/base/cpp11addition.hpp"
#include "../../general/tinyXML2/tinyxml2.hpp"

#include <QDateTime>
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
