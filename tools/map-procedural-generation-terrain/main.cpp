#include <QApplication>
#include <QSettings>
#include <QTime>
#include <iostream>

#include "../../client/tiled/tiled_mapwriter.h"
#include "../../client/tiled/tiled_mapobject.h"
#include "../../client/tiled/tiled_objectgroup.h"
#include "../../client/tiled/tiled_tile.h"

#include "znoise/headers/Simplex.hpp"
#include "VoronioForTiledMapTmx.h"
#include "LoadMap.h"
#include "TransitionTerrain.h"

#include <unordered_set>
#include <unordered_map>
#include <QFileInfo>
#include <QDir>

/*To do: Tree/Grass, Rivers
http://www-cs-students.stanford.edu/~amitp/game-programming/polygon-map-generation/*/
/*template<typename T, size_t C, size_t R>  Matrix { std::array<T, C*R> };
 * operator[](int c, int r) { return arr[C*r+c]; }*/

struct MapTemplate
{
    const Tiled::Map * tiledMap;
    std::vector<uint8_t> templateLayerNumberToMapLayerNumber;
    std::unordered_map<Tiled::Tileset *,Tiled::Tileset *> templateTilesetToMapTileset;
    uint8_t width,height,x,y;
    uint8_t baseLayerIndex;
};

struct MapPlantsOptions
{
    QString tmx;
    Tiled::Map *map;
    MapTemplate mapTemplate;
};

static MapPlantsOptions mapPlantsOptions[5][6];

void loadTypeToMap(std::vector</*heigh*/std::vector</*moisure*/MapTemplate> > &templateResolver,
                   const unsigned int heigh/*heigh, starting at 0*/,
                   const unsigned int moisure/*moisure, starting at 0*/,
                   const MapTemplate &templateMap
                   )
{
    if(templateResolver.size()<4)
    {
        MapTemplate mapTemplate;
        mapTemplate.height=0;
        mapTemplate.width=0;
        mapTemplate.tiledMap=NULL;
        mapTemplate.x=0;
        mapTemplate.y=0;
        templateResolver.resize(4);
        for(int i=0;i<4;i++)
        {
            std::fill(templateResolver[i].begin(),templateResolver[i].end(),mapTemplate);
            templateResolver[i].resize(6);
        }
    }
    if(templateResolver.size()<(heigh+1))
        templateResolver.resize((heigh+1));
    if(templateResolver[heigh].size()<(moisure+1))
        templateResolver[heigh].resize((moisure+1));
    templateResolver[heigh][moisure]=templateMap;
}

bool detectBorder(Tiled::TileLayer *layer,MapTemplate *templateMap)
{
    uint8_t min_x=layer->width(),min_y=layer->height(),max_x=0,max_y=0;
    uint8_t y=0;
    while(y<layer->height())
    {
        uint8_t x=0;
        while(x<layer->width())
        {
            if(!layer->cellAt(x,y).isEmpty())
            {
                if(x<min_x)
                    min_x=x;
                if(y<min_y)
                    min_y=y;
                if(x>max_x)
                    max_x=x;
                if(y>max_y)
                    max_y=y;
            }
            x++;
        }
        y++;
    }
    if(max_x<min_x)
        return false;
    templateMap->width=max_x-min_x+1;
    templateMap->height=max_y-min_y+1;
    templateMap->x=min_x;
    templateMap->y=min_y;
    return true;
}

MapTemplate tiledMapToMapTemplate(const Tiled::Map *templateMap,Tiled::Map &worldMap)
{
    uint8_t lastDimensionLayerUsed=99;
    MapTemplate returnedVar;
    returnedVar.tiledMap=templateMap;
    returnedVar.templateLayerNumberToMapLayerNumber.resize(templateMap->layerCount());
    returnedVar.baseLayerIndex=0;
    //returnedVar.templateTilesetToMapTileset.resize(templateMap->tilesetCount());

    {
        std::unordered_set<uint8_t> alreadyBlockedLayer;
        uint8_t templateLayerIndex=0;
        while(templateLayerIndex<templateMap->layerCount())
        {
            Tiled::Layer * layer=templateMap->layerAt(templateLayerIndex);
            if(layer->isTileLayer())
            {
                Tiled::TileLayer * tileLayer=static_cast<Tiled::TileLayer *>(layer);
                //search into the world map
                uint8_t worldLayerIndex=0;
                while(worldLayerIndex<worldMap.layerCount())
                {
                    Tiled::Layer * layer=worldMap.layerAt(worldLayerIndex);
                    if(layer->isTileLayer())
                    {
                        Tiled::TileLayer * worldTileLayer=static_cast<Tiled::TileLayer *>(layer);
                        //search into the world map
                        if(alreadyBlockedLayer.find(worldLayerIndex)==alreadyBlockedLayer.cend() && worldTileLayer->name()==tileLayer->name())
                        {
                            alreadyBlockedLayer.insert(worldLayerIndex);
                            returnedVar.templateLayerNumberToMapLayerNumber[templateLayerIndex]=worldLayerIndex;
                            break;
                        }
                    }
                    worldLayerIndex++;
                }
                if(worldLayerIndex>=worldMap.layerCount())
                {
                    std::cerr << "a template layer is not found: " << tileLayer->name().toStdString() << " (abort)" << std::endl;
                    abort();
                }
                if(tileLayer->name()=="Collisions")
                {
                    if(detectBorder(tileLayer,&returnedVar))
                    {
                        returnedVar.baseLayerIndex=templateLayerIndex;
                        lastDimensionLayerUsed=0;
                    }
                }
                else if(tileLayer->name()=="Grass" && lastDimensionLayerUsed>0)
                {
                    if(detectBorder(tileLayer,&returnedVar))
                    {
                        returnedVar.baseLayerIndex=templateLayerIndex;
                        lastDimensionLayerUsed=1;
                    }
                }
                else if(tileLayer->name()=="OnGrass" && lastDimensionLayerUsed>1)
                {
                    if(detectBorder(tileLayer,&returnedVar))
                    {
                        returnedVar.baseLayerIndex=templateLayerIndex;
                        lastDimensionLayerUsed=2;
                    }
                }
            }
            templateLayerIndex++;
        }
    }
    {
        uint8_t templateTilesetIndex=0;
        while(templateTilesetIndex<templateMap->tilesetCount())
        {
            Tiled::Tileset * tileset=templateMap->tilesetAt(templateTilesetIndex);
            QFileInfo tilesetFile(tileset->fileName());
            QString tilesetPath=tilesetFile.absoluteFilePath();
            uint8_t templateTilesetWorldIndex=0;
            while(templateTilesetWorldIndex<worldMap.tilesetCount())
            {
                Tiled::Tileset * tilesetWorld=worldMap.tilesetAt(templateTilesetWorldIndex);
                QFileInfo tilesetWorldFile(tilesetWorld->fileName());
                QString tilesetWorldPath=tilesetWorldFile.absoluteFilePath();
                if(tilesetPath==tilesetWorldPath)
                {
                    returnedVar.templateTilesetToMapTileset[tileset]=tilesetWorld;
                    break;
                }
                templateTilesetWorldIndex++;
            }
            if(templateTilesetWorldIndex>=templateMap->tilesetCount())
            {
                QDir templateDir(QCoreApplication::applicationDirPath()+"/dest/template/");
                const QString &tilesetTemplateRelativePath=templateDir.relativeFilePath(tilesetPath);
                returnedVar.templateTilesetToMapTileset[tileset]=LoadMap::readTileset("map/"+tilesetTemplateRelativePath,&worldMap);
            }
            templateTilesetIndex++;
        }
    }
    return returnedVar;
}

//brush down/right to the point
void brushTheMap(Tiled::Map &worldMap,const MapTemplate &selectedTemplate,const int x,const int y,uint8_t * const mapMask)
{
    const Tiled::Map *brush=selectedTemplate.tiledMap;
    unsigned int layerIndex=0;
    while(layerIndex<(unsigned int)brush->layerCount())
    {
        const Tiled::Layer * const layer=brush->layerAt(layerIndex);
        if(layer->isTileLayer())
        {
            const Tiled::TileLayer * const castedLayer=static_cast<const Tiled::TileLayer * const>(layer);
            Tiled::TileLayer * const worldLayer=static_cast<Tiled::TileLayer *>(worldMap.layerAt(selectedTemplate.templateLayerNumberToMapLayerNumber.at(layerIndex)));
            int index_y=0;
            while(index_y<brush->height())
            {
                const int y_world=index_y-selectedTemplate.y+y;
                if(y_world>=0 && y_world<worldMap.height())
                {
                    int index_x=0;
                    while(index_x<brush->width())
                    {
                        const int x_world=index_x-selectedTemplate.x+x;
                        if(x_world>=0 && x_world<worldMap.width())
                        {
                            const Tiled::Cell &cell=castedLayer->cellAt(index_x,index_y);
                            if(!cell.isEmpty())
                            {
                                const unsigned int &tileId=cell.tile->id();
                                Tiled::Tileset *worldTileset=selectedTemplate.templateTilesetToMapTileset.at(cell.tile->tileset());
                                Tiled::Cell cell;
                                cell.flippedHorizontally=false;
                                cell.flippedVertically=false;
                                cell.flippedAntiDiagonally=false;
                                cell.tile=worldTileset->tileAt(tileId);
                                worldLayer->setCell(x_world,y_world,cell);
                                if(layerIndex==selectedTemplate.baseLayerIndex)
                                {
                                    const unsigned int &bitMask=x_world+y_world*worldMap.width();
                                    const unsigned int maxMapSize=(worldMap.width()*worldMap.height()/8+1);
                                    if(bitMask/8>=maxMapSize)
                                        abort();
                                    mapMask[bitMask/8]|=(1<<(7-bitMask%8));
                                }
                            }
                        }
                        index_x++;
                    }
                }
                index_y++;
            }
        }
        layerIndex++;
    }
}

//brush down/right to the point
bool brushHaveCollision(Tiled::Map &worldMap,const MapTemplate &selectedTemplate,const int x,const int y,const uint8_t * const mapMask)
{
    const Tiled::Map *brush=selectedTemplate.tiledMap;
    unsigned int layerIndex=0;
    while(layerIndex<(unsigned int)brush->layerCount())
    {
        const Tiled::Layer * const layer=brush->layerAt(layerIndex);
        if(layer->isTileLayer())
        {
            const Tiled::TileLayer * const castedLayer=static_cast<const Tiled::TileLayer * const>(layer);
            Tiled::TileLayer * const worldLayer=static_cast<Tiled::TileLayer *>(worldMap.layerAt(selectedTemplate.templateLayerNumberToMapLayerNumber.at(layerIndex)));
            int index_y=0;
            while(index_y<brush->height())
            {
                const int y_world=index_y-selectedTemplate.y+y;
                if(y_world>=0 && y_world<worldMap.height())
                {
                    int index_x=0;
                    while(index_x<brush->width())
                    {
                        const int x_world=index_x-selectedTemplate.x+x;
                        if(x_world>=0 && x_world<worldMap.width())
                        {
                            const Tiled::Cell &cell=castedLayer->cellAt(index_x,index_y);
                            if(!cell.isEmpty())
                            {
                                if(!worldLayer->cellAt(x_world,y_world).isEmpty())
                                    return false;
                                if(layerIndex==selectedTemplate.baseLayerIndex)
                                {
                                    const unsigned int &bitMask=x_world+y_world*worldMap.width();
                                    if(mapMask[bitMask/8] & (1<<(7-bitMask%8)))
                                        return false;
                                }
                            }
                        }
                        index_x++;
                    }
                }
                index_y++;
            }
        }
        layerIndex++;
    }
    return true;
}

void addVegetation(Tiled::Map &worldMap,const VoronioForTiledMapTmx::PolygonZoneMap &vd)
{
    std::vector<std::vector</*moisure*/MapTemplate> > templateResolver;
    for(int height=1;height<5;height++)
        for(int moisure=0;moisure<6;moisure++)
        {
            MapPlantsOptions &mapPlantsOption=mapPlantsOptions[height][moisure];
            if(!mapPlantsOption.tmx.isEmpty())
            {
                const Tiled::Map *map=LoadMap::readMap("template/"+mapPlantsOption.tmx+".tmx");
                mapPlantsOption.mapTemplate=tiledMapToMapTemplate(map,worldMap);
                loadTypeToMap(templateResolver,height-1,moisure,mapPlantsOption.mapTemplate);
            }
        }

    const unsigned int mapMaskSize=(worldMap.width()*worldMap.height()/8+1);
    uint8_t mapMask[mapMaskSize];
    memset(mapMask,0x00,mapMaskSize);
    unsigned int y=0;
    while(y<(unsigned int)worldMap.height())
    {
        unsigned int x=0;
        while(x<(unsigned int)worldMap.width())
        {
            //resolve into zone
            const VoronioForTiledMapTmx::PolygonZoneIndex &zoneIndex=vd.tileToPolygonZoneIndex[x+y*worldMap.width()];
            const VoronioForTiledMapTmx::PolygonZone &zone=vd.zones[zoneIndex.index];
            //resolve into MapTemplate
            if(zone.height>0)
            {
                const MapTemplate &selectedTemplate=templateResolver.at(zone.height-1).at(zone.moisure-1);
                if(selectedTemplate.tiledMap!=NULL)
                {
                    if(selectedTemplate.width==0 || selectedTemplate.height==0)
                        abort();
                    if(x%selectedTemplate.width==0 && y%selectedTemplate.height==0)
                    {
                        //check if all the collions layer is into the zone
                        bool collionsIsIntoZone=brushHaveCollision(worldMap,selectedTemplate,x,y,mapMask);
                        /*{
                            const Tiled::Map *brush=selectedTemplate.tiledMap;
                            int index_y=0;
                            while(index_y<brush->height())
                            {
                                const int y_world=index_y-selectedTemplate.y+y;
                                if(y_world>=0 && y_world<worldMap.height())
                                {
                                    int index_x=0;
                                    while(index_x<brush->width())
                                    {
                                        const int x_world=index_x-selectedTemplate.x+x;
                                        if(x_world>=0 && x_world<worldMap.width())
                                        {
                                            const VoronioForTiledMapTmx::PolygonZoneIndex &exploredZoneIndex=vd.tileToPolygonZoneIndex[x_world+y_world*worldMap.height()];
                                            const VoronioForTiledMapTmx::PolygonZone &exploredZone=vd.zones[exploredZoneIndex.index];
                                            if(LoadMap::heightAndMoisureToZoneType(exploredZone.height,exploredZone.moisure)!=LoadMap::heightAndMoisureToZoneType(zone.height,zone.moisure))
                                            {
                                                collionsIsIntoZone=false;
                                                break;
                                            }
                                        }
                                        index_x++;
                                    }
                                    if(index_x<brush->width())
                                        break;
                                }
                                index_y++;
                            }
                        }*/
                        if(collionsIsIntoZone)
                            brushTheMap(worldMap,selectedTemplate,x,y,mapMask);
                    }
                }
            }
            x++;
        }
        y++;
    }
}

void putDefaultSettings(QSettings &settings)
{
    //do tile to zone converter
    settings.beginGroup("terrain");
        settings.beginGroup("water");
            if(!settings.contains("tsx"))
                settings.setValue("tsx","animation.tsx");
            if(!settings.contains("tileId"))
                settings.setValue("tileId",0);
            if(!settings.contains("heightmoisurelist"))
                settings.setValue("heightmoisurelist","0,1;0,2;0,3;0,4;0,5;0,6");//the index
            if(!settings.contains("layer"))
                settings.setValue("layer","Water");
        settings.endGroup();
        settings.beginGroup("grass");
            if(!settings.contains("tsx"))
                settings.setValue("tsx","terra.tsx");
            if(!settings.contains("tileId"))
                settings.setValue("tileId",0);
            if(!settings.contains("heightmoisurelist"))
                settings.setValue("heightmoisurelist","1,1;1,2;1,3;1,4;1,5;1,6;2,1;2,2;2,3;2,4;2,5;2,6;3,1;3,2;3,3;3,4;3,5;3,6");//the index
            if(!settings.contains("layer"))
                settings.setValue("layer","Walkable");
        settings.endGroup();
        settings.beginGroup("mountain");
            if(!settings.contains("tsx"))
                settings.setValue("tsx","terra.tsx");
            if(!settings.contains("tileId"))
                settings.setValue("tileId",353);
            if(!settings.contains("heightmoisurelist"))
                settings.setValue("heightmoisurelist","4,1;4,2;4,3;4,4;4,5;4,6");//the index
            if(!settings.contains("layer"))
                settings.setValue("layer","Walkable");
        settings.endGroup();
    settings.endGroup();
    settings.beginGroup("plants");
        settings.beginGroup("flowers");
            if(!settings.contains("tmx"))
                settings.setValue("tmx","flowers");
            if(!settings.contains("heightmoisurelist"))
                settings.setValue("heightmoisurelist","1,1");
        settings.endGroup();
        settings.beginGroup("grass");
            if(!settings.contains("tmx"))
                settings.setValue("tmx","grass");
            if(!settings.contains("heightmoisurelist"))
                settings.setValue("heightmoisurelist","1,2");
        settings.endGroup();
        settings.beginGroup("tree-1");
            if(!settings.contains("tmx"))
                settings.setValue("tmx","tree-1");
            if(!settings.contains("heightmoisurelist"))
                settings.setValue("heightmoisurelist","1,3");
        settings.endGroup();
        settings.beginGroup("tree-2");
            if(!settings.contains("tmx"))
                settings.setValue("tmx","tree-2");
            if(!settings.contains("heightmoisurelist"))
                settings.setValue("heightmoisurelist","1,4");
        settings.endGroup();
        settings.beginGroup("tree-3");
            if(!settings.contains("tmx"))
                settings.setValue("tmx","tree-3");
            if(!settings.contains("heightmoisurelist"))
                settings.setValue("heightmoisurelist","1,5;1,6");
        settings.endGroup();
    settings.endGroup();
    settings.beginGroup("transition");
        settings.beginGroup("waterborder");
            if(!settings.contains("from_type"))
                settings.setValue("from_type","water");
            if(!settings.contains("to_type"))
                settings.setValue("to_type","grass");
            if(!settings.contains("transition_tsx"))
                settings.setValue("transition_tsx","terra.tsx");
            if(!settings.contains("transition_tile"))
                settings.setValue("transition_tile","184,185,186,218,250,249,248,216,281,282,314,313");
            if(!settings.contains("replace_tile"))
                settings.setValue("replace_tile",false);
        settings.endGroup();
        settings.beginGroup("monstainborder");
            if(!settings.contains("from_type"))
                settings.setValue("from_type","mountain");
            if(!settings.contains("to_type"))
                settings.setValue("to_type","grass");
            if(!settings.contains("transition_tsx"))
                settings.setValue("transition_tsx","terra.tsx");
            if(!settings.contains("transition_tile"))
                settings.setValue("transition_tile","320,321,322,354,386,385,384,352,323,324,356,355");//missing moutain piece
            if(!settings.contains("collision_tsx"))
                settings.setValue("collision_tsx","terra.tsx");
            if(!settings.contains("collision_tile"))
                settings.setValue("collision_tile","21,22,23,55,87,86,85,53,82,83,115,114");//the mountain borden
            if(!settings.contains("replace_tile"))
                settings.setValue("replace_tile",true);//put the nearest tile into current (grass replace mountain)
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
    if(!settings.contains("displayzone"))
        settings.setValue("displayzone",false);
    if(!settings.contains("dotransition"))
        settings.setValue("dotransition",true);
    if(!settings.contains("dovegetation"))
        settings.setValue("dovegetation",true);

    settings.sync();
}

void loadSettings(QSettings &settings,unsigned int &mapWidth,unsigned int &mapHeight,unsigned int &mapXCount,unsigned int &mapYCount,unsigned int &seed,bool &displayzone,std::vector<LoadMap::TerrainTransition> &terrainTransitionList,
                  bool &dotransition,bool &dovegetation)
{
    bool ok;
    {
        for(int height=0;height<5;height++)
            for(int moisure=0;moisure<6;moisure++)
            {
                LoadMap::terrainList[height][moisure].tile=NULL;
                LoadMap::terrainList[height][moisure].tileLayer=NULL;
                LoadMap::terrainList[height][moisure].tileId=0;
            }
        settings.beginGroup("terrain");
        const QStringList &groupsNames=settings.childGroups();
        unsigned int index=0;
        while(index<(unsigned int)groupsNames.size())
        {
            const QString &terrainName=groupsNames.at(index);
            settings.beginGroup(terrainName);
                const QString &tsx=settings.value("tsx").toString();
                const unsigned int &tileId=settings.value("tileId").toUInt(&ok);
                if(!ok)
                {
                    std::cerr << "tileId not number: " << settings.value("tileId").toString().toStdString() << std::endl;
                    abort();
                }
                const QString &layerString=settings.value("layer").toString();

                QStringList heightmoisurelist=settings.value("heightmoisurelist").toString().split(';');
                unsigned int heightmoisurelistIndex=0;
                while(heightmoisurelistIndex<(unsigned int)heightmoisurelist.size())
                {
                    QStringList heightmoisure=heightmoisurelist.at(heightmoisurelistIndex).split(',');
                    if(heightmoisure.size()!=2)
                    {
                        std::cerr << "heightmoisure.size()!=2" << std::endl;
                        abort();
                    }
                    const unsigned int &height=heightmoisure.at(0).toUInt(&ok);
                    if(!ok)
                    {
                        std::cerr << "height not a number" << std::endl;
                        abort();
                    }
                    if(height>4)
                    {
                        std::cerr << "height not in valid range" << std::endl;
                        abort();
                    }
                    const unsigned int &moisure=heightmoisure.at(1).toUInt(&ok);
                    if(!ok)
                    {
                        std::cerr << "moisure not a number" << std::endl;
                        abort();
                    }
                    if(moisure>6 && moisure<1)
                    {
                        std::cerr << "moisure not in valid range" << std::endl;
                        abort();
                    }
                    LoadMap::terrainList[height][moisure-1].tsx=tsx;
                    LoadMap::terrainList[height][moisure-1].tileId=tileId;
                    LoadMap::terrainList[height][moisure-1].layerString=layerString;
                    LoadMap::terrainList[height][moisure-1].terrainName=terrainName;
                    heightmoisurelistIndex++;
                }
            settings.endGroup();
            index++;
        }
        settings.endGroup();
    }
    {
        settings.beginGroup("plants");
        const QStringList &groupsNames=settings.childGroups();
        unsigned int index=0;
        while(index<(unsigned int)groupsNames.size())
        {
            const QString &terrainName=groupsNames.at(index);
            settings.beginGroup(terrainName);
                const QString &tmx=settings.value("tmx").toString();

                QStringList heightmoisurelist=settings.value("heightmoisurelist").toString().split(';');
                unsigned int heightmoisurelistIndex=0;
                while(heightmoisurelistIndex<(unsigned int)heightmoisurelist.size())
                {
                    QStringList heightmoisure=heightmoisurelist.at(heightmoisurelistIndex).split(',');
                    if(heightmoisure.size()!=2)
                    {
                        std::cerr << "heightmoisure.size()!=2" << std::endl;
                        abort();
                    }
                    const unsigned int &height=heightmoisure.at(0).toUInt(&ok);
                    if(!ok)
                    {
                        std::cerr << "height not a number" << std::endl;
                        abort();
                    }
                    if(height>4)
                    {
                        std::cerr << "height not in valid range" << std::endl;
                        abort();
                    }
                    const unsigned int &moisure=heightmoisure.at(1).toUInt(&ok);
                    if(!ok)
                    {
                        std::cerr << "moisure not a number" << std::endl;
                        abort();
                    }
                    if(moisure>6 && moisure<1)
                    {
                        std::cerr << "moisure not in valid range" << std::endl;
                        abort();
                    }
                    mapPlantsOptions[height][moisure-1].tmx=tmx;
                    heightmoisurelistIndex++;
                }
            settings.endGroup();
            index++;
        }
        settings.endGroup();
    }
    mapWidth=settings.value("mapWidth").toUInt(&ok);
    if(ok==false)
    {
        std::cerr << "mapWidth not number into the config file" << std::endl;
        abort();
    }
    mapHeight=settings.value("mapHeight").toUInt(&ok);
    if(ok==false)
    {
        std::cerr << "mapHeight not number into the config file" << std::endl;
        abort();
    }
    mapXCount=settings.value("mapXCount").toUInt(&ok);
    if(ok==false)
    {
        std::cerr << "mapXCount not number into the config file" << std::endl;
        abort();
    }
    mapYCount=settings.value("mapYCount").toUInt(&ok);
    if(ok==false)
    {
        std::cerr << "mapYCount not number into the config file" << std::endl;
        abort();
    }
    seed=settings.value("seed").toUInt(&ok);
    if(ok==false)
    {
        std::cerr << "seed not number into the config file" << std::endl;
        abort();
    }
    displayzone=settings.value("displayzone").toBool();
    dotransition=settings.value("dotransition").toBool();
    dovegetation=settings.value("dovegetation").toBool();
    {
        settings.beginGroup("transition");
        const QStringList &groupsNames=settings.childGroups();
        {
            unsigned int index=0;
            while(index<(unsigned int)groupsNames.size())
            {
                settings.beginGroup(groupsNames.at(index));
                    LoadMap::TerrainTransition terrainTransition;
                    //before map creation
                    terrainTransition.replace_tile=settings.value("replace_tile").toBool();
                    //tempory value
                    terrainTransition.tmp_from_type=settings.value("from_type").toString();
                    {
                        const QStringList &to_type_list=settings.value("to_type").toString().split(',');
                        unsigned int to_type_index=0;
                        while(to_type_index<(unsigned int)to_type_list.size())
                        {
                            terrainTransition.tmp_to_type.push_back(to_type_list.at(to_type_index));
                            to_type_index++;
                        }
                    }
                    terrainTransition.tmp_transition_tsx=settings.value("transition_tsx").toString();
                    {
                        bool ok;
                        const QString &tmp_transition_tile_settings=settings.value("transition_tile").toString();
                        if(!tmp_transition_tile_settings.isEmpty())
                        {
                            const QStringList &tmp_transition_tile_list=tmp_transition_tile_settings.split(',');
                            unsigned int tmp_transition_tile_index=0;
                            while(tmp_transition_tile_index<(unsigned int)tmp_transition_tile_list.size())
                            {
                                const QString &s=tmp_transition_tile_list.at(tmp_transition_tile_index);
                                terrainTransition.tmp_transition_tile.push_back(s.toInt(&ok));
                                if(!ok)
                                {
                                    std::cerr << "into transition_tile some is not a number: " << s.toStdString() << " from: " << tmp_transition_tile_settings.toStdString() << " (abort)" << std::endl;
                                    abort();
                                }
                                tmp_transition_tile_index++;
                            }
                        }
                        while(terrainTransition.tmp_transition_tile.size()<=12)
                            terrainTransition.tmp_transition_tile.push_back(-1);
                    }
                    terrainTransition.tmp_collision_tsx=settings.value("collision_tsx").toString();
                    {
                        bool ok;
                        const QString &tmp_collision_tile_settings=settings.value("collision_tile").toString();
                        if(!tmp_collision_tile_settings.isEmpty())
                        {
                            const QStringList &tmp_collision_tile_list=tmp_collision_tile_settings.split(',');
                            unsigned int tmp_transition_tile_index=0;
                            while(tmp_transition_tile_index<(unsigned int)tmp_collision_tile_list.size())
                            {
                                const QString &s=tmp_collision_tile_list.at(tmp_transition_tile_index);
                                terrainTransition.tmp_collision_tile.push_back(s.toInt(&ok));
                                if(!ok)
                                {
                                    std::cerr << "into transition_tile some is not a number: " << s.toStdString() << " from: " << tmp_collision_tile_settings.toStdString() << " (abort)" << std::endl;
                                    abort();
                                }
                                tmp_transition_tile_index++;
                            }
                        }
                        while(terrainTransition.tmp_collision_tile.size()<=12)
                            terrainTransition.tmp_collision_tile.push_back(-1);
                    }
                    terrainTransitionList.push_back(terrainTransition);
                settings.endGroup();
                index++;
            }
        }
        settings.endGroup();
    }
}

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

    QSettings settings(QCoreApplication::applicationDirPath()+"/settings.xml",QSettings::NativeFormat);
    putDefaultSettings(settings);
    unsigned int mapWidth;
    unsigned int mapHeight;
    unsigned int mapXCount;
    unsigned int mapYCount;
    unsigned int seed;
    bool displayzone,dotransition,dovegetation;
    std::vector<LoadMap::TerrainTransition> terrainTransitionList;
    loadSettings(settings,mapWidth,mapHeight,mapXCount,mapYCount,seed,displayzone,terrainTransitionList,dotransition,dovegetation);
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
        VoronioForTiledMapTmx::PolygonZoneMap vd = VoronioForTiledMapTmx::computeVoronoi(grid,totalWidth,totalHeight,2);
        if(vd.zones.size()!=grid.size())
            abort();
        qDebug("computeVoronoi took %d ms", t.elapsed());

        {
            Tiled::Map tiledMap(Tiled::Map::Orientation::Orthogonal,totalWidth,totalHeight,16,16);
            QHash<QString,Tiled::Tileset *> cachedTileset;
            LoadMap::addTerrainLayer(tiledMap);
            LoadMap::loadAllTileset(cachedTileset,tiledMap);
            LoadMap::load_terrainTransitionList(cachedTileset,terrainTransitionList,tiledMap);
            if(displayzone)
            {
                std::vector<std::vector<Tiled::ObjectGroup *> > arrayTerrainPolygon;
                Tiled::ObjectGroup *layerZoneWaterPolygon=LoadMap::addDebugLayer(tiledMap,arrayTerrainPolygon,true);
                std::vector<std::vector<Tiled::ObjectGroup *> > arrayTerrainTile;
                Tiled::ObjectGroup *layerZoneWaterTile=LoadMap::addDebugLayer(tiledMap,arrayTerrainTile,false);
                LoadMap::addPolygoneTerrain(arrayTerrainPolygon,layerZoneWaterPolygon,arrayTerrainTile,layerZoneWaterTile,grid,vd,heighmap,moisuremap,noiseMapScale,tiledMap.width(),tiledMap.height());
            }
            {
                t.start();
                LoadMap::addTerrain(grid,vd,heighmap,moisuremap,noiseMapScale,tiledMap.width(),tiledMap.height());
                qDebug("Add terrain took %d ms", t.elapsed());
                if(dotransition)
                {
                    t.start();
                    TransitionTerrain::addTransitionOnMap(tiledMap,terrainTransitionList);
                    qDebug("Transitions took %d ms", t.elapsed());
                }
            }
            if(dovegetation)
            {
                t.start();
                addVegetation(tiledMap,vd);
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