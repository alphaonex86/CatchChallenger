#ifndef MINIMAP_H
#define MINIMAP_H

#include "znoise/headers/Simplex.hpp"

class MiniMap
{
public:
    static void makeMap(const Simplex &heighmap, const Simplex &moisuremap, const float &noiseMapScaleMoisure, const float &noiseMapScaleMap,
            const unsigned int widthMap, const unsigned int heightMap, const float miniMapDivisor);
    static void makeMapTiled(const unsigned int widthMap, const unsigned int heightMap);
};

#endif // MINIMAP_H
