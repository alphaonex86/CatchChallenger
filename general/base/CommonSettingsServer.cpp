#include "CommonSettingsServer.hpp"

CommonSettingsServer CommonSettingsServer::commonSettingsServer;

CommonSettingsServer::CommonSettingsServer()
{
    everyBodyIsRoot=false;
    forcedSpeed=0;
    dontSendPseudo=false;
    forceClientToSendAtMapChange=false;
    waitBeforeConnectAfterKick=0;

    //rates
    rates_xp=0;
    rates_xp_pow=0;
    rates_drop=0;
    rates_gold=0;

    //chat allowed
    chat_allow_all=false;
    chat_allow_local=false;
    chat_allow_private=false;
    chat_allow_clan=false;

    //trade
    factoryPriceChange=0;
}
