#ifndef COMMMONSETTINGSSERVER_H
#define COMMMONSETTINGSSERVER_H

#include <vector>
#include <string>

class CommonSettingsServer
{
public:
    CommonSettingsServer();
    #ifndef CATCHCHALLENGER_CLASS_GATEWAY
    std::string httpDatapackMirrorServer;
    #endif
    bool plantOnlyVisibleByPlayer;
    uint8_t forcedSpeed;
    bool tcpCork;
    bool useSP;
    bool autoLearn;
    bool dontSendPseudo;
    bool forceClientToSendAtMapChange;
    uint32_t waitBeforeConnectAfterKick;
    std::vector<char> datapackHashServerMain;
    std::vector<char> datapackHashServerSub;
    #ifndef CATCHCHALLENGER_CLASS_GATEWAY
    std::string mainDatapackCode;
    std::string subDatapackCode;
    #endif
    std::string exportedXml;

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
    uint8_t factoryPriceChange;

    static CommonSettingsServer commonSettingsServer;
};

#endif // COMMMONSETTINGSSERVER_H
