#include "MiniMapAll.h"
#include "LoadMapAll.h"
#include <QCoreApplication>
#include <QPainter>

QImage MiniMapAll::makeMapTiled(const unsigned int worldWidthMap, const unsigned int worldHeightMap, const unsigned int mapWidth, const unsigned int mapHeight)
{
    QImage destination=MiniMap::makeMapTiled(worldWidthMap,worldHeightMap);
    QImage minimapcitybig(QCoreApplication::applicationDirPath()+"/minimap-citybig.png");
    QImage minimapcitymedium(QCoreApplication::applicationDirPath()+"/minimap-citymedium.png");
    QImage minimapcitysmall(QCoreApplication::applicationDirPath()+"/minimap-citysmall.png");

    unsigned int indexCity=0;
    while(indexCity<LoadMapAll::cities.size())
    {
        const LoadMapAll::City &city=LoadMapAll::cities.at(indexCity);
        const uint32_t x=city.x;
        const uint32_t y=city.y;
        QImage minimapcity;
        switch(city.type)
        {
            case LoadMapAll::CityType_big:
                minimapcity=minimapcitybig;
            break;
            case LoadMapAll::CityType_medium:
                minimapcity=minimapcitymedium;
            break;
            default:
            case LoadMapAll::CityType_small:
                minimapcity=minimapcitysmall;
            break;
        }

        QPainter p(&destination);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.drawImage(QPoint(x*mapWidth+mapWidth/2-minimapcity.width()/2,y*mapHeight+mapHeight/2-minimapcity.height()/2),minimapcity);
        p.end();

        indexCity++;
    }

    return destination;
}
