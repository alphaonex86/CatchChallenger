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
#include "MiniMap.h"

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
    float scale_TerrainMap;
    float scale_TerrainMoisure;
    float scale_Zone;
    float miniMapDivisor;
    bool displayzone,dotransition,dovegetation,dominimap;
    unsigned int tileStep;
    Settings::loadSettings(settings,mapWidth,mapHeight,mapXCount,mapYCount,seed,displayzone,dotransition,dovegetation,tileStep,scale_TerrainMap,scale_TerrainMoisure,
                           scale_Zone,dominimap,miniMapDivisor);
    srand(seed);

    {
        const unsigned int totalWidth=mapWidth*mapXCount;
        const unsigned int totalHeight=mapHeight*mapYCount;
        t.start();
        const Grid &grid = VoronioForTiledMapTmx::generateGrid(totalWidth,totalHeight,seed,30*mapXCount*mapYCount*scale_Zone,VoronioForTiledMapTmx::SCALE);
        qDebug("generateGrid took %d ms", t.elapsed());

        const float noiseMapScaleMoisure=0.005f/((mapXCount+mapYCount)/2)*scale_TerrainMoisure*((mapXCount+mapYCount)/2);
        const float noiseMapScaleMap=0.005f/((mapXCount+mapYCount)/2)*scale_TerrainMap*((mapXCount+mapYCount)/2);
        Simplex heighmap(seed+500);
        Simplex moisuremap(seed+5200);

        t.start();
        VoronioForTiledMapTmx::voronoiMap=VoronioForTiledMapTmx::computeVoronoi(grid,totalWidth,totalHeight,tileStep);
        VoronioForTiledMapTmx::voronoiMap1px=VoronioForTiledMapTmx::computeVoronoi(grid,totalWidth,totalHeight,1);
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
                LoadMap::addPolygoneTerrain(arrayTerrainPolygon,layerZoneWaterPolygon,arrayTerrainTile,layerZoneWaterTile,grid,VoronioForTiledMapTmx::voronoiMap,heighmap,moisuremap,noiseMapScaleMoisure,noiseMapScaleMap,tiledMap.width(),tiledMap.height());
            }
            {
                t.start();
                LoadMap::addTerrain(grid,VoronioForTiledMapTmx::voronoiMap,heighmap,moisuremap,noiseMapScaleMoisure,noiseMapScaleMap,tiledMap.width(),tiledMap.height());
                LoadMap::addTerrain(grid,VoronioForTiledMapTmx::voronoiMap1px,heighmap,moisuremap,noiseMapScaleMoisure,noiseMapScaleMap,tiledMap.width(),tiledMap.height(),0,0,false);
                qDebug("Add terrain took %d ms", t.elapsed());
                MapBrush::initialiseMapMask(tiledMap);
                if(dotransition)
                {
                    t.start();
                    TransitionTerrain::addTransitionGroupOnMap(tiledMap);
                    TransitionTerrain::addTransitionOnMap(tiledMap);
                    qDebug("Transitions took %d ms", t.elapsed());
                }
                t.start();
                TransitionTerrain::mergeDown(tiledMap);
                qDebug("mergeDown took %d ms", t.elapsed());
                TransitionTerrain::changeTileLayerOrder(tiledMap);
            }
            if(dominimap)
            {
                t.start();
                MiniMap::makeMap(heighmap,moisuremap,noiseMapScaleMoisure,noiseMapScaleMap,tiledMap.width(),tiledMap.height(),miniMapDivisor);
                MiniMap::makeMapTiled(tiledMap.width(),tiledMap.height());
                qDebug("dominimap %d ms", t.elapsed());
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
            Tiled::MapWriter maprwriter;maprwriter.writeMap(&tiledMap,QCoreApplication::applicationDirPath()+"/dest/main/official/all.tmx");
        }
        //do tmx split
    }

    return 0;
}
