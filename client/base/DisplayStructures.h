#ifndef CATCHCHALLENGER_DISPLAY_STRUCTURES_H
#define CATCHCHALLENGER_DISPLAY_STRUCTURES_H

#include <QHash>
#include <QDomElement>

#include "libtiled/mapobject.h"
#include "libtiled/tileset.h"

namespace CatchChallenger {

//on the server, this is directly parsed and never manipulat the xml
struct BotDisplay
{
    Tiled::MapObject *mapObject;
    Tiled::Tileset *tileset;
};

}

#endif // CATCHCHALLENGER_DISPLAY_STRUCTURES_H
