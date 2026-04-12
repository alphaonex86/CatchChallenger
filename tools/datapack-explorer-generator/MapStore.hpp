#ifndef MAPSTORE_HPP
#define MAPSTORE_HPP

#include <string>
#include <vector>
#include <cstdint>

#include "../../general/base/CommonMap/CommonMap.hpp"

// Persistent storage of parsed maps from every mainDatapackCode of the
// datapack. The loader only keeps data for the currently parsed main/sub,
// so after each call to parseDatapackMainSub() we snapshot the loader's
// state into a dedicated MainCodeSet here.
namespace MapStore {

struct MainCodeSet
{
    std::string mainCode;
    // Map paths as reported by the loader (relative to map/main/<mainCode>/,
    // e.g. "city/town.tmx"). Indexed by local map id.
    std::vector<std::string> mapPaths;
    // Deep copy of the loader's mapList. Index-compatible with mapPaths.
    std::vector<CatchChallenger::CommonMap> mapList;
};

struct MapRef
{
    size_t set;     // index into sets()
    size_t mapId;   // index within that set
};

// Snapshot the loader's current state into a new MainCodeSet.
void addFromCurrentLoader(const std::string &mainCode);

const std::vector<MainCodeSet> &sets();

// Convenience accessors
const std::string &mainCodeOf(size_t setIndex);
const std::string &pathOf(const MapRef &r);
const CatchChallenger::CommonMap &mapOf(const MapRef &r);

} // namespace MapStore

#endif // MAPSTORE_HPP
