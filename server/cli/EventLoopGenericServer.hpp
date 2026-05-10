#ifndef EVENT_LOOP_GENERIC_SERVER_H
#define EVENT_LOOP_GENERIC_SERVER_H

#include "win32_compat.hpp"
#ifndef _WIN32
#include <sys/socket.h>
#endif
#include <vector>
#include <chrono>

#include "BaseClassSwitch.hpp"

namespace CatchChallenger {
class EventLoopGenericServer : public BaseClassSwitch
{
public:
    EventLoopGenericServer();
    ~EventLoopGenericServer();
    bool tryListenInternal(const char* const ip,const char* const port);
    void close();
    int accept(sockaddr *in_addr,socklen_t *in_len);
    std::vector<int> getSfd() const;
    virtual BaseClassSwitch::EventLoopObjectType getType() const;
    bool isListening() const;
    //set at the very top of main() so we can report the wall time taken to
    //reach the "correctly bind" log (datapack/db init etc.).
    static std::chrono::steady_clock::time_point processStart;
private:
    std::vector<int> sfd_list;
};
}

#endif // EVENT_LOOP_GENERIC_SERVER_H
