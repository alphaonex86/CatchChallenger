#include "EpollServerStats.h"

using namespace CatchChallenger;

EpollServerStats::EpollServerStats()
{

}

bool EpollServerStats::tryListen(const char * const path)
{
    return EpollUnixSocketServer::tryListen(path);
}
