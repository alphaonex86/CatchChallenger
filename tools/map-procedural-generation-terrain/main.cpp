#include <QApplication>
#include <QSettings>
#include <QTime>
#include <QFile>
#include <iostream>

#include "../../client/tiled/tiled_mapwriter.h"
#include "../../client/tiled/tiled_mapobject.h"
#include "../../client/tiled/tiled_objectgroup.h"

#include "znoise/headers/Simplex.hpp"
#include "VoronioForTiledMapTmx.h"
#include "LoadMap.h"
#include "TransitionTerrain.h"
#include "Settings.h"
#include "MapPlants.h"

/*To do: Tree/Grass, Rivers
http://www-cs-students.stanford.edu/~amitp/game-programming/polygon-map-generation/*/
/*template<typename T, size_t C, size_t R>  Matrix { std::array<T, C*R> };
 * operator[](int c, int r) { return arr[C*r+c]; }*/

int main(int argc, char *argv[])
{
    // Avoid performance issues with X11 engine when rendering objects
#ifdef Q_WS_X11
    QApplication::setGraphicsSystem(QStringLiteral("raster"));
#endif

    QApplication a(argc, argv);
    QTime t;

    a.setOrganizationDomain(QStringLiteral("catchchallenger"));
    a.setApplicationName(QStringLiteral("map-procedural-generation-terrain"));
    a.setApplicationVersion(QStringLiteral("1.0"));

    if(!QFile::exists(QCoreApplication::applicationDirPath()+"/settings.xml"))
        QFile::copy(":/settings.xml",QCoreApplication::applicationDirPath()+"/settings.xml");
    QSettings settings(QCoreApplication::applicationDirPath()+"/settings.xml",QSettings::NativeFormat);
    Settings::putDefaultSettings(settings);
    unsigned int mapWidth;
    unsigned int mapHeight;
    unsigned int mapXCount;
    unsigned int mapYCount;
    unsigned int seed;
    bool displayzone,dotransition,dovegetation;
    unsigned int tileStep;
    Settings::loadSettings(settings,mapWidth,mapHeight,mapXCount,mapYCount,seed,displayzone,dotransition,dovegetation,tileStep);
    srand(seed);

    {
        const unsigned int totalWidth=mapWidth*mapXCount;
        const unsigned int totalHeight=mapHeight*mapYCount;
        t.start();
        const Grid &grid = VoronioForTiledMapTmx::generateGrid(totalWidth,totalHeight,seed,1000);
        qDebug("generateGrid took %d ms", t.elapsed());

        /*t.start();Tiled::ObjectGroup *layerPoint=new Tiled::ObjectGroup("Point",0,0,tiledMap.width(),tiledMap.height());
        tiledMap.addLayer(layerPoint);layerPoint->setVisible(false);for (const auto &p : grid)
        {
            Tiled::MapObject *object = new Tiled::MapObject("P","",QPointF(((float)p.x()/VoronioForTiledMapTmx::SCALE),((float)p.y()/VoronioForTiledMapTmx::SCALE)),QSizeF(0.0,0.0));
            object->setPolygon(QPolygonF(QVector<QPointF>()<<QPointF(0.05,0.0)<<QPointF(0.05,0.0)<<QPointF(0.05,0.05)<<QPointF(0.0,0.05)));
            object->setShape(Tiled::MapObject::Polygon);
            layerPoint->addObject(object);
        }        qDebug("do point took %d ms", t.elapsed());*/

        const float noiseMapScale=0.005f/((mapXCount+mapYCount)/2);
        Simplex heighmap(seed+500);
        Simplex moisuremap(seed+5200);

        t.start();
        VoronioForTiledMapTmx::computeVoronoi(grid,totalWidth,totalHeight,tileStep);
        if(VoronioForTiledMapTmx::voronoiMap.zones.size()!=grid.size())
            abort();
        qDebug("computeVoronoi took %d ms", t.elapsed());

        {
            Tiled::Map tiledMap(Tiled::Map::Orientation::Orthogonal,totalWidth,totalHeight,16,16);
            QHash<QString,Tiled::Tileset *> cachedTileset;
            LoadMap::addTerrainLayer(tiledMap,dotransition);
            LoadMap::loadAllTileset(cachedTileset,tiledMap);
            if(displayzone)
            {
                std::vector<std::vector<Tiled::ObjectGroup *> > arrayTerrainPolygon;
                Tiled::ObjectGroup *layerZoneWaterPolygon=LoadMap::addDebugLayer(tiledMap,arrayTerrainPolygon,true);
                std::vector<std::vector<Tiled::ObjectGroup *> > arrayTerrainTile;
                Tiled::ObjectGroup *layerZoneWaterTile=LoadMap::addDebugLayer(tiledMap,arrayTerrainTile,false);
                LoadMap::addPolygoneTerrain(arrayTerrainPolygon,layerZoneWaterPolygon,arrayTerrainTile,layerZoneWaterTile,grid,VoronioForTiledMapTmx::voronoiMap,heighmap,moisuremap,noiseMapScale,tiledMap.width(),tiledMap.height());
            }
            {
                t.start();
                LoadMap::addTerrain(grid,VoronioForTiledMapTmx::voronoiMap,heighmap,moisuremap,noiseMapScale,tiledMap.width(),tiledMap.height());
                qDebug("Add terrain took %d ms", t.elapsed());
                if(dotransition)
                {
                    t.start();
                    TransitionTerrain::addTransitionGroupOnMap(tiledMap);
                    TransitionTerrain::addTransitionOnMap(tiledMap,false);
                    qDebug("Transitions took %d ms", t.elapsed());
                }
            }
            if(dovegetation)
            {
                t.start();
                MapPlants::addVegetation(tiledMap,VoronioForTiledMapTmx::voronoiMap);
                qDebug("Vegetation took %d ms", t.elapsed());
            }
            {
                Tiled::ObjectGroup *layerZoneChunk=new Tiled::ObjectGroup("Chunk",0,0,tiledMap.width(),tiledMap.height());
                layerZoneChunk->setColor(QColor("#ffe000"));
                tiledMap.addLayer(layerZoneChunk);

                unsigned int mapY=0;
                while(mapY<mapYCount)
                {
                    unsigned int mapX=0;
                    while(mapX<mapXCount)
                    {
                        Tiled::MapObject *object = new Tiled::MapObject(QString::number(mapX)+","+QString::number(mapY),"",QPointF(0,0), QSizeF(0.0,0.0));
                        object->setPolygon(QPolygonF(QRectF(mapX*mapWidth,mapY*mapHeight,mapWidth,mapHeight)));
                        object->setShape(Tiled::MapObject::Polygon);
                        layerZoneChunk->addObject(object);
                        mapX++;
                    }
                    mapY++;
                }
                layerZoneChunk->setVisible(false);
            }
            Tiled::MapWriter maprwriter;maprwriter.writeMap(&tiledMap,QCoreApplication::applicationDirPath()+"/dest/map/all.tmx");
        }
        //do tmx split
    }

    return 0;
}
