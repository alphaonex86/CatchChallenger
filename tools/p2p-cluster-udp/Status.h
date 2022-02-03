#ifndef CATCHCHALLENGER_Status_H
#define CATCHCHALLENGER_Status_H

#ifdef EPOLLCATCHCHALLENGERSERVER

#include "../../server/epoll/EpollTimer.h"
#include <string>

namespace CatchChallenger {
class Status
        : public EpollTimer
{
public:
    explicit Status();
    void exec();
    void clear();

    static Status status;
private:
    std::string oldStatus;
};
}

#endif // def EPOLLCATCHCHALLENGERSERVER
#endif // Status_H
