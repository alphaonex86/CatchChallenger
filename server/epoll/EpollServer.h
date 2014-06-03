#ifndef EPOLL_SERVER_H
#define EPOLL_SERVER_H

#ifdef SERVERNOSSL

#include <sys/socket.h>

#include "BaseClassSwitch.h"
#include "../base/BaseServer.h"
#include "../base/ServerStructures.h"

namespace CatchChallenger {
class EpollServer : public BaseClassSwitch, public CatchChallenger::BaseServer
{
public:
    EpollServer();
    ~EpollServer();
    bool tryListen();
    void close();
    int accept(sockaddr *in_addr,socklen_t *in_len);
    int getSfd();
    Type getType() const;
    void preload_the_data();
    CatchChallenger::ClientMapManagement * getClientMapManagement();
    void unload_the_data();
    void setNormalSettings(const NormalServerSettings &settings);
    NormalServerSettings getNormalSettings() const;
    void loadAndFixSettings();
private:
    int sfd;
    NormalServerSettings normalServerSettings;
    int yes;
};
}

#endif

#endif // EPOLL_SERVER_H
