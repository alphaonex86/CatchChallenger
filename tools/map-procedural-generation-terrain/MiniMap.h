#ifndef MINIMAP_H
#define MINIMAP_H

#include "znoise/headers/Simplex.hpp"
#include <QImage>

class MiniMap
{
public:
    static QImage makeMap(const Simplex &heighmap, const Simplex &moisuremap, const float &noiseMapScaleMoisure, const float &noiseMapScaleMap,
            const unsigned int widthMap, const unsigned int heightMap, const float miniMapDivisor);
    static QImage makeMapTiled(const unsigned int widthMap, const unsigned int heightMap);
    static bool makeMapTerrainOverview();
};

#endif // MINIMAP_H
