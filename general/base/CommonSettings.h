#ifndef COMMMONSETTINGS_H
#define COMMMONSETTINGS_H

#include <QtGlobal>
#include <QString>

class CommonSettings
{
public:
    QString httpDatapackMirror;
    quint8 max_character;//if 0, not allowed, else the character max allowed
    quint8 min_character;
    quint8 max_pseudo_size;
    quint32 character_delete_time;//in seconds
    quint8 forcedSpeed;
    bool tcpCork;
    bool useSP;
    bool autoLearn;
    bool dontSendPseudo;
    bool forceClientToSendAtMapChange;
    int waitBeforeConnectAfterKick;
    QByteArray datapackHash;
    quint8 maxPlayerMonsters;
    quint8 maxWarehousePlayerMonsters;
    quint8 maxPlayerItems;
    quint8 maxWarehousePlayerItems;

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

    static CommonSettings commonSettings;
};

#endif // COMMMONSETTINGS_H
