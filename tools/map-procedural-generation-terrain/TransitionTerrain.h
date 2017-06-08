#ifndef TRANSITIONTERRAIN_H
#define TRANSITIONTERRAIN_H

#include "../../client/tiled/tiled_map.h"
#include "LoadMap.h"

class TransitionTerrain
{
public:
    static void addTransitionOnMap(Tiled::Map &tiledMap,const std::vector<LoadMap::TerrainTransition> &terrainTransitionList);
};

#endif // TRANSITIONTERRAIN_H
