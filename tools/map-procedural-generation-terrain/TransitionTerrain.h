#ifndef TRANSITIONTERRAIN_H
#define TRANSITIONTERRAIN_H

#include "../../client/tiled/tiled_map.h"
#include "LoadMap.h"

class TransitionTerrain
{
public:
    static void addTransitionOnMap(Tiled::Map &tiledMap);
    static void addTransitionGroupOnMap(Tiled::Map &tiledMap);
};

#endif // TRANSITIONTERRAIN_H
