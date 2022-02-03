#ifndef MAPBRUSH_H
#define MAPBRUSH_H

#include <unordered_map>
#include <vector>

#include "../../client/tiled/tiled_map.hpp"

class MapBrush
{
public:
    struct MapTemplate
    {
        Tiled::Map * tiledMap;
        std::string name;
        std::vector<uint8_t> templateLayerNumberToMapLayerNumber;
        std::unordered_map<const Tiled::Tileset *,Tiled::Tileset *> templateTilesetToMapTileset;
        uint8_t width,height,x,y;
        uint8_t baseLayerIndex;
        std::vector<Tiled::Map *> otherMap;
        std::vector<std::string> otherMapName;
    };
    static uint8_t *mapMask;
    static void initialiseMapMask(Tiled::Map &worldMap);
    static bool detectBorder(Tiled::TileLayer *layer,MapTemplate *templateMap);
    static MapTemplate tiledMapToMapTemplate(Tiled::Map *templateMap, Tiled::Map &worldMap);
    static void brushTheMap(Tiled::Map &worldMap, const MapTemplate &selectedTemplate, const int x, const int y, uint8_t * const mapMask, const bool &allTileIsMask=false);
    static bool brushHaveCollision(Tiled::Map &worldMap,const MapTemplate &selectedTemplate,const int x,const int y,const uint8_t * const mapMask);

};

#endif // MAPBRUSH_H
