#ifndef EPOLL_SERVER_LOGIN_MASTER_H
#define EPOLL_SERVER_LOGIN_MASTER_H

#ifndef SERVERSSL

#include "../epoll/EpollGenericServer.h"
#include "EpollClientLoginMaster.h"
#include "CharactersGroup.h"
#include "../epoll/db/EpollPostgresql.h"

#include <QSettings>

namespace CatchChallenger {
class EpollServerLoginMaster : public CatchChallenger::EpollGenericServer
{
public:
    EpollServerLoginMaster();
    ~EpollServerLoginMaster();
    bool tryListen();
private:
    char * server_ip;
    char * server_port;
private:
    void generateToken(QSettings &settings);

    void load_account_max_id();
    static void load_account_max_id_static(void *object);
    void load_account_max_id_return();

    EpollPostgresql *databaseBaseLogin;
};
}

#endif

#endif // EPOLL_SERVER_H
