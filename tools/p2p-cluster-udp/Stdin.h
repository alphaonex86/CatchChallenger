#ifndef CATCHCHALLENGER_Stdin_H
#define CATCHCHALLENGER_Stdin_H

#ifdef EPOLLCATCHCHALLENGERSERVER

#include "../../server/epoll/EpollStdin.h"

namespace CatchChallenger {
class Stdin
        : public EpollStdin
{
public:
    explicit Stdin();
    void input(const char *input,unsigned int size);
};
}

#endif // def EPOLLCATCHCHALLENGERSERVER
#endif // CATCHCHALLENGER_Stdin_H
