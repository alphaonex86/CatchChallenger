#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QSettings>

#include <vector>
#include <iostream>
#include <fstream>
#include <memory.h>
#include <cmath>
#include <random>
#include <limits>

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
#include "znoise/headers/Simplex.hpp"

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

static const int SCALE = 100;

struct Terrain
{
    QString tsx;
    uint32_t tile;
    Tiled::Tileset *tileset;
    uint32_t baseX,baseY;
};

static Grid generateGrid(const unsigned int w, const unsigned int h, const unsigned int seed, const int num) {
    std::mt19937 gen(seed);
    std::uniform_real_distribution<double> dis(-0.75, 0.75);

    Grid g;
    g.reserve(size_t(w) * size_t(h));

    PoissonGenerator::DefaultPRNG PRNG(seed);
    const auto points = PoissonGenerator::GeneratePoissonPoints(num, PRNG, num, false);
    //qDebug("Size: %ld", points.size());

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

static std::vector<QPolygonF> computeVoronoi(const Grid &g, double w, double h) {
    QPolygonF rect;
    rect.append({0.0, 0.0});
    rect.append({0.0, SCALE*h});
    rect.append({SCALE*w, SCALE*h});
    rect.append({SCALE*w, 0.0});

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
                    poly.append(QPointF(e->vertex0()->x(), e->vertex0()->y()));
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
                        poly.append(QPointF(e->vertex0()->x(), e->vertex0()->y()));
                    else
                        poly.append(QPointF(ox - dx * coef, oy - dy * coef));

                    if (e->vertex1())
                        poly.append(QPointF(e->vertex1()->x(), e->vertex1()->y()));
                    else
                        poly.append(QPointF(ox + dx * coef, oy + dy * coef));
                }
            }
            e = e->next();
        } while (e != c.incident_edge());

        poly = poly.intersected(rect);
        for (auto &p : poly)
            p /= SCALE;

        //cells.push_back(poly);
        cells[final_index]=poly;
    }

    return cells;
}

unsigned int floatToHigh(const float f)
{
    if(f<-0.1)
        return 0;
    else if(f<0.2)
        return 1;
    else if(f<0.4)
        return 2;
    else if(f<0.6)
        return 3;
    else
        return 4;
}

unsigned int floatToMoisure(const float f)
{
    if(f<-0.6)
        return 1;
    else if(f<-0.3)
        return 2;
    else if(f<0.0)
        return 3;
    else if(f<0.3)
        return 4;
    else if(f<0.6)
        return 5;
    else
        return 6;
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

        Tiled::ObjectGroup *layerZoneWater=new Tiled::ObjectGroup("Water",0,0,tiledMap.width(),tiledMap.height());
        layerZoneWater->setColor(QColor("#6273cc"));
        tiledMap.addLayer(layerZoneWater);

        Tiled::ObjectGroup *layerZoneSnow=new Tiled::ObjectGroup("Snow",0,0,tiledMap.width(),tiledMap.height());
        layerZoneSnow->setColor(QColor("#ffffff"));
        tiledMap.addLayer(layerZoneSnow);
        Tiled::ObjectGroup *layerZoneTundra=new Tiled::ObjectGroup("Tundra",0,0,tiledMap.width(),tiledMap.height());
        layerZoneTundra->setColor(QColor("#ddddbb"));
        tiledMap.addLayer(layerZoneTundra);
        Tiled::ObjectGroup *layerZoneBare=new Tiled::ObjectGroup("Bare",0,0,tiledMap.width(),tiledMap.height());
        layerZoneBare->setColor(QColor("#bbbbbb"));
        tiledMap.addLayer(layerZoneBare);
        Tiled::ObjectGroup *layerZoneScorched=new Tiled::ObjectGroup("Scorched",0,0,tiledMap.width(),tiledMap.height());
        layerZoneScorched->setColor(QColor("#999999"));
        tiledMap.addLayer(layerZoneScorched);
        Tiled::ObjectGroup *layerZoneTaiga=new Tiled::ObjectGroup("Taiga",0,0,tiledMap.width(),tiledMap.height());
        layerZoneTaiga->setColor(QColor("#ccd4bb"));
        tiledMap.addLayer(layerZoneTaiga);
        Tiled::ObjectGroup *layerZoneShrubland=new Tiled::ObjectGroup("Shrubland",0,0,tiledMap.width(),tiledMap.height());
        layerZoneShrubland->setColor(QColor("#c4ccbb"));
        tiledMap.addLayer(layerZoneShrubland);
        Tiled::ObjectGroup *layerZoneTemperateDesert=new Tiled::ObjectGroup("Temperate Desert",0,0,tiledMap.width(),tiledMap.height());
        layerZoneTemperateDesert->setColor(QColor("#e4e8ca"));
        tiledMap.addLayer(layerZoneTemperateDesert);
        Tiled::ObjectGroup *layerZoneTemperateRainForest=new Tiled::ObjectGroup("Temperate Rain Forest",0,0,tiledMap.width(),tiledMap.height());
        layerZoneTemperateRainForest->setColor(QColor("#a4c4a8"));
        tiledMap.addLayer(layerZoneTemperateRainForest);
        Tiled::ObjectGroup *layerZoneTemperateDeciduousForest=new Tiled::ObjectGroup("Temperate Deciduous Forest",0,0,tiledMap.width(),tiledMap.height());
        layerZoneTemperateDeciduousForest->setColor(QColor("#b4c9a9"));
        tiledMap.addLayer(layerZoneTemperateDeciduousForest);
        Tiled::ObjectGroup *layerZoneGrassland=new Tiled::ObjectGroup("Grassland",0,0,tiledMap.width(),tiledMap.height());
        layerZoneGrassland->setColor(QColor("#c4d4aa"));
        tiledMap.addLayer(layerZoneGrassland);
        Tiled::ObjectGroup *layerZoneTropicalRainForest=new Tiled::ObjectGroup("Tropical Rain Forest",0,0,tiledMap.width(),tiledMap.height());
        layerZoneTropicalRainForest->setColor(QColor("#9cbba9"));
        tiledMap.addLayer(layerZoneTropicalRainForest);
        Tiled::ObjectGroup *layerZoneTropicalSeasonalForest=new Tiled::ObjectGroup("Tropical Seasonal Forest",0,0,tiledMap.width(),tiledMap.height());
        layerZoneTropicalSeasonalForest->setColor(QColor("#a9cca4"));
        tiledMap.addLayer(layerZoneTropicalSeasonalForest);
        Tiled::ObjectGroup *layerZoneSubtropicalDesert=new Tiled::ObjectGroup("Subtropical Desert",0,0,tiledMap.width(),tiledMap.height());
        layerZoneSubtropicalDesert->setColor(QColor("#e9ddc7"));
        tiledMap.addLayer(layerZoneSubtropicalDesert);

        Tiled::ObjectGroup * arrayTerrain[4][6];
        //high 1
        arrayTerrain[0][0]=layerZoneSubtropicalDesert;//Moisture 1
        arrayTerrain[0][1]=layerZoneGrassland;
        arrayTerrain[0][2]=layerZoneTropicalSeasonalForest;
        arrayTerrain[0][3]=layerZoneTropicalSeasonalForest;
        arrayTerrain[0][4]=layerZoneTropicalRainForest;
        arrayTerrain[0][5]=layerZoneTropicalRainForest;
        //high 2
        arrayTerrain[1][0]=layerZoneTemperateDesert;//Moisture 1
        arrayTerrain[1][1]=layerZoneGrassland;
        arrayTerrain[1][2]=layerZoneGrassland;
        arrayTerrain[1][3]=layerZoneTemperateDeciduousForest;
        arrayTerrain[1][4]=layerZoneTemperateDeciduousForest;
        arrayTerrain[1][5]=layerZoneTemperateRainForest;
        //high 3
        arrayTerrain[2][0]=layerZoneTemperateDesert;//Moisture 1
        arrayTerrain[2][1]=layerZoneTemperateDesert;
        arrayTerrain[2][2]=layerZoneShrubland;
        arrayTerrain[2][3]=layerZoneShrubland;
        arrayTerrain[2][4]=layerZoneTaiga;
        arrayTerrain[2][5]=layerZoneTaiga;
        //high 4
        arrayTerrain[3][0]=layerZoneScorched;//Moisture 1
        arrayTerrain[3][1]=layerZoneBare;
        arrayTerrain[3][2]=layerZoneTundra;
        arrayTerrain[3][3]=layerZoneSnow;
        arrayTerrain[3][4]=layerZoneSnow;
        arrayTerrain[3][5]=layerZoneSnow;

        Tiled::ObjectGroup *layerPoint=new Tiled::ObjectGroup("Point",0,0,tiledMap.width(),tiledMap.height());
        tiledMap.addLayer(layerPoint);
        layerPoint->setVisible(false);

        auto grid = generateGrid(tiledMap.width(),tiledMap.height(),seed,1000);
        for (const auto &p : grid)
        {
            Tiled::MapObject *object = new Tiled::MapObject("P","",QPointF(
                                                                ((float)p.x()/SCALE),
                                                                ((float)p.y()/SCALE)
                                                                ),
                                                            QSizeF(0.0,0.0));
            object->setPolygon(QPolygonF(QVector<QPointF>()<<QPointF(0.05,0.0)<<QPointF(0.05,0.0)<<QPointF(0.05,0.05)<<QPointF(0.0,0.05)));
            object->setShape(Tiled::MapObject::Polygon);
            layerPoint->addObject(object);
        }

        Simplex heighmap(seed+500);
        Simplex moisuremap(seed+5200);
        auto vd = computeVoronoi(grid,tiledMap.width(),tiledMap.height());
        if(vd.size()!=grid.size())
            abort();
        unsigned int index=0;
        while(index<grid.size())
        {
            //const Point &centroid=grid.at(index);
            const QPolygonF &poly=vd.at(index);
            Tiled::MapObject *object = new Tiled::MapObject("C","",QPointF(0,0), QSizeF(0.0,0.0));
            object->setPolygon(poly);
            object->setShape(Tiled::MapObject::Polygon);
            const QList<QPointF> &edges=poly.toList();
            if(!edges.isEmpty())
            {
                const QPointF &edge=edges.first();
                const unsigned int &heigh=floatToHigh(heighmap.Get({(float)edge.x(),(float)edge.y()},0.05f));
                const unsigned int &moisure=floatToMoisure(moisuremap.Get({(float)edge.x(),(float)edge.y()},0.05f));
                if(heigh==0)
                    layerZoneWater->addObject(object);
                else
                    arrayTerrain[heigh-1][moisure-1]->addObject(object);
            }
            index++;
        }

        Tiled::MapWriter maprwriter;
        maprwriter.writeMap(&tiledMap,QCoreApplication::applicationDirPath()+"/dest/map/all.tmx");
    }

    return 0;
}
