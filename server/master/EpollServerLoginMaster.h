#ifndef EPOLL_SERVER_LOGIN_MASTER_H
#define EPOLL_SERVER_LOGIN_MASTER_H

#ifndef SERVERSSL

#include "../epoll/EpollGenericServer.h"
#include "EpollClientLoginMaster.h"
#include "CharactersGroup.h"
#include "../epoll/db/EpollPostgresql.h"
#include "../base/BaseServerMasterLoadDictionary.h"
#include "../base/BaseServerMasterSendDatapack.h"
#include "../base/TinyXMLSettings.h"
#include "PurgeTheLockedAccount.h"
#include "CheckTimeoutGameServer.h"
#include "AutomaticPingSend.h"

#include <string>
#include <vector>

namespace CatchChallenger {
class EpollServerLoginMaster : public EpollGenericServer, public BaseServerMasterLoadDictionary
{
public:
    EpollServerLoginMaster();
    ~EpollServerLoginMaster();
    bool tryListen();
    void doTheServerList();
    void doTheReplyCache();
    static EpollServerLoginMaster *epollServerLoginMaster;
private:
    PurgeTheLockedAccount * purgeTheLockedAccount;
    CheckTimeoutGameServer * checkTimeoutGameServer;
    AutomaticPingSend * automaticPingSend;
    char * server_ip;
    char * server_port;
    char * rawServerListForC211;
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
    void SQL_common_load_finish();
};
}

#endif

#endif // EPOLL_SERVER_H
