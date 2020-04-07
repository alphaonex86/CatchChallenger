#ifndef CONNEXIONINFO_H
#define CONNEXIONINFO_H

#include <QString>

class ConnexionInfo
{
public:
    QString unique_code;
    QString name;
    bool isCustom;

    //hightest priority
    QString host;
    uint16_t port;
    //lower priority
    QString ws;

    uint32_t connexionCounter;
    uint32_t lastConnexion;

    QString register_page;
    QString lost_passwd_page;
    QString site_page;

    QString proxyHost;
    uint16_t proxyPort;

    bool operator<(const ConnexionInfo &connexionInfo) const;
};


#endif // CONNEXIONINFO_H
