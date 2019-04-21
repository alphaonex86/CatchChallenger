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
#include "MiniMapAll.h"

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

    QDir dir(QCoreApplication::applicationDirPath()+"/dest/map/main/official/");
    dir.removeRecursively();
    if(!dir.mkpath(dir.path()))
    {
        std::cerr << "Unable to create path: " << dir.path().toStdString() << std::endl;
        abort();
    }
    QDir dirZone(QCoreApplication::applicationDirPath()+"/dest/map/main/official/zone/");
    if(!dir.mkpath(dirZone.path()))
    {
        std::cerr << "Unable to create path: " << dir.path().toStdString() << std::endl;
        abort();
    }
    QFile info(QCoreApplication::applicationDirPath()+"/dest/map/main/official/informations.xml");
    if(info.open(QFile::WriteOnly))
    {
        QString content("<?xml version='1.0'?>\n"
                            "<informations color=\"#23c71f\">\n"
                            "    <name>Generated map</name>\n"
                            "    <name lang=\"fr\">Map généré</name>\n"
                            "    <initial>G</initial>\n"
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

    SettingsAll::SettingsExtra config;
    Settings::putDefaultSettings(settings);
    Settings::populateSettings(settings, config);

    SettingsAll::putDefaultSettings(settings);
    SettingsAll::populateSettings(settings, config);

    srand(config.seed);

    {
        const unsigned int totalWidth=config.mapWidth*config.mapXCount;
        const unsigned int totalHeight=config.mapHeight*config.mapYCount;
        t.start();
        const Grid &grid = VoronioForTiledMapTmx::generateGrid(totalWidth,totalHeight,config.seed,30*config.mapXCount*config.mapYCount*config.scale_Zone,VoronioForTiledMapTmx::SCALE);
        const Grid &gridCity = VoronioForTiledMapTmx::generateGrid(config.mapXCount-1,config.mapYCount-1,config.seed,0.1*config.mapXCount*config.mapYCount*config.scale_City,1);
        qDebug("generateGrid took %d ms", t.elapsed());

        const float noiseMapScaleMoisure=0.005f/((config.mapXCount+config.mapYCount)/2)*config.scale_TerrainMoisure*((config.mapXCount+config.mapYCount)/2);
        const float noiseMapScaleMap=0.005f/((config.mapXCount+config.mapYCount)/2)*config.scale_TerrainMap*((config.mapXCount+config.mapYCount)/2);
        Simplex heightmap(config.seed+500);
        Simplex moisuremap(config.seed+5200);
        Simplex levelmap(config.seed+212);

        t.start();
        VoronioForTiledMapTmx::voronoiMap=VoronioForTiledMapTmx::computeVoronoi(grid,totalWidth,totalHeight,config.tileStep);
        VoronioForTiledMapTmx::voronoiMap1px=VoronioForTiledMapTmx::computeVoronoi(grid,totalWidth,totalHeight,1);
        if(VoronioForTiledMapTmx::voronoiMap.zones.size()!=grid.size())
            abort();
        qDebug("computeVoronoi took %d ms", t.elapsed());

        Tiled::Map tiledMap(Tiled::Map::Orientation::Orthogonal,totalWidth,totalHeight,16,16);
        {
            QHash<QString,Tiled::Tileset *> cachedTileset;
            LoadMap::addTerrainLayer(tiledMap,config.dotransition);
            LoadMap::loadAllTileset(cachedTileset,tiledMap);

            Tiled::ObjectGroup *layerObject=new Tiled::ObjectGroup("Object",0,0,tiledMap.width(),tiledMap.height());
            tiledMap.addLayer(layerObject);

            if(config.displayzone)
            {
                std::vector<std::vector<Tiled::ObjectGroup *> > arrayTerrainPolygon;
                Tiled::ObjectGroup *layerZoneWaterPolygon=LoadMap::addDebugLayer(tiledMap,arrayTerrainPolygon,true);
                std::vector<std::vector<Tiled::ObjectGroup *> > arrayTerrainTile;
                Tiled::ObjectGroup *layerZoneWaterTile=LoadMap::addDebugLayer(tiledMap,arrayTerrainTile,false);
                LoadMap::addPolygoneTerrain(arrayTerrainPolygon,layerZoneWaterPolygon,arrayTerrainTile,layerZoneWaterTile,grid,
                                            VoronioForTiledMapTmx::voronoiMap,heightmap,moisuremap,noiseMapScaleMoisure,noiseMapScaleMap,
                                            tiledMap.width(),tiledMap.height());
            }
            {
                t.start();
                LoadMap::addTerrain(grid,VoronioForTiledMapTmx::voronoiMap,heightmap,moisuremap,noiseMapScaleMoisure,noiseMapScaleMap,
                                    tiledMap.width(),tiledMap.height());
                LoadMap::addTerrain(grid,VoronioForTiledMapTmx::voronoiMap1px,heightmap,moisuremap,noiseMapScaleMoisure,noiseMapScaleMap,
                                    tiledMap.width(),tiledMap.height(),0,0,false);
                qDebug("Add terrain took %d ms", t.elapsed());
                MapBrush::initialiseMapMask(tiledMap);
                t.start();
                LoadMapAll::addCity(tiledMap,gridCity,config.citiesNames,config.mapXCount,config.mapYCount,config.maxCityLinks,config.cityRadius,
                                    levelmap,config.levelmapscale,config.levelmapmin,config.levelmapmax,heightmap,moisuremap,noiseMapScaleMoisure,noiseMapScaleMap);
                //LoadMapAll::addCityContent(tiledMap,config.mapXCount,config.mapYCount,false);
                qDebug("place cities took %d ms", t.elapsed());
                if(config.dotransition)
                {

                    t.start();
                    TransitionTerrain::addTransitionGroupOnMap(tiledMap);
                    qDebug("Transitions group took %d ms", t.elapsed());
                }
                t.start();
                LoadMapAll::generateRoadContent(tiledMap, config);
                qDebug("generate road content took %d ms", t.elapsed());
                if(config.dotransition)
                {
                    t.start();
                    TransitionTerrain::addTransitionOnMap(tiledMap);
                    qDebug("Transitions took %d ms", t.elapsed());
                }
                t.start();
                TransitionTerrain::mergeDown(tiledMap);
                qDebug("mergeDown took %d ms", t.elapsed());
                t.start();
                //LoadMapAll::addCityContent(tiledMap, config.mapXCount, config.mapYCount,true);
                LoadMapAll::addMapChange(tiledMap,config.mapXCount,config.mapYCount);
                qDebug("add city content took %d ms", t.elapsed());
                t.start();
                LoadMapAll::addRoadContent(tiledMap, config);
                qDebug("add road content took %d ms", t.elapsed());
                //TransitionTerrain::changeTileLayerOrder(tiledMap);
            }
            if(config.displaycity)
                LoadMapAll::addDebugCity(tiledMap,config.mapWidth,config.mapHeight);
            if(config.dominimap)
            {
                t.start();
                //MiniMap::makeMap(heighmap,moisuremap,noiseMapScaleMoisure,noiseMapScaleMap,tiledMap.width(),tiledMap.height(),miniMapDivisor).save(QCoreApplication::applicationDirPath()+"/miniMapLinear.png","PNG");
                MiniMapAll::makeMapTiled(tiledMap.width(),tiledMap.height(),config.mapWidth,config.mapHeight).save(QCoreApplication::applicationDirPath()+"/miniMapPixel.png","PNG");
                qDebug("dominimap %d ms", t.elapsed());
            }
            if(config.dovegetation)
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
                while(mapY<config.mapYCount)
                {
                    unsigned int mapX=0;
                    while(mapX<config.mapXCount)
                    {
                        Tiled::MapObject *object = new Tiled::MapObject(QString::number(mapX)+","+QString::number(mapY),"",QPointF(0,0), QSizeF(0.0,0.0));
                        object->setPolygon(QPolygonF(QRectF(mapX*config.mapWidth,mapY*config.mapHeight,config.mapWidth,config.mapHeight)));
                        object->setShape(Tiled::MapObject::Polygon);
                        layerZoneChunk->addObject(object);
                        mapX++;
                    }
                    mapY++;
                }
                layerZoneChunk->setVisible(false);
            }
            if(config.doallmap)
            {
                Tiled::MapWriter maprwriter;
                if(!maprwriter.writeMap(&tiledMap,QCoreApplication::applicationDirPath()+"/dest/map/main/official/all.tmx"))
                {
                    std::cerr << "Unable to write " << QCoreApplication::applicationDirPath().toStdString() << "/dest/map/main/official/all.tmx" << std::endl;
                    abort();
                }
                qDebug("Write all.tmx %d ms", t.elapsed());
            }
        }
        //do tmx split
        t.start();
        PartialMap::RecuesPoint startPoint;
        std::vector<PartialMap::RecuesPoint> recuesPoints;
        {
            const unsigned int singleMapWitdh=tiledMap.width()/config.mapXCount;
            const unsigned int singleMapHeight=tiledMap.height()/config.mapYCount;

            unsigned int indexCity=0;
            while(indexCity<LoadMapAll::cities.size())
            {
                std::vector<PartialMap::RecuesPoint> newRecuesPoints;
                const LoadMapAll::City &city=LoadMapAll::cities.at(indexCity);
                const std::string &cityLowerCaseName=LoadMapAll::lowerCase(city.name);
                const uint32_t x=city.x;
                const uint32_t y=city.y;
                const std::string &file=cityLowerCaseName+"/"+cityLowerCaseName+".tmx";
                if(!PartialMap::save(tiledMap,
                                 x*singleMapWitdh,y*singleMapHeight,
                                 x*singleMapWitdh+singleMapWitdh,y*singleMapHeight+singleMapHeight,
                                 file,
                                 newRecuesPoints,
                                 "city",cityLowerCaseName,city.name
                                 ))
                {
                    std::cerr << "Unable to write " << file << "" << std::endl;
                    abort();
                }
                if(config.levelmapmin==city.level)
                {
                    if(newRecuesPoints.empty())
                    {
                        std::cerr << "newRecuesPoints empty for city (abort)" << std::endl;
                        abort();
                    }
                    startPoint=newRecuesPoints.front();
                }
                recuesPoints.insert(recuesPoints.cend(),newRecuesPoints.cbegin(),newRecuesPoints.cend());
                if(LoadMapAll::zones.find(cityLowerCaseName)==LoadMapAll::zones.cend())
                {
                    QFile xmlinfo(QCoreApplication::applicationDirPath()+"/dest/map/main/official/zone/"+QString::fromStdString(cityLowerCaseName)+".xml");
                    if(xmlinfo.open(QFile::WriteOnly))
                    {
                        QString content("<zone>\n"
                                        "  <name>"+QString::fromStdString(city.name)+"</name>\n"
                                        "</zone>");
                        QByteArray contentData(content.toUtf8());
                        xmlinfo.write(contentData.constData(),contentData.size());
                        xmlinfo.close();
                    }
                    else
                    {
                        std::cerr << "Unable to write zone " << cityLowerCaseName << std::endl;
                        abort();
                    }
                    LoadMapAll::Zone zone;
                    zone.name=city.name;
                    LoadMapAll::zones[cityLowerCaseName]=zone;
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

                    const uint8_t &zoneOrientation=LoadMapAll::mapPathDirection[x+y*config.mapXCount];
                    if(zoneOrientation!=0)
                    {
                        const LoadMapAll::RoadIndex &roadIndex=LoadMapAll::roadCoordToIndex.at(x).at(y);

                        //compose string
                        std::string file;
                        std::string zoneName;
                        if(road.haveOnlySegmentNearCity)
                        {
                            if(roadIndex.cityIndex.empty())
                            {
                                std::cerr << "road.haveOnlySegmentNearCity and indexRoad.cityIndex.empty()" << std::endl;
                                abort();
                            }
                            const LoadMapAll::RoadToCity &cityIndex=roadIndex.cityIndex.front();
                            const std::string &cityLowerCaseName=LoadMapAll::lowerCase(LoadMapAll::cities.at(cityIndex.cityIndex).name);
                            file=cityLowerCaseName+"/road-"+std::to_string(roadIndex.roadIndex+1)+
                                    "-"+LoadMapAll::orientationToString(LoadMapAll::reverseOrientation(cityIndex.orientation))+".tmx";
                            zoneName=cityLowerCaseName;
                        }
                        else
                        {
                            std::string cityLowerCaseName="road-"+std::to_string(roadIndex.roadIndex+1);
                            file="road-"+std::to_string(roadIndex.roadIndex+1)+"/"+std::to_string(indexCoord+1)+".tmx";
                            if(LoadMapAll::zones.find(cityLowerCaseName)==LoadMapAll::zones.cend())
                            {
                                QFile xmlinfo(QCoreApplication::applicationDirPath()+"/dest/map/main/official/zone/"+QString::fromStdString(cityLowerCaseName)+".xml");
                                if(xmlinfo.open(QFile::WriteOnly))
                                {
                                    QString content("<zone>\n"
                                                    "  <name>Road "+QString::number(roadIndex.roadIndex+1)+"</name>\n"
                                                    "</zone>");
                                    QByteArray contentData(content.toUtf8());
                                    xmlinfo.write(contentData.constData(),contentData.size());
                                    xmlinfo.close();
                                }
                                else
                                {
                                    std::cerr << "Unable to write zone " << cityLowerCaseName << std::endl;
                                    abort();
                                }
                                LoadMapAll::Zone zone;
                                zone.name="Road "+std::to_string(roadIndex.roadIndex+1);
                                LoadMapAll::zones[cityLowerCaseName]=zone;
                            }
                            zoneName="road-"+std::to_string(roadIndex.roadIndex+1);
                        }

                        std::string additionalXmlInfo;
                        if(!roadIndex.roadMonsters.empty())
                        {
                            additionalXmlInfo+="  <grass>\n";
                            unsigned int roadMonsterIndex=0;
                            while(roadMonsterIndex<roadIndex.roadMonsters.size())
                            {
                                const LoadMapAll::RoadMonster &roadMonster=roadIndex.roadMonsters.at(roadMonsterIndex);
                                additionalXmlInfo+="    <monster id=\""+std::to_string(roadMonster.monsterId)+
                                        "\" minLevel=\""+std::to_string(roadMonster.minLevel)+
                                        "\" maxLevel=\""+std::to_string(roadMonster.maxLevel)+
                                        "\" luck=\""+std::to_string(roadMonster.luck)+
                                        "\"/>\n";
                                roadMonsterIndex++;
                            }
                            additionalXmlInfo+="  </grass>\n";
                        }
                        if(!PartialMap::save(tiledMap,
                                         x*singleMapWitdh,y*singleMapHeight,
                                         x*singleMapWitdh+singleMapWitdh,y*singleMapHeight+singleMapHeight,
                                         file,
                                         recuesPoints,
                                         "outdoor",zoneName,"Road "+std::to_string(roadIndex.roadIndex+1),
                                         additionalXmlInfo
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
        t.start();
        LoadMapAll::writeRoadContent(tiledMap, config.mapXCount, config.mapYCount);
        qDebug("Write bots xml %d ms", t.elapsed());
        //do the start point
        QFile start(QCoreApplication::applicationDirPath()+"/dest/map/main/official/start.xml");
        if(start.open(QFile::WriteOnly))
        {
            //const PartialMap::RecuesPoint &startPoint=recuesPoints.front();
            /*if(recuesPoints.empty())
            {
                std::cerr << "recuesPoints.empty() then can't do start.xml (abort)" << std::endl;
                abort();
            }*/
            QString content("<!--\n"
                            "/!\\ warning, directly put this information into db\n"
                            "/!\\ not check if x,y is into the range of the map\n"
                            "-->\n"
                            "<profile>\n"
                            "  <start id=\"Normal\">\n"
                            "    <map x=\""+QString::number(startPoint.x)+"\" y=\""+QString::number(startPoint.y)+"\" file=\""+QString::fromStdString(startPoint.map)+"\"/>\n"
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
