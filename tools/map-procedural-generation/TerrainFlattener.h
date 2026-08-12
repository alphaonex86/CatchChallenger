#ifndef TERRAINFLATTENER_H
#define TERRAINFLATTENER_H

#include <vector>
#include <unordered_map>

#include <QPolygonF>
#include <QRectF>
#include <QPointF>

#include "../map-procedural-generation-terrain/TerrainShaper.h"

//Forces the height/moisure noise to ONE value over a chosen area, so what is
//built there (a city) can not be cut in half by two terrains, then ramps back to
//the natural noise over `falloff` tiles: the gradient RESTARTS at the flat level,
//so the border of the flat zone gets no brutal step (a height gap of 4 is where
//the terrain generator draws its mountain wall).
//
//The area is ALWAYS a polygon in world TILE coordinates. A rectangle and a circle
//are only two ways of building that polygon - one shape test, one distance
//function, no second algorithm to maintain.
class TerrainFlattener : public TerrainShaper
{
public:
    TerrainFlattener();
    virtual ~TerrainFlattener();

    //all three return the index of the created flat zone (for bindZone)
    unsigned int addPolygon(const QPolygonF &polygon,const float height,const float moisure,const float falloff);
    unsigned int addRectangle(const QRectF &rectangle,const float height,const float moisure,const float falloff);
    unsigned int addCircle(const QPointF &center,const float radius,const float height,const float moisure,const float falloff);

    //The terrain is painted per Voronoi zone, ONE noise sample at its site for the
    //whole zone. A zone whose site falls outside the flat area but whose cells
    //reach inside it would still paint its own terrain there - that is exactly the
    //seam cutting a city in two. Such a zone is bound here and always takes the
    //flat value, whatever its distance says.
    void bindZone(const unsigned int voronoiZoneIndex,const unsigned int flatZoneIndex);
    //A PORT keeps its bay: on such a zone a sample whose NATURAL height is the
    //water band is left alone, so the sea reaches into the town instead of being
    //flattened into a field. Off by default — an ordinary town wants one terrain.
    void setKeepWater(const unsigned int flatZoneIndex,const bool keepWater);
    //world size in tiles, needed to look up the Voronoi zone of a sample
    void setWorldSize(const unsigned int width,const unsigned int height);

    virtual void shape(const float x,const float y,float &height,float &moisure) const override;

    unsigned int size() const;

    //polygon builders: a circle is a regular polygon, an octagon a rectangle with
    //cut corners. Both live here so every shape goes through the same code path.
    static QPolygonF circlePolygon(const QPointF &center,const float radius,const unsigned int segments);
    static QPolygonF octagonPolygon(const QRectF &rectangle,const float cut);
private:
    struct FlatZone
    {
        QPolygonF polygon;
        QRectF influence;//polygon bounding box grown by falloff: the cheap reject test
        float height;
        float moisure;
        float falloff;
        bool keepWater;
    };
    std::vector<FlatZone> flatZones;
    std::unordered_map<unsigned int,unsigned int> zoneToFlatZone;
    unsigned int worldWidth;
    unsigned int worldHeight;

    //the flat zone this sample is bound to through its Voronoi zone, NULL when none
    const FlatZone *boundFlatZone(const float x,const float y) const;
    //0 when the point is inside the polygon, else its distance in tiles to the
    //closest edge
    static float outsideDistance(const QPolygonF &polygon,const QPointF &point);
    static float segmentDistance(const QPointF &a,const QPointF &b,const QPointF &point);
};

#endif // TERRAINFLATTENER_H
