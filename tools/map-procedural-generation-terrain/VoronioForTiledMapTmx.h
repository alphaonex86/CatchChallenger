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
    static Grid generateGrid(const unsigned int w, const unsigned int h, const unsigned int seed, const int num);
    static std::vector<QPolygonF> computeVoronoi(const Grid &g, double w, double h);
    static const int SCALE;
};

#endif // VORONIOFORTILEDMAPTMX_H
