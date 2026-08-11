#include "LoadMap.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QCoreApplication>
#include <QRegularExpression>
#include <iostream>

#include "../../general/base/GeneralVariable.hpp"

#include <libtiled/mapreader.h>
#include <libtiled/tileset.h>
#include <libtiled/objectgroup.h>
#include <libtiled/mapobject.h>

#include "../../general/base/cpp11addition.hpp"
#include "TerrainShaper.h"

extern std::vector<Tiled::SharedTileset> LoadMap_tilesets_hack;
extern std::vector<std::unique_ptr<Tiled::Map>> LoadMap_map_hack;

LoadMap::Terrain LoadMap::terrainList[5][6];
QStringList LoadMap::terrainFlatList;
QHash<QString,LoadMap::Terrain *> LoadMap::terrainNameToObject;
std::vector<LoadMap::GroupedTerrain> LoadMap::groupedTerrainList;

unsigned int LoadMap::floatToHigh(const float f)
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

unsigned int LoadMap::floatToMoisure(const float f)
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

static QString loadMapMainCode=QString::fromLatin1(DATAPACK_MAINCODE_GENERATED);

const QString &LoadMap::mainCode()
{
    return loadMapMainCode;
}

void LoadMap::resolveMainCode(QSettings &settings)
{
    const QString configured=settings.value("maincode").toString().trimmed();
    //CATCHCHALLENGER_CHECK_MAINDATAPACKCODE: the engine only accepts [a-z0-9]+ (the
    //"-" is the separator of the http datapack download paths)
    const QRegularExpression valid(QString::fromLatin1(CATCHCHALLENGER_CHECK_MAINDATAPACKCODE));
    if(!configured.isEmpty() && valid.match(configured).hasMatch() &&
            configured!=QString::fromLatin1(DATAPACK_MAINCODE_TEST))
        loadMapMainCode=configured;
    else
    {
        if(!configured.isEmpty())
            std::cerr << "maincode \"" << configured.toStdString()
                      << "\" of the settings is not a usable generated map label, using \""
                      << DATAPACK_MAINCODE_GENERATED << "\"" << std::endl;
        loadMapMainCode=QString::fromLatin1(DATAPACK_MAINCODE_GENERATED);
        settings.setValue("maincode",loadMapMainCode);
        settings.sync();
    }
}

QString LoadMap::destMainDir()
{
    return QCoreApplication::applicationDirPath()+"/dest/map/main/"+mainCode()+"/";
}

QString LoadMap::shippedTilesetDir()
{
    return destMainDir()+"tileset/";
}

QString LoadMap::pooledTileset(const QString &fileName)
{
    const QString pool=QCoreApplication::applicationDirPath()+"/dest/map/tileset/"+fileName;
    if(QFile::exists(pool))
        return pool;
    const QString mainPool=QCoreApplication::applicationDirPath()+"/dest/map/main/tileset/"+fileName;
    if(QFile::exists(mainPool))
        return mainPool;
    return QString();
}

//the two dirs of the run staging pool: dest/map/tileset/ is the canonical one,
//dest/map/main/tileset/ is the path the "main/tileset/x.tsx" settings values use
static QStringList tilesetPoolDirs()
{
    return QStringList()
            <<(QCoreApplication::applicationDirPath()+"/dest/map/tileset/")
            <<(QCoreApplication::applicationDirPath()+"/dest/map/main/tileset/");
}

bool LoadMap::stageTilesetPool(const QString &sourceDir)
{
    const QDir source(sourceDir);
    if(!source.exists())
        return true;//nothing to stage from, not an error: the pool may already be filled
    const QStringList files=source.entryList(QDir::Files,QDir::Name);
    const QStringList destinations=tilesetPoolDirs();
    unsigned int staged=0;
    int destinationIndex=0;
    while(destinationIndex<destinations.size())
    {
        const QString &destination=destinations.at(destinationIndex);
        if(!QDir().mkpath(destination))
        {
            std::cerr << "Unable to create " << destination.toStdString() << std::endl;
            return false;
        }
        int fileIndex=0;
        while(fileIndex<files.size())
        {
            const QString target=destination+files.at(fileIndex);
            //never overwrite: the first stager wins, so --datapack keeps priority
            //over the tilesets the tool ships itself
            if(!QFile::exists(target))
            {
                if(!QFile::copy(source.absoluteFilePath(files.at(fileIndex)),target))
                {
                    std::cerr << "Unable to stage " << files.at(fileIndex).toStdString()
                              << " into " << destination.toStdString() << std::endl;
                    return false;
                }
                staged++;
            }
            fileIndex++;
        }
        destinationIndex++;
    }
    if(staged>0)
        std::cout << "Staged " << staged << " tileset file(s) from "
                  << source.absolutePath().toStdString() << " into the run pool" << std::endl;
    return true;
}

//the image files a tsx references, by file NAME (they sit next to it)
static QStringList tilesetImageNames(const QString &tsxPath)
{
    QStringList images;
    QFile tsxFile(tsxPath);
    if(!tsxFile.open(QFile::ReadOnly))
        return images;
    const QString content=QString::fromUtf8(tsxFile.readAll());
    tsxFile.close();
    int imageIndex=content.indexOf("<image source=\"");
    while(imageIndex>=0)
    {
        const int start=imageIndex+(int)QString("<image source=\"").size();
        const int end=content.indexOf("\"",start);
        if(end>start)
            images << QFileInfo(content.mid(start,end-start)).fileName();
        imageIndex=content.indexOf("<image source=\"",imageIndex+1);
    }
    return images;
}

void LoadMap::cleanTilesetPool()
{
    //what the generated maps really reference is exactly what shipTileset() copied
    //into the label; everything else in the pool was staged and never used
    const QDir shipped(shippedTilesetDir());
    if(!shipped.exists())
    {
        std::cerr << "cleanTileset: no " << shippedTilesetDir().toStdString()
                  << ", the pool is left untouched" << std::endl;
        return;
    }
    QSet<QString> keep;
    const QStringList shippedFiles=shipped.entryList(QDir::Files,QDir::Name);
    int shippedIndex=0;
    while(shippedIndex<shippedFiles.size())
    {
        const QString &name=shippedFiles.at(shippedIndex);
        keep.insert(name);
        if(name.endsWith(".tsx"))
        {
            const QStringList images=tilesetImageNames(shipped.absoluteFilePath(name));
            int imageIndex=0;
            while(imageIndex<images.size())
            {
                keep.insert(images.at(imageIndex));
                imageIndex++;
            }
        }
        shippedIndex++;
    }
    unsigned int dropped=0;
    const QStringList pools=tilesetPoolDirs();
    int poolIndex=0;
    while(poolIndex<pools.size())
    {
        const QDir pool(pools.at(poolIndex));
        if(pool.exists())
        {
            const QStringList poolFiles=pool.entryList(QDir::Files,QDir::Name);
            int fileIndex=0;
            while(fileIndex<poolFiles.size())
            {
                const QString &name=poolFiles.at(fileIndex);
                if(!keep.contains(name))
                {
                    if(QFile::remove(pool.absoluteFilePath(name)))
                        dropped++;
                    else
                        std::cerr << "cleanTileset: unable to drop "
                                  << pool.absoluteFilePath(name).toStdString() << std::endl;
                }
                fileIndex++;
            }
        }
        poolIndex++;
    }
    std::cout << "cleanTileset: " << keep.size() << " file(s) used, " << dropped
              << " dropped from the run pool" << std::endl;
}

bool LoadMap::copyTilesetWithImages(const QString &sourceTsx,const QString &destinationDir)
{
    const QFileInfo source(sourceTsx);
    if(!QDir().mkpath(destinationDir))
    {
        std::cerr << "Unable to create " << destinationDir.toStdString() << std::endl;
        return false;
    }
    QFile tsxFile(source.absoluteFilePath());
    if(!tsxFile.open(QFile::ReadOnly))
    {
        std::cerr << "Unable to read the tileset " << source.absoluteFilePath().toStdString() << std::endl;
        return false;
    }
    const QString content=QString::fromUtf8(tsxFile.readAll());
    tsxFile.close();
    //the tsx references its image relatively, so the image goes next to it
    int imageIndex=content.indexOf("<image source=\"");
    while(imageIndex>=0)
    {
        const int start=imageIndex+(int)QString("<image source=\"").size();
        const int end=content.indexOf("\"",start);
        if(end>start)
        {
            const QString image=content.mid(start,end-start);
            const QString imageSource=QFileInfo(source.absolutePath()+"/"+image).absoluteFilePath();
            const QString imageDestination=destinationDir+QFileInfo(image).fileName();
            if(!QFile::exists(imageDestination))
                if(!QFile::copy(imageSource,imageDestination))
                {
                    std::cerr << "Unable to copy the tileset image " << imageSource.toStdString()
                              << " into " << imageDestination.toStdString() << std::endl;
                    return false;
                }
        }
        imageIndex=content.indexOf("<image source=\"",imageIndex+1);
    }
    const QString destination=destinationDir+source.fileName();
    if(!QFile::exists(destination))
        if(!QFile::copy(source.absoluteFilePath(),destination))
        {
            std::cerr << "Unable to copy the tileset " << source.absoluteFilePath().toStdString()
                      << " into " << destination.toStdString() << std::endl;
            return false;
        }
    return true;
}

//The generated maps are ONE datapack map label: the owner copies
//dest/map/main/<label>/ into map/main/<label>/ of a datapack, and that datapack
//does not have to own the tilesets the generator uses (it holds none of the
//gym/heal/shop/house sheets the tool ships). So the label carries its OWN
//tileset/ dir and every written reference points there (tileset/x.tsx next to
//the maps); the label folder is then self-contained and copyable ANYWHERE, and
//map/tileset/ of the target datapack is never written. dest/map/tileset/ and
//dest/map/main/tileset/ stay the RUN STAGING pool where the datapack tilesets
//(--datapack) and the tool ones are collected, and only what a map really
//references is shipped out of it.
QString LoadMap::shipTileset(const QString &tsxPath)
{
    const QFileInfo source(tsxPath);
    const QString destination=shippedTilesetDir()+source.fileName();
    if(QFileInfo(destination).absoluteFilePath()==source.absoluteFilePath())
        return destination;
    if(!QFile::exists(destination))
    {
        if(!QFile::exists(source.absoluteFilePath()))
            return QString();
        if(!copyTilesetWithImages(source.absoluteFilePath(),shippedTilesetDir()))
            abort();
    }
    return destination;
}

Tiled::Tileset *LoadMap::readTileset(const QString &tsx,Tiled::Map *tiledMap)
{
    QDir mapDir(LoadMap::destMainDir());

    //dest/map/tileset/ is the canonical pool dir; map/main/tileset/ is only a
    //run-staging convenience (settings paths). The world keeps the POOL path:
    //what a map really uses is shipped into the label at WRITE time (shipTileset),
    //so a tileset no generated map references is never shipped.
    QString tsxResolved=tsx;
    if(tsxResolved.startsWith("main/tileset/"))
    {
        const QString canonical="tileset/"+tsxResolved.mid(QString("main/tileset/").size());
        if(QFile::exists(QCoreApplication::applicationDirPath()+"/dest/map/"+canonical))
            tsxResolved=canonical;
    }

    Tiled::MapReader reader;
    Tiled::SharedTileset tilesetBase=reader.readTileset(QCoreApplication::applicationDirPath()+"/dest/map/"+tsxResolved);
    if(tilesetBase==NULL)
    {
        std::cerr << "File not found: " << QCoreApplication::applicationDirPath().toStdString()
                  << "/dest/map/" << tsxResolved.toStdString() << " " << reader.errorString().toStdString() << std::endl;
        abort();
    }
    /*if(tilesetBase->tileWidth()!=tiledMap->tileWidth())
    {
        std::cerr << "tile Width not match with map tile Width" << std::endl;
        abort();
    }
    if(tilesetBase->tileHeight()!=tiledMap->tileHeight())
    {
        std::cerr << "tile height not match with map tile height" << std::endl;
        abort();
    }*/
    tiledMap->addTileset(tilesetBase);

    // FIX Libtiled v1.3.5: Tiled::SharedTileset.getFileName() no longer returns file path for some reason
    QString tsx_abs_path = QCoreApplication::applicationDirPath()+"/dest/map/"+tsxResolved;
    QString tsx_relative_path = mapDir.relativeFilePath(tsx_abs_path);

    tilesetBase->setFileName(tsx_relative_path);

    LoadMap_tilesets_hack.push_back(tilesetBase);

    return tilesetBase.get(); // TODO: propagate smart pointer
}

Tiled::Map *LoadMap::readMap(const QString &tmx)
{
    // TODO: DEBUG
    //std::cout << "LoadMap::readMap() Called with tmx = " << tmx.toStdString() << std::endl;

    Tiled::MapReader reader;
    std::unique_ptr<Tiled::Map> map=reader.readMap(QCoreApplication::applicationDirPath()+"/"+tmx);
    if(map==NULL)
    {
        std::cerr << "File not found: " << QCoreApplication::applicationDirPath().toStdString() << "/" << tmx.toStdString() <<
                  ": " << reader.errorString().toStdString() << std::endl;
        abort();
    }


    Tiled::Map *map_ptr = map.get();

    //A template read from the SOURCE tree carries the path of the source copy of
    //its tilesets. When the run staging pool holds that tileset (dest/map/tileset/,
    //same file), point the template at THAT copy: the world tileset of the same
    //file is then found instead of a duplicate being added (MapBrush pairs them by
    //path), and the written reference is computed from the pool copy.
    {
        const QDir stagedDir(QCoreApplication::applicationDirPath()+"/dest/map/tileset/");
        int tilesetIndex=0;
        while(tilesetIndex<map_ptr->tilesetCount())
        {
            const Tiled::SharedTileset tileset=map_ptr->tilesetAt(tilesetIndex);
            const QString name=tileset->fileName();
            if(!name.isEmpty() && QFileInfo(name).isAbsolute())
            {
                const QString staged=stagedDir.absoluteFilePath(QFileInfo(name).fileName());
                if(QFile::exists(staged) && QFileInfo(name).absoluteFilePath()!=staged)
                    tileset->setFileName(staged);
            }
            tilesetIndex++;
        }
    }

    LoadMap_map_hack.push_back(std::move(map)); //  Hack FIX Libtiled 1.3.x - Tiled::Map is now smart pointer and is deleted automaticaly

    return map_ptr; // TODO: Temp, should just propagate smart pointer
}

Tiled::Tileset *LoadMap::readTilesetWithTileId(const uint32_t &tile,const QString &tsx,Tiled::Map *tiledMap)
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

void LoadMap::loadAllTileset(QHash<QString,Tiled::Tileset *> &cachedTileset,Tiled::Map &tiledMap)
{
    for(int height=0;height<5;height++)
        for(int moisure=0;moisure<6;moisure++)
        {
            LoadMap::Terrain &terrain=LoadMap::terrainList[height][moisure];
            const QString &layerString=terrain.tmp_layerString;
            const QString &terrainName=terrain.terrainName;
            const QString &tsx=terrain.tmp_tsx;
            const QString &transition_tsx=terrain.tmp_transition_tsx;
            const unsigned int &tileId=terrain.tmp_tileId;
            if(!layerString.isEmpty())
            {
                Tiled::Tileset *tilesetBase;
                if(cachedTileset.contains(tsx))
                    tilesetBase=cachedTileset.value(tsx);
                else
                {
                    tilesetBase=readTilesetWithTileId(tileId,tsx,&tiledMap);
                    cachedTileset[tsx]=tilesetBase;
                }
                terrain.tile=tilesetBase->tileAt(tileId);
                if(terrain.outsideBorder)
                    terrain.tileLayer=searchTileLayerByName(tiledMap,"[T]"+terrainName);
                else
                    terrain.tileLayer=searchTileLayerByName(tiledMap,terrain.tmp_layerString);

                //load the transition tile
                Tiled::Tileset *tilesetTransition;
                if(cachedTileset.contains(transition_tsx))
                    tilesetTransition=cachedTileset.value(transition_tsx);
                else
                {
                    tilesetTransition=readTileset(transition_tsx,&tiledMap);
                    cachedTileset[transition_tsx]=tilesetTransition;
                }
                unsigned int index=0;
                while(index<terrain.tmp_transition_tile.size())
                {
                    terrain.transition_tile.push_back(tilesetTransition->tileAt(terrain.tmp_transition_tile.at(index)));
                    index++;
                }

                LoadMap::terrainNameToObject.insert(terrain.terrainName,&terrain);
                if(!terrainFlatList.contains(terrain.terrainName))
                    terrainFlatList << terrain.terrainName;
            }
        }
    unsigned int groupedTerrainIndex=0;
    while(groupedTerrainIndex<groupedTerrainList.size())
    {
        GroupedTerrain &groupedTerrain=groupedTerrainList[groupedTerrainIndex];

        groupedTerrain.tileLayer=searchTileLayerByName(tiledMap,groupedTerrain.tmp_layerString);

        //load the transition tile
        Tiled::Tileset *tilesetTransition;
        if(cachedTileset.contains(groupedTerrain.tmp_transition_tsx))
            tilesetTransition=cachedTileset.value(groupedTerrain.tmp_transition_tsx);
        else
        {
            tilesetTransition=readTileset(groupedTerrain.tmp_transition_tsx,&tiledMap);
            cachedTileset[groupedTerrain.tmp_transition_tsx]=tilesetTransition;
        }
        unsigned int index=0;
        while(index<groupedTerrain.tmp_transition_tile.size())
        {
            groupedTerrain.transition_tile.push_back(tilesetTransition->tileAt(groupedTerrain.tmp_transition_tile.at(index)));
            index++;
        }

        groupedTerrainIndex++;
    }
}

Tiled::ObjectGroup *LoadMap::addDebugLayer(Tiled::Map &tiledMap,std::vector<std::vector<Tiled::ObjectGroup *> > &arrayTerrain,bool polygon)
{
    QString addText;
    if(polygon==true)
        addText=" (Polygon)";
    else
        addText=" (Tile)";

    // ObjectGroup constructor no longer needs width and height
    Tiled::ObjectGroup *layerZoneWater=new Tiled::ObjectGroup("WaterZone"+addText,0,0);
    layerZoneWater->setColor(QColor("#6273cc"));
    tiledMap.addLayer(layerZoneWater);

    Tiled::ObjectGroup *layerZoneSnow=new Tiled::ObjectGroup("Snow"+addText,0,0);
    layerZoneSnow->setColor(QColor("#ffffff"));
    tiledMap.addLayer(layerZoneSnow);
    Tiled::ObjectGroup *layerZoneTundra=new Tiled::ObjectGroup("Tundra"+addText,0,0);
    layerZoneTundra->setColor(QColor("#ddddbb"));
    tiledMap.addLayer(layerZoneTundra);
    Tiled::ObjectGroup *layerZoneBare=new Tiled::ObjectGroup("Bare"+addText,0,0);
    layerZoneBare->setColor(QColor("#bbbbbb"));
    tiledMap.addLayer(layerZoneBare);
    Tiled::ObjectGroup *layerZoneScorched=new Tiled::ObjectGroup("Scorched"+addText,0,0);
    layerZoneScorched->setColor(QColor("#999999"));
    tiledMap.addLayer(layerZoneScorched);
    Tiled::ObjectGroup *layerZoneTaiga=new Tiled::ObjectGroup("Taiga"+addText,0,0);
    layerZoneTaiga->setColor(QColor("#ccd4bb"));
    tiledMap.addLayer(layerZoneTaiga);
    Tiled::ObjectGroup *layerZoneShrubland=new Tiled::ObjectGroup("Shrubland"+addText,0,0);
    layerZoneShrubland->setColor(QColor("#c4ccbb"));
    tiledMap.addLayer(layerZoneShrubland);
    Tiled::ObjectGroup *layerZoneTemperateDesert=new Tiled::ObjectGroup("Temperate Desert"+addText,0,0);
    layerZoneTemperateDesert->setColor(QColor("#e4e8ca"));
    tiledMap.addLayer(layerZoneTemperateDesert);
    Tiled::ObjectGroup *layerZoneTemperateRainForest=new Tiled::ObjectGroup("Temperate Rain Forest"+addText,0,0);
    layerZoneTemperateRainForest->setColor(QColor("#a4c4a8"));
    tiledMap.addLayer(layerZoneTemperateRainForest);
    Tiled::ObjectGroup *layerZoneTemperateDeciduousForest=new Tiled::ObjectGroup("Temperate Deciduous Forest"+addText,0,0);
    layerZoneTemperateDeciduousForest->setColor(QColor("#b4c9a9"));
    tiledMap.addLayer(layerZoneTemperateDeciduousForest);
    Tiled::ObjectGroup *layerZoneGrassland=new Tiled::ObjectGroup("Grassland"+addText,0,0);
    layerZoneGrassland->setColor(QColor("#c4d4aa"));
    tiledMap.addLayer(layerZoneGrassland);
    Tiled::ObjectGroup *layerZoneTropicalRainForest=new Tiled::ObjectGroup("Tropical Rain Forest"+addText,0,0);
    layerZoneTropicalRainForest->setColor(QColor("#9cbba9"));
    tiledMap.addLayer(layerZoneTropicalRainForest);
    Tiled::ObjectGroup *layerZoneTropicalSeasonalForest=new Tiled::ObjectGroup("Tropical Seasonal Forest"+addText,0,0);
    layerZoneTropicalSeasonalForest->setColor(QColor("#a9cca4"));
    tiledMap.addLayer(layerZoneTropicalSeasonalForest);
    Tiled::ObjectGroup *layerZoneSubtropicalDesert=new Tiled::ObjectGroup("Subtropical Desert"+addText,0,0);
    layerZoneSubtropicalDesert->setColor(QColor("#e9ddc7"));
    tiledMap.addLayer(layerZoneSubtropicalDesert);

    arrayTerrain.resize(4);
    //high 1
    arrayTerrain[0].resize(6);
    arrayTerrain[0][0]=layerZoneSubtropicalDesert;//Moisture 1
    arrayTerrain[0][1]=layerZoneGrassland;
    arrayTerrain[0][2]=layerZoneTropicalSeasonalForest;
    arrayTerrain[0][3]=layerZoneTropicalSeasonalForest;
    arrayTerrain[0][4]=layerZoneTropicalRainForest;
    arrayTerrain[0][5]=layerZoneTropicalRainForest;
    //high 2
    arrayTerrain[1].resize(6);
    arrayTerrain[1][0]=layerZoneTemperateDesert;//Moisture 1
    arrayTerrain[1][1]=layerZoneGrassland;
    arrayTerrain[1][2]=layerZoneGrassland;
    arrayTerrain[1][3]=layerZoneTemperateDeciduousForest;
    arrayTerrain[1][4]=layerZoneTemperateDeciduousForest;
    arrayTerrain[1][5]=layerZoneTemperateRainForest;
    //high 3
    arrayTerrain[2].resize(6);
    arrayTerrain[2][0]=layerZoneTemperateDesert;//Moisture 1
    arrayTerrain[2][1]=layerZoneTemperateDesert;
    arrayTerrain[2][2]=layerZoneShrubland;
    arrayTerrain[2][3]=layerZoneShrubland;
    arrayTerrain[2][4]=layerZoneTaiga;
    arrayTerrain[2][5]=layerZoneTaiga;
    //high 4
    arrayTerrain[3].resize(6);
    arrayTerrain[3][0]=layerZoneScorched;//Moisture 1
    arrayTerrain[3][1]=layerZoneBare;
    arrayTerrain[3][2]=layerZoneTundra;
    arrayTerrain[3][3]=layerZoneSnow;
    arrayTerrain[3][4]=layerZoneSnow;
    arrayTerrain[3][5]=layerZoneSnow;

    return layerZoneWater;
}

Tiled::TileLayer *LoadMap::addTerrainLayer(Tiled::Map &tiledMap,const bool dotransition)
{
    (void)dotransition;
    Tiled::TileLayer *layerZoneWater=new Tiled::TileLayer("Water",0,0,tiledMap.width(),tiledMap.height());
    tiledMap.addLayer(layerZoneWater);
    Tiled::TileLayer *layerZoneOnWater=new Tiled::TileLayer("OnWater",0,0,tiledMap.width(),tiledMap.height());
    tiledMap.addLayer(layerZoneOnWater);
    Tiled::TileLayer *layerZoneWalkable=new Tiled::TileLayer("Walkable",0,0,tiledMap.width(),tiledMap.height());
    tiledMap.addLayer(layerZoneWalkable);
    Tiled::TileLayer *layerZoneOnGrass=new Tiled::TileLayer("OnGrass",0,0,tiledMap.width(),tiledMap.height());
    tiledMap.addLayer(layerZoneOnGrass);
    Tiled::TileLayer *layerZoneOnGrass2=new Tiled::TileLayer("OnGrass2",0,0,tiledMap.width(),tiledMap.height());
    tiledMap.addLayer(layerZoneOnGrass2);
    Tiled::TileLayer *layerZoneOnGrass3=new Tiled::TileLayer("OnGrass3",0,0,tiledMap.width(),tiledMap.height());
    tiledMap.addLayer(layerZoneOnGrass3);
    Tiled::TileLayer *layerZoneGrass=new Tiled::TileLayer("Grass",0,0,tiledMap.width(),tiledMap.height());
    tiledMap.addLayer(layerZoneGrass);
    Tiled::TileLayer *layerZoneLedgesDown=new Tiled::TileLayer("LedgesDown",0,0,tiledMap.width(),tiledMap.height());
    tiledMap.addLayer(layerZoneLedgesDown);
    Tiled::TileLayer *layerZoneLedgesLeft=new Tiled::TileLayer("LedgesLeft",0,0,tiledMap.width(),tiledMap.height());
    tiledMap.addLayer(layerZoneLedgesLeft);
    Tiled::TileLayer *layerZoneLedgesRight=new Tiled::TileLayer("LedgesRight",0,0,tiledMap.width(),tiledMap.height());
    tiledMap.addLayer(layerZoneLedgesRight);
    Tiled::TileLayer *layerZoneCollisions=new Tiled::TileLayer("Collisions",0,0,tiledMap.width(),tiledMap.height());
    tiledMap.addLayer(layerZoneCollisions);
    Tiled::TileLayer *layerZoneWalkBehind=new Tiled::TileLayer("WalkBehind",0,0,tiledMap.width(),tiledMap.height());
    tiledMap.addLayer(layerZoneWalkBehind);
    Tiled::TileLayer *layerZoneWalkBehind2=new Tiled::TileLayer("WalkBehind",0,0,tiledMap.width(),tiledMap.height());
    tiledMap.addLayer(layerZoneWalkBehind2);
    Tiled::ObjectGroup *layerMoving=new Tiled::ObjectGroup("Moving",0,0);
    tiledMap.addLayer(layerMoving);

    //add temporary layer
    QSet<QString> terrainLayerNameAlreadySet;
    for(int height=0;height<5;height++)
        for(int moisure=0;moisure<6;moisure++)
        {
            const LoadMap::Terrain &terrain=LoadMap::terrainList[height][moisure];
            const QString &terrainName=terrain.terrainName;
            if(terrain.outsideBorder)
                if(!terrainLayerNameAlreadySet.contains(terrainName))
                {
                    Tiled::TileLayer *layerZoneterrainName=new Tiled::TileLayer("[T]"+terrainName,0,0,tiledMap.width(),tiledMap.height());
                    tiledMap.addLayer(layerZoneterrainName);
                    terrainLayerNameAlreadySet << terrainName;
                }
        }

    //add invisible tileset
    QDir mapDir(LoadMap::destMainDir());
    const QString tilesetPath=pooledTileset("invisible.tsx");
    if(tilesetPath.isEmpty())
    {
        std::cerr << "invisible.tsx not staged, neither in dest/map/tileset/ nor in dest/map/main/tileset/" << std::endl;
        abort();
    }
    Tiled::MapReader reader;
    Tiled::SharedTileset tilesetBase=reader.readTileset(tilesetPath);
    if(tilesetBase==NULL)
    {
        std::cerr << "File not found: " << tilesetPath.toStdString() << std::endl;
        abort();
    }
    tiledMap.addTileset(tilesetBase);
    tilesetBase->setFileName(mapDir.relativeFilePath(tilesetPath));

    LoadMap_tilesets_hack.push_back(tilesetBase);

    return layerZoneWater;
}

void LoadMap::addPolygoneTerrain(std::vector<std::vector<Tiled::ObjectGroup *> > &arrayTerrainPolygon,Tiled::ObjectGroup *layerZoneWaterPolygon,
                        std::vector<std::vector<Tiled::ObjectGroup *> > &arrayTerrainTile,Tiled::ObjectGroup *layerZoneWaterTile,
                        const Grid &grid,
                        const VoronioForTiledMapTmx::PolygonZoneMap &vd,const Simplex &heightmap,const Simplex &moisuremap,
                        const float &noiseMapScaleMoisure,const float &noiseMapScaleMap,
                        const int widthMap,const int heightMap,
                        const int offsetX,const int offsetY)
{
    const QPolygonF polyMap(QRectF(-offsetX,-offsetY,widthMap,heightMap));
    unsigned int index=0;
    while(index<grid.size())
    {
        const Point &centroid=grid.at(index);
        const VoronioForTiledMapTmx::PolygonZone &zone=vd.zones.at(index);

        QPolygonF poly;
        poly=zone.polygon;
        poly=poly.intersected(polyMap);
        // TODO: Position of MapObject() may need conversion of units from tiles to pixels (API change introduced in v0.10.0)
        Tiled::MapObject *objectPolygon = new Tiled::MapObject("Zone "+QString::number(index),"",QPointF(offsetX,offsetY), QSizeF(0.0,0.0));
        objectPolygon->setPolygon(poly);
        objectPolygon->setShape(Tiled::MapObject::Polygon);

        QPolygonF polyTile;
        polyTile=zone.pixelizedPolygon;
        polyTile=polyTile.intersected(polyMap);
        // TODO: Position of MapObject() may need conversion of units from tiles to pixels (API change introduced in v0.10.0)
        Tiled::MapObject *objectTile = new Tiled::MapObject("Zone "+QString::number(index),"",QPointF(offsetX,offsetY), QSizeF(0.0,0.0));
        objectTile->setPolygon(polyTile);
        objectTile->setShape(Tiled::MapObject::Polygon);

        const QList<QPointF> &edges=poly.toList();
        if(!edges.isEmpty())
        {
            //const QPointF &edge=edges.first();
            float heightFloat=heightmap.Get({(float)centroid.x()/100,(float)centroid.y()/100},noiseMapScaleMap);
            float moisureFloat=moisuremap.Get({(float)centroid.x()/100,(float)centroid.y()/100},noiseMapScaleMoisure);
            //the grid is stored scaled by SCALE, the shaper works in world tiles
            TerrainShaper::active()->shape((float)centroid.x()/(float)VoronioForTiledMapTmx::SCALE,
                                           (float)centroid.y()/(float)VoronioForTiledMapTmx::SCALE,
                                           heightFloat,moisureFloat);
            const unsigned int height=floatToHigh(heightFloat);
            const unsigned int moisure=floatToMoisure(moisureFloat);
            if(height==0)
            {
                layerZoneWaterPolygon->addObject(objectPolygon);
                if(!polyTile.isEmpty())
                    layerZoneWaterTile->addObject(objectTile);
            }
            else
            {
                arrayTerrainPolygon[height-1][moisure-1]->addObject(objectPolygon);
                if(!polyTile.isEmpty())
                    arrayTerrainTile[height-1][moisure-1]->addObject(objectTile);
            }
        }
        index++;
    }
}

void LoadMap::addTerrain(const Grid &grid,
                        VoronioForTiledMapTmx::PolygonZoneMap &vd, const Simplex &heightmap, const Simplex &moisuremap, const float &noiseMapScaleMoisure,
                        const float &noiseMapScaleMap,
                        const int widthMap, const int heightMap,
                        const int offsetX, const int offsetY, bool draw)
{
    const QPolygonF polyMap(QRectF(-offsetX,-offsetY,widthMap,heightMap));
    unsigned int index=0;
    while(index<grid.size())
    {
        const Point &centroid=grid.at(index);
        VoronioForTiledMapTmx::PolygonZone &zone=vd.zones[index];

        QPolygonF polyTile;
        polyTile=zone.pixelizedPolygon;
        polyTile=polyTile.intersected(polyMap);

        const QList<QPointF> &edges=polyTile.toList();
        if(!edges.isEmpty())
        {
            //const QPointF &edge=edges.first();
            float xMap=(float)centroid.x();
            float yMap=(float)centroid.y();
            zone.heightFloat=heightmap.Get({xMap/100,yMap/100},noiseMapScaleMap);
            zone.moisureFloat=moisuremap.Get({xMap/100,yMap/100},noiseMapScaleMoisure);
            //the ONE place a zone gets its terrain: a shaper may force it flat here.
            //xMap/yMap come from the grid, stored scaled by SCALE; the shaper works
            //in world tiles.
            TerrainShaper::active()->shape(xMap/(float)VoronioForTiledMapTmx::SCALE,
                                           yMap/(float)VoronioForTiledMapTmx::SCALE,
                                           zone.heightFloat,zone.moisureFloat);
            zone.height=floatToHigh(zone.heightFloat);
            zone.moisure=floatToMoisure(zone.moisureFloat);
            Terrain &terrain=LoadMap::terrainList[zone.height][zone.moisure-1];
            if(draw)
            {
                unsigned int pointIndex=0;
                while(pointIndex<zone.points.size())
                {
                    const Point &point=zone.points.at(pointIndex);
                    Tiled::Cell cell;
                    cell.setFlippedHorizontally(false);
                    cell.setFlippedVertically(false);
                    cell.setFlippedAntiDiagonally(false);
                    cell.setTile(terrain.tile);
                    terrain.tileLayer->setCell(point.x(),point.y(),cell);
                    pointIndex++;
                }
            }
        }
        index++;
    }
}

Tiled::TileLayer * LoadMap::searchTileLayerByName(const Tiled::Map &tiledMap,const QString &name)
{
    unsigned int tileLayerIndex=0;
    while(tileLayerIndex<(unsigned int)tiledMap.layerCount())
    {
        Tiled::Layer * const layer=tiledMap.layerAt(tileLayerIndex);
        if(layer->isTileLayer() && layer->name()==name)
            return static_cast<Tiled::TileLayer *>(layer);
        tileLayerIndex++;
    }
    std::cerr << "Unable to found layer with name: " << name.toStdString() << std::endl;
    abort();
}

Tiled::ObjectGroup * LoadMap::searchObjectGroupByName(const Tiled::Map &tiledMap,const QString &name)
{
    unsigned int tileLayerIndex=0;
    while(tileLayerIndex<(unsigned int)tiledMap.layerCount())
    {
        Tiled::Layer * const layer=tiledMap.layerAt(tileLayerIndex);
        if(layer->isObjectGroup() && layer->name()==name)
            return static_cast<Tiled::ObjectGroup *>(layer);
        tileLayerIndex++;
    }
    /*std::cerr << "Unable to found layer with name: " << name.toStdString() << std::endl;
    abort();*/
    return NULL;
}

Tiled::Tileset *LoadMap::searchTilesetByName(const Tiled::Map &tiledMap,const QString &name)
{
    unsigned int tilesetIndex=0;
    while(tilesetIndex<(unsigned int)tiledMap.tilesetCount())
    {
        Tiled::SharedTileset const layer=tiledMap.tilesetAt(tilesetIndex);

        LoadMap_tilesets_hack.push_back(layer);

        if(layer->name()==name)
            return layer.get(); // TODO: temp, should return smartpointer
        tilesetIndex++;
    }
    std::cerr << "Unable to found layer with name: " << name.toStdString() << std::endl;
    std::cerr << std::endl << "Available names: " << std::endl;

    tilesetIndex = 0;
    while(tilesetIndex<(unsigned int)tiledMap.tilesetCount())
    {
        Tiled::SharedTileset const layer=tiledMap.tilesetAt(tilesetIndex);
        std::cerr << " - " << layer->name().toStdString() << std::endl;
        tilesetIndex++;
    }

    abort();
}

unsigned int LoadMap::searchTileIndexByName(const Tiled::Map &tiledMap,const QString &name)
{
    unsigned int tileLayerIndex=0;
    while(tileLayerIndex<(unsigned int)tiledMap.layerCount())
    {
        Tiled::Layer * const layer=tiledMap.layerAt(tileLayerIndex);
        if(layer->isTileLayer() && layer->name()==name)
            return tileLayerIndex;
        tileLayerIndex++;
    }
    std::cerr << "Unable to found layer with name: " << name.toStdString() << std::endl;
    abort();
}

bool LoadMap::haveTileLayer(const Tiled::Map &tiledMap,const QString &name)
{
    unsigned int tileLayerIndex=0;
    while(tileLayerIndex<(unsigned int)tiledMap.layerCount())
    {
        Tiled::Layer * const layer=tiledMap.layerAt(tileLayerIndex);
        if(layer->isTileLayer() && layer->name()==name)
            return true;
        tileLayerIndex++;
    }
    return false;
}

std::vector<Tiled::Tile *> LoadMap::getTileAt(const Tiled::Map &tiledMap,const unsigned int x,const unsigned int y)
{
    std::vector<Tiled::Tile *> tiles;
    unsigned int tileLayerIndex=0;
    while(tileLayerIndex<(unsigned int)tiledMap.layerCount())
    {
        Tiled::Layer * const layer=tiledMap.layerAt(tileLayerIndex);
        if(layer->isTileLayer())
        {
            if(layer->x()!=0 || layer->y()!=0)
                abort();
            
            // layer no longer has functions width() and height()
            //if(x>=(unsigned int)layer->width() || y>=(unsigned int)layer->height())
            //    abort();
            tiles.push_back(static_cast<Tiled::TileLayer *>(layer)->cellAt(x,y).tile());
        }
        tileLayerIndex++;
    }
    return tiles;
}

Tiled::TileLayer *LoadMap::haveTileAt(const Tiled::Map &tiledMap,const unsigned int x,const unsigned int y,const Tiled::Tile * const tile)
{
    if(tile==NULL)
        abort();
    unsigned int tileLayerIndex=0;
    while(tileLayerIndex<(unsigned int)tiledMap.layerCount())
    {
        Tiled::Layer * const layer=tiledMap.layerAt(tileLayerIndex);
        if(layer->isTileLayer())
        {
            Tiled::TileLayer * const castedLayer=static_cast<Tiled::TileLayer *>(layer);
            if(layer->x()!=0 || layer->y()!=0)
                abort();

            // layer no longer has functions width() and height()// layer no longer have functions width() and height()
            // if(x>=(unsigned int)layer->width() || y>=(unsigned int)layer->height())
            //    abort();
            if(castedLayer->cellAt(x,y).tile()==tile)
                return castedLayer;
        }
        tileLayerIndex++;
    }
    return NULL;
}

Tiled::Tile * LoadMap::haveTileAtReturnTile(const Tiled::Map &tiledMap,const unsigned int x,const unsigned int y,const std::vector<Tiled::Tile *> &tiles)
{
    if(vectorcontainsAtLeastOne(tiles,static_cast<Tiled::Tile *>(NULL)))
        abort();
    unsigned int tileLayerIndex=0;
    while(tileLayerIndex<(unsigned int)tiledMap.layerCount())
    {
        Tiled::Layer * const layer=tiledMap.layerAt(tileLayerIndex);
        if(layer->isTileLayer())
        {
            Tiled::TileLayer * const castedLayer=static_cast<Tiled::TileLayer *>(layer);
            if(layer->x()!=0 || layer->y()!=0)
                abort();

            // layer no longer has functions width() and height()// layer no longer have functions width() and height()
            //if(x>=(unsigned int)layer->width() || y>=(unsigned int)layer->height())
            //    abort();
            Tiled::Tile * const tile=castedLayer->cellAt(x,y).tile();
            if(vectorcontainsAtLeastOne(tiles,tile))
                return tile;
        }
        tileLayerIndex++;
    }
    return NULL;
}

Tiled::Tile * LoadMap::haveTileAtReturnTileUniqueLayer(const unsigned int x,const unsigned int y,const std::vector<Tiled::TileLayer *> &tilesLayers,const std::vector<Tiled::Tile *> &tiles)
{
    if(vectorcontainsAtLeastOne(tiles,static_cast<Tiled::Tile *>(NULL)))
        abort();
    unsigned int tileLayerIndex=0;
    while(tileLayerIndex<(unsigned int)tilesLayers.size())
    {
        const Tiled::TileLayer * const layer=tilesLayers.at(tileLayerIndex);
        const Tiled::Tile * const tileToSearch=tiles.at(tileLayerIndex);
        Tiled::Tile * const tile=layer->cellAt(x,y).tile();
        if(tile==tileToSearch)
            return tile;
        tileLayerIndex++;
    }
    return NULL;
}
