#ifndef EPOLL_SERVER_H
#define EPOLL_SERVER_H

#ifndef SERVERSSL

#include "EpollGenericServer.hpp"
#include "../base/BaseServer.hpp"
#include "../base/ServerStructures.hpp"

namespace CatchChallenger {
class EpollServer : public CatchChallenger::EpollGenericServer, public CatchChallenger::BaseServer
{
public:
    EpollServer();
    ~EpollServer();
    void initTheDatabase();
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
    bool ready;
};
}

#endif

#endif // EPOLL_SERVER_H
