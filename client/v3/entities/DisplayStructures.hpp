// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_ENTITIES_DISPLAYSTRUCTURES_HPP_
#define CLIENT_V3_ENTITIES_DISPLAYSTRUCTURES_HPP_

#include <unordered_map>
#include <vector>

#include "../libraries/tiled/tiled_mapobject.hpp"
#include "../libraries/tiled/tiled_tileset.hpp"
#include "render/TemporaryTile.hpp"

namespace CatchChallenger {

// on the server, this is directly parsed and never manipulat the xml
enum BotMove { BotMove_Fixed, BotMove_Random, BotMove_Loop, BotMove_Move };

struct BotDisplay {
  Tiled::MapObject *mapObject;
  Tiled::Tileset *tileset;
  std::vector<Tiled::MapObject *> flags;
  TemporaryTile *temporaryTile;
  BotMove botMove;
};

}  // namespace CatchChallenger

#endif  // CLIENT_V3_ENTITIES_DISPLAYSTRUCTURES_HPP_
