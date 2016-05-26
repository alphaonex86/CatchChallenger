#ifndef CATCHCHALLENGER_DictionaryServer_H
#define CATCHCHALLENGER_DictionaryServer_H

#include <string>
#include <vector>
#include "../../general/base/GeneralStructures.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/cpp11addition.h"
#include "MapServer.h"

namespace CatchChallenger {
class DictionaryServer
{
public:
    static std::vector<MapServer *> dictionary_map_database_to_internal;
    ///used only at map loading, \see BaseServer::preload_the_map()
    static std::map<std::string,std::map<std::pair<uint8_t/*x*/,uint8_t/*y*/>,uint16_t/*db code*/> > dictionary_pointOnMap_internal_to_database;

    struct MapAndPoint
    {
        MapServer *map;
        uint8_t x;
        uint8_t y;
    };
    static std::vector<MapAndPoint> dictionary_pointOnMap_database_to_internal;
};
}

#endif
