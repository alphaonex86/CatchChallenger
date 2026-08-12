#include "MiniMapAll.h"
#include "LoadMapAll.h"
#include <QCoreApplication>

QImage MiniMapAll::minimapcitybig;
QImage MiniMapAll::minimapcitymedium;
QImage MiniMapAll::minimapcitysmall;
QImage MiniMapAll::minimapboat;

QImage MiniMapAll::minimap1way;
QImage MiniMapAll::minimap2way1;
QImage MiniMapAll::minimap2way2;
QImage MiniMapAll::minimap3way;
QImage MiniMapAll::minimap4way;

QImage MiniMapAll::minimap1wayWater;
QImage MiniMapAll::minimap2way1Water;
QImage MiniMapAll::minimap2way2Water;
QImage MiniMapAll::minimap3wayWater;
QImage MiniMapAll::minimap4wayWater;

//Orientation_top=1,Orientation_right=2,Orientation_bottom=4,Orientation_left=8
void MiniMapAll::drawRoad(const uint8_t orientation,QPainter &p,const unsigned int x,const unsigned int y,const unsigned int mapWidth,const unsigned int mapHeight,const bool water)
{
    QImage minimaptemp;
    QTransform rotating;
    //a SEA route draws the water variants of the same shapes
    const QImage &minimap1way=water?minimap1wayWater:MiniMapAll::minimap1way;
    const QImage &minimap2way1=water?minimap2way1Water:MiniMapAll::minimap2way1;
    const QImage &minimap2way2=water?minimap2way2Water:MiniMapAll::minimap2way2;
    const QImage &minimap3way=water?minimap3wayWater:MiniMapAll::minimap3way;
    const QImage &minimap4way=water?minimap4wayWater:MiniMapAll::minimap4way;
    switch(orientation)
    {
    default:
    case 0:
    break;
    case 1:
        minimaptemp=minimap1way;rotating.rotate(180);minimaptemp=minimaptemp.transformed(rotating);
        p.drawImage(QPoint(x*mapWidth+mapWidth/2-minimaptemp.width()/2,y*mapHeight+mapHeight/2-minimaptemp.height()/2),minimaptemp);
    break;
    case 2:
        minimaptemp=minimap1way;rotating.rotate(270);minimaptemp=minimaptemp.transformed(rotating);
        p.drawImage(QPoint(x*mapWidth+mapWidth/2-minimaptemp.width()/2,y*mapHeight+mapHeight/2-minimaptemp.height()/2),minimaptemp);
    break;
    case 3:
        minimaptemp=minimap2way2;rotating.rotate(270);minimaptemp=minimaptemp.transformed(rotating);
        p.drawImage(QPoint(x*mapWidth+mapWidth/2-minimaptemp.width()/2,y*mapHeight+mapHeight/2-minimaptemp.height()/2),minimaptemp);
    break;
    case 4:
        minimaptemp=minimap1way;
        p.drawImage(QPoint(x*mapWidth+mapWidth/2-minimaptemp.width()/2,y*mapHeight+mapHeight/2-minimaptemp.height()/2),minimaptemp);
    break;
    case 5:
        minimaptemp=minimap2way1;
        p.drawImage(QPoint(x*mapWidth+mapWidth/2-minimaptemp.width()/2,y*mapHeight+mapHeight/2-minimaptemp.height()/2),minimaptemp);
    break;
    case 6:
        minimaptemp=minimap2way2;
        p.drawImage(QPoint(x*mapWidth+mapWidth/2-minimaptemp.width()/2,y*mapHeight+mapHeight/2-minimaptemp.height()/2),minimaptemp);
    break;
    case 7:
        minimaptemp=minimap3way;
        p.drawImage(QPoint(x*mapWidth+mapWidth/2-minimaptemp.width()/2,y*mapHeight+mapHeight/2-minimaptemp.height()/2),minimaptemp);
    break;
    case 8:
        minimaptemp=minimap1way;rotating.rotate(90);minimaptemp=minimaptemp.transformed(rotating);
        p.drawImage(QPoint(x*mapWidth+mapWidth/2-minimaptemp.width()/2,y*mapHeight+mapHeight/2-minimaptemp.height()/2),minimaptemp);
    break;
    case 9:
        minimaptemp=minimap2way2;rotating.rotate(180);minimaptemp=minimaptemp.transformed(rotating);
        p.drawImage(QPoint(x*mapWidth+mapWidth/2-minimaptemp.width()/2,y*mapHeight+mapHeight/2-minimaptemp.height()/2),minimaptemp);
    break;
    case 10:
        minimaptemp=minimap2way1;rotating.rotate(90);minimaptemp=minimaptemp.transformed(rotating);
        p.drawImage(QPoint(x*mapWidth+mapWidth/2-minimaptemp.width()/2,y*mapHeight+mapHeight/2-minimaptemp.height()/2),minimaptemp);
    break;
    case 11:
        minimaptemp=minimap3way;rotating.rotate(270);minimaptemp=minimaptemp.transformed(rotating);
        p.drawImage(QPoint(x*mapWidth+mapWidth/2-minimaptemp.width()/2,y*mapHeight+mapHeight/2-minimaptemp.height()/2),minimaptemp);
    break;
    case 12:
        minimaptemp=minimap2way2;rotating.rotate(90);minimaptemp=minimaptemp.transformed(rotating);
        p.drawImage(QPoint(x*mapWidth+mapWidth/2-minimaptemp.width()/2,y*mapHeight+mapHeight/2-minimaptemp.height()/2),minimaptemp);
    break;
    case 13:
        minimaptemp=minimap3way;rotating.rotate(180);minimaptemp=minimaptemp.transformed(rotating);
        p.drawImage(QPoint(x*mapWidth+mapWidth/2-minimaptemp.width()/2,y*mapHeight+mapHeight/2-minimaptemp.height()/2),minimaptemp);
    break;
    case 14:
        minimaptemp=minimap3way;rotating.rotate(90);minimaptemp=minimaptemp.transformed(rotating);
        p.drawImage(QPoint(x*mapWidth+mapWidth/2-minimaptemp.width()/2,y*mapHeight+mapHeight/2-minimaptemp.height()/2),minimaptemp);
    break;
    case 15:
        minimaptemp=minimap4way;
        p.drawImage(QPoint(x*mapWidth+mapWidth/2-minimaptemp.width()/2,y*mapHeight+mapHeight/2-minimaptemp.height()/2),minimaptemp);
    break;
    }
}

QImage MiniMapAll::makeMapTiled(const unsigned int worldWidthMap, const unsigned int worldHeightMap, const unsigned int mapWidth, const unsigned int mapHeight)
{
    const unsigned int mapXCount=worldWidthMap/mapWidth;

    QImage destination=MiniMap::makeMapTiled(worldWidthMap,worldHeightMap);
    QPainter p(&destination);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    MiniMapAll::minimapcitybig=QImage(QCoreApplication::applicationDirPath()+"/minimap-citybig.png");
    MiniMapAll::minimapcitymedium=QImage(QCoreApplication::applicationDirPath()+"/minimap-citymedium.png");
    MiniMapAll::minimapcitysmall=QImage(QCoreApplication::applicationDirPath()+"/minimap-citysmall.png");

    MiniMapAll::minimapboat=QImage(QCoreApplication::applicationDirPath()+"/boat.png");

    MiniMapAll::minimap1way=QImage(QCoreApplication::applicationDirPath()+"/minimap-1way.png");
    MiniMapAll::minimap2way1=QImage(QCoreApplication::applicationDirPath()+"/minimap-2way1.png");
    MiniMapAll::minimap2way2=QImage(QCoreApplication::applicationDirPath()+"/minimap-2way2.png");
    MiniMapAll::minimap3way=QImage(QCoreApplication::applicationDirPath()+"/minimap-3way.png");
    MiniMapAll::minimap4way=QImage(QCoreApplication::applicationDirPath()+"/minimap-4way.png");

    MiniMapAll::minimap1wayWater=QImage(QCoreApplication::applicationDirPath()+"/minimap-1way-water.png");
    MiniMapAll::minimap2way1Water=QImage(QCoreApplication::applicationDirPath()+"/minimap-2way1-water.png");
    MiniMapAll::minimap2way2Water=QImage(QCoreApplication::applicationDirPath()+"/minimap-2way2-water.png");
    MiniMapAll::minimap3wayWater=QImage(QCoreApplication::applicationDirPath()+"/minimap-3way-water.png");
    MiniMapAll::minimap4wayWater=QImage(QCoreApplication::applicationDirPath()+"/minimap-4way-water.png");

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

        const uint8_t &zoneOrientation=LoadMapAll::mapPathDirection[x+y*mapXCount];
        drawRoad(zoneOrientation,p,x,y,mapWidth,mapHeight);
        p.drawImage(QPoint(x*mapWidth+mapWidth/2-minimapcity.width()/2,y*mapHeight+mapHeight/2-minimapcity.height()/2),minimapcity);

        indexCity++;
    }

    unsigned int indexIntRoad=0;
    while(indexIntRoad<LoadMapAll::roads.size())
    {
        const LoadMapAll::Road &road=LoadMapAll::roads.at(indexIntRoad);
        unsigned int indexCoord=0;
        while(indexCoord<road.coords.size())
        {
            const std::pair<uint16_t,uint16_t> &coord=road.coords.at(indexCoord);
            const unsigned int &x=coord.first;
            const unsigned int &y=coord.second;

            const uint8_t &zoneOrientation=LoadMapAll::mapPathDirection[x+y*mapXCount];
            if(zoneOrientation!=0)
            {
                //a SEA route reads as water on the world map, not as a road. A
                //boat chunk shows the ONE-WAY water icon and that is right: its
                //border path really does end there, the crossing carries on by
                //teleport.
                bool water=false;
                if(LoadMapAll::roadCoordToIndex.find((uint16_t)x)!=LoadMapAll::roadCoordToIndex.cend()
                        && LoadMapAll::roadCoordToIndex.at((uint16_t)x).find((uint16_t)y)!=LoadMapAll::roadCoordToIndex.at((uint16_t)x).cend())
                    water=LoadMapAll::roadCoordToIndex.at((uint16_t)x).at((uint16_t)y).isWater;
                drawRoad(zoneOrientation,p,x,y,mapWidth,mapHeight,water);
                //a boat crossing is a PLACE on the world map, like a town: the
                //route leaves the coast there and comes back on the far shore
                if(LoadMapAll::roadCoordToIndex.at((uint16_t)x).at((uint16_t)y).isBoat
                        && !minimapboat.isNull())
                    p.drawImage(QPoint(x*mapWidth+mapWidth/2-minimapboat.width()/2,
                                       y*mapHeight+mapHeight/2-minimapboat.height()/2),minimapboat);
            }
            indexCoord++;
        }
        indexIntRoad++;
    }

    p.end();
    return destination;
}
