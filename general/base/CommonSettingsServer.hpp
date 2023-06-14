#ifndef COMMMONSETTINGSSERVER_H
#define COMMMONSETTINGSSERVER_H

#include <vector>
#include <string>
#include <cstdint>
#include "lib.h"

class DLL_PUBLIC CommonSettingsServer
{
public:
    CommonSettingsServer();
    #ifndef CATCHCHALLENGER_CLASS_GATEWAY
    std::string httpDatapackMirrorServer;
    #endif
    uint8_t forcedSpeed;
    bool useSP;
    bool autoLearn;
    bool dontSendPseudo;
    bool forceClientToSendAtMapChange;
    bool everyBodyIsRoot;
    uint32_t waitBeforeConnectAfterKick;
    std::vector<char> datapackHashServerMain;
    std::vector<char> datapackHashServerSub;
    #ifndef CATCHCHALLENGER_CLASS_GATEWAY
    std::string mainDatapackCode;
    std::string subDatapackCode;
    #endif
    std::string exportedXml;

    //rates
    uint32_t rates_xp;//*1000 of real rate
    uint32_t rates_xp_pow;//*1000 of real rate
    uint32_t rates_drop;//*1000 of real rate
    uint32_t rates_gold;//*1000 of real rate

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
