#ifndef COMMMONSETTINGSSERVER_H
#define COMMMONSETTINGSSERVER_H

#include <QtGlobal>
#include <QString>

class CommonSettingsServer
{
public:
    QString httpDatapackMirrorServer;
    bool plantOnlyVisibleByPlayer;
    quint8 forcedSpeed;
    bool tcpCork;
    bool useSP;
    bool autoLearn;
    bool dontSendPseudo;
    bool forceClientToSendAtMapChange;
    int waitBeforeConnectAfterKick;
    QByteArray datapackHashServerMain;
    QByteArray datapackHashServerSub;
    QString mainDatapackCode;
    QString subDatapackCode;
    QString exportedXml;

    //rates
    float rates_xp;
    float rates_xp_pow;
    float rates_drop;
    float rates_gold;

    //chat allowed
    bool chat_allow_all;
    bool chat_allow_local;
    bool chat_allow_private;
    bool chat_allow_clan;

    //trade
    quint8 factoryPriceChange;

    static CommonSettingsServer commonSettingsServer;
};

#endif // COMMMONSETTINGSSERVER_H
