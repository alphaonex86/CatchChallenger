// test/testingpathfinding/StubMapServer.hpp
//
// The user's brief asks the test to hold an "internal fixed MapServer
// object reference". The real server/base/MapServer.hpp drags the whole
// server stack (BaseServer, ClientList, GlobalServerData, fight engine,
// DB, epoll/io_uring, …) which would defeat "no network, no syscall,
// just internal C++, fast".
//
// PathFinding only ever consumes the CommonMap slice of a map (its
// searchPath() takes std::vector<CatchChallenger::CommonMap>). A
// MapServer that simply *is-a* CommonMap is therefore a faithful and
// sufficient stand-in: same type name, same base, zero server deps.
//
// We pre-define the real header's include guard so any transitive
// "#include MapServer.hpp" expands to nothing.
#ifndef CC_TPF_STUBMAPSERVER_H
#define CC_TPF_STUBMAPSERVER_H

#define CATCHCHALLENGER_MAP_VISIBILITY_SIMPLE_SERVERMAP_H 1

#include "../../general/base/CommonMap/CommonMap.hpp"

namespace CatchChallenger {
class MapServer : public CommonMap
{
public:
    MapServer() {}
};
}

#endif // CC_TPF_STUBMAPSERVER_H
