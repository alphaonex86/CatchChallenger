#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QSettings>

#include <vector>
#include <iostream>
#include <fstream>
#include <memory.h>

#include "../../client/tiled/tiled_mapobject.h"
#include "../../client/tiled/tiled_objectgroup.h"
#include "../../client/base/render/MapVisualiserThread.h"

#include "../../general/base/GeneralStructures.h"
#include "../../general/base/CommonMap.h"
#include "../../client/base/Map_client.h"
#include "../../general/base/Map_loader.h"

#include "../../client/tiled/tiled_isometricrenderer.h"
#include "../../client/tiled/tiled_map.h"
#include "../../client/tiled/tiled_mapobject.h"
#include "../../client/tiled/tiled_mapreader.h"
#include "../../client/tiled/tiled_mapwriter.h"
#include "../../client/tiled/tiled_objectgroup.h"
#include "../../client/tiled/tiled_orthogonalrenderer.h"
#include "../../client/tiled/tiled_tilelayer.h"
#include "../../client/tiled/tiled_tileset.h"

#include "PoissonGenerator.h"

#include <boost/polygon/point_data.hpp>
#include <boost/polygon/segment_data.hpp>
#include <boost/polygon/voronoi.hpp>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/segment.hpp>
#include <boost/geometry/geometries/geometries.hpp>

using boost::polygon::voronoi_builder;
using boost::polygon::voronoi_diagram;

typedef boost::polygon::point_data<int> Point;
typedef boost::polygon::segment_data<int> Segment;
typedef std::vector<Point> Grid;

/*To do:
Polygons
Graph Representation
Islands
Elevation
Rivers
Moisture
Biomes
Noisy Edges
http://www-cs-students.stanford.edu/~amitp/game-programming/polygon-map-generation/*/

static const int tile_size = 16;

struct Terrain
{
    QString tsx;
    uint32_t tile;
    Tiled::Tileset *tileset;
    uint32_t baseX,baseY;
};

static Grid generateGrid(const unsigned int &w, const unsigned int &h, const unsigned int &seed) {
    Grid g;
    g.reserve(size_t(w) * size_t(h));

    PoissonGenerator::DefaultPRNG PRNG(seed);
    const std::vector<PoissonGenerator::sPoint> Points = PoissonGenerator::GeneratePoissonPoints(30,PRNG,30,false);
    unsigned int index=0;
    for(auto i=Points.begin();i!=Points.end();i++)
    {
        float x=i->x,y=i->y;
        x=x*w+((double)rand()/(double)RAND_MAX)*1.5-0.75;
        if(x>w)
            x=w;
        if(x<0)
            x=0;
        y=y*h+((double)rand()/(double)RAND_MAX)*1.5-0.75;
        if(y>h)
            y=h;
        if(y<0)
            y=0;
        g.emplace_back(x,y);
        index++;
    }

    /*
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(-rnd/2, rnd/2);

    Grid g;
    g.reserve(size_t(w) * size_t(h));

    for (int x = 0; x < w; ++x)
        for (int y = 0; y < h; ++y)
            g.emplace_back(tile_size*x + dis(gen) + tile_size/2,
                           tile_size*y + dis(gen) + tile_size/2);

    */
    return g;
}

static std::vector<QPolygonF> computeVoronoi(const Grid &g, int w, int h) {
    QPolygonF rect;
    rect.append({0.0, 0.0});
    rect.append({0.0, tile_size * double(h)});
    rect.append({tile_size * double(w), tile_size * double(h)});
    rect.append({tile_size * double(w), 0.0});

    // create and add segments to ensure that the diagram spans the whole rect
    std::vector<Segment> s;

    for (int x = -1; x <= w; ++x) {
        Point p1(tile_size*(x+0), -2*tile_size);
        Point p2(tile_size*(x+1), -2*tile_size);
        s.emplace_back(p1, p2);

        p1 = Point(tile_size*(x+0), (h+2) * tile_size);
        p2 = Point(tile_size*(x+1), (h+2) * tile_size);
        s.emplace_back(p1, p2);
    }

    for (int y = -1; y <= h; ++y) {
        Point p1(-2*tile_size, tile_size*(y+0));
        Point p2(-2*tile_size, tile_size*(y+1));
        s.emplace_back(p1, p2);

        p1 = Point((w+2) * tile_size, tile_size*(y+0));
        p2 = Point((w+2) * tile_size, tile_size*(y+1));
        s.emplace_back(p1, p2);
    }

    boost::polygon::voronoi_diagram<double> vd;
    boost::polygon::construct_voronoi(g.begin(), g.end(), s.begin(), s.end(), &vd);

    std::vector<QPolygonF> cells;

    for (auto &c : vd.cells()) {
        if (!c.contains_point())
            continue;

        auto e = c.incident_edge();
        QPolygonF poly;
        do {
            if (e->is_primary()) {
                if (e->is_finite())
                    poly.append(QPointF(e->vertex0()->x(), e->vertex0()->y()));
                else {
                   if (e->vertex0())
                      poly.append(QPointF(e->vertex0()->x(), e->vertex0()->y()));
                   else if (e->vertex1())
                      poly.append(QPointF(e->vertex1()->x(), e->vertex1()->y()));
                }
            }
            e = e->next();
        } while (e != c.incident_edge());

        cells.push_back(poly.intersected(rect));
    }

    return cells;
}

unsigned int floatToLevel(const float f)
{
    if(f<-0.1)
        return 0;
    else if(f<0.3)
        return 1;
    else if(f<0.6)
        return 2;
    else
        return 3;
}

Tiled::Tileset *readTileset(const QString &tsx,Tiled::Map *tiledMap)
{
    QDir mapDir(QCoreApplication::applicationDirPath()+"/dest/map/");

    Tiled::MapReader reader;
    Tiled::Tileset *tilesetBase=reader.readTileset(QCoreApplication::applicationDirPath()+"/dest/"+tsx);
    if(tilesetBase==NULL)
    {
        std::cerr << "File not found: " << QCoreApplication::applicationDirPath().toStdString() << "/" << tsx.toStdString() << std::endl;
        abort();
    }
    if(tilesetBase->tileWidth()!=tiledMap->tileWidth())
    {
        std::cerr << "tile Width not match with map tile Width" << std::endl;
        abort();
    }
    if(tilesetBase->tileHeight()!=tiledMap->tileHeight())
    {
        std::cerr << "tile height not match with map tile height" << std::endl;
        abort();
    }
    tiledMap->addTileset(tilesetBase);
    tilesetBase->setFileName(mapDir.relativeFilePath(tilesetBase->fileName()));
    return tilesetBase;
}

Tiled::Tileset *readTileset(const uint32_t &tile,const QString &tsx,Tiled::Map *tiledMap)
{
    Tiled::Tileset *tilesetBase=readTileset(tsx,tiledMap);
    if(tilesetBase!=NULL)
    {
        if(tile>=(uint32_t)tilesetBase->tileCount())
        {
            std::cerr << "tile: " << tile << " too high number for: " << tsx.toStdString() << std::endl;
            abort();
        }
    }
    return tilesetBase;
}

void loadTileset(Terrain &terrain,QHash<QString,Tiled::Tileset *> &cachedTileset,Tiled::Map &tiledMap)
{
    if(cachedTileset.contains(terrain.tsx))
        terrain.tileset=cachedTileset.value(terrain.tsx);
    else
    {
        Tiled::Tileset *tilesetBase=readTileset(terrain.tile,terrain.tsx,&tiledMap);
        terrain.tileset=tilesetBase;
        cachedTileset[terrain.tsx]=tilesetBase;
    }
}

int main(int argc, char *argv[])
{
    // Avoid performance issues with X11 engine when rendering objects
#ifdef Q_WS_X11
    QApplication::setGraphicsSystem(QStringLiteral("raster"));
#endif

    QApplication a(argc, argv);

    a.setOrganizationDomain(QStringLiteral("catchchallenger"));
    a.setApplicationName(QStringLiteral("map-procedural-generation-terrain"));
    a.setApplicationVersion(QStringLiteral("1.0"));

    QSettings settings(QCoreApplication::applicationDirPath()+"/settings.xml",QSettings::NativeFormat);
    settings.beginGroup("terrain");
    /*do the settings table:
        altitude
        heat
        Give type

        For each type:
        tsx
        tile*/
        //do tile to zone converter
        settings.beginGroup("water");
            if(!settings.contains("tsx"))
                settings.setValue("tsx","animation.tsx");
            if(!settings.contains("tile"))
                settings.setValue("tile",0);
        settings.endGroup();
        settings.beginGroup("grass");
            if(!settings.contains("tsx"))
                settings.setValue("tsx","terra.tsx");
            if(!settings.contains("tile"))
                settings.setValue("tile",0);
        settings.endGroup();
        settings.beginGroup("montain");
            if(!settings.contains("tsx"))
                settings.setValue("tsx","terra.tsx");
            if(!settings.contains("tile"))
                settings.setValue("tile",353);
        settings.endGroup();
        settings.beginGroup("transition");
            settings.beginGroup("waterborder");
                if(!settings.contains("from_type"))
                    settings.setValue("from_type","water");
                if(!settings.contains("to_type"))
                    settings.setValue("to_type","grass,grass2");
                if(!settings.contains("transition_tsx"))
                    settings.setValue("transition_tsx","animation.tsx");
                if(!settings.contains("transition_tile"))
                    settings.setValue("transition_tile","184,185,186,218,250,249,248,216,281,282,314,313");
                if(!settings.contains("collision"))
                    settings.setValue("collision",false);
                if(!settings.contains("insamelayer"))
                    settings.setValue("insamelayer",false);
            settings.endGroup();
        settings.endGroup();
    settings.endGroup();
    if(!settings.contains("resize_TerrainMap"))
        settings.setValue("resize_TerrainMap",4);
    if(!settings.contains("resize_TerrainHeat"))
        settings.setValue("resize_TerrainHeat",4);
    if(!settings.contains("scale_TerrainMap"))
        settings.setValue("scale_TerrainMap",(double)0.01f);
    if(!settings.contains("scale_TerrainHeat"))
        settings.setValue("scale_TerrainHeat",(double)0.01f);
    if(!settings.contains("mapWidth"))
        settings.setValue("mapWidth",44);
    if(!settings.contains("mapHeight"))
        settings.setValue("mapHeight",44);
    if(!settings.contains("mapXCount"))
        settings.setValue("mapXCount",3);
    if(!settings.contains("mapYCount"))
        settings.setValue("mapYCount",3);
    if(!settings.contains("seed"))
        settings.setValue("seed",0);

    settings.sync();

    bool ok;
    Terrain grass,water,montain;
    settings.beginGroup("terrain");
        settings.beginGroup("grass");
            grass.tsx=settings.value("tsx").toString();
            grass.tile=settings.value("tile").toUInt(&ok);
            if(ok==false)
            {
                std::cerr << "tile not number into the config file" << std::endl;
                abort();
            }
            grass.baseX=1;
            grass.baseY=1;
        settings.endGroup();
        settings.beginGroup("water");
            water.tsx=settings.value("tsx").toString();
            water.tile=settings.value("tile").toUInt(&ok);
            if(ok==false)
            {
                std::cerr << "tile not number into the config file" << std::endl;
                abort();
            }
            water.baseX=4;
            water.baseY=4;
        settings.endGroup();
        settings.beginGroup("montain");
            montain.tsx=settings.value("tsx").toString();
            montain.tile=settings.value("tile").toUInt(&ok);
            if(ok==false)
            {
                std::cerr << "tile not number into the config file" << std::endl;
                abort();
            }
            montain.baseX=8;
            montain.baseY=8;
        settings.endGroup();
    settings.endGroup();
    const unsigned int mapWidth=settings.value("mapWidth").toUInt(&ok);
    if(ok==false)
    {
        std::cerr << "mapWidth not number into the config file" << std::endl;
        abort();
    }
    const unsigned int mapHeight=settings.value("mapHeight").toUInt(&ok);
    if(ok==false)
    {
        std::cerr << "mapHeight not number into the config file" << std::endl;
        abort();
    }
    const unsigned int mapXCount=settings.value("mapXCount").toUInt(&ok);
    if(ok==false)
    {
        std::cerr << "mapXCount not number into the config file" << std::endl;
        abort();
    }
    const unsigned int mapYCount=settings.value("mapYCount").toUInt(&ok);
    if(ok==false)
    {
        std::cerr << "mapYCount not number into the config file" << std::endl;
        abort();
    }
    const unsigned int seed=settings.value("seed").toUInt(&ok);
    if(ok==false)
    {
        std::cerr << "seed not number into the config file" << std::endl;
        abort();
    }
    srand(seed);

    {
        Tiled::Map tiledMap(Tiled::Map::Orientation::Orthogonal,mapWidth*mapXCount,mapHeight*mapYCount,16,16);
        QHash<QString,Tiled::Tileset *> cachedTileset;
        loadTileset(water,cachedTileset,tiledMap);
        loadTileset(grass,cachedTileset,tiledMap);
        loadTileset(montain,cachedTileset,tiledMap);
        //Tiled::Tileset *tilesetDebug=readTileset("mapgen.tsx",&tiledMap);

        //layer
        Tiled::TileLayer *layerWalkable=new Tiled::TileLayer("Walkable",0,0,tiledMap.width(),tiledMap.height());
        tiledMap.addLayer(layerWalkable);
        Tiled::TileLayer *layerHeat=new Tiled::TileLayer("Heat",0,0,tiledMap.width(),tiledMap.height());
        tiledMap.addLayer(layerHeat);
        Tiled::ObjectGroup *layerZone=new Tiled::ObjectGroup("Zone",0,0,tiledMap.width(),tiledMap.height());
        tiledMap.addLayer(layerZone);
        Tiled::ObjectGroup *layerPoint=new Tiled::ObjectGroup("Point",0,0,tiledMap.width(),tiledMap.height());
        tiledMap.addLayer(layerPoint);
        layerHeat->setVisible(false);

        auto grid = generateGrid(tiledMap.width(),tiledMap.height(),seed);
        for (const auto &p : grid)
        {
            Tiled::MapObject *object = new Tiled::MapObject("P","",QPointF(
                                                                ((float)p.x()),
                                                                ((float)p.y())
                                                                ),
                                                            QSizeF(0.0,0.0));
            object->setPolygon(QPolygonF(QVector<QPointF>()<<QPointF(0.05,0.0)<<QPointF(0.05,0.0)<<QPointF(0.05,0.05)<<QPointF(0.0,0.05)));
            object->setShape(Tiled::MapObject::Polygon);
            layerPoint->addObject(object);
        }

        auto vd = computeVoronoi(grid,tiledMap.width(),tiledMap.height());
        for (const auto &poly : vd)
        {
            Tiled::MapObject *object = new Tiled::MapObject("C","",QPointF(0,0), QSizeF(0.0,0.0));
            object->setPolygon(poly);
            object->setShape(Tiled::MapObject::Polygon);
            layerZone->addObject(object);
        }

        Tiled::MapWriter maprwriter;
        maprwriter.writeMap(&tiledMap,QCoreApplication::applicationDirPath()+"/dest/map/all.tmx");
    }

    return 0;
}
