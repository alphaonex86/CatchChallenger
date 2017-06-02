#include "VoronioForTiledMapTmx.h"

#include <vector>
#include <iostream>
#include <cmath>
#include <random>

#include "PoissonGenerator.h"
#include "znoise/headers/Simplex.hpp"

#include <boost/polygon/segment_data.hpp>
#include <boost/polygon/voronoi.hpp>

using boost::polygon::voronoi_builder;
using boost::polygon::voronoi_diagram;

typedef boost::polygon::segment_data<int> Segment;

const int VoronioForTiledMapTmx::SCALE = 1000;

Grid VoronioForTiledMapTmx::generateGrid(const unsigned int w, const unsigned int h, const unsigned int seed, const int num) {
    std::mt19937 gen(seed);
    /*std::uniform_real_distribution<double> disw(0,w);
    std::uniform_real_distribution<double> dish(0,h);*/
    std::uniform_real_distribution<double> dis(-0.75, 0.75);

    Grid g;
    g.reserve(size_t(w) * size_t(h));

    /*    for (int x = 0; x < num; ++x)
        g.push_back(Point(disw(gen)*SCALE,dish(gen)*SCALE));*/
    PoissonGenerator::DefaultPRNG PRNG(seed);
    const auto points = PoissonGenerator::GeneratePoissonPoints(num, PRNG, 30, false);

    for(const auto &p : points) {
        double x = double(p.x) * w + dis(gen);
        if (x > w)
            x = w;
        if (x < 0)
            x = 0;
        double y = double(p.y) * h + dis(gen);
        if (y > h)
            y = h;
        if (y < 0)
            y = 0;
        g.emplace_back(SCALE*x, SCALE*y);
    }

    return g;
}

std::vector<QPolygonF> VoronioForTiledMapTmx::computeVoronoi(const Grid &g, int w, int h) {
    QPolygonF rect(QRectF(0.0, 0.0, w, h));

    boost::polygon::voronoi_diagram<double> vd;
    boost::polygon::construct_voronoi(g.begin(), g.end(), &vd);

    std::vector<QPolygonF> cells;
    cells.resize(g.size());

    for (auto &c : vd.cells()) {
        auto e = c.incident_edge();
        unsigned int final_index=c.source_index();
        QPolygonF poly;
        do {
            if (e->is_primary()) {
                if (e->is_finite()) {
                    poly.append(QPointF(e->vertex0()->x(), e->vertex0()->y()) / SCALE);
                }
                else {
                    const auto &cell1 = e->cell();
                    const auto &cell2 = e->twin()->cell();
                    auto p1 = g[cell1->source_index()];
                    auto p2 = g[cell2->source_index()];
                    double ox = 0.5 * (p1.x() + p2.x());
                    double oy = 0.5 * (p1.y() + p2.y());
                    double dx = p1.y() - p2.y();
                    double dy = p2.x() - p1.x();
                    double coef = SCALE * w / std::max(fabs(dx), fabs(dy));

                    if (e->vertex0())
                        poly.append(QPointF(e->vertex0()->x(), e->vertex0()->y()) / SCALE);
                    else
                        poly.append(QPointF(ox - dx * coef, oy - dy * coef) / SCALE);

                    if (e->vertex1())
                        poly.append(QPointF(e->vertex1()->x(), e->vertex1()->y()) / SCALE);
                    else
                        poly.append(QPointF(ox + dx * coef, oy + dy * coef) / SCALE);
                }
            }
            e = e->next();
        } while (e != c.incident_edge());

        cells[final_index]=poly.intersected(rect);
    }

    return cells;
}
