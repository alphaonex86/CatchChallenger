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
    static QHash<QString,QHash<QPair<quint8/*x*/,quint8/*y*/>,quint16/*db code*/> > dictionary_itemOnMap_internal_to_database;
    static QList<quint8> dictionary_itemOnMap_database_to_internal;
};
}

#endif
