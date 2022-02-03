#ifndef VORONIOFORTILEDMAPTMX_H
#define VORONIOFORTILEDMAPTMX_H

#include <boost/polygon/segment_data.hpp>
#include <boost/polygon/voronoi.hpp>
#include <vector>
#include <QPolygonF>

typedef boost::polygon::point_data<int> Point;
typedef std::vector<Point> Grid;

class VoronioForTiledMapTmx
{
public:
    struct PolygonZoneIndex
    {
        float area;
        unsigned int index;
    };
    struct PolygonZone
    {
        QPolygonF polygon;
        std::vector<Point> points;
        QPolygonF pixelizedPolygon;
        unsigned int height;//0-4 (0 is water)
        unsigned int moisure;//1-6
        float heightFloat;
        float moisureFloat;
    };
    struct PolygonZoneMap
    {
        std::vector<PolygonZone> zones;
        PolygonZoneIndex *tileToPolygonZoneIndex;
    };
    static VoronioForTiledMapTmx::PolygonZoneMap voronoiMap;
    static VoronioForTiledMapTmx::PolygonZoneMap voronoiMap1px;

    static Grid generateGrid(const unsigned int w, const unsigned int h, const unsigned int seed, const int num, const int scale);
    static VoronioForTiledMapTmx::PolygonZoneMap computeVoronoi(const Grid &g, const unsigned int w, const unsigned int h, const unsigned int tileStep=1);
    static const int SCALE;
    static double area(const QPolygonF &p);
};

#endif // VORONIOFORTILEDMAPTMX_H
