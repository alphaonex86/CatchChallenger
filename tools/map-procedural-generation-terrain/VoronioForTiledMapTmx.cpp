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

double VoronioForTiledMapTmx::area(const QPolygonF &p) {
    double a = 0.0;
    for (int i = 0; i < p.size() - 1; ++i)
        a += p[i].x() * p[i+1].y() - p[i+1].x() * p[i].y();
    return 0.5 * a;
}

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

VoronioForTiledMapTmx::PolygonZoneMap VoronioForTiledMapTmx::computeVoronoi(const Grid &g, const unsigned int w, const unsigned int h) {
    QPolygonF rect(QRectF(0.0, 0.0, w, h));

    boost::polygon::voronoi_diagram<double> vd;
    boost::polygon::construct_voronoi(g.begin(), g.end(), &vd);

    PolygonZoneMap polygonZoneMap;
    polygonZoneMap.tileToPolygonZoneIndex=(PolygonZoneIndex *)malloc(sizeof(PolygonZoneIndex)*w*h);
    memset(polygonZoneMap.tileToPolygonZoneIndex,0,sizeof(PolygonZoneIndex)*w*h);
    polygonZoneMap.zones.resize(g.size());

    for (auto &c : vd.cells()) {
        float minX=99999999999,minY=99999999999,maxX=0,maxY=0;
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

        const QPolygonF &resultPolygon=poly.intersected(rect);
        polygonZoneMap.zones[final_index].polygon=resultPolygon;
        if(resultPolygon.size()>2)
        {
            QList<QPointF> points=resultPolygon.toList();
            {
                QPointF point=points.first();
                float x=point.x(),y=point.y();
                if(x<0)
                    x=0;
                if(y<0)
                    y=0;
                if(x>w)
                    x=w;
                if(y>h)
                    y=h;
                float tempminX=floor(x),tempminY=floor(y),tempmaxX=ceil(x),tempmaxY=ceil(y);
                minX=tempminX;
                minY=tempminY;
                maxX=tempmaxX;
                maxY=tempmaxY;
                unsigned int index=1;
                while(index<(unsigned int)points.size())
                {
                    const QPointF &point=points.at(index);
                    float x=point.x(),y=point.y();
                    if(x<0)
                        x=0;
                    if(y<0)
                        y=0;
                    if(x>w)
                        x=w;
                    if(y>h)
                        y=h;
                    float tempminX=floor(x),tempminY=floor(y),tempmaxX=ceil(x),tempmaxY=ceil(y);
                    if(tempminX<minX)
                        minX=tempminX;
                    if(tempminY<minY)
                        minY=tempminY;
                    if(tempmaxX>maxX)
                        maxX=tempmaxX;
                    if(tempmaxY>maxY)
                        maxY=tempmaxY;
                    index++;
                }
            }
            unsigned int y=minY;
            while(y<maxY)
            {
                unsigned int x=minX;
                while(x<maxX)
                {
                    const QPolygonF tileRect(QRectF(x,y,1.0,1.0));
                    const QPolygonF &tilePolygon=resultPolygon.intersected(tileRect);
                    const double &a=area(tilePolygon);
                    if(a>1.01)
                        abort();
                    PolygonZoneIndex &tileIndex=polygonZoneMap.tileToPolygonZoneIndex[x+y*w];
                    if(a>tileIndex.area)
                    {
                        tileIndex.area=a;
                        tileIndex.index=final_index;
                    }
                    x++;
                }
                y++;
            }
        }
    }

    //resolv the return function
    {
        unsigned int y=0;
        while(y<h)
        {
            unsigned int x=0;
            while(x<w)
            {
                PolygonZoneIndex &tileIndex=polygonZoneMap.tileToPolygonZoneIndex[x+y*w];
                std::vector<Point> &points=polygonZoneMap.zones[tileIndex.index].points;
                points.push_back(Point(x,y));
                x++;
            }
            y++;
        }
    }

    //dump to output
    /*{
        unsigned int y=0;
        while(y<h)
        {
            unsigned int x=0;
            while(x<w)
            {
                const PolygonZoneIndex &i=polygonZoneMap.tileToPolygonZoneIndex[x+y*w];
                if(i.index<10)
                    std::cout << "00" << std::to_string(i.index) << " ";
                else if(i.index<100)
                    std::cout << "0" << std::to_string(i.index) << " ";
                else
                    std::cout << std::to_string(i.index) << " ";
                x++;
            }
            std::cout << std::endl;
            y++;
        }
        std::cout << std::endl;
    }*/

    //group the tile into pixelised polygon
    {
        enum Direction
        {
            Direction_top,
            Direction_right,
            Direction_bottom,
            Direction_left,
        };
        unsigned int y=0;
        while(y<h)
        {
            unsigned int x=0;
            while(x<w)
            {
                PolygonZoneIndex &tileIndex=polygonZoneMap.tileToPolygonZoneIndex[x+y*w];
                PolygonZone &zone=polygonZoneMap.zones[tileIndex.index];
                QPolygonF &pixelizedPolygon=zone.pixelizedPolygon;
                if(!zone.points.empty() && pixelizedPolygon.empty())
                {
                    bool stopIt=false;
                    unsigned int polygonx=x;
                    unsigned int polygony=y;
                    //start detour the new tile block
                    pixelizedPolygon << QPointF(polygonx,polygony);
                    Direction direction=Direction_right;
                    while(!stopIt)
                        switch(direction)
                        {
                            case Direction_right:
                            {
                                polygonx++;
                                QPointF p(polygonx,polygony);
                                if(pixelizedPolygon.first()==p)//then polygon finished
                                {
                                    stopIt=true;
                                    break;
                                }
                                if(polygony>0 && polygonx<(w-1))//check if can go top
                                {
                                    PolygonZoneIndex &exploredTileIndex=polygonZoneMap.tileToPolygonZoneIndex[polygonx+(polygony-1)*w];
                                    if(exploredTileIndex.index==tileIndex.index)//direction change
                                    {
                                        pixelizedPolygon << QPointF(polygonx,polygony);
                                        direction=Direction_top;
                                        break;
                                    }
                                }
                                if(polygonx<(w-1) && polygony<(h-1))//check if can go right
                                {
                                    PolygonZoneIndex &exploredTileIndex=polygonZoneMap.tileToPolygonZoneIndex[polygonx+polygony*w];
                                    if(exploredTileIndex.index==tileIndex.index)//same direction
                                        break;
                                }
                                if(polygony<(h-1))//check if can go bottom
                                {
                                    pixelizedPolygon << QPointF(polygonx,polygony);
                                    direction=Direction_bottom;
                                }
                                else
                                    abort();
                            }
                            break;
                            case Direction_bottom:
                            {
                                polygony++;
                                QPointF p(polygonx,polygony);
                                if(pixelizedPolygon.first()==p)//then polygon finished
                                {
                                    stopIt=true;
                                    break;
                                }
                                if(polygonx<(w-1) && polygony<(h-1))//check if can go right
                                {
                                    PolygonZoneIndex &exploredTileIndex=polygonZoneMap.tileToPolygonZoneIndex[polygonx+polygony*w];
                                    if(exploredTileIndex.index==tileIndex.index)//direction change
                                    {
                                        pixelizedPolygon << QPointF(polygonx,polygony);
                                        direction=Direction_right;
                                    }
                                }
                                if(polygony<(h-1) && polygonx>0)//check if can go bottom
                                {
                                    PolygonZoneIndex &exploredTileIndex=polygonZoneMap.tileToPolygonZoneIndex[(polygonx-1)+polygony*w];
                                    if(exploredTileIndex.index==tileIndex.index)//same direction
                                        break;
                                }
                                if(polygonx>0)//check if can go left
                                {
                                    pixelizedPolygon << QPointF(polygonx,polygony);
                                    direction=Direction_left;
                                }
                                else
                                    abort();
                            }
                            break;
                            case Direction_left:
                            {
                                polygonx--;
                                QPointF p(polygonx,polygony);
                                if(pixelizedPolygon.first()==p)//then polygon finished
                                {
                                    stopIt=true;
                                    break;
                                }
                                if(polygony<(h-1) && polygonx>0)//check if can go bottom
                                {
                                    PolygonZoneIndex &exploredTileIndex=polygonZoneMap.tileToPolygonZoneIndex[(polygonx-1)+polygony*w];
                                    if(exploredTileIndex.index==tileIndex.index)//direction change
                                    {
                                        pixelizedPolygon << QPointF(polygonx,polygony);
                                        direction=Direction_bottom;
                                    }
                                }
                                if(polygonx>0 && polygony>0)//check if can go left
                                {
                                    PolygonZoneIndex &exploredTileIndex=polygonZoneMap.tileToPolygonZoneIndex[(polygonx-1)+(polygony-1)*w];
                                    if(exploredTileIndex.index==tileIndex.index)//same direction
                                        break;
                                }
                                if(polygony>0)//check if can go top
                                {
                                    pixelizedPolygon << QPointF(polygonx,polygony);
                                    direction=Direction_top;
                                }
                                else
                                    abort();
                            }
                            break;
                            case Direction_top:
                            {
                                polygony--;
                                QPointF p(polygonx,polygony);
                                if(pixelizedPolygon.first()==p)//then polygon finished
                                {
                                    stopIt=true;
                                    break;
                                }
                                if(polygonx>0 && polygony>0)//check if can go left
                                {
                                    PolygonZoneIndex &exploredTileIndex=polygonZoneMap.tileToPolygonZoneIndex[(polygonx-1)+(polygony-1)*w];
                                    if(exploredTileIndex.index==tileIndex.index)//direction change
                                    {
                                        pixelizedPolygon << QPointF(polygonx,polygony);
                                        direction=Direction_left;
                                    }
                                }
                                if(polygony>0 && polygony<(h-1))//check if can go top
                                {
                                    PolygonZoneIndex &exploredTileIndex=polygonZoneMap.tileToPolygonZoneIndex[polygonx+(polygony-1)*w];
                                    if(exploredTileIndex.index==tileIndex.index)//same direction
                                        break;
                                }
                                if(polygonx<(w-1))//check if can go right
                                {
                                    pixelizedPolygon << QPointF(polygonx,polygony);
                                    direction=Direction_right;
                                }
                                else
                                    abort();
                            }
                            break;
                        }
                }
                x++;
            }
            y++;
        }
    }

    return polygonZoneMap;
}
