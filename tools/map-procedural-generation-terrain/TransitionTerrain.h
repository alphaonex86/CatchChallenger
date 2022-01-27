#ifndef TRANSITIONTERRAIN_H
#define TRANSITIONTERRAIN_H

#include "../../client/tiled/tiled_map.hpp"
#include "LoadMap.h"

class TransitionTerrain
{
public:
    static void addTransitionOnMap(Tiled::Map &tiledMap);
    static void addTransitionGroupOnMap(Tiled::Map &tiledMap);
    static uint16_t layerMask(const Tiled::TileLayer * const terrainLayer, const unsigned int &x, const unsigned int &y, Tiled::Tile *tile, const bool XORop);
    static void changeTileLayerOrder(Tiled::Map &tiledMap);
    static void mergeDown(Tiled::Map &tiledMap);
};

#endif // TRANSITIONTERRAIN_H
