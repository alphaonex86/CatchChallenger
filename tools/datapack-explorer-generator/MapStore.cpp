#include "MapStore.hpp"

#include "../../client/libqtcatchchallenger/QtDatapackClientLoader.hpp"

namespace MapStore {

static std::vector<MainCodeSet> g_sets;

void addFromCurrentLoader(const std::string &mainCode)
{
    MainCodeSet s;
    s.mainCode=mainCode;
    s.mapPaths=QtDatapackClientLoader::datapackLoader->get_maps();
    s.mapList=QtDatapackClientLoader::datapackLoader->get_mapList(); // deep copy
    g_sets.push_back(std::move(s));
}

const std::vector<MainCodeSet> &sets()
{
    return g_sets;
}

const std::string &mainCodeOf(size_t setIndex)
{
    static const std::string empty;
    if(setIndex>=g_sets.size())
        return empty;
    return g_sets[setIndex].mainCode;
}

const std::string &pathOf(const MapRef &r)
{
    static const std::string empty;
    if(r.set>=g_sets.size() || r.mapId>=g_sets[r.set].mapPaths.size())
        return empty;
    return g_sets[r.set].mapPaths[r.mapId];
}

const CatchChallenger::CommonMap &mapOf(const MapRef &r)
{
    return g_sets.at(r.set).mapList.at(r.mapId);
}

} // namespace MapStore
