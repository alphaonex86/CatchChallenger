#include "EpollServerStats.h"
#include <iostream>
#include <unistd.h>

using namespace CatchChallenger;

EpollServerStats EpollServerStats::epollServerStats;

EpollServerStats::EpollServerStats()
{
}

bool EpollServerStats::tryListen(const char * const path)
{
    this->path=path;
    return EpollUnixSocketServer::tryListen(path);
}

bool EpollServerStats::reopen()
{
    std::cout << "EpollServerStats::reopen()" << std::endl;
    close();
    ::unlink(path.c_str());
    return EpollUnixSocketServer::tryListen(path.c_str());
}
