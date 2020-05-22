#include "VoronioForTiledMapTmx.h"

#include <vector>
#include <iostream>
#include <cmath>
#include <random>
#include <QList>
#include <QPointF>

#include "PoissonGenerator.h"
#include "znoise/headers/Simplex.hpp"

#include <boost/polygon/segment_data.hpp>
#include <boost/polygon/voronoi.hpp>

using boost::polygon::voronoi_builder;
using boost::polygon::voronoi_diagram;

typedef boost::polygon::segment_data<int> Segment;

const int VoronioForTiledMapTmx::SCALE = 1000;
VoronioForTiledMapTmx::PolygonZoneMap VoronioForTiledMapTmx::voronoiMap;
VoronioForTiledMapTmx::PolygonZoneMap VoronioForTiledMapTmx::voronoiMap1px;

double VoronioForTiledMapTmx::area(const QPolygonF &p) {
    double a = 0.0;
    for (int i = 0; i < p.size() - 1; ++i)
        a += p[i].x() * p[i+1].y() - p[i+1].x() * p[i].y();
    return 0.5 * a;
}

Grid VoronioForTiledMapTmx::generateGrid(const unsigned int w, const unsigned int h, const unsigned int seed, const int num,const int scale) {
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
        g.emplace_back(scale*x, scale*y);
    }

    return g;
}

VoronioForTiledMapTmx::PolygonZoneMap VoronioForTiledMapTmx::computeVoronoi(const Grid &g, const unsigned int w, const unsigned int h,const unsigned int tileStep) {
    QPolygonF rect(QRectF(0.0, 0.0, w, h));

    if(w%tileStep!=0)
        abort();
    if(h%tileStep!=0)
        abort();
    boost::polygon::voronoi_diagram<double> vd;
    boost::polygon::construct_voronoi(g.begin(), g.end(), &vd);

    PolygonZoneMap polygonZoneMap;
    polygonZoneMap.tileToPolygonZoneIndex=(PolygonZoneIndex *)malloc(sizeof(PolygonZoneIndex)*(w*h+1));
    memset(polygonZoneMap.tileToPolygonZoneIndex,0,sizeof(PolygonZoneIndex)*w*h);
    polygonZoneMap.zones.resize(g.size());
    double maxArea=((double)pow(tileStep,2))*1.01;

    for (auto &c : vd.cells()) {
        unsigned int minX=9999999,minY=9999999,maxX=0,maxY=0;
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
                    unsigned int tempminXInt=floor(x),tempminYInt=floor(y),tempmaxXInt=ceil(x),tempmaxYInt=ceil(y);
                    if(maxX>w)
                        abort();
                    if(maxY>h)
                        abort();
                    tempminXInt-=tempminXInt%tileStep;
                    tempminYInt-=tempminYInt%tileStep;
                    if(tempmaxXInt%tileStep!=0)
                        tempmaxXInt+=tileStep-tempmaxXInt%tileStep;
                    if(tempmaxYInt%tileStep!=0)
                        tempmaxYInt+=tileStep-tempmaxYInt%tileStep;
                    if(tempminXInt<minX)
                        minX=tempminXInt;
                    if(tempminYInt<minY)
                        minY=tempminYInt;
                    if(tempmaxXInt>maxX)
                        maxX=tempmaxXInt;
                    if(tempmaxYInt>maxY)
                        maxY=tempmaxYInt;
                    if(maxX>w)
                    {
                        unsigned int tempmaxXIntdebug1=ceil(x);
                        unsigned int tempmaxXIntdebug3=tempmaxXIntdebug1;
                        if(tempmaxXInt%tileStep!=0)
                            tempmaxXIntdebug3+=tileStep-tempmaxXIntdebug3%tileStep;
                        abort();
                    }
                    if(maxY>h)
                    {
                        unsigned int tempmaxYIntdebug1=ceil(y);
                        unsigned int tempmaxYIntdebug3=tempmaxYIntdebug1;
                        if(tempmaxYInt%tileStep!=0)
                            tempmaxYIntdebug3+=tileStep-tempmaxYIntdebug3%tileStep;
                        abort();
                    }
                    index++;
                }
            }
            unsigned int y=minY;
            while(y<maxY)
            {
                unsigned int x=minX;
                while(x<maxX)
                {
                    const QPolygonF tileRect(QRectF(x,y,tileStep,tileStep));
                    const QPolygonF &tilePolygon=resultPolygon.intersected(tileRect);
                    const double &a=area(tilePolygon);
                    if(a>maxArea)
                        abort();
                    PolygonZoneIndex &tileIndex=polygonZoneMap.tileToPolygonZoneIndex[x+y*w];
                    if(a>tileIndex.area)
                    {
                        unsigned offsety=0;
                        while(offsety<tileStep)
                        {
                            unsigned offsetx=0;
                            while(offsetx<tileStep)
                            {
                                PolygonZoneIndex &tileIndex=polygonZoneMap.tileToPolygonZoneIndex[x+offsetx+(y+offsety)*w];
                                tileIndex.area=a;
                                tileIndex.index=final_index;
                                offsetx++;
                            }
                            offsety++;
                        }
                    }
                    x+=tileStep;
                }
                y+=tileStep;
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
    if(false)
    {
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
    }

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
                    //std::unordered_set<unsigned int> groupedZone; CAN'T GROUP HERE, no info about height/moisure
                    bool stopIt=false;
                    unsigned int polygonx=x;
                    unsigned int polygony=y;
                    //start detour the new tile block
                    pixelizedPolygon << QPointF(polygonx,polygony);
                    Direction direction=Direction_right;
                    while(!stopIt)
                    {
                        switch(direction)
                        {
                            case Direction_right:
                            {
                                polygonx++;
                                QPointF p(polygonx,polygony);
                                if(polygony>0 && polygonx<w)//check if can go top
                                {
                                    PolygonZoneIndex &exploredTileIndex=polygonZoneMap.tileToPolygonZoneIndex[polygonx+(polygony-1)*w];
                                    if(exploredTileIndex.index==tileIndex.index)//direction change
                                    {
                                        if(pixelizedPolygon.last()==QPointF(polygonx,polygony))
                                            abort();
                                        pixelizedPolygon << QPointF(polygonx,polygony);
                                        direction=Direction_top;
                                        break;
                                    }
                                }
                                if(polygonx<w && polygony<h)//check if can go right
                                {
                                    PolygonZoneIndex &exploredTileIndex=polygonZoneMap.tileToPolygonZoneIndex[polygonx+polygony*w];
                                    if(exploredTileIndex.index==tileIndex.index)//same direction
                                        break;
                                }
                                if(polygony<h)//check if can go bottom
                                {
                                    if(pixelizedPolygon.last()==QPointF(polygonx,polygony))
                                        abort();
                                    pixelizedPolygon << QPointF(polygonx,polygony);
                                    direction=Direction_bottom;
                                    break;
                                }
                                else
                                    abort();
                            }
                            break;
                            case Direction_bottom:
                            {
                                polygony++;
                                if(polygonx<w && polygony<h)//check if can go right
                                {
                                    PolygonZoneIndex &exploredTileIndex=polygonZoneMap.tileToPolygonZoneIndex[polygonx+polygony*w];
                                    if(exploredTileIndex.index==tileIndex.index)//direction change
                                    {
                                        if(pixelizedPolygon.last()==QPointF(polygonx,polygony))
                                            abort();
                                        pixelizedPolygon << QPointF(polygonx,polygony);
                                        direction=Direction_right;
                                        break;
                                    }
                                }
                                if(polygony<h && polygonx>0)//check if can go bottom
                                {
                                    PolygonZoneIndex &exploredTileIndex=polygonZoneMap.tileToPolygonZoneIndex[(polygonx-1)+polygony*w];
                                    if(exploredTileIndex.index==tileIndex.index)//same direction
                                        break;
                                }
                                if(polygonx>0)//check if can go left
                                {
                                    if(pixelizedPolygon.last()==QPointF(polygonx,polygony))
                                        abort();
                                    pixelizedPolygon << QPointF(polygonx,polygony);
                                    direction=Direction_left;
                                    break;
                                }
                                else
                                    abort();
                            }
                            break;
                            case Direction_left:
                            {
                                polygonx--;
                                if(polygony<h && polygonx>0)//check if can go bottom
                                {
                                    PolygonZoneIndex &exploredTileIndex=polygonZoneMap.tileToPolygonZoneIndex[(polygonx-1)+polygony*w];
                                    if(exploredTileIndex.index==tileIndex.index)//direction change
                                    {
                                        if(pixelizedPolygon.last()==QPointF(polygonx,polygony))
                                            abort();
                                        pixelizedPolygon << QPointF(polygonx,polygony);
                                        direction=Direction_bottom;
                                        break;
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
                                    if(pixelizedPolygon.last()==QPointF(polygonx,polygony))
                                        abort();
                                    pixelizedPolygon << QPointF(polygonx,polygony);
                                    direction=Direction_top;
                                    break;
                                }
                                else
                                    abort();
                            }
                            break;
                            case Direction_top:
                            {
                                polygony--;
                                if(polygonx>0 && polygony>0)//check if can go left
                                {
                                    PolygonZoneIndex &exploredTileIndex=polygonZoneMap.tileToPolygonZoneIndex[(polygonx-1)+(polygony-1)*w];
                                    if(exploredTileIndex.index==tileIndex.index)//direction change
                                    {
                                        if(pixelizedPolygon.last()==QPointF(polygonx,polygony))
                                            abort();
                                        pixelizedPolygon << QPointF(polygonx,polygony);
                                        direction=Direction_left;
                                        break;
                                    }
                                }
                                if(polygony>0 && polygonx<w)//check if can go top
                                {
                                    PolygonZoneIndex &exploredTileIndex=polygonZoneMap.tileToPolygonZoneIndex[polygonx+(polygony-1)*w];
                                    if(exploredTileIndex.index==tileIndex.index)//same direction
                                        break;
                                }
                                if(polygonx<w)//check if can go right
                                {
                                    if(pixelizedPolygon.last()==QPointF(polygonx,polygony))
                                        abort();
                                    pixelizedPolygon << QPointF(polygonx,polygony);
                                    direction=Direction_right;
                                    break;
                                }
                                else
                                    abort();
                            }
                            break;
                        }
                        {
                            QPointF p(polygonx,polygony);
                            if(pixelizedPolygon.first()==p)//then polygon finished
                            {
                                stopIt=true;
                                break;
                            }
                            if((unsigned int)pixelizedPolygon.size()>(zone.points.size()*4))
                            {
                                {
                                    unsigned int y=0;
                                    while(y<h)
                                    {
                                        std::cout << y << ": ";
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
                                }

                                std::cerr << "Too many border for the zone " << tileIndex.index << " at " << x << "," << y << ":" << std::endl;
                                const QList<QPointF> &points=pixelizedPolygon.toList();
                                unsigned int index=0;
                                while(index<(unsigned int)points.size())
                                {
                                    const QPointF &point=points.at(index);
                                    std::cerr << point.x() << "," << point.y() << " ";
                                    index++;
                                }
                                std::cerr << std::endl;
                                abort();
                            }
                        }
                    }
                }
                x+=tileStep;
            }
            y+=tileStep;
        }
    }

    return polygonZoneMap;
}
