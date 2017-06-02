#ifndef VORONIOFORTILEDMAPTMX_H
#define VORONIOFORTILEDMAPTMX_H

#include <boost/polygon/segment_data.hpp>
#include <boost/polygon/voronoi.hpp>
#include <vector>
#include <QPolygonF>

typedef boost::polygon::point_data<int> Point;
typedef std::vector<Point> Grid;

template <typename T, size_t R, size_t C>
struct Matrix {
    Matrix() = default;
    void fill(const T &t) { arr.fill(t); }
    T& operator()(size_t r, size_t c) { return arr[R * c + r]; }
    const T& operator()(size_t r, size_t c) const { return arr[R * c + r]; }
private:
    std::array<T, C*R> arr;
};

template <typename T, size_t R, size_t C>
std::ostream & operator<<(std::ostream &os, const Matrix<T, R, C> &m) {
    for(size_t r = 0; r < R; ++r) {
        for(size_t c = 0; c < C; ++c) {
            os << m(r, c) << " ";
        }
        os << "\n";
    }
    return os;
}

class VoronioForTiledMapTmx
{
public:
    struct PolygonZoneIndex
    {
        float area;
        uint8_t index;
    };
    struct PolygonZone
    {
        QPolygonF polygons;
        std::vector<Point> points;
    };
    struct PolygonZoneMap
    {
        std::vector<PolygonZone> zones;
        PolygonZoneIndex *tileToPolygonZoneIndex;
    };
    static Grid generateGrid(const unsigned int w, const unsigned int h, const unsigned int seed, const int num);
    static std::vector<PolygonZoneOnMap> computeVoronoi(const Grid &g, int w, int h);
    static const int SCALE;
};

#endif // VORONIOFORTILEDMAPTMX_H
