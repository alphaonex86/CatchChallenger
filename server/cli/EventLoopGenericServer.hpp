#ifndef EPOLLGENERICSERVER_H
#define EPOLLGENERICSERVER_H

#include <sys/socket.h>
#include <vector>
#include <chrono>

#include "BaseClassSwitch.hpp"

namespace CatchChallenger {
class EpollGenericServer : public BaseClassSwitch
{
public:
    EpollGenericServer();
    ~EpollGenericServer();
    bool tryListenInternal(const char* const ip,const char* const port);
    void close();
    int accept(sockaddr *in_addr,socklen_t *in_len);
    std::vector<int> getSfd() const;
    virtual BaseClassSwitch::EpollObjectType getType() const;
    bool isListening() const;
    //set at the very top of main() so we can report the wall time taken to
    //reach the "correctly bind" log (datapack/db init etc.).
    static std::chrono::steady_clock::time_point processStart;
private:
    std::vector<int> sfd_list;
};
}

#endif // EPOLLGENERICSERVER_H
