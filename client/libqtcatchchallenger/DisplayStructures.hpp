#ifndef CATCHCHALLENGER_DISPLAY_STRUCTURES_H
#define CATCHCHALLENGER_DISPLAY_STRUCTURES_H

#include <unordered_map>
#include <vector>
#include <tiled/mapobject.h>
#include <tiled/tileset.h>
#include "../../general/base/GeneralStructures.hpp"

class TemporaryTile;

namespace CatchChallenger {

//on the server, this is directly parsed and never manipulat the xml
enum BotMove
{
    BotMove_Fixed,
    BotMove_Random,
    BotMove_Loop,
    BotMove_Move
};

struct BotDisplay
{
    Tiled::MapObject *mapObject;
    Tiled::SharedTileset tileset;
    std::vector<Tiled::MapObject *> flags;
    TemporaryTile * temporaryTile;
    BotMove botMove;
    COORD_TYPE x,y;//if move, then set the next x,y at start move to allow start to move to previous tile, mostly used with bot with random move
};

}

#endif // CATCHCHALLENGER_DISPLAY_STRUCTURES_H
