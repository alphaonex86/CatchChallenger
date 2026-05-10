#include "UnixServerStats.h"
#include <iostream>
#include <unistd.h>

using namespace CatchChallenger;

UnixServerStats UnixServerStats::unixServerStats;

UnixServerStats::UnixServerStats()
{
}

bool UnixServerStats::tryListen(const char * const path)
{
    this->path=path;
    return UnixSocketServer::tryListen(path);
}

bool UnixServerStats::reopen()
{
    std::cout << "UnixServerStats::reopen()" << std::endl;
    close();
    ::unlink(path.c_str());
    return UnixSocketServer::tryListen(path.c_str());
}
