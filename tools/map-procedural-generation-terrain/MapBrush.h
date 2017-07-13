#ifndef MAPBRUSH_H
#define MAPBRUSH_H

#include <unordered_map>
#include <vector>

#include "../../client/tiled/tiled_map.h"

class MapBrush
{
public:
    struct MapTemplate
    {
        const Tiled::Map * tiledMap;
        std::vector<uint8_t> templateLayerNumberToMapLayerNumber;
        std::unordered_map<Tiled::Tileset *,Tiled::Tileset *> templateTilesetToMapTileset;
        uint8_t width,height,x,y;
        uint8_t baseLayerIndex;
    };
    static uint8_t *mapMask;
    static void initialiseMapMask(Tiled::Map &worldMap);
    static bool detectBorder(Tiled::TileLayer *layer,MapTemplate *templateMap);
    static MapTemplate tiledMapToMapTemplate(const Tiled::Map *templateMap,Tiled::Map &worldMap);
    static void brushTheMap(Tiled::Map &worldMap,const MapTemplate &selectedTemplate,const int x,const int y,uint8_t * const mapMask);
    static bool brushHaveCollision(Tiled::Map &worldMap,const MapTemplate &selectedTemplate,const int x,const int y,const uint8_t * const mapMask);

};

#endif // MAPBRUSH_H
