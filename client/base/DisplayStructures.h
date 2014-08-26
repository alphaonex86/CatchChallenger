#ifndef CATCHCHALLENGER_DISPLAY_STRUCTURES_H
#define CATCHCHALLENGER_DISPLAY_STRUCTURES_H

#include <QHash>
#include <QDomElement>

#include "../tiled/tiled_mapobject.h"
#include "../tiled/tiled_tileset.h"
#include "interface/TemporaryTile.h"

namespace CatchChallenger {

//on the server, this is directly parsed and never manipulat the xml
struct BotDisplay
{
    Tiled::MapObject *mapObject;
    Tiled::Tileset *tileset;
    QList<Tiled::MapObject *> flags;
    TemporaryTile * temporaryTile;
};

}

#endif // CATCHCHALLENGER_DISPLAY_STRUCTURES_H
