#include "TerrainFlattener.h"

#include <cmath>
#include <iostream>

#include "../map-procedural-generation-terrain/VoronioForTiledMapTmx.h"
#include "../map-procedural-generation-terrain/LoadMap.h"

TerrainFlattener::TerrainFlattener() :
    worldWidth(0),
    worldHeight(0)
{
}

TerrainFlattener::~TerrainFlattener()
{
}

unsigned int TerrainFlattener::addPolygon(const QPolygonF &polygon,const float height,const float moisure,const float falloff)
{
    if(polygon.size()<3)
    {
        //a degenerate area would read as "distance 0 everywhere" and flatten the world
        std::cerr << "TerrainFlattener: ignored a flat zone of " << polygon.size() << " point(s)" << std::endl;
        return flatZones.size();
    }
    FlatZone flatZone;
    flatZone.polygon=polygon;
    flatZone.height=height;
    flatZone.moisure=moisure;
    flatZone.falloff=falloff;
    flatZone.keepWater=false;
    flatZone.influence=polygon.boundingRect().adjusted(-falloff,-falloff,falloff,falloff);
    flatZones.push_back(flatZone);
    return flatZones.size()-1;
}

unsigned int TerrainFlattener::addRectangle(const QRectF &rectangle,const float height,const float moisure,const float falloff)
{
    return addPolygon(QPolygonF(rectangle),height,moisure,falloff);
}

unsigned int TerrainFlattener::addCircle(const QPointF &center,const float radius,const float height,const float moisure,const float falloff)
{
    //32 segments: on a town sized radius the polygon is less than a tenth of a
    //tile off the true circle, which no tile grid can show
    return addPolygon(circlePolygon(center,radius,32),height,moisure,falloff);
}

void TerrainFlattener::bindZone(const unsigned int voronoiZoneIndex,const unsigned int flatZoneIndex)
{
    if(flatZoneIndex<flatZones.size())
        zoneToFlatZone[voronoiZoneIndex]=flatZoneIndex;
}

void TerrainFlattener::setKeepWater(const unsigned int flatZoneIndex,const bool keepWater)
{
    if(flatZoneIndex<flatZones.size())
        flatZones[flatZoneIndex].keepWater=keepWater;
}

//the WATER band of the terrain, the one place that answers it: LoadMap::floatToHigh
//returns 0 under the sea level, and that is what the terrain painter reads too
static bool sampleIsWater(const float height)
{
    return LoadMap::floatToHigh(height)==0;
}

void TerrainFlattener::setWorldSize(const unsigned int width,const unsigned int height)
{
    worldWidth=width;
    worldHeight=height;
}

unsigned int TerrainFlattener::size() const
{
    return flatZones.size();
}

const TerrainFlattener::FlatZone *TerrainFlattener::boundFlatZone(const float x,const float y) const
{
    const FlatZone *found=NULL;
    if(!zoneToFlatZone.empty() && worldWidth>0 && worldHeight>0
            && VoronioForTiledMapTmx::voronoiMap.tileToPolygonZoneIndex!=NULL)
    {
        int tileX=(int)x;
        int tileY=(int)y;
        if(tileX<0)
            tileX=0;
        if(tileY<0)
            tileY=0;
        if(tileX>=(int)worldWidth)
            tileX=worldWidth-1;
        if(tileY>=(int)worldHeight)
            tileY=worldHeight-1;
        const unsigned int voronoiIndex=VoronioForTiledMapTmx::voronoiMap.tileToPolygonZoneIndex[tileX+tileY*(int)worldWidth].index;
        const std::unordered_map<unsigned int,unsigned int>::const_iterator bound=zoneToFlatZone.find(voronoiIndex);
        if(bound!=zoneToFlatZone.cend())
            found=&flatZones.at(bound->second);
    }
    return found;
}

void TerrainFlattener::shape(const float x,const float y,float &height,float &moisure) const
{
    if(!flatZones.empty())
    {
        const FlatZone * const bound=boundFlatZone(x,y);
        if(bound!=NULL)
        {
            //paints inside the flat area: fully flat, no blend at all — except on
            //a PORT, where the water the town stands beside is kept as it is, so
            //the sea comes into the map instead of being flattened into a field
            if(!(bound->keepWater && sampleIsWater(height)))
            {
                height=bound->height;
                moisure=bound->moisure;
            }
        }
        else
        {
            //outside: ramp from the flat level back to the natural noise. The
            //closest flat zone wins, so two neighbour towns never cancel each other.
            const QPointF point(x,y);
            const FlatZone *best=NULL;
            float bestWeight=0.0;
            unsigned int index=0;
            while(index<flatZones.size())
            {
                const FlatZone &flatZone=flatZones.at(index);
                if(flatZone.influence.contains(point))
                {
                    float weight=0.0;
                    const float distance=outsideDistance(flatZone.polygon,point);
                    if(distance<=0.0)
                        weight=1.0;
                    else
                    {
                        if(distance<flatZone.falloff)
                        {
                            //smoothstep: the slope is 0 on both ends, so the ramp
                            //leaves the flat level and joins the natural noise
                            //without a break on either side
                            const float t=1.0-distance/flatZone.falloff;
                            weight=t*t*(3.0-2.0*t);
                        }
                    }
                    if(weight>bestWeight)
                    {
                        bestWeight=weight;
                        best=&flatZone;
                    }
                }
                index++;
            }
            //the ramp of a PORT leaves the water alone too: pulling the coast
            //just outside the town up to the town level cut the bay off from the
            //sea it belongs to
            if(best!=NULL && !(best->keepWater && sampleIsWater(height)))
            {
                height=height*(1.0-bestWeight)+best->height*bestWeight;
                moisure=moisure*(1.0-bestWeight)+best->moisure*bestWeight;
            }
        }
    }
}

QPolygonF TerrainFlattener::circlePolygon(const QPointF &center,const float radius,const unsigned int segments)
{
    QPolygonF polygon;
    unsigned int usedSegments=segments;
    if(usedSegments<3)
        usedSegments=3;
    unsigned int index=0;
    while(index<usedSegments)
    {
        //M_PI is not standard C++, spell it out rather than depend on the toolchain
        const double angle=2.0*3.14159265358979323846*(double)index/(double)usedSegments;
        polygon.append(QPointF(center.x()+cos(angle)*radius,center.y()+sin(angle)*radius));
        index++;
    }
    return polygon;
}

QPolygonF TerrainFlattener::octagonPolygon(const QRectF &rectangle,const float cut)
{
    float usedCut=cut;
    if(usedCut<0.0)
        usedCut=0.0;
    if(usedCut>rectangle.width()/2.0)
        usedCut=rectangle.width()/2.0;
    if(usedCut>rectangle.height()/2.0)
        usedCut=rectangle.height()/2.0;
    QPolygonF polygon;
    polygon.append(QPointF(rectangle.left()+usedCut,rectangle.top()));
    polygon.append(QPointF(rectangle.right()-usedCut,rectangle.top()));
    polygon.append(QPointF(rectangle.right(),rectangle.top()+usedCut));
    polygon.append(QPointF(rectangle.right(),rectangle.bottom()-usedCut));
    polygon.append(QPointF(rectangle.right()-usedCut,rectangle.bottom()));
    polygon.append(QPointF(rectangle.left()+usedCut,rectangle.bottom()));
    polygon.append(QPointF(rectangle.left(),rectangle.bottom()-usedCut));
    polygon.append(QPointF(rectangle.left(),rectangle.top()+usedCut));
    return polygon;
}

float TerrainFlattener::segmentDistance(const QPointF &a,const QPointF &b,const QPointF &point)
{
    const double vx=b.x()-a.x();
    const double vy=b.y()-a.y();
    const double lengthSquare=vx*vx+vy*vy;
    double ratio=0.0;
    if(lengthSquare>0.0)
    {
        ratio=((point.x()-a.x())*vx+(point.y()-a.y())*vy)/lengthSquare;
        if(ratio<0.0)
            ratio=0.0;
        if(ratio>1.0)
            ratio=1.0;
    }
    const double dx=point.x()-(a.x()+vx*ratio);
    const double dy=point.y()-(a.y()+vy*ratio);
    return (float)sqrt(dx*dx+dy*dy);
}

float TerrainFlattener::outsideDistance(const QPolygonF &polygon,const QPointF &point)
{
    float distance=0.0;
    if(polygon.size()<2)
        distance=0.0;
    else
    {
        if(polygon.containsPoint(point,Qt::OddEvenFill))
            distance=0.0;
        else
        {
            bool first=true;
            int index=0;
            while(index<polygon.size())
            {
                const QPointF &a=polygon.at(index);
                const QPointF &b=polygon.at((index+1)%polygon.size());
                const float edgeDistance=segmentDistance(a,b,point);
                if(first || edgeDistance<distance)
                {
                    distance=edgeDistance;
                    first=false;
                }
                index++;
            }
        }
    }
    return distance;
}
