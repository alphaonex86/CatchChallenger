#ifndef CATCHCHALLENGER_DictionaryServer_H
#define CATCHCHALLENGER_DictionaryServer_H

#include <string>
#include <vector>
#include "MapServer.hpp"

namespace CatchChallenger {
class DictionaryServer
{
public:
    //used at runtime (at player loading)
    static std::vector<CATCHCHALLENGER_TYPE_MAPID> dictionary_map_database_to_internal;//65535 not found
};
}

#endif
