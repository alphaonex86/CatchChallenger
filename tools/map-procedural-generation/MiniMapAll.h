#ifndef MINIMAPALL_H
#define MINIMAPALL_H

#include "../map-procedural-generation-terrain/MiniMap.h"

class MiniMapAll
{
public:
    static QImage makeMapTiled(const unsigned int worldWidthMap, const unsigned int worldHeightMap,
                               const unsigned int mapWidth,const unsigned int mapHeight);
};

#endif // MINIMAPALL_H
