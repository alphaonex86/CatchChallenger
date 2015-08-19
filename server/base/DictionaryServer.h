#ifndef CATCHCHALLENGER_DictionaryServer_H
#define CATCHCHALLENGER_DictionaryServer_H

#include <string>
#include <vector>
#include "../../general/base/GeneralStructures.h"
#include "../../general/base/GeneralVariable.h"
#include "MapServer.h"

namespace CatchChallenger {
class DictionaryServer
{
public:
    static std::vector<MapServer *> dictionary_map_database_to_internal;
    ///used only at map loading, \see BaseServer::preload_the_map()
    static std::unordered_map<std::basic_string<char>,std::unordered_map<std::pair<quint8/*x*/,quint8/*y*/>,quint16/*db code*/> > dictionary_pointOnMap_internal_to_database;

    struct MapAndPoint
    {
        MapServer *map;
        quint8 x;
        quint8 y;
        //can't be common for plant and item on map, else plant + item will < 255
        #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
        quint8 indexOfDirtOnMap;
        #endif
        quint8 indexOfItemOnMap;
    };
    static QList<MapAndPoint> dictionary_pointOnMap_database_to_internal;
};
}

#endif
