#ifndef PARTIALMAP_H
#define PARTIALMAP_H

#include "../../client/tiled/tiled_map.h"
#include <string>

class PartialMap
{
public:
    static bool save(const Tiled::Map &world, const unsigned int &minX, const unsigned int &minY, const unsigned int &maxX, const unsigned int &maxY, const std::string &file);
};

#endif // PARTIALMAP_H
