#ifndef CATCHCHALLENGER_DISPLAY_STRUCTURES_H
#define CATCHCHALLENGER_DISPLAY_STRUCTURES_H

#include <unordered_map>
#include <vector>

namespace Tiled {
class MapObject;
class Tileset;
}

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
    Tiled::Tileset *tileset;
    std::vector<Tiled::MapObject *> flags;
    TemporaryTile * temporaryTile;
    BotMove botMove;
};

}

#endif // CATCHCHALLENGER_DISPLAY_STRUCTURES_H
