#ifndef EPOLL_SERVER_H
#define EPOLL_SERVER_H

#ifndef SERVERSSL

#include "EpollGenericServer.h"
#include "../base/BaseServer.h"
#include "../base/ServerStructures.h"

namespace CatchChallenger {
class EpollServer : public CatchChallenger::EpollGenericServer, public CatchChallenger::BaseServer
{
public:
    EpollServer();
    ~EpollServer();
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

#endif // EPOLL_SERVER_H
