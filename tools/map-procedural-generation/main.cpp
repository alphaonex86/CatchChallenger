#include <QApplication>
#include <QSettings>
#include <QTime>
#include <QFile>
#include <QDir>
#include <iostream>

#include "../../client/tiled/tiled_mapwriter.h"
#include "../../client/tiled/tiled_mapobject.h"
#include "../../client/tiled/tiled_objectgroup.h"

#include "../map-procedural-generation-terrain/znoise/headers/Simplex.hpp"
#include "../map-procedural-generation-terrain/VoronioForTiledMapTmx.h"
#include "../map-procedural-generation-terrain/LoadMap.h"
#include "../map-procedural-generation-terrain/TransitionTerrain.h"
#include "../map-procedural-generation-terrain/Settings.h"
#include "../map-procedural-generation-terrain/MapPlants.h"
#include "../map-procedural-generation-terrain/MiniMap.h"
#include "SettingsAll.h"
#include "LoadMapAll.h"
#include "PartialMap.h"

/*To do: Tree/Grass, Rivers
http://www-cs-students.stanford.edu/~amitp/game-programming/polygon-map-generation/*/
/*template<typename T, size_t C, size_t R>  Matrix { std::array<T, C*R> };
 * operator[](int c, int r) { return arr[C*r+c]; }*/

int main(int argc, char *argv[])
{
    // Avoid performance issues with X11 engine when rendering objects
#ifdef Q_WS_X11
    QApplication::setGraphicsSystem(QStringLiteral("raster"));//now use: -platform offscreen
#endif
    QApplication a(argc, argv);
    QTime t;
    QTime total;
    total.start();

    a.setOrganizationDomain(QStringLiteral("catchchallenger"));
    a.setApplicationName(QStringLiteral("map-procedural-generation"));
    a.setApplicationVersion(QStringLiteral("1.0"));

    if(!QFile::exists(QCoreApplication::applicationDirPath()+"/settings.xml"))
        QFile::copy(":/settings.xml",QCoreApplication::applicationDirPath()+"/settings.xml");
    QSettings settings(QCoreApplication::applicationDirPath()+"/settings.xml",QSettings::NativeFormat);

    QDir dir(QCoreApplication::applicationDirPath()+"/dest/main/official/");
    dir.removeRecursively();
    if(!dir.mkpath(dir.path()))
    {
        std::cerr << "Unable to create path: " << dir.path().toStdString() << std::endl;
        abort();
    }
    QFile info(QCoreApplication::applicationDirPath()+"/dest/main/official/informations.xml");
    if(info.open(QFile::WriteOnly))
    {
        QString content("<?xml version='1.0'?>\n"
                            "<informations color=\"#688BFF\">\n"
                            "    <name>Generated map</name>\n"
                            "    <name lang=\"fr\">Map généré</name>\n"
                            "    <initial>T</initial>\n"
                            "    <options>\n");
        QFile optionsFile(QCoreApplication::applicationDirPath()+"/settings.xml");
        if(!optionsFile.open(QFile::ReadOnly))
        {
            std::cerr << "!optionsFile.open(QFile::ReadOnly)" << std::endl;
            abort();
        }
        content+=optionsFile.readAll();
        optionsFile.close();
        content+="    </options>\n";
        content+="</informations>";
        QByteArray contentData(content.toUtf8());
        info.write(contentData.constData(),contentData.size());
        info.close();
    }
    else
    {
        std::cerr << "Unable to write informations.xml" << std::endl;
        abort();
    }

    Settings::putDefaultSettings(settings);
    unsigned int mapWidth=0;
    unsigned int mapHeight=0;
    unsigned int mapXCount=0;
    unsigned int mapYCount=0;
    unsigned int seed=0;
    float scale_TerrainMap=0;
    float scale_TerrainMoisure=0;
    float scale_Zone=0;
    float miniMapDivisor=0;
    bool displayzone=false,dotransition=false,dovegetation=false,dominimap=false;
    unsigned int tileStep=0;
    Settings::loadSettings(settings,mapWidth,mapHeight,mapXCount,mapYCount,seed,displayzone,dotransition,dovegetation,tileStep,scale_TerrainMap,scale_TerrainMoisure,
                           scale_Zone,dominimap,miniMapDivisor);

    SettingsAll::putDefaultSettings(settings);
    bool displaycity=false,doallmap=false;
    float scale_City=0;
    std::vector<std::string> citiesNames;
    unsigned int maxCityLinks=0;
    unsigned int cityRadius=0;
    SettingsAll::loadSettings(settings,displaycity,citiesNames,scale_City,doallmap,maxCityLinks,cityRadius);

    srand(seed);

    {
        const unsigned int totalWidth=mapWidth*mapXCount;
        const unsigned int totalHeight=mapHeight*mapYCount;
        t.start();
        const Grid &grid = VoronioForTiledMapTmx::generateGrid(totalWidth,totalHeight,seed,30*mapXCount*mapYCount*scale_Zone,VoronioForTiledMapTmx::SCALE);
        const Grid &gridCity = VoronioForTiledMapTmx::generateGrid(mapXCount-1,mapYCount-1,seed,0.1*mapXCount*mapYCount*scale_City,1);
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

        Tiled::Map tiledMap(Tiled::Map::Orientation::Orthogonal,totalWidth,totalHeight,16,16);
        {
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
                t.start();
                LoadMapAll::addCity(tiledMap,gridCity,citiesNames,mapXCount,mapYCount,maxCityLinks,cityRadius);
                LoadMapAll::addCityContent(tiledMap,mapXCount,mapYCount,false);
                qDebug("place cities took %d ms", t.elapsed());
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
                t.start();
                LoadMapAll::addCityContent(tiledMap,mapXCount,mapYCount,true);
                LoadMapAll::addMapChange(tiledMap,mapXCount,mapYCount);
                qDebug("add city content took %d ms", t.elapsed());
                TransitionTerrain::changeTileLayerOrder(tiledMap);
            }
            if(displaycity)
                LoadMapAll::addDebugCity(tiledMap,mapWidth,mapHeight);
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
            t.start();
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
            if(doallmap)
            {
                Tiled::MapWriter maprwriter;
                if(!maprwriter.writeMap(&tiledMap,QCoreApplication::applicationDirPath()+"/dest/main/official/all.tmx"))
                {
                    std::cerr << "Unable to write " << QCoreApplication::applicationDirPath().toStdString() << "/dest/main/official/all.tmx" << std::endl;
                    abort();
                }
            }
            qDebug("Write all.tmx %d ms", t.elapsed());
        }
        //do tmx split
        t.start();
        std::vector<PartialMap::RecuesPoint> recuesPoints;
        {
            const unsigned int singleMapWitdh=tiledMap.width()/mapXCount;
            const unsigned int singleMapHeight=tiledMap.height()/mapYCount;

            unsigned int indexCity=0;
            while(indexCity<LoadMapAll::cities.size())
            {
                const LoadMapAll::City &city=LoadMapAll::cities.at(indexCity);
                const uint32_t x=city.x;
                const uint32_t y=city.y;
                const std::string &file=LoadMapAll::lowerCase(city.name)+"/"+LoadMapAll::lowerCase(city.name)+".tmx";
                if(!PartialMap::save(tiledMap,
                                 x*singleMapWitdh,y*singleMapHeight,
                                 x*singleMapWitdh+singleMapWitdh,y*singleMapHeight+singleMapHeight,
                                 file,
                                 recuesPoints,
                                 "city","",city.name
                                 ))
                {
                    std::cerr << "Unable to write " << file << "" << std::endl;
                    abort();
                }

                indexCity++;
            }

            unsigned int indexIntRoad=0;
            while(indexIntRoad<LoadMapAll::roads.size())
            {
                const LoadMapAll::Road &road=LoadMapAll::roads.at(indexIntRoad);
                unsigned int indexCoord=0;
                while(indexCoord<road.coords.size())
                {
                    const std::pair<uint16_t,uint16_t> &coord=road.coords.at(indexCoord);
                    const unsigned int &x=coord.first;
                    const unsigned int &y=coord.second;

                    const uint8_t &zoneOrientation=LoadMapAll::mapPathDirection[x+y*mapXCount];
                    if(zoneOrientation!=0)
                    {
                        const LoadMapAll::RoadIndex &roadIndex=LoadMapAll::roadCoordToIndex.at(x).at(y);

                        //compose string
                        std::string file;
                        if(road.haveOnlySegmentNearCity)
                        {
                            if(roadIndex.cityIndex.empty())
                            {
                                std::cerr << "road.haveOnlySegmentNearCity and indexRoad.cityIndex.empty()" << std::endl;
                                abort();
                            }
                            const LoadMapAll::RoadToCity &cityIndex=roadIndex.cityIndex.front();
                            file=LoadMapAll::lowerCase(LoadMapAll::cities.at(cityIndex.cityIndex).name)+"/road-"+std::to_string(roadIndex.roadIndex+1)+
                                    "-"+LoadMapAll::orientationToString(LoadMapAll::reverseOrientation(cityIndex.orientation))+".tmx";
                        }
                        else
                            file="road-"+std::to_string(roadIndex.roadIndex+1)+"/"+std::to_string(indexCoord+1)+".tmx";

                        if(!PartialMap::save(tiledMap,
                                         x*singleMapWitdh,y*singleMapHeight,
                                         x*singleMapWitdh+singleMapWitdh,y*singleMapHeight+singleMapHeight,
                                         file,
                                         recuesPoints,
                                         "outdoor","","Road "+std::to_string(roadIndex.roadIndex+1)
                                         ))
                        {
                            std::cerr << "Unable to write " << file << "" << std::endl;
                            abort();
                        }
                    }
                    else
                    {
                        std::cerr << "zoneOrientation is wrong: " << std::to_string(zoneOrientation) << std::endl;
                        abort();
                    }

                    indexCoord++;
                }
                indexIntRoad++;
            }
        }
        qDebug("Write chunk tmx %d ms", t.elapsed());
        //do the start point
        QFile start(QCoreApplication::applicationDirPath()+"/dest/main/official/start.xml");
        if(start.open(QFile::WriteOnly))
        {
            const PartialMap::RecuesPoint &recuesPoint=recuesPoints.front();
            if(recuesPoints.empty())
            {
                std::cerr << "recuesPoints.empty() then can't do start.xml (abort)" << std::endl;
                abort();
            }
            QString content("<!--\n"
                            "/!\\ warning, directly put this information into db\n"
                            "/!\\ not check if x,y is into the range of the map\n"
                            "-->\n"
                            "<profile>\n"
                            "  <start id=\"Normal\">\n"
                            "    <map x=\""+QString::number(recuesPoint.x)+"\" y=\""+QString::number(recuesPoint.y)+"\" file=\""+QString::fromStdString(recuesPoint.map)+"\"/>\n"
                            "  </start>\n"
                            "</profile>");
            QByteArray contentData(content.toUtf8());
            start.write(contentData.constData(),contentData.size());
            start.close();
        }
        else
        {
            std::cerr << "Unable to write informations.xml" << std::endl;
            abort();
        }
    }
    qDebug("Total time %d ms", total.elapsed());

    return 0;
}
