#ifndef COMMMONSETTINGS_H
#define COMMMONSETTINGS_H

#include <QtGlobal>

class CommonSettings
{
public:
    //fight
    bool pvp;

    //rates
    qreal rates_xp;
    qreal rates_gold;
    qreal rates_shiny;

    //chat allowed
    bool chat_allow_all;
    bool chat_allow_local;
    bool chat_allow_private;
    bool chat_allow_aliance;
    bool chat_allow_clan;

    //trade
    quint8 factoryPriceChange;

    static CommonSettings commonSettings;
};

#endif // COMMMONSETTINGS_H
