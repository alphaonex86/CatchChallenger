#ifndef MINIMAPALL_H
#define MINIMAPALL_H

#include "../map-procedural-generation-terrain/MiniMap.h"
#include <QPainter>

class MiniMapAll
{
public:
    static QImage makeMapTiled(const unsigned int worldWidthMap, const unsigned int worldHeightMap,
                               const unsigned int mapWidth,const unsigned int mapHeight);
    //water=true draws the SEA variants (minimap-*-water.png): a sea route has to
    //read as one on the world map, not as a road
    static void drawRoad(const uint8_t orientation,QPainter &p,const unsigned int x,const unsigned int y,const unsigned int mapWidth,const unsigned int mapHeight,const bool water=false);

    static QImage minimapcitybig;
    static QImage minimapcitymedium;
    static QImage minimapcitysmall;

    static QImage minimap1way;
    static QImage minimap2way1;
    static QImage minimap2way2;
    static QImage minimap3way;
    static QImage minimap4way;

    static QImage minimap1wayWater;
    static QImage minimap2way1Water;
    static QImage minimap2way2Water;
    static QImage minimap3wayWater;
    static QImage minimap4wayWater;
};

#endif // MINIMAPALL_H
