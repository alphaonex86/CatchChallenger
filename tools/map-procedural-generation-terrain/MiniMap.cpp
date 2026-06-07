#include "MiniMap.h"
#include <QImage>
#include <QCoreApplication>
#include <iostream>
#include "LoadMap.h"
#include <libtiled/tile.h>
#include <QPainter>

#include "VoronioForTiledMapTmx.h"
#include <thread>
#include <vector>
#include <cmath>

// Worker: fill rows [y0,y1) of the LINEAR (per-pixel noise) minimap.  The Simplex
// noise is a deterministic pure read and the rows are disjoint, so splitting the
// rows across threads is BIT-IDENTICAL to the serial loop — same arithmetic, and
// mc[] (the source converted to ARGB32) is exactly the QRgb that the old
// setPixelColor(pixelColor(...)) wrote on an ARGB32 destination.
static void miniMapNoiseRows(unsigned int y0,unsigned int y1,unsigned int destW,QRgb *destBits,
                             const Simplex *heightmap,const Simplex *moisuremap,
                             float noiseMapScaleMoisure,float noiseMapScaleMap,float worldScale,float miniMapDivisor,
                             const QRgb *mc,int mcW,int mcH)
{
    unsigned int y=y0;
    while(y<y1)
    {
        unsigned int x=0;
        while(x<destW)
        {
            float xMap=(float)x*worldScale/miniMapDivisor;
            float yMap=(float)y*worldScale/miniMapDivisor;
            float height=heightmap->Get({xMap/100,yMap/100},noiseMapScaleMap);
            float moisure=moisuremap->Get({xMap/100,yMap/100},noiseMapScaleMoisure);
            height=(height+1.0)/2.0;
            moisure=(moisure+1.0)/2.0;
            unsigned int cx=floor(moisure*(mcW+0));
            if(cx>=(unsigned int)mcW) cx=mcW-1;
            unsigned int cy=floor(height*(mcH+0));
            if(cy>=(unsigned int)mcH) cy=mcH-1;
            destBits[(size_t)y*destW+x]=mc[(size_t)cy*mcW+cx];
            x++;
        }
        y++;
    }
}

// Worker: fill rows [y0,y1) of the TILED (per-tile Voronoi-zone) minimap.  Reads
// the immutable voronoiMap; rows disjoint -> identical output.
static void miniMapTiledRows(unsigned int y0,unsigned int y1,unsigned int destW,QRgb *destBits,
                             const QRgb *mc,int mcW,int mcH)
{
    unsigned int y=y0;
    while(y<y1)
    {
        unsigned int x=0;
        while(x<destW)
        {
            const VoronioForTiledMapTmx::PolygonZone &zone=VoronioForTiledMapTmx::voronoiMap.zones.at(VoronioForTiledMapTmx::voronoiMap.tileToPolygonZoneIndex[x+y*destW].index);
            float height=zone.heightFloat;
            float moisure=zone.moisureFloat;
            height=(height+1.0)/2.0;
            moisure=(moisure+1.0)/2.0;
            unsigned int cx=floor(moisure*(mcW+0));
            if(cx>=(unsigned int)mcW) cx=mcW-1;
            unsigned int cy=floor(height*(mcH+0));
            if(cy>=(unsigned int)mcH) cy=mcH-1;
            destBits[(size_t)y*destW+x]=mc[(size_t)cy*mcW+cx];
            x++;
        }
        y++;
    }
}

// number of worker threads to use for a height-`destH` image (all cores, capped by rows)
static unsigned int miniMapThreads(unsigned int destH)
{
    unsigned int nth=std::thread::hardware_concurrency();
    if(nth<1) nth=1;
    if(nth>destH && destH>0) nth=destH;
    return nth;
}

QImage MiniMap::makeMap(const Simplex &heightmap, const Simplex &moisuremap, const float &noiseMapScaleMoisure, const float &noiseMapScaleMap,
                      const unsigned int widthMap, const unsigned int heightMap, const float miniMapDivisor)
{
    QImage miniMapColor(QCoreApplication::applicationDirPath()+"/miniMapColor.png");
    if(miniMapColor.isNull())
    {
        std::cerr << QCoreApplication::applicationDirPath().toStdString() << "/miniMapColor.png" << " is not found or invalid" << std::endl;
        abort();
    }
    miniMapColor=miniMapColor.convertToFormat(QImage::Format_ARGB32);
    const int mcW=miniMapColor.width(), mcH=miniMapColor.height();
    const QRgb *mc=reinterpret_cast<const QRgb*>(miniMapColor.constBits());
    QImage destination(widthMap*miniMapDivisor,heightMap*miniMapDivisor,QImage::Format_ARGB32);
    destination.fill(QColor(0,0,0,0));
    const unsigned int destW=destination.width(), destH=destination.height();
    QRgb *destBits=reinterpret_cast<QRgb*>(destination.bits());
    const unsigned int nth=miniMapThreads(destH);
    const unsigned int chunk=(destH+nth-1)/nth;
    std::vector<std::thread> pool;
    unsigned int ti=0;
    while(ti<nth)
    {
        const unsigned int yy0=ti*chunk;
        unsigned int yy1=yy0+chunk;
        if(yy1>destH) yy1=destH;
        if(yy0<yy1)
            pool.push_back(std::thread(miniMapNoiseRows,yy0,yy1,destW,destBits,&heightmap,&moisuremap,
                                       noiseMapScaleMoisure,noiseMapScaleMap,(float)VoronioForTiledMapTmx::SCALE,miniMapDivisor,mc,mcW,mcH));
        ti++;
    }
    size_t i=0;
    while(i<pool.size()) { pool[i].join(); i++; }
    return destination;
}

QImage MiniMap::makeMapTiled(const unsigned int widthMap, const unsigned int heightMap)
{
    QImage miniMapColor(QCoreApplication::applicationDirPath()+"/miniMapColor.png");
    if(miniMapColor.isNull())
    {
        std::cerr << QCoreApplication::applicationDirPath().toStdString() << "/miniMapColor.png" << " is not found or invalid" << std::endl;
        abort();
    }
    miniMapColor=miniMapColor.convertToFormat(QImage::Format_ARGB32);
    const int mcW=miniMapColor.width(), mcH=miniMapColor.height();
    const QRgb *mc=reinterpret_cast<const QRgb*>(miniMapColor.constBits());
    QImage destination(widthMap,heightMap,QImage::Format_ARGB32);
    destination.fill(QColor(0,0,0,0));
    const unsigned int destW=destination.width(), destH=destination.height();
    QRgb *destBits=reinterpret_cast<QRgb*>(destination.bits());
    const unsigned int nth=miniMapThreads(destH);
    const unsigned int chunk=(destH+nth-1)/nth;
    std::vector<std::thread> pool;
    unsigned int ti=0;
    while(ti<nth)
    {
        const unsigned int yy0=ti*chunk;
        unsigned int yy1=yy0+chunk;
        if(yy1>destH) yy1=destH;
        if(yy0<yy1)
            pool.push_back(std::thread(miniMapTiledRows,yy0,yy1,destW,destBits,mc,mcW,mcH));
        ti++;
    }
    size_t i=0;
    while(i<pool.size()) { pool[i].join(); i++; }
    return destination;
}

bool MiniMap::makeMapTerrainOverview()
{
    QPixmap miniMapColor(QCoreApplication::applicationDirPath()+"/high-map.png");
    if(miniMapColor.isNull())
    {
        std::cerr << QCoreApplication::applicationDirPath().toStdString() << "/high-map.png" << " is not found or invalid" << std::endl;
        abort();
    }
    unsigned int x=0;
    while(x<5)
    {
        unsigned int y=0;
        while(y<6)
        {
            QPixmap tile=LoadMap::terrainList[x][y].tile->image();
            tile=tile.scaled(32,32,Qt::IgnoreAspectRatio,Qt::FastTransformation);
            QPainter painter(&miniMapColor);
            painter.drawPixmap(80+y*(16+32), 80+x*(16+32), tile);
            y++;
        }
        x++;
    }
    return miniMapColor.save(QCoreApplication::applicationDirPath()+"/high-map.png");
}
