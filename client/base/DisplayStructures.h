#ifndef POKECRAFT_DISPLAY_STRUCTURES_H
#define POKECRAFT_DISPLAY_STRUCTURES_H

#include <QHash>
#include <QDomElement>

#include "mapobject.h"
#include "tileset.h"

namespace Pokecraft {

//on the server, this is directly parsed and never manipulat the xml
struct BotDisplay
{
    Tiled::MapObject *mapObject;
    Tiled::Tileset *tileset;
};

}

#endif // POKECRAFT_DISPLAY_STRUCTURES_H
