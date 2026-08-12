#include <QApplication>
#include <QSettings>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <iostream>
#include <algorithm>

#include <libtiled/mapwriter.h>
#include <libtiled/mapobject.h>
#include <libtiled/objectgroup.h>
#include <libtiled/tileset.h>

#include "../map-procedural-generation-terrain/znoise/headers/Simplex.hpp"
#include "../map-procedural-generation-terrain/VoronioForTiledMapTmx.h"
#include "../map-procedural-generation-terrain/LoadMap.h"
#include "../map-procedural-generation-terrain/TransitionTerrain.h"
#include "../map-procedural-generation-terrain/Settings.h"
#include "../map-procedural-generation-terrain/MapPlants.h"
#include "../map-procedural-generation-terrain/MapBrush.h"
#include "../map-procedural-generation-terrain/MiniMap.h"
#include "SettingsAll.h"
#include "LoadMapAll.h"
#include "PartialMap.h"
#include "MiniMapAll.h"
#include "TerrainFlattener.h"


// A hack to keep all smart pointers alive during the whole program lifetime
std::vector<Tiled::SharedTileset> PartialMap_tilesets_hack;
std::vector<Tiled::SharedTileset> LoadMap_tilesets_hack;
std::vector<Tiled::SharedTileset> MapBrush_tilesets_hack;
std::vector<std::unique_ptr<Tiled::Map>> LoadMap_map_hack;

/*To do: Tree/Grass, Rivers
http://www-cs-students.stanford.edu/~amitp/game-programming/polygon-map-generation/*/

//one start point per city in the lowest 10% of the level range
struct CityStart
{
    std::string cityName;
    PartialMap::RecuesPoint point;
    uint8_t level;
};
static bool cityStartLess(const CityStart &a, const CityStart &b)
{
    if(a.level!=b.level)
        return a.level<b.level;
    return a.cityName<b.cityName;
}
/*template<typename T, size_t C, size_t R>  Matrix { std::array<T, C*R> };
 * operator[](int c, int r) { return arr[C*r+c]; }*/

int main(int argc, char *argv[])
{
    // Avoid performance issues with X11 engine when rendering objects
#ifdef Q_WS_X11
    QApplication::setGraphicsSystem(QStringLiteral("raster"));//now use: -platform offscreen
#endif
    QApplication a(argc, argv);
    QElapsedTimer t;
    QElapsedTimer total;
    total.start();

    a.setOrganizationDomain(QStringLiteral("catchchallenger"));
    a.setApplicationName(QStringLiteral("map-procedural-generation"));
    a.setApplicationVersion(QStringLiteral("1.0"));

    //Optional "--config <path>" selects a per-datapack settings file so the same
    //binary can target different datapacks (each datapack needs its own tile
    //indices / tileset paths / item & monster ids). Defaults to settings.ini next
    //to the binary.
    QString settingsPath=QCoreApplication::applicationDirPath()+"/settings.ini";
    QString datapackPath;
    {
        const QStringList args=a.arguments();
        int ai=1;
        while(ai<args.size())
        {
            if((args.at(ai)=="--config" || args.at(ai)=="-c") && ai+1<args.size())
            {
                settingsPath=args.at(ai+1);
                ai++;
            }
            //"--datapack <dir>": stage the tilesets of that datapack into dest/
            //so a fresh build directory generates without any manual copy
            else if((args.at(ai)=="--datapack" || args.at(ai)=="-d") && ai+1<args.size())
            {
                datapackPath=args.at(ai+1);
                ai++;
            }
            ai++;
        }
    }
    //Run staging pool. "--datapack <dir>" goes FIRST so a datapack tileset always
    //wins over the tool's own copy of the same file name; the tool's tileset/ then
    //fills whatever is left, which is what lets a fresh clone build and generate
    //with no argument at all.
    if(!datapackPath.isEmpty())
    {
        if(!QDir(datapackPath+"/map/tileset").exists())
        {
            std::cerr << "No map/tileset/ in the datapack " << datapackPath.toStdString() << std::endl;
            return 1;
        }
        if(!LoadMap::stageTilesetPool(datapackPath+"/map/tileset"))
            return 1;
    }
    if(!LoadMap::stageTilesetPool(QCoreApplication::applicationDirPath()+"/tileset"))
        return 1;
    if(!QFile::exists(settingsPath))
    {
        std::cerr << "No settings file at " << settingsPath.toStdString()
                  << " (build the map-procedural-generation-runtime target, or pass --config <file>)" << std::endl;
        return 1;
    }
    //QSettings WRITES BACK on sync: putDefaultSettings adds any key the file does
    //not have yet, and QSettings then rewrites the whole file — which strips every
    //comment out of the operator's config. The generator reads the settings, it
    //has no business editing them, so it works on a COPY and the checked-in
    //settings.ini keeps its comments whatever defaults are added later.
    const QString settingsRunPath=QCoreApplication::applicationDirPath()+"/settings-run.ini";
    if(QFile::exists(settingsRunPath))
        if(!QFile::remove(settingsRunPath))
        {
            std::cerr << "Unable to replace " << settingsRunPath.toStdString() << std::endl;
            return 1;
        }
    if(!QFile::copy(settingsPath,settingsRunPath))
    {
        std::cerr << "Unable to copy " << settingsPath.toStdString()
                  << " to " << settingsRunPath.toStdString() << std::endl;
        return 1;
    }
    //IniFormat, not NativeFormat: NativeFormat only means "ini file" on Unix — on
    //Windows it is the system REGISTRY and the file would be silently ignored
    QSettings settings(settingsRunPath,QSettings::IniFormat);
    //the label the world is written under, once, for every path below
    LoadMap::resolveMainCode(settings);
    std::cout << "Generating the map label \"" << LoadMap::mainCode().toStdString() << "\"" << std::endl;

    //Validate the input BEFORE dest/ is wiped: a template defect that no tool can repair
    //without knowing the author's intent must not produce half a world. Every error of
    //every template is reported in one go so the author fixes them in one pass.
    {
        std::vector<std::string> templateErrors;
        if(!LoadMapAll::precheckTemplates(templateErrors))
        {
            std::cerr << templateErrors.size() << " error(s) in the input templates, nothing was generated:" << std::endl;
            unsigned int errorIndex=0;
            while(errorIndex<templateErrors.size())
            {
                std::cerr << "  " << templateErrors.at(errorIndex) << std::endl;
                errorIndex++;
            }
            std::cerr << "Fix them and run again (template-check.py --fix repairs the mechanical ones)." << std::endl;
            return 1;
        }
    }

    QDir dir(LoadMap::destMainDir());
    dir.removeRecursively();
    if(!dir.mkpath(dir.path()))
    {
        std::cerr << "Unable to create path: " << dir.path().toStdString() << std::endl;
        abort();
    }
    QDir dirZone(LoadMap::destMainDir()+"zone/");
    if(!dir.mkpath(dirZone.path()))
    {
        std::cerr << "Unable to create path: " << dir.path().toStdString() << std::endl;
        abort();
    }
    QFile info(LoadMap::destMainDir()+"informations.xml");
    if(info.open(QFile::WriteOnly))
    {
        //no <options> block here: the generator settings are neither read by the engine nor
        //useful to the player who receives this datapack, and they don't allow rebuilding
        //another server either. The settings stay in settings.ini, next to the generator.
        QString content("<?xml version='1.0'?>\n"
                            "<informations color=\"#23c71f\">\n"
                            "    <name>Generated map</name>\n"
                            "    <name lang=\"fr\">Map généré</name>\n"
                            "    <initial>G</initial>\n"
                            "</informations>");
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
    //THE DATAPACK IS THE AUTHORITY on what it takes to walk on water: its
    //map/layers.xml declares the item, the settings value is only the fallback for
    //a run with no --datapack. Coastal shops sell whatever comes out of here.
    if(!datapackPath.isEmpty())
    {
        std::vector<unsigned int> waterWalkItems;
        if(LoadMapAll::readWaterWalkItems(datapackPath,waterWalkItems))
            config.waterWalkItems=waterWalkItems;
    }
    if(!config.waterWalkItems.empty())
    {
        std::cout << "coastal shops sell the water walk item(s):";
        unsigned int itemIndex=0;
        while(itemIndex<config.waterWalkItems.size())
        {
            std::cout << " " << config.waterWalkItems.at(itemIndex);
            itemIndex++;
        }
        std::cout << std::endl;
    }

    srand(config.seed);

    {
        const unsigned int totalWidth=config.mapWidth*config.mapXCount;
        const unsigned int totalHeight=config.mapHeight*config.mapYCount;
        t.start();
        const Grid &grid = VoronioForTiledMapTmx::generateGrid(totalWidth,totalHeight,config.seed,30*config.mapXCount*config.mapYCount*config.scale_Zone,VoronioForTiledMapTmx::SCALE);
        const Grid &gridCity = VoronioForTiledMapTmx::generateGrid(config.mapXCount-1,config.mapYCount-1,config.seed,0.1*config.mapXCount*config.mapYCount*config.scale_City,1);
        qDebug("generateGrid took %lld ms", t.elapsed());

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
        qDebug("computeVoronoi took %lld ms", t.elapsed());

        Tiled::Map tiledMap(Tiled::Map::Orientation::Orthogonal,totalWidth,totalHeight,16,16);
        //stays alive as long as the map: it is installed as the terrain shaper and
        //every later terrain sample (vegetation, minimap) goes through it
        TerrainFlattener cityFlattener;
        {
            QHash<QString,Tiled::Tileset *> cachedTileset;
            LoadMap::addTerrainLayer(tiledMap,config.dotransition);
            LoadMap::loadAllTileset(cachedTileset,tiledMap);

            Tiled::ObjectGroup *layerObject=new Tiled::ObjectGroup("Object",0,0); // ObjectGroup contructor no longer accept width and height
            tiledMap.addLayer(layerObject);

            {
                t.start();
                //FIRST terrain pass, nothing drawn: the city picker only needs the
                //per zone height, to know how much land a chunk has
                LoadMap::addTerrain(grid,VoronioForTiledMapTmx::voronoiMap,heightmap,moisuremap,noiseMapScaleMoisure,noiseMapScaleMap,
                                    tiledMap.width(),tiledMap.height(),0,0,false);
                LoadMap::addTerrain(grid,VoronioForTiledMapTmx::voronoiMap1px,heightmap,moisuremap,noiseMapScaleMoisure,noiseMapScaleMap,
                                    tiledMap.width(),tiledMap.height(),0,0,false);
                qDebug("Add terrain took %lld ms", t.elapsed());
                //what is a SEA and what is only a lake. BEFORE addCity: the water
                //paths join the road graph there, and that is the last moment the
                //graph can still be changed.
                t.start();
                LoadMapAll::detectWaterBodies(tiledMap,config);
                qDebug("detect water bodies took %lld ms", t.elapsed());
                t.start();
                //the terrain a road takes its wild monsters from is read from the
                //voronoi zones, not sampled again from the noise
                LoadMapAll::addCity(tiledMap,gridCity,config.citiesNames,config.mapXCount,config.mapYCount,config.maxCityLinks,config.cityRadius,
                                    levelmap,config.levelmapscale,config.levelmapmin,config.levelmapmax,config);
                qDebug("place cities took %lld ms", t.elapsed());
                //Now that the towns are known, FLATTEN the terrain under each of
                //them and sample the terrain AGAIN, this time drawing it: a town cut
                //in half by two terrains looks wrong, and the gradient restarting at
                //the town level keeps the mountain wall away from its border.
                if(config.cityFlatten)
                {
                    t.start();
                    LoadMapAll::addCityFlatZones(cityFlattener,tiledMap.width(),tiledMap.height(),config);
                    TerrainShaper::setActive(&cityFlattener);
                    qDebug("flatten %u cities took %lld ms", cityFlattener.size(), t.elapsed());
                }
                t.start();
                LoadMap::addTerrain(grid,VoronioForTiledMapTmx::voronoiMap,heightmap,moisuremap,noiseMapScaleMoisure,noiseMapScaleMap,
                                    tiledMap.width(),tiledMap.height());
                LoadMap::addTerrain(grid,VoronioForTiledMapTmx::voronoiMap1px,heightmap,moisuremap,noiseMapScaleMoisure,noiseMapScaleMap,
                                    tiledMap.width(),tiledMap.height(),0,0,false);
                qDebug("Draw terrain took %lld ms", t.elapsed());
                MapBrush::initialiseMapMask(tiledMap);
                //BEFORE the transitions: the outer-border pass is mask-gated, so
                //this is what keeps the mountain COLLISION ridge out of the town
                //square (it used to cut it in two and refuse every building lot),
                //and the same mark later keeps the vegetation out
                LoadMapAll::maskCityHoles(tiledMap,config);
                //the debug zone overlay must show the terrain that was DRAWN, so it
                //comes after the flattening pass
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
                if(config.dotransition)
                {

                    t.start();
                    TransitionTerrain::addTransitionGroupOnMap(tiledMap);
                    qDebug("Transitions group took %lld ms", t.elapsed());
                }
                t.start();
                LoadMapAll::generateRoadContent(tiledMap, config);
                qDebug("generate road content took %lld ms", t.elapsed());
                if(config.dotransition)
                {
                    t.start();
                    TransitionTerrain::addTransitionOnMap(tiledMap);
                    qDebug("Transitions took %lld ms", t.elapsed());
                }
                t.start();
                TransitionTerrain::mergeDown(tiledMap);
                qDebug("mergeDown took %lld ms", t.elapsed());
                t.start();
                LoadMapAll::addMapChange(tiledMap,config.mapXCount,config.mapYCount);
                qDebug("add city content took %lld ms", t.elapsed());
                t.start();
                LoadMapAll::addRoadContent(tiledMap, config);
                qDebug("add road content took %lld ms", t.elapsed());
                //TransitionTerrain::changeTileLayerOrder(tiledMap);
            }
            if(config.displaycity)
                LoadMapAll::addDebugCity(tiledMap,config.mapWidth,config.mapHeight);
            //the hole every town was laid out in, with its key numbers on one line
            if(config.cityDebug)
                LoadMapAll::addDebugCityLimits(tiledMap,config);
            //the outline of every sea and lake
            if(config.terrainDebug)
                LoadMapAll::addDebugWaterBodies(tiledMap,config);
            if(config.dominimap)
            {
                t.start();
                //MiniMap::makeMap(heighmap,moisuremap,noiseMapScaleMoisure,noiseMapScaleMap,tiledMap.width(),tiledMap.height(),miniMapDivisor).save(QCoreApplication::applicationDirPath()+"/miniMapLinear.png","PNG");
                MiniMapAll::makeMapTiled(tiledMap.width(),tiledMap.height(),config.mapWidth,config.mapHeight).save(QCoreApplication::applicationDirPath()+"/miniMapPixel.png","PNG");
                qDebug("dominimap %lld ms", t.elapsed());
            }
            //both shores of every boat crossing are painted now: point each
            //teleport at the cell the other side moored on
            LoadMapAll::wireBoatCrossings();
            //template/on-<terrain>/ decorations, BEFORE the vegetation so each one
            //can mask the trees off its own footprint
            {
                t.start();
                LoadMapAll::addTerrainDecorations(tiledMap,config);
                qDebug("terrain decorations took %lld ms", t.elapsed());
            }
            if(config.dovegetation)
            {
                //Keep towns readable like the hand-made reference (open interior +
                //a tree-wall border): pre-mask every CITY chunk in the vegetation
                //mask so addVegetation (which only OR-s the mask, never clears it)
                //scatters NO trees inside a town.  The terrain forest already
                //carved around the town stays as the wall; only the cluttering
                //interior clumps are suppressed.
                //the town holes were masked before the transitions already
                //(maskCityHoles), so no tree is planted inside a town here; the
                //frame of trees AROUND it is exactly the unmasked rest of the chunk.
                //A cave chunk keeps NATURAL vegetation everywhere (the cave is
                //interior-only), its mouth pockets are masked at carve time.
                t.start();
                MapPlants::addVegetation(tiledMap,VoronioForTiledMapTmx::voronoiMap);
                //a tree canopy (WalkBehind) must never hide a city sign
                LoadMapAll::reassertCitySigns(tiledMap);
                qDebug("Vegetation took %lld ms", t.elapsed());
            }
            t.start();
            //flavour townsfolk on the open city ground: LAST pass, after the
            //buildings AND after the vegetation, else a tree brushed on the city
            //border ring (only the town INTERIOR is masked) lands on an NPC that
            //was already placed and the character is drawn inside the trunk.
            LoadMapAll::addCityTownsfolk(tiledMap, config, config.mapWidth, config.mapHeight);
            qDebug("add city townsfolk took %lld ms", t.elapsed());
            //WALKABILITY GUARD, on the FINAL map: it has to run after the last
            //pass that can add a COLLISION — the terrain decorations and the
            //vegetation both do, and a tree dropped on the road would otherwise
            //cut a chunk in two behind the guard's back.
            {
                //NO ISOLATED MAP: the sea routes join the land masses the land
                //router cannot, so every written map is reachable from the start
                //town — walking or by boat. A world where it is not is broken and
                //is not written at all.
                std::vector<std::string> walkErrors;
                if(!LoadMapAll::checkNoIsolatedMap(config,walkErrors))
                {
                    std::cerr << walkErrors.size() << " map(s) cannot be reached from the start town,"
                                 " nothing was generated:" << std::endl;
                    unsigned int errorIndex=0;
                    while(errorIndex<walkErrors.size() && errorIndex<10)
                    {
                        std::cerr << "  " << walkErrors.at(errorIndex) << std::endl;
                        errorIndex++;
                    }
                    if(walkErrors.size()>10)
                        std::cerr << "  ... and " << (walkErrors.size()-10) << " more" << std::endl;
                    return 1;
                }
                if(!LoadMapAll::checkWalkability(tiledMap,config,walkErrors))
                {
                    std::cerr << walkErrors.size() << " walkability error(s), nothing was generated:" << std::endl;
                    unsigned int errorIndex=0;
                    while(errorIndex<walkErrors.size() && errorIndex<40)
                    {
                        std::cerr << "  " << walkErrors.at(errorIndex) << std::endl;
                        errorIndex++;
                    }
                    if(walkErrors.size()>40)
                        std::cerr << "  ... and " << (walkErrors.size()-40) << " more" << std::endl;
                    return 1;
                }
            }
            t.start();
            {
                Tiled::ObjectGroup *layerZoneChunk=new Tiled::ObjectGroup("Chunk",0,0); // ObjectGroup contructor no longer accepts width and height 
                layerZoneChunk->setColor(QColor("#ffe000"));
                tiledMap.addLayer(layerZoneChunk);

                unsigned int mapY=0;
                while(mapY<config.mapYCount)
                {
                    unsigned int mapX=0;
                    while(mapX<config.mapXCount)
                    {
                        //the REAL name the chunk is kept under (city, road + step,
                        //cave); the x,y grid position is already the polygon itself.
                        //Chunks that produce no map keep their coordinates so the
                        //grid stays complete when the layer is shown in Tiled.
                        QString chunkName=QString::fromStdString(LoadMapAll::chunkDebugName(mapX,mapY));
                        if(chunkName.isEmpty())
                            chunkName=QString::number(mapX)+","+QString::number(mapY);
                        Tiled::MapObject *object = new Tiled::MapObject(chunkName,"",QPointF(0,0), QSizeF(0.0,0.0));

                        unsigned int tiles_x = mapX*config.mapWidth;
                        unsigned int tiles_y = mapY*config.mapHeight;

                        // Convert to pixel units when creating a new Tiled::MapObject
                        // FIX: API change in v0.10.x - MapObjects now use pixel units instead of tile units
                        unsigned int pixels_x = tiles_x * tiledMap.tileWidth();
                        unsigned int pixels_y = tiles_y * tiledMap.tileHeight();

                        object->setPolygon(QPolygonF(QRectF(pixels_x,pixels_y,config.mapWidth*tiledMap.tileWidth(),config.mapHeight*tiledMap.tileHeight())));
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

#ifdef TILED_CSV
                tiledMap.setLayerDataFormat(Tiled::Map::CSV);  // DEBUG
#endif

                //same rule as the chunks: point every tileset at the copy shipped
                //inside the label, so this dump too stays readable once the folder
                //is copied into a datapack. Restored right after, the world map is
                //still the source of the chunks.
                std::vector<std::pair<Tiled::Tileset *,QString> > tilesetNames;
                {
                    int tilesetIndex=0;
                    while(tilesetIndex<tiledMap.tilesetCount())
                    {
                        Tiled::Tileset * const tileset=tiledMap.tilesetAt(tilesetIndex).get();
                        const QString shipped=LoadMap::shipTileset(
                                    LoadMap::pooledTileset(QFileInfo(tileset->fileName()).fileName()));
                        if(!tileset->fileName().isEmpty() && !shipped.isEmpty())
                        {
                            tilesetNames.push_back(std::pair<Tiled::Tileset *,QString>(tileset,tileset->fileName()));
                            tileset->setFileName(shipped);
                        }
                        tilesetIndex++;
                    }
                }
                const bool allWritten=maprwriter.writeMap(&tiledMap,LoadMap::destMainDir()+"all.tmx");
                {
                    unsigned int restoreIndex=0;
                    while(restoreIndex<tilesetNames.size())
                    {
                        tilesetNames.at(restoreIndex).first->setFileName(tilesetNames.at(restoreIndex).second);
                        restoreIndex++;
                    }
                }
                if(!allWritten)
                {
                    std::cerr << "Unable to write " << LoadMap::destMainDir().toStdString() << "all.tmx" << std::endl;
                    abort();
                }
                qDebug("Write all.tmx %lld ms", t.elapsed());
            }
        }
        //do tmx split
        t.start();
        //one start point per city whose level falls in the LOWEST 10% of the
        //level range; the lowest-level one is the "Normal" profile start
        std::vector<CityStart> cityStarts;
        const float startLevelLimit=(float)config.levelmapmin+(float)(config.levelmapmax-config.levelmapmin)*0.10f;
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
                //inline <bot> defs for the townsfolk that landed in this city chunk
                const std::string cityBotXml=LoadMapAll::emitCityBotsForChunk(tiledMap,x,y,singleMapWitdh,singleMapHeight,config).toStdString();
                if(!PartialMap::save(tiledMap,
                                 x*singleMapWitdh,y*singleMapHeight,
                                 x*singleMapWitdh+singleMapWitdh,y*singleMapHeight+singleMapHeight,
                                 file,
                                 newRecuesPoints,
                                 "city",cityLowerCaseName,city.name,
                                 cityBotXml
                                 ))
                {
                    std::cerr << "Unable to write " << file << "" << std::endl;
                    abort();
                }
                if((float)city.level<=startLevelLimit)
                {
                    if(newRecuesPoints.empty())
                    {
                        if(config.levelmapmin==city.level)
                        {
                            std::cerr << "newRecuesPoints empty for city (abort)" << std::endl;
                            abort();
                        }
                    }
                    else
                    {
                        CityStart cityStart;
                        cityStart.cityName=cityLowerCaseName;
                        cityStart.point=newRecuesPoints.front();
                        cityStart.level=city.level;
                        cityStarts.push_back(cityStart);
                    }
                }
                recuesPoints.insert(recuesPoints.cend(),newRecuesPoints.cbegin(),newRecuesPoints.cend());
                if(LoadMapAll::zones.find(cityLowerCaseName)==LoadMapAll::zones.cend())
                {
                    QFile xmlinfo(LoadMap::destMainDir()+"zone/"+QString::fromStdString(cityLowerCaseName)+".xml");
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
                                QFile xmlinfo(LoadMap::destMainDir()+"zone/"+QString::fromStdString(cityLowerCaseName)+".xml");
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
                        //a cave chunk overworld is natural terrain: no tall grass,
                        //no encounters, no trainers — all of that lives in the
                        //separate -cave interior map
                        if(!roadIndex.roadMonsters.empty() && !roadIndex.isCave)
                        {
                            additionalXmlInfo+="  <grass>\n";
                            unsigned int roadMonsterIndex=0;
                            while(roadMonsterIndex<roadIndex.roadMonsters.size())
                            {
                                const LoadMapAll::RoadMonster &roadMonster=roadIndex.roadMonsters.at(roadMonsterIndex);
                                //monster by lowercase NAME when known; min==max collapses to level=
                                additionalXmlInfo+="    <monster id=\""+LoadMapAll::monsterRef(roadMonster.monsterId,config).toStdString()+"\"";
                                if(roadMonster.minLevel==roadMonster.maxLevel)
                                    additionalXmlInfo+=" level=\""+std::to_string(roadMonster.minLevel)+"\"";
                                else
                                    additionalXmlInfo+=" minLevel=\""+std::to_string(roadMonster.minLevel)+
                                            "\" maxLevel=\""+std::to_string(roadMonster.maxLevel)+"\"";
                                additionalXmlInfo+=" luck=\""+std::to_string(roadMonster.luck)+"\"/>\n";
                                roadMonsterIndex++;
                            }
                            additionalXmlInfo+="  </grass>\n";
                        }
                        //inline trainer <bot> defs for this chunk (renumbers the
                        //chunk's world bot objects to local ids) — the engine reads
                        //bots only from the map's own .xml.
                        additionalXmlInfo+=LoadMapAll::emitRoadBotsForChunk(tiledMap,x,y,singleMapWitdh,singleMapHeight,roadIndex,config).toStdString();
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
                        //the cave interior: painted over the (already saved) region
                        //and written as <chunk>-cave.tmx, reached through the mouth
                        if(roadIndex.isCave)
                        {
                            if(!LoadMapAll::writeCaveInterior(tiledMap,x,y,singleMapWitdh,singleMapHeight,
                                                              roadIndex,config,file,zoneName))
                            {
                                std::cerr << "Unable to write the cave interior of " << file << std::endl;
                                abort();
                            }
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
        qDebug("Write chunk tmx %lld ms", t.elapsed());
        //road trainer bots are now emitted inline per chunk during the split
        //loop above (LoadMapAll::emitRoadBotsForChunk via additionalXmlInfo).
        //do the start point
        QFile start(LoadMap::destMainDir()+"start.xml");
        if(start.open(QFile::WriteOnly))
        {
            if(cityStarts.empty())
            {
                std::cerr << "no city start point found (abort)" << std::endl;
                abort();
            }
            //lowest level first (then by name): that one is the "Normal" profile;
            //the other low-level cities are extra profiles named after the city
            //(active once a matching profile id exists in player/start.xml)
            std::sort(cityStarts.begin(),cityStarts.end(),cityStartLess);
            QString content("<!--\n"
                            "/!\\ warning, directly put this information into db\n"
                            "/!\\ not check if x,y is into the range of the map\n"
                            "-->\n"
                            "<profile>\n");
            unsigned int indexStart=0;
            while(indexStart<cityStarts.size())
            {
                const CityStart &cityStart=cityStarts.at(indexStart);
                QString startId("Normal");
                if(indexStart>0)
                    startId=QString::fromStdString(cityStart.cityName);
                content+="  <start id=\""+startId+"\">\n"
                         "    <map x=\""+QString::number(cityStart.point.x)+"\" y=\""+QString::number(cityStart.point.y)+"\" file=\""+QString::fromStdString(cityStart.point.map)+"\"/>\n"
                         "  </start>\n";
                indexStart++;
            }
            content+="</profile>";
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
    //the npc lines that were generated, with their city context: npcfill.py
    //rewrites them with the local LLM (the datapack is already valid without it)
    LoadMapAll::writeNpcSlots(config);

    //a tileset the staging pool holds but no generated map references only makes
    //the next run slower and the folder confusing; the pool is refilled at startup
    if(config.cleanTileset)
        LoadMap::cleanTilesetPool();

    qDebug("Total time %lld ms", total.elapsed());

    return 0;
}
