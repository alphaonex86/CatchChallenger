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
        //...and it FOLLOWS THE SEA. A straight line between the two shores is
        //drawn right across the land whenever the ferry goes round a cape, and
        //the world map then reads as a water route cutting through the continent.
        //The leg is walked over the chunks that hold water instead, and only when
        //no such way exists is the straight line kept.
        const unsigned int chunkXCount=worldWidthMap/mapWidth;
        const unsigned int chunkYCount=worldHeightMap/mapHeight;
        //cost of stepping on a chunk: the more water it holds the cheaper, so the
        //leg BOWS OUT INTO THE SEA instead of hugging the coast over the land
        std::vector<unsigned int> seaChunk(chunkXCount*chunkYCount,0);
        if(!LoadMapAll::waterBodyOfTile.empty())
        {
            unsigned int chunk=0;
            while(chunk<chunkXCount*chunkYCount)
            {
                unsigned int waterTiles=0;
                unsigned int localY=0;
                while(localY<mapHeight)
                {
                    unsigned int localX=0;
                    while(localX<mapWidth)
                    {
                        const unsigned int tileX=(chunk%chunkXCount)*mapWidth+localX;
                        const unsigned int tileY=(chunk/chunkXCount)*mapHeight+localY;
                        if(LoadMapAll::waterBodyOfTile.at(tileX+tileY*worldWidthMap)!=LoadMapAll::waterNoBody)
                            waterTiles++;
                        localX++;
                    }
                    localY++;
                }
                const unsigned int waterPercent=waterTiles*100/(mapWidth*mapHeight);
                if(waterPercent>=75)
                    seaChunk[chunk]=1;
                else if(waterPercent>=50)
                    seaChunk[chunk]=3;
                else if(waterPercent>=25)
                    seaChunk[chunk]=10;
                chunk++;
            }
        }
        unsigned int indexCrossing=0;
        while(indexCrossing<LoadMapAll::boatCrossings.size())
        {
            const LoadMapAll::BoatCrossing &crossing=LoadMapAll::boatCrossings.at(indexCrossing);
            const unsigned int fromChunk=(unsigned int)crossing.fromX+(unsigned int)crossing.fromY*chunkXCount;
            const unsigned int toChunk=(unsigned int)crossing.toX+(unsigned int)crossing.toY*chunkXCount;
            std::vector<int> parent(chunkXCount*chunkYCount,-2);
            std::vector<unsigned int> cost(chunkXCount*chunkYCount,0xFFFFFFFF);
            std::priority_queue<std::pair<unsigned int,unsigned int>,
                    std::vector<std::pair<unsigned int,unsigned int> >,
                    std::greater<std::pair<unsigned int,unsigned int> > > walk;
            parent[fromChunk]=-1;
            cost[fromChunk]=0;
            walk.push(std::pair<unsigned int,unsigned int>(0,fromChunk));
            bool found=false;
            while(!walk.empty() && !found)
            {
                const std::pair<unsigned int,unsigned int> top=walk.top();
                walk.pop();
                const unsigned int chunk=top.second;
                if(top.first<=cost.at(chunk))
                {
                    if(chunk==toChunk)
                        found=true;
                    else
                    {
                        const int chunkX=(int)(chunk%chunkXCount);
                        const int chunkY=(int)(chunk/chunkXCount);
                        const int stepX[4]={-1,1,0,0};
                        const int stepY[4]={0,0,-1,1};
                        unsigned int direction=0;
                        while(direction<4)
                        {
                            const int nextX=chunkX+stepX[direction];
                            const int nextY=chunkY+stepY[direction];
                            if(nextX>=0 && nextY>=0 && nextX<(int)chunkXCount && nextY<(int)chunkYCount)
                            {
                                const unsigned int next=(unsigned int)nextX+(unsigned int)nextY*chunkXCount;
                                const unsigned int stepCost=(next==toChunk)?1:seaChunk.at(next);
                                if(stepCost>0 && cost.at(chunk)+stepCost<cost.at(next))
                                {
                                    cost[next]=cost.at(chunk)+stepCost;
                                    parent[next]=(int)chunk;
                                    walk.push(std::pair<unsigned int,unsigned int>(cost.at(next),next));
                                }
                            }
                            direction++;
                        }
                    }
                }
            }
            if(!found)
                p.drawLine((int)(crossing.fromX*mapWidth+mapWidth/2),
                           (int)(crossing.fromY*mapHeight+mapHeight/2),
                           (int)(crossing.toX*mapWidth+mapWidth/2),
                           (int)(crossing.toY*mapHeight+mapHeight/2));
            else
            {
                int walkChunk=(int)toChunk;
                while(parent.at((unsigned int)walkChunk)>=0)
                {
                    const unsigned int previous=(unsigned int)parent.at((unsigned int)walkChunk);
                    p.drawLine((int)((unsigned int)walkChunk%chunkXCount*mapWidth+mapWidth/2),
                               (int)((unsigned int)walkChunk/chunkXCount*mapHeight+mapHeight/2),
                               (int)(previous%chunkXCount*mapWidth+mapWidth/2),
                               (int)(previous/chunkXCount*mapHeight+mapHeight/2));
                    walkChunk=(int)previous;
                }
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
