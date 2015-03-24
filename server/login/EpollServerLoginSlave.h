#ifndef EPOLL_SERVER_LOGIN_MASTER_H
#define EPOLL_SERVER_LOGIN_MASTER_H

#ifndef SERVERSSL

#include "../epoll/EpollGenericServer.h"
#include "../base/BaseServerCommon.h"
#include "EpollClientLoginSlave.h"

#include <QSettings>

namespace CatchChallenger {
class EpollServerLoginSlave : public EpollGenericServer, public BaseServerCommon
{
public:
    EpollServerLoginSlave();
    ~EpollServerLoginSlave();
    bool tryListen();
    void close();
public:
    bool tcpNodelay,tcpCork;
    bool serverReady;
private:
    char * server_ip;
    char * server_port;
private:
    void generateToken(QSettings &settings);
    void SQL_common_load_finish();
};
}

#endif

#endif // EPOLL_SERVER_H
