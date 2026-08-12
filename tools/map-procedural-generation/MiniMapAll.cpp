#include "MiniMapAll.h"

#include <queue>
#include "LoadMapAll.h"
#include <QCoreApplication>
#include <map>

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

    //THE BOAT LEG ITSELF, so the eye follows the WHOLE journey. A crossing is a
    //teleport: nothing joined the two shores on the world map, so the route read
    //as stopping dead at the coast. It is drawn BEFORE the road icons, in the
    //colour of the sea route icons and dashed — a line the player does not walk.
    if(!LoadMapAll::boatCrossings.empty())
    {
        //the colour of the sea route icons: their most frequent OPAQUE pixel. The
        //middle of the icon is transparent, so sampling it drew a black line.
        QColor crossingColor(0,0,0);
        if(!minimap1wayWater.isNull())
        {
            const QImage sample=minimap1wayWater.convertToFormat(QImage::Format_ARGB32);
            std::map<QRgb,unsigned int> count;
            int pixelY=0;
            while(pixelY<sample.height())
            {
                int pixelX=0;
                while(pixelX<sample.width())
                {
                    const QRgb pixel=sample.pixel(pixelX,pixelY);
                    if(qAlpha(pixel)==255)
                        count[pixel]++;
                    pixelX++;
                }
                pixelY++;
            }
            unsigned int best=0;
            std::map<QRgb,unsigned int>::const_iterator colorIterator=count.cbegin();
            while(colorIterator!=count.cend())
            {
                if(colorIterator->second>best)
                {
                    best=colorIterator->second;
                    crossingColor=QColor(colorIterator->first);
                }
                ++colorIterator;
            }
        }
        QPen crossingPen(crossingColor);
        int crossingWidth=(int)(mapWidth/12);
        if(crossingWidth<2)
            crossingWidth=2;
        crossingPen.setWidth(crossingWidth);
        crossingPen.setStyle(Qt::DashLine);
        p.setPen(crossingPen);
        //...and it FOLLOWS THE WATER, TILE BY TILE. A straight line between the
        //two shores is drawn right across the land whenever the ferry rounds a
        //cape, and the world map then reads as a water route cutting through the
        //continent — which a water path never does. The leg is the shortest way
        //through the WATER between the two harbours, drawn as a polyline; a
        //crossing whose two ends share no water cannot be built in the first
        //place (addWaterPaths), so the straight line is only a last resort.
        const unsigned int chunkXCount=worldWidthMap/mapWidth;
        const unsigned int chunkYCount=worldHeightMap/mapHeight;
        unsigned int indexCrossing=0;
        while(indexCrossing<LoadMapAll::boatCrossings.size())
        {
            const LoadMapAll::BoatCrossing &crossing=LoadMapAll::boatCrossings.at(indexCrossing);
            std::vector<QPoint> leg;
            if(!LoadMapAll::waterBodyOfTile.empty()
                    && crossing.fromX<chunkXCount && crossing.toX<chunkXCount
                    && crossing.fromY<chunkYCount && crossing.toY<chunkYCount)
            {
                std::vector<int> parent(worldWidthMap*worldHeightMap,-2);
                std::vector<unsigned int> queue;
                //FROM THE BOAT TO THE BOAT: the leg starts and ends where the two
                //ships are moored, so it comes out between the two icons instead
                //of hugging whichever edge of the strait happens to be nearest
                unsigned int fromCell=(crossing.fromX*mapWidth+mapWidth/2)
                        +(crossing.fromY*mapHeight+mapHeight/2)*worldWidthMap;
                unsigned int toCell=(crossing.toX*mapWidth+mapWidth/2)
                        +(crossing.toY*mapHeight+mapHeight/2)*worldWidthMap;
                {
                    const std::pair<uint16_t,uint16_t> from(crossing.fromX,crossing.fromY);
                    const std::pair<uint16_t,uint16_t> to(crossing.toX,crossing.toY);
                    if(LoadMapAll::boatLandingCells.find(from)!=LoadMapAll::boatLandingCells.cend())
                        fromCell=(crossing.fromX*mapWidth+LoadMapAll::boatLandingCells.at(from).first)
                                +(crossing.fromY*mapHeight+LoadMapAll::boatLandingCells.at(from).second)
                                 *worldWidthMap;
                    if(LoadMapAll::boatLandingCells.find(to)!=LoadMapAll::boatLandingCells.cend())
                        toCell=(crossing.toX*mapWidth+LoadMapAll::boatLandingCells.at(to).first)
                                +(crossing.toY*mapHeight+LoadMapAll::boatLandingCells.at(to).second)
                                 *worldWidthMap;
                }
                parent[fromCell]=-1;
                queue.push_back(fromCell);
                int reached=-1;
                unsigned int queueIndex=0;
                while(queueIndex<queue.size() && reached<0)
                {
                    const unsigned int cell=queue.at(queueIndex);
                    queueIndex++;
                    if(cell==toCell)
                        reached=(int)cell;
                    else
                    {
                        const int cellX=(int)(cell%worldWidthMap);
                        const int cellY=(int)(cell/worldWidthMap);
                        const int stepX[4]={-1,1,0,0};
                        const int stepY[4]={0,0,-1,1};
                        unsigned int direction=0;
                        while(direction<4)
                        {
                            const int nextX=cellX+stepX[direction];
                            const int nextY=cellY+stepY[direction];
                            if(nextX>=0 && nextY>=0 && nextX<(int)worldWidthMap && nextY<(int)worldHeightMap)
                            {
                                const unsigned int next=(unsigned int)nextX+(unsigned int)nextY*worldWidthMap;
                                //over the water, and onto the far quay to finish
                                if(parent.at(next)==-2
                                        && (LoadMapAll::waterBodyOfTile.at(next)!=LoadMapAll::waterNoBody
                                            || next==toCell))
                                {
                                    parent[next]=(int)cell;
                                    queue.push_back(next);
                                }
                            }
                            direction++;
                        }
                    }
                }
                if(reached>=0)
                {
                    //THE WHOLE LINE RIDES HALF AN ICON LOWER, and it ENDS ON THE
                    //TWO BOATS. The icon is drawn centred on its map, the water
                    //path is drawn on the moorings: left as they are, the leg came
                    //out above both boats and stopped short of them.
                    const int legOffsetY=minimapboat.isNull()?0:(minimapboat.height()/2);
                    leg.push_back(QPoint((int)(crossing.toX*mapWidth+mapWidth/2),
                                         (int)(crossing.toY*mapHeight+mapHeight/2)));
                    int walkCell=reached;
                    while(walkCell>=0)
                    {
                        leg.push_back(QPoint((int)((unsigned int)walkCell%worldWidthMap),
                                             (int)((unsigned int)walkCell/worldWidthMap)+legOffsetY));
                        walkCell=parent.at((unsigned int)walkCell);
                    }
                    leg.push_back(QPoint((int)(crossing.fromX*mapWidth+mapWidth/2),
                                         (int)(crossing.fromY*mapHeight+mapHeight/2)));
                }
            }
            if(leg.size()<2)
                //the last resort: straight from one boat to the other
                p.drawLine((int)(crossing.fromX*mapWidth+mapWidth/2),
                           (int)(crossing.fromY*mapHeight+mapHeight/2),
                           (int)(crossing.toX*mapWidth+mapWidth/2),
                           (int)(crossing.toY*mapHeight+mapHeight/2));
            else
            {
                //one segment every few tiles: the polyline reads as a dashed sea
                //leg instead of a staircase of single pixels
                static const unsigned int legStep=6;
                unsigned int pointIndex=0;
                while(pointIndex+legStep<leg.size())
                {
                    p.drawLine(leg.at(pointIndex),leg.at(pointIndex+legStep));
                    pointIndex+=legStep;
                }
                p.drawLine(leg.at(pointIndex),leg.back());
            }
            indexCrossing++;
        }
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
