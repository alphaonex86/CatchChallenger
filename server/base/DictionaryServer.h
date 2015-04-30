#ifndef CATCHCHALLENGER_DictionaryServer_H
#define CATCHCHALLENGER_DictionaryServer_H

#include <QList>
#include "../../general/base/GeneralStructures.h"
#include "../../general/base/GeneralVariable.h"
#include "MapServer.h"

namespace CatchChallenger {
class DictionaryServer
{
public:
    static QList<MapServer *> dictionary_map_database_to_internal;
};
}

#endif
