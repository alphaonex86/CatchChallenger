#ifndef EPOLL_SSL_SERVER_H
#define EPOLL_SSL_SERVER_H

#ifdef SERVERSSL

#include "EpollGenericSslServer.h"
#include "../base/BaseServer.h"
#include "../base/ServerStructures.h"

namespace CatchChallenger {
class EpollSslServer : public CatchChallenger::EpollGenericSslServer, public CatchChallenger::BaseServer
{
public:
    EpollSslServer();
    ~EpollSslServer();
    bool tryListen();
    void preload_the_data();
    void unload_the_data();
    void setNormalSettings(const NormalServerSettings &settings);
    NormalServerSettings getNormalSettings() const;
    void loadAndFixSettings();
    void preload_finish();
    bool isReady();
    void quitForCriticalDatabaseQueryFailed();
private:
    NormalServerSettings normalServerSettings;
    int yes;
    bool ready;
};
}
#endif

#endif // EPOLL_SSL_SERVER_H
