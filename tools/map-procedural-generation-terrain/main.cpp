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

#include <boost/polygon/voronoi.hpp>
using boost::polygon::voronoi_builder;
using boost::polygon::voronoi_diagram;

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

struct Point {
  int a;
  int b;
  Point (int x, int y) : a(x), b(y) {}
};

struct Segment {
  Point p0;
  Point p1;
  Segment (int x1, int y1, int x2, int y2) : p0(x1, y1), p1(x2, y2) {}
};

struct Terrain
{
    QString tsx;
    uint32_t tile;
    Tiled::Tileset *tileset;
    uint32_t baseX,baseY;
};

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
/*    const unsigned int resize_TerrainMap=settings.value("resize_TerrainMap").toUInt(&ok);
    if(ok==false)
    {
        std::cerr << "resize_TerrainMap not number into the config file" << std::endl;
        abort();
    }
    const unsigned int resize_TerrainHeat=settings.value("resize_TerrainHeat").toUInt(&ok);
    if(ok==false)
    {
        std::cerr << "resize_TerrainHeat not number into the config file" << std::endl;
        abort();
    }
    const float scale_TerrainMap=settings.value("scale_TerrainMap").toFloat(&ok);
    if(ok==false)
    {
        std::cerr << "scale_TerrainMap not number into the config file" << std::endl;
        abort();
    }
    const float scale_TerrainHeat=settings.value("scale_TerrainHeat").toFloat(&ok);
    if(ok==false)
    {
        std::cerr << "scale_TerrainHeat not number into the config file" << std::endl;
        abort();
    }*/
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

    //fill
    /*Simplex simplexWalkable,simplexHeat;
    simplexWalkable.Shuffle(10+seed%10000);
    simplexHeat.Shuffle(5+seed%10000);*/

    /*uint8_t y_map=0;
    while(y_map<mapXCount)
    {
        uint8_t x_map=0;
        while(x_map<mapYCount)
        {
            Tiled::Map tiledMap(Tiled::Map::Orientation::Orthogonal,mapWidth,mapHeight,16,16);
            QHash<QString,Tiled::Tileset *> cachedTileset;
            loadTileset(water,cachedTileset,tiledMap);
            loadTileset(grass,cachedTileset,tiledMap);
            loadTileset(montain,cachedTileset,tiledMap);
            Tiled::Tileset *tilesetDebug=readTileset("mapgen.tsx",&tiledMap);

            //layer
            Tiled::TileLayer *layerWalkable=new Tiled::TileLayer("Walkable",0,0,tiledMap.width(),tiledMap.height());
            tiledMap.addLayer(layerWalkable);
            Tiled::TileLayer *layerHeat=new Tiled::TileLayer("Heat",0,0,tiledMap.width(),tiledMap.height());
            tiledMap.addLayer(layerHeat);
            layerHeat->setVisible(false);

            uint32_t y=0;
            while(y<(uint32_t)tiledMap.height())
            {
                uint32_t x=0;
                while(x<(uint32_t)tiledMap.width())
                {
                    // simplexWalkable.Get() returned noise value is between -1 and 1
                    // Set noise scale with the last parameter
                    unsigned int intWalkable=0;
                    {
                        float valueWalkable = simplexWalkable.Get(
                        {(float)(x/resize_TerrainMap+x_map*tiledMap.width()/resize_TerrainMap),
                         (float)(y/resize_TerrainMap+y_map*tiledMap.height()/resize_TerrainMap),
                         1.f,2.f},
                                    scale_TerrainMap*resize_TerrainMap);
                        intWalkable=floatToLevel(valueWalkable);
                    }
                    float valueHeat = simplexHeat.Get(
                    {(float)(x/resize_TerrainHeat+x_map*tiledMap.width()/resize_TerrainHeat),
                     (float)(y/resize_TerrainHeat+y_map*tiledMap.height()/resize_TerrainHeat),
                     1.f,2.f},
                                scale_TerrainHeat*resize_TerrainHeat);
                    {
                        Tiled::Cell cell;//=layerWalkable.cellAt(x,y)

                        if(intWalkable==0)
                            cell.tile=water.tileset->tileAt(water.tile);
                        else if(intWalkable==1)
                            cell.tile=grass.tileset->tileAt(grass.tile);
                        else
                            cell.tile=montain.tileset->tileAt(montain.tile);

                        layerWalkable->setCell(x,y,cell);
                    }
                    {
                        Tiled::Cell cell;//=layerHeat.cellAt(x,y)

                        valueHeat+=1;
                        valueHeat/=2;
                        valueHeat*=6;
                        valueHeat-=0.01f;
                        //valueHeat range: 0.0 at 5.99
                        cell.tile=tilesetDebug->tileAt((int)valueHeat);

                        layerHeat->setCell(x,y,cell);
                    }
                    ++x;
                }
                ++y;
            }

            Tiled::MapWriter maprwriter;
            maprwriter.writeMap(&tiledMap,QCoreApplication::applicationDirPath()+"/dest/map/"+QString::number(x_map)+"."+QString::number(y_map)+".tmx");
            x_map++;
        }
        y_map++;
    }*/

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

        PoissonGenerator::DefaultPRNG PRNG;
        const std::vector<PoissonGenerator::sPoint> Points = PoissonGenerator::GeneratePoissonPoints(30,PRNG,30,false);

        std::vector<Point> points;
        std::vector<Segment> segments;
        for(auto i=Points.begin();i!=Points.end();i++)
        {
            float x=i->x,y=i->y;
            x=x*tiledMap.width()+((double)rand()/(double)RAND_MAX)*1.5-0.75;
            if(x>tiledMap.width())
                x=tiledMap.width();
            if(x<0)
                x=0;
            y=y*tiledMap.height()+((double)rand()/(double)RAND_MAX)*1.5-0.75;
            if(y>tiledMap.height())
                y=tiledMap.height();
            if(y<0)
                y=0;

            points.push_back(Point(x*16,y*16));

            Tiled::MapObject *object = new Tiled::MapObject("P","",QPointF(i->x,i->y), QSizeF(0.0,0.0));
            object->setPolygon(QPolygonF(QVector<QPointF>()<<QPointF(0.05,0.0)<<QPointF(0.05,0.0)<<QPointF(0.05,0.05)<<QPointF(0.0,0.05)));
            object->setShape(Tiled::MapObject::Polygon);
            layerPoint->addObject(object);
        }

        voronoi_diagram<double> vd;
        boost::polygon::construct_voronoi(points.begin(), points.end(), &vd);

        for (voronoi_diagram<double>::const_cell_iterator it = vd.cells().begin();
             it != vd.cells().end(); ++it) {
          const voronoi_diagram<double>::cell_type &cell = *it;
          const voronoi_diagram<double>::edge_type *edge = cell.incident_edge();
          Tiled::MapObject *object = new Tiled::MapObject("C","",QPointF(0,0), QSizeF(0.0,0.0));
          QVector<QPointF> pointsFloat;
          // This is convenient way to iterate edges around Voronoi cell.
          do {
            pointsFloat<<QPointF(edge->,point.y);
            edge = edge->next();
          } while (edge != cell.incident_edge());
          object->setPolygon(QPolygonF(pointsFloat));
          object->setShape(Tiled::MapObject::Polygon);
          layerZone->addObject(object);
        }

        /*unsigned int indexSites=0;
        while(indexSites<sites.size())
        {
            const Point2 &site=sites.at(indexSites);

            indexSites++;
        }

        unsigned int indexCells=0;
        while(indexCells<diagram->cells.size())
        {
            const Cell &cell=*diagram->cells.at(indexCells);

            Tiled::MapObject *object = new Tiled::MapObject("C"+QString::number(indexCells),"",QPointF(0,0), QSizeF(0.0,0.0));
            QVector<QPointF> points;

            std::vector<HalfEdge*> halfEdges=cell.halfEdges;
            unsigned int halfEdgesIndex=0;
            while(halfEdgesIndex<halfEdges.size())
            {
                HalfEdge* halfEdge=halfEdges.at(halfEdgesIndex);
                const Point2 point=*halfEdge->startPoint();
                points<<QPointF(point.x,point.y);
                halfEdgesIndex++;
            }

            object->setPolygon(QPolygonF(points));
            object->setShape(Tiled::MapObject::Polygon);
            layerZone->addObject(object);

            indexCells++;
        }*/

        delete diagram;

/*        uint32_t y=0;
        while(y<(uint32_t)tiledMap.height())
        {
            uint32_t x=0;
            while(x<(uint32_t)tiledMap.width())
            {
                // simplexWalkable.Get() returned noise value is between -1 and 1
                // Set noise scale with the last parameter
                unsigned int intWalkable=0;
                {
                    float valueWalkable = simplexWalkable.Get(
                    {(float)(x/resize_TerrainMap),
                     (float)(y/resize_TerrainMap),
                     1.f,2.f},
                                scale_TerrainMap*resize_TerrainMap);
                    intWalkable=floatToLevel(valueWalkable);
                }


                float valueHeat = simplexHeat.Get(
                {(float)(x/resize_TerrainHeat),
                 (float)(y/resize_TerrainHeat),
                 1.f,2.f},
                            scale_TerrainHeat*resize_TerrainHeat);
                {
                    Tiled::Cell cell;//=layerWalkable.cellAt(x,y)
                    if(intWalkable==0)
                        cell.tile=water.tileset->tileAt(water.tile);
                    else if(intWalkable==1)
                        cell.tile=grass.tileset->tileAt(grass.tile);
                    else
                        cell.tile=montain.tileset->tileAt(montain.tile);

                    layerWalkable->setCell(x,y,cell);
                }
                {
                    Tiled::Cell cell;//=layerHeat.cellAt(x,y)

                    valueHeat+=1;
                    valueHeat/=2;
                    valueHeat*=6;
                    valueHeat-=0.01f;
                    //valueHeat range: 0.0 at 5.99
                    cell.tile=tilesetDebug->tileAt((int)valueHeat);

                    layerHeat->setCell(x,y,cell);
                }
                ++x;
            }
            ++y;
        }*/

        Tiled::MapWriter maprwriter;
        maprwriter.writeMap(&tiledMap,QCoreApplication::applicationDirPath()+"/dest/map/all.tmx");
    }

    return 0;
}
