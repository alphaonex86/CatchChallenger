#ifndef CONNEXIONINFO_H
#define CONNEXIONINFO_H

#include <QByteArray>
#include <cstdint>
#include <QSettings>
#include <QString>
#include <string>
#include <vector>

/// \brief one entry of the server list, its settings load/save and server_list.xml loader, shared by the clients
class ConnexionInfo
{
public:
    ConnexionInfo();

    QString unique_code;
    QString name;
    bool isCustom;
    bool isArgument;

    //hightest priority
    QString host;
    uint16_t port;
    //lower priority
    QString ws;

    uint32_t connexionCounter;
    uint64_t lastConnexion;

    QString register_page;
    QString lost_passwd_page;
    QString site_page;

    QString proxyHost;
    uint16_t proxyPort;

    bool lan;

    //most used first, then most recent first
    bool operator<(const ConnexionInfo &connexionInfo) const;

    //the counters/proxy come from the settings group "Xml-<unique_code>"; name and pages in language, else in english
    static std::vector<ConnexionInfo> loadXmlList(const QByteArray &xmlContent,const std::string &language,QSettings &settings);
    static std::vector<ConnexionInfo> loadXmlListFromFile(const QString &file,const std::string &language,QSettings &settings);
    //the downloaded server_list.xml if any, else defaultFile
    static std::vector<ConnexionInfo> loadXmlList(const QString &defaultFile,const std::string &language,QSettings &settings);

    //custom servers: one settings group "Custom-<unique_code>" each
    static std::vector<ConnexionInfo> loadCustomList(QSettings &settings);
    //custom entries in their group, server_list.xml entries' counters/proxy in "Xml-<unique_code>";
    //LAN and command line entries are not saved; drops the Custom- groups no more listed
    static void saveList(const std::vector<ConnexionInfo> &list,QSettings &settings);
    //the old qtcpu800x600 keys (connexionList, nameList...) become Custom- groups and are removed; return the count moved
    static unsigned int migrateOldCustomList(QSettings &settings);
    static QString newCustomCode();
    //datapack cache folder: argument-, Custom- or Xml-<unique_code> under the app data folder
    QString datapackCachePath() const;
};

#endif // CONNEXIONINFO_H
