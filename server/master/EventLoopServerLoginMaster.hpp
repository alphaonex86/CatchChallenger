#ifndef EVENTLOOP_SERVER_LOGIN_MASTER_H
#define EVENTLOOP_SERVER_LOGIN_MASTER_H

#include "../cli/EventLoopGenericServer.hpp"
#include "EventLoopClientLoginMaster.hpp"
#include "CharactersGroup.hpp"
#include "../cli/db/EventLoopDatabase.hpp"
#include "../base/BaseServer/BaseServerMasterLoadDictionary.hpp"
#include "../base/BaseServer/BaseServerMasterSendDatapack.hpp"
#include "../base/TinyXMLSettings.hpp"
#include "PurgeTheLockedAccount.hpp"
#include "CheckTimeoutGameServer.hpp"
#include "AutomaticPingSend.hpp"

#include <string>
#include <vector>

namespace CatchChallenger {
class EventLoopServerLoginMaster : public EventLoopGenericServer, public BaseServerMasterLoadDictionary
{
public:
    EventLoopServerLoginMaster();
    ~EventLoopServerLoginMaster();
    bool tryListen();
    void doTheServerList();
    void doTheReplyCache();
    static EventLoopServerLoginMaster *unixServerLoginMaster;
    static std::vector<char> fixedValuesRawDictionaryCacheForGameserver;
    static int fixedValuesRawDictionaryCacheForGameserverSize;
private:
    PurgeTheLockedAccount * purgeTheLockedAccount;
    CheckTimeoutGameServer * checkTimeoutGameServer;
    AutomaticPingSend * automaticPingSend;
    char * server_ip;
    char * server_port;
    std::vector<char> rawServerListForC211;
    int rawServerListForC211Size;

    CatchChallenger::DatabaseBase *databaseBaseLogin;
    CatchChallenger::DatabaseBase *databaseBaseBase;

    BaseServerMasterSendDatapack baseServerMasterSendDatapack;
    uint16_t purgeLockPeriod;
    uint16_t maxLockAge;
private:
    void generateToken(TinyXMLSettings &settings);

    void load_account_max_id();
    static void load_account_max_id_static(void *object);
    void load_account_max_id_return();

    void loadTheDatapack();
    void loadTheDatapackFileList();
    void loadLoginSettings(TinyXMLSettings &settings);
    void loadDBLoginSettings(TinyXMLSettings &settings);
    std::vector<std::string> loadCharactersGroup(TinyXMLSettings &settings);
    void charactersGroupListReply(std::vector<std::string> &charactersGroupList);
    void doTheLogicalGroup(TinyXMLSettings &settings);
    void loadTheProfile();
    void loadTheDictionary();
    void SQL_common_load_finish();
};
}

#endif // EVENT_LOOP_SERVER_H
