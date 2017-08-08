#include "MiniMap.h"
#include <QImage>
#include <QCoreApplication>
#include <iostream>

#include "VoronioForTiledMapTmx.h"

QImage MiniMap::makeMap(const Simplex &heightmap, const Simplex &moisuremap, const float &noiseMapScaleMoisure, const float &noiseMapScaleMap,
                      const unsigned int widthMap, const unsigned int heightMap, const float miniMapDivisor)
{
    QImage miniMapColor(QCoreApplication::applicationDirPath()+"/miniMapColor.png");
    if(miniMapColor.isNull())
    {
        std::cerr << QCoreApplication::applicationDirPath().toStdString() << "/miniMapColor.png" << " is not found or invalid" << std::endl;
        abort();
    }
    QImage destination(widthMap*miniMapDivisor,heightMap*miniMapDivisor,QImage::Format_ARGB32);
    destination.fill(QColor(0,0,0,0));
    unsigned int y=0;
    while(y<(unsigned int)destination.height())
    {
        unsigned int x=0;
        while(x<(unsigned int)destination.width())
        {
            float xMap=(float)x*VoronioForTiledMapTmx::SCALE/miniMapDivisor;
            float yMap=(float)y*VoronioForTiledMapTmx::SCALE/miniMapDivisor;
            float height=heightmap.Get({xMap/100,yMap/100},noiseMapScaleMap);
            float moisure=moisuremap.Get({xMap/100,yMap/100},noiseMapScaleMoisure);
            height=(height+1.0)/2.0;
            moisure=(moisure+1.0)/2.0;
            unsigned int miniMapColorX=floor(moisure*(miniMapColor.width()+0));
            if(miniMapColorX>=(unsigned int)miniMapColor.width())
                miniMapColorX=miniMapColor.width()-1;
            unsigned int miniMapColorY=floor(height*(miniMapColor.height()+0));
            if(miniMapColorY>=(unsigned int)miniMapColor.height())
                miniMapColorY=miniMapColor.height()-1;
            const QColor &pixelColor=miniMapColor.pixelColor(miniMapColorX,miniMapColorY);
            destination.setPixelColor(x,y,pixelColor);
            //destination.setPixelColor(QPoint(x,y),QColor(height*255,height*255,height*255));
            x++;
        }
        y++;
    }
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
    QImage destination(widthMap,heightMap,QImage::Format_ARGB32);
    destination.fill(QColor(0,0,0,0));
    unsigned int y=0;
    while(y<(unsigned int)destination.height())
    {
        unsigned int x=0;
        while(x<(unsigned int)destination.width())
        {
            const VoronioForTiledMapTmx::PolygonZone &zone=VoronioForTiledMapTmx::voronoiMap.zones.at(VoronioForTiledMapTmx::voronoiMap.tileToPolygonZoneIndex[x+y*destination.width()].index);
            float height=zone.heightFloat;
            float moisure=zone.moisureFloat;
            height=(height+1.0)/2.0;
            moisure=(moisure+1.0)/2.0;
            unsigned int miniMapColorX=floor(moisure*(miniMapColor.width()+0));
            if(miniMapColorX>=(unsigned int)miniMapColor.width())
                miniMapColorX=miniMapColor.width()-1;
            unsigned int miniMapColorY=floor(height*(miniMapColor.height()+0));
            if(miniMapColorY>=(unsigned int)miniMapColor.height())
                miniMapColorY=miniMapColor.height()-1;
            const QColor &pixelColor=miniMapColor.pixelColor(miniMapColorX,miniMapColorY);
            destination.setPixelColor(x,y,pixelColor);
            x++;
        }
        y++;
    }
    return destination;
}
