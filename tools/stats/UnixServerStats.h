#ifndef UNIXSERVERSTATS_H
#define UNIXSERVERSTATS_H

#include "../../server/cli/UnixSocketServer.hpp"
#include <string>

namespace CatchChallenger {
class UnixServerStats : public UnixSocketServer
{
public:
    UnixServerStats();
    bool tryListen(const char * const path);
    bool reopen();
public:
    static UnixServerStats unixServerStats;
private:
    std::string path;
};
}

#endif // UNIXSERVERSTATS_H
