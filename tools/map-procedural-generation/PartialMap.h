#ifndef PARTIALMAP_H
#define PARTIALMAP_H

#include "../../client/tiled/tiled_map.h"
#include <string>

class PartialMap
{
public:
    struct RecuesPoint
    {
        std::string map;
        uint8_t x,y;
    };
    static bool save(const Tiled::Map &world, const unsigned int &minX, const unsigned int &minY,
                     const unsigned int &maxX, const unsigned int &maxY, const std::string &file, std::vector<RecuesPoint> &recuesPoints,
                     const std::string &type,const std::string &zone,const std::string &name);
};

#endif // PARTIALMAP_H