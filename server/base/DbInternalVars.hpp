// DbInternalVars.hpp -- in-memory storage for CATCHCHALLENGER_DB_INTERNAL_VARS
// (renamed from CATCHCHALLENGER_DB_FILE_RAM).
//
// DB_INTERNAL_VARS reuses the whole FileDB record logic (it implies
// CATCHCHALLENGER_DB_FILE), but its STORAGE is this process-global map
// instead of files: every DB record lives as
//     dbInternalVarsStore[path] = serialised bytes
// so there is no open()/read()/write()/close() syscall at all (not even
// on tmpfs). Each FileDB I/O site has an `#ifdef CATCHCHALLENGER_DB_INTERNAL_VARS`
// branch that reads/writes this map via std::istringstream/std::ostringstream;
// the `#else` keeps the original std::ifstream/std::ofstream code verbatim.
//
// init()/shutdown() just clear the map (process-lifetime store, so a
// player can disconnect and reconnect in the same process and find their
// data intact). Empty TU in every other build.

#ifndef CATCHCHALLENGER_DB_INTERNAL_VARS_HPP
#define CATCHCHALLENGER_DB_INTERNAL_VARS_HPP

#ifdef CATCHCHALLENGER_DB_INTERNAL_VARS

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>

namespace CatchChallenger {

// path -> serialised bytes. Defined in DbInternalVars.cpp.
extern std::unordered_map<std::string, std::vector<uint8_t> > dbInternalVarsStore;

class DbInternalVars
{
public:
    static void init();
    static void shutdown();
};

}

#endif // CATCHCHALLENGER_DB_INTERNAL_VARS

#endif // CATCHCHALLENGER_DB_INTERNAL_VARS_HPP
